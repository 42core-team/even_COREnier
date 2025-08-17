#include "Bridge.h"
#include "Board.h"

#include <sys/socket.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <errno.h>
#include <chrono>

Bridge::Bridge(int socket_fd, unsigned int teamId)
	: socket_fd_(socket_fd), team_id_(teamId)
{
	timeval tv{};
	tv.tv_sec = 1;
	tv.tv_usec = 0;
	if (setsockopt(socket_fd_, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0)
		Logger::LogWarn(std::string("setsockopt(SO_RCVTIMEO) failed: ") + strerror(errno));
	if (setsockopt(socket_fd_, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv)) < 0)
		Logger::LogWarn(std::string("setsockopt(SO_SNDTIMEO) failed: ") + strerror(errno));
}

Bridge::~Bridge()
{
	shutdown(socket_fd_, SHUT_RD);
	closing_.store(true);
	writeCv_.notify_all();
	readCv_.notify_all();

	{
		std::unique_lock<std::mutex> lk(readMutex_);
		readCv_.wait_for(lk, std::chrono::seconds(5), [this]{ return readQueue_.empty(); });
	}

	if (writeThread_.joinable())
		writeThread_.join();

	shutdown(socket_fd_, SHUT_WR);

	if (readThread_.joinable())
		readThread_.join();

	close(socket_fd_);
}

void Bridge::start()
{
	readThread_ = std::thread(&Bridge::readLoop, this);
	writeThread_ = std::thread(&Bridge::writeLoop, this);
}

void Bridge::sendMessage(const json &message)
{
	std::string msg = message.dump() + "\n";
	{
		std::lock_guard<std::mutex> lock(writeMutex_);
		writeQueue_.push(msg);
	}
	writeCv_.notify_one();
}

bool Bridge::receiveMessage(json &message)
{
	std::unique_lock<std::mutex> lock(readMutex_);
	if (readQueue_.empty())
		readCv_.wait(lock, [this]{ return !readQueue_.empty() || closing_.load(); });
	if (readQueue_.empty())
		return false;
	message = readQueue_.front();
	readQueue_.pop();
	return true;
}

bool Bridge::tryReceiveMessage(json &message)
{
	std::lock_guard<std::mutex> lock(readMutex_);
	if (readQueue_.empty())
		return false;
	message = readQueue_.front();
	readQueue_.pop();
	return true;
}

void Bridge::readLoop()
{
	constexpr size_t buffer_size = 1024;
	char buffer[buffer_size];
	std::string data;

	for (;;)
	{
		if (closing_.load())
			break;

		ssize_t n = ::read(socket_fd_, buffer, buffer_size);
		if (n < 0)
		{
			if (errno == EAGAIN || errno == EWOULDBLOCK)
				continue;
			Logger::LogWarn(std::string("Read error: ") + strerror(errno));
			break;
		}
		if (n == 0)
			break;

		data.append(buffer, n);

		size_t pos;
		while ((pos = data.find('\n')) != std::string::npos)
		{
			std::string line = data.substr(0, pos);
			data.erase(0, pos + 1);
			if (line.empty())
				continue;
			try
			{
				json j = json::parse(line);
				{
					std::lock_guard<std::mutex> lock(readMutex_);
					readQueue_.push(j);
				}
				readCv_.notify_one();
			}
			catch (json::parse_error &e)
			{
				Logger::LogWarn(std::string("JSON parse error: ") + e.what());
			}
		}
	}
}

void Bridge::writeLoop()
{
	for (;;)
	{
		std::unique_lock<std::mutex> lock(writeMutex_);
		writeCv_.wait(lock, [this]{ return closing_.load() || !writeQueue_.empty(); });
		if (closing_.load() && writeQueue_.empty())
			break;

		while (!writeQueue_.empty())
		{
			std::string msg = writeQueue_.front();
			writeQueue_.pop();
			lock.unlock();

			const char *data = msg.c_str();
			size_t remaining = msg.size();
			while (remaining > 0)
			{
				ssize_t n = ::send(socket_fd_, data, remaining, MSG_NOSIGNAL);
				if (n < 0)
				{
					Logger::LogWarn(std::string("Write error: ") + strerror(errno));
					remaining = 0;
					break;
				}
				remaining -= static_cast<size_t>(n);
				data += n;
			}

			lock.lock();
		}
	}
}
