#include<arpa/inet.h>
#include<cstdlib>
#include<filesystem>
#include<iostream>
#include<sys/socket.h>
#include<sys/stat.h>
#include<thread>
#include<unistd.h>

// TODO handle frees
// TODO handle const strings (state when functions return const char*)
// TODO add mutex and locks

bool myStrcmp(const char *s1, const char *s2) {
	while (*s2 != '\0') {
		if (*s1 != *s2) {
			return 0;
		}
		++s1;
		++s2;
	}
	return 1;
}

char* formResponse(int status, char *body) {
	int response_len = 24 + 1 + strlen(body) + 1;
  // 24 for status code + message + \n, 1 for \n, body, 1 for \0

  char *response = malloc(response_len);
  if (body) {
    snprintf(response, response_len, "%d %s\n\n%s", status, getMessage(status), body);
  }
  else {
    snprintf(response, response_len, "%d %s", status, getMessage(status));
  }

	return request;
}

bool sendResponse(int client_fd, char *response) {
	if (send(client_fd, response, strlen(response))) {
		std::cerr << "Failed to send response to client\n";
		free(response);
		return 1;
	}
	free(response);
	return 0;
}

int checkPathValidity(char *path, bool dir) {
	using fs = std::filesystem;

	try {
		fs::path cwd = fs::current_path();
		fs::path full_path = fs::canonical(cwd / path);

		if (!full_path.string().starts_with(cwd.string())) {
			return 403;
		}

		if (!fs::exists(full_path)) {
			return 404;
		}

		if ((dir && !fs::is_directory(full_path)) || (!dir && !fs::is_regular_file(full_path))) {
			return 400;
		}
		return 200;
	}
	catch (const fs::filesystem_error& e) {
		std::cerr << "Filesystem error: " << e.what() << "\n";
		return 500;
	}
}

const char* getMessage(int status) {
	switch(status) {
	case 200:
		return "OK";
	case 400:
		return "Bad request";
	case 401:
		return "Unauthorized";
	case 403:
		return "Forbidden";
	case 404:
		return "Not found";
	case 500:
		return "Error";
	case 503:
		return "Service unavailable";
	default:
		return "Unknown";
	}
}

char* readRequest(int client_fd) {
	size_t buf_cap = 1024;
	char *buf = malloc(buf_cap * sizeof(char));
	
	ssize_t bytes_read = 0, buf_lenlen = 0;
	while ((bytes_read = recv(client_fd, buf + buf_len, buf_cap - buf_len, 0)) > 0) {
		buf_len += bytes_read;
		if (bytes_read >= buf_cap - buf_len - 1) {
			buf_cap *= 2;
			buf = realloc(buf, buf_cap);
		}
	}
	if (bytes_read < 0) {
		std::cerr << "Failed to receive request\n";
		free(buf);
		return nullptr;
	}
	buf[buf_len] = '\0';
	return buf;
}

char* listDirectory(char *path) {
	size_t cap = 1024, len = 0;
	char *response = malloc(cap * sizeof(char));
	if (!response) {
		return nullptr;
	}

	for (const auto& entry: std::filesystem::directory_iterator(path)) {
		char *entry_c_str = entry.path().c_str();
		size_t entry_len = strlen(entry_c_str);
		if (entry_len + len > cap - 1) {
			cap *= 2;
			response = realloc(cap);
			if (!response) {
				return nullptr;
			}
		}

		memcpy(response + len, entry_c_str, entry_len);
		len += entry_len;
	}
	response[len] = '\0';

	return response;
}

const char* getFileContent(char *path) {
	std::ifstream file(path, std::ios::binary);
	auto file_size = std::filesystem::file_size(path);
	std::string content(file_size);
	file.read(content.data(), file_size);
	
	return content.c_str();
}

const char* getContentToPost(char *buf) {
	
}


char* extractPath(char* buf) {
	while (*buf != '\0' && *buf != ' ' && *buf != '\n') {
		++buf;
	}

	while (*buf == ' ' || *buf == '\n') {
		++buf;
	}

	if (*buf == '\0') {
		return nullptr;
	}

	int i = 0;
	while (*(buf + i) != '\0' && *(buf + i) != ' ' && *buf != '\n') {
		++i;
	}

	char *path = malloc((i + 1) * sizeof(char));
	memcpy(path, buf, i);
	path[i] = '\0';
	return path;
}

void handleClient(int client_fd) {
  while (1) {
		char *buf;
		if (!(buf = readRequest(client_fd))) {
			break;
		}

		if (myStrcmp(buf, "GET") || myStrcmp(buf, "POST") || myStrcmp(buf, "LIST")) {
			char *path = extractPath(buf);
			int status = path ? checkPathValidity(path, myStrcmp(buf, "LIST")) : 500;
			if (status != 200) {
				char *response = formResponse(status, nullptr);
				if (sendResponse(client_fd, response)) {
					free(path);
					free(response);
					break;
				}
				free(path);
				free(response);
				continue;
			}
			if (myStrcmp(buf, "POST")) {
				char *content = getContentToPost(buf);
				// TODO get contents
				// TODO post data
			}
			else if (myStrcmp(buf, "GET")) {
				const char *body = getFileContent(path);
				char *response = formResponse(body ? 200 : 200, body);
				free(body);
				if (sendResponse(client_fd, response)) {
					free(response);
					break;
				}
				free(response);
			}
			else {
				char *body = listDirectory(path);
				char *response = formResponse(body ? 200 : 500, body);
				free(body);
				if (sendResponse(client_fd, response)) {
					free(response);
					break;
				}
				free(response);
			}
		}

		else if (myStrcmp(buf, "STATUS") || myStrcmp(buf, "QUIT")) {
			char *response = formResponse(200, nullptr);
			if (sendResponse(client_fd, response)) {
				break;
			}
			if (myStrcmp(buf, "QUIT")) {
				break;
			}
		}

		free(buf);
  }

  close(client_fd);
}

int main() {
	int server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (server_fd < 0) {
		std::cerr << "Failed to create a socket\n";
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
