#include<arpa/inet.h>
#include<sys/socket.h>
#include<unistd.h>

#include<cstdlib>
#include<fstream>
#include<iostream>
#include<thread>

#include"string_utilities.h"
#include"request_parsing_response_preparation.h"
#include"network.h"
#include"file_manipulation.h"

void handleClient(int client_fd) {
  while (1) {
		char *buf;
		if (!(buf = receiveRequest(client_fd))) {
			break;
		}

		if (myStrcmp(buf, "GET") || myStrcmp(buf, "POST") || myStrcmp(buf, "LIST")) {
			if (!isRequestValid(buf, 1, myStrcmp(buf, "POST"))) {
				std::cerr << "Request not valid\n";
				char *response = formResponse(400, nullptr);
				free(buf);
				if (!response || sendResponse(client_fd, response)) {
					break;
				}
				continue;
			}

			char *path = extractPath(buf);
			int status = 200;
			if (!path) {
				status = 500;
			}
			else {
				path = checkPathValidity(path, status, myStrcmp(buf, "LIST"), myStrcmp(buf, "POST"));
			}
			if (status != 200) {
				char *response = formResponse(status, nullptr);
				free(buf);
				if (!response || sendResponse(client_fd, response)) {
					break;
				}
				continue;
			}
			if (myStrcmp(buf, "POST")) {
				char *response = nullptr;
				char *content = nullptr;
				if (access(path, W_OK)) {
					response = formResponse(403, nullptr);
				}
				else {
					content = extractContentToPost(buf);
					response = formResponse( (!content || post(path, content)) ? 500 : 200, nullptr);
				}
				free(buf);
				free(path);
				free(content);
				if (!response || sendResponse(client_fd, response)) {
					break;
				}
			}
			else {
				char *body = nullptr;
				char *response = nullptr;
				if (access(path, R_OK)) {
					response = formResponse(403, nullptr);
				}
				else {
					body = myStrcmp(buf, "GET") ? getFileContent(path) : listDirectory(path);
					response = formResponse(body ? 200 : 500, body);
				}
				free(buf);
				free(path);
				free(body);
				if (!response || sendResponse(client_fd, response)) {
					break;
				}
			}
		}

		else if (myStrcmp(buf, "STATUS") || myStrcmp(buf, "QUIT")) {
			if (!isRequestValid(buf, 0, 0)) {
				std::cerr << "Request not valid\n";
				free(buf);
				char *response = formResponse(400, nullptr);
				if (!response || sendResponse(client_fd, response)) {
					break;
				}
				continue;
			}
			char *response = formResponse(200, nullptr);
			if (!response || sendResponse(client_fd, response)) {
				break;
			}
			if (myStrcmp(buf, "QUIT")) {
				free(buf);
				break;
			}
			free(buf);
		}

		else {
			free(buf);
			char *response = formResponse(400, nullptr);
			if (!response || sendResponse(client_fd, response)) {
				break;
			}
		}
  }

  close(client_fd);
}

int main() {
	int server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (server_fd < 0) {
		std::cerr << "Failed to create a socket\n";
		return 1;
	}

	const int opt = 1;
	if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
		std::cerr << "Failed to set socket option\n";
		close(server_fd);
		return 1;
	}

	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(8080);
	addr.sin_addr.s_addr = INADDR_ANY;

	if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
		std::cerr << "Failed to bind\n";
		close(server_fd);
		return 1;
	}

	if (listen(server_fd, 10) < 0) {
		std::cerr << "Failed to listen\n";
		close(server_fd);
		return 1;
	}

	while (1) {
		int client_fd = accept(server_fd, NULL, NULL);
		if (client_fd < 0) {
			std::cerr << "Failed to accept client\n";
			continue;
		}
		std::thread(handleClient, client_fd).detach();
	}
	close(server_fd);
	return 0;
}
