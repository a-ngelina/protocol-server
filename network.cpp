#include<arpa/inet.h>
#include<fcntl.h>
#include<sys/file.h>
#include<sys/socket.h>
#include<unistd.h>

#include<cstdlib>
#include<cstring>
#include<iostream>

#include"string_utilities.h"
#include"network.h"

bool sendResponse(int client_fd, char *response) {
	int sent_total = 0;
	int to_send = strlen(response);

	char *len_str = intToStr(to_send);
	int len_sent_total = 0, len_to_send = strlen(len_str);
	if (!len_str) {
		free(response);
		return 1;
	}
	while (len_to_send) {
		ssize_t bytes_sent = send(client_fd, len_str + len_sent_total, len_to_send, 0);
		if (bytes_sent < 0) {
			std::cerr << "Failed to send response to client\n";
			free(len_str);
			free(response);
			return 1;
		}
		len_sent_total += bytes_sent;
		len_to_send -= bytes_sent;
	}
	free(len_str);

	while (to_send > 0) {
		ssize_t bytes_sent = send(client_fd, response + sent_total, to_send, 0);
		if (bytes_sent < 0) {
			std::cerr << "Failed to send response to client\n";
			free(response);
			return 1;
		}
		sent_total += bytes_sent;
		to_send -= bytes_sent;
	}

#ifdef DEBUG
	std::cout << "Server sent response to client: \"" << response << "\"\n";
#endif

	free(response);
	return 0;
}

char* receiveRequest(int client_fd) {
	ssize_t bytes_read = 0;
	int request_len = 0;
	char tmp[1];
	while ((bytes_read = recv(client_fd, tmp, 1, 0)) > 0 && tmp[0] != '\n') {
		if (!(tmp[0] - '0' >= 0 && tmp[0] - '0' < 10)) {
			std::cerr << "Request length not valid\n";
			return nullptr;
		}
		request_len = request_len * 10 + tmp[0] - '0';
	}

	char *buf = (char *)malloc((request_len + 1) * sizeof(char));
	if (!buf) {
		std::cerr << "Failed to allocate memory for request\n";
		return nullptr;
	}

	int left_to_read = request_len, read = 0;
	while (left_to_read > 0 && (bytes_read = recv(client_fd, buf + read, left_to_read, 0)) > 0) {
		left_to_read -= bytes_read;
		read += bytes_read;
	}
	if (bytes_read < 0) {
		std::cerr << "Failed to receive request\n";
		free(buf);
		return nullptr;
	}
	buf[read] = '\0';
#ifdef DEBUG
	std::cout << "Server received request: \"" << buf << "\"\n";
#endif
	return buf;
}
