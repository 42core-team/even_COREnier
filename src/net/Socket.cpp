#include <stdexcept>
#include <sys/socket.h>
#include <string.h>
#include <unistd.h>

#include "Socket.hpp"

Socket::Socket(int domain, int type, int protocol) {
	sock_fd = socket(domain, type, protocol);
	if (sock_fd == -1) {
		throw std::runtime_error("Failed to create socket: "
				+ std::string(strerror(errno)));
	}
}

Socket::~Socket() {
	if (sock_fd != -1) {
		close(sock_fd);
	}
}

Socket::Socket(Socket&& other) noexcept : sock_fd(other.sock_fd) {
	other.sock_fd = -1;
}

Socket& Socket::operator=(Socket&& other) noexcept {
	if (this != &other) {
		if (sock_fd != -1)
			close(sock_fd);
		sock_fd = other.sock_fd;
		other.sock_fd = -1;
	}
	return *this;
}

int Socket::get_fd() const {
	return sock_fd;
}
