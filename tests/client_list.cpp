#include<arpa/inet.h>
#include<iostream>
#include<sys/socket.h>
#include<unistd.h>

#include"client_functions.h"

int main() {
	int server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (server_fd < 0) {
		std::cerr << "Failed to create a socket\n";
		return 1;
	}

	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(8080);

	if (inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr) <= 0) {
		std::cerr << "Invalid address\n";
		close(server_fd);
		return 1;
	}

	if (connect(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		std::cerr << "Failed to connect to server\n";
		close(server_fd);
		return 1;
	}

#ifdef DEBUG
	std::cout << "Connected to server\n";
#endif

	if (sendRequest(server_fd, "LIST test")) {
		return 1;
	}

	if (receiveResponse(server_fd)) {
		return 1;
	}

	quitServer(server_fd);

	return 0;
}
