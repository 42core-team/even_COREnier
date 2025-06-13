#ifndef SOCKET_H
#define SOCKET_H

class Socket {
	public:
		explicit Socket(int domain, int type, int protocol);
		~Socket();

		Socket(const Socket&) = delete;
		Socket& operator=(const Socket&) = delete;

		Socket(Socket&& other) noexcept;
		Socket& operator=(Socket&& other) noexcept;

		int get_fd() const;

	private:
		int sock_fd;
};

#endif /* SOCKET_H */
