#include<arpa/inet.h>
#include<fcntl.h>
#include<sys/file.h>
#include<sys/socket.h>
#include<unistd.h>

#include<cstdlib>
#include<cstring>
#include<filesystem>
#include<fstream>
#include<iostream>
#include<thread>

const char *getMessage(int status);
char *skipWord(char *buf);
char *skipWhitespace(char *buf);

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
	int response_len = 24 + 1 + (body ? strlen(body) : 0) + 1;
  // 24 for status code + message + \n, 1 for \n, body, 1 for \0

  char *response = (char *)malloc(response_len);
	if (!response) {
		std::cerr << "Failed to allocate memory\n";
		return nullptr;
	}
  if (body) {
    snprintf(response, response_len, "%d %s\n\n%s", status, getMessage(status), body);
  }
  else {
    snprintf(response, response_len, "%d %s", status, getMessage(status));
  }

	return response;
}

char *intToStr(int x) {
	if (!x) {
		char *ans = (char *)malloc(3 * sizeof(char));
		if (!ans) {
			std::cerr << "Failed to allocate memory\n";
			return nullptr;
		}
		ans[0] = '0';
		ans[1] = '\n';
		ans[2] = '\0';
		return ans;
	}

	int log = 0, temp_x = x;
	while (temp_x) {
		temp_x /= 10;
		++log;
	}

	char *ans = (char *)malloc((log + 2) * sizeof(char));
	if (!ans) {
		std::cerr << "Failed to allocate memory\n";
		return nullptr;
	}

	int i = log - 1;
	ans[log + 1] = '\0';
	ans[log] = '\n';
	while (x) {
		ans[i--] = x % 10 + '0';
		x /= 10;
	}

	return ans;
}

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

char* checkPathValidity(char *path, int& status, bool dir, bool creat) {
	try {
		char *relative_path = path;
		while (*relative_path == '/') {
			++relative_path;
		}
		std::filesystem::path cwd = std::filesystem::current_path();
		std::filesystem::path full_path = std::filesystem::weakly_canonical(cwd / "public" / relative_path);

#ifdef DEBUG
		std::cout << "Checking validity of full path: " << full_path << "\n";
#endif

		free(path);

		if (!full_path.string().starts_with(cwd.string())) {
			status = 403;
			std::cerr << "Parent directories access forbidden\n";
			return nullptr;
		}

		if (creat) {
			std::filesystem::path parent_path = full_path.parent_path();
			if (!std::filesystem::exists(parent_path) && 
					!std::filesystem::create_directories(parent_path)) {
				status = 500;
				std::cerr << "Failed to create directory\n";
				return nullptr;
			}
			char *ret = strdup(full_path.c_str());
			status = ret ? 200 : 500;
			if (!ret) {
				std::cerr << "Failed to duplicate path\n";
			}
			return ret;
		}

		if (!std::filesystem::exists(full_path)) {
			status = 404;
			std::cerr << "Path doesn't exist\n";
			return nullptr;
		}

		if ((dir && !std::filesystem::is_directory(full_path)) || 
				(!dir && !std::filesystem::is_regular_file(full_path))) {
			status = 400;
			std::cerr << "Incorrect path type\n";
			return nullptr;
		}
		char *ret = strdup(full_path.c_str());
		status = ret ? 200 : 500;
		if (!ret) {
			std::cerr << "Failed to duplicate path\n";
		}
		return ret;
	}
	catch (const std::filesystem::filesystem_error& e) {
		std::cerr << "Filesystem error: " << e.what() << "\n";
		status = 500;
		return nullptr;
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

int lockFile(char *path, bool exclusive) {
	int fd = open(path, O_RDONLY);
	if (fd < 0) {
		std::cerr << "Failed to access " << path << "\n";
		return fd;
	}

	if (flock(fd, exclusive ? LOCK_EX : LOCK_SH)) {
		close(fd);
		std::cerr << "Failed to lock " << path << "\n";
		return -1;
	}

	return fd;
}

bool unlockFile(char *path, int fd) {
	if (flock(fd, LOCK_UN)) {
		std::cerr << "Failed to unlock " << path << "\n";
		close(fd);
		return 1;
	}

	close(fd);
	return 0;
}

char* listDirectory(char *path) {
	int fd = lockFile(path, 0);
	if (fd < 0) {
		return nullptr;
	}

	size_t cap = 1024, len = 0;
	char *response = (char *)malloc(cap * sizeof(char));
	if (!response) {
		std::cerr << "Failed to allocate memory\n";
		unlockFile(path, fd);
		return nullptr;
	}

	for (const auto& entry: std::filesystem::directory_iterator(path)) {
		const char *entry_c_str = entry.path().filename().c_str();
		size_t entry_len = strlen(entry_c_str);
		if (entry_len + len + 1 > cap - 1) {
			cap *= 2;
			char *new_response = (char *)realloc(response, cap);
			if (!new_response) {
				std::cerr << "Failed to reallocate memory\n";
				free(response);
				unlockFile(path, fd);
				return nullptr;
			}
			response = new_response;
		}

		memcpy(response + len, entry_c_str, entry_len);
		len += entry_len;
		*(response + len) = ' ';
		++len;
	}
	response[len >= 1 ? (len - 1) : 0] = '\0'; // -1 for extra space

	if (unlockFile(path, fd)) {
		free(response);
		return nullptr;
	}

	return response;
}

char* getFileContent(char *path) {
	int fd = lockFile(path, 0);
	if (fd < 0) {
		return nullptr;
	}

	std::ifstream file(path, std::ios::binary);
	if (!file) {
		std::cerr << "Error opening file " << path << "\n";
		unlockFile(path, fd);
		return nullptr;
	}

	auto file_size = std::filesystem::file_size(path);
	char *content = (char *)malloc((file_size + 1) * sizeof(char));
	if (!content) {
		std::cerr << "Failed to allocate memory\n";
		unlockFile(path, fd);
		return nullptr;
	}
	file.read(content, file_size);
	content[file_size] = '\0';
	
	if (unlockFile(path, fd)) {
		free(content);
		return nullptr;
	}

	return content;
}

char* extractContentToPost(char *buf) {
	for (int i = 0; i < 2; ++i) {
		buf = skipWord(buf);
		buf = skipWhitespace(buf);
	}

	size_t content_len = strlen(buf);
	char *content = (char *)malloc((content_len + 1) * sizeof(char));
	if (!content) {
		std::cerr << "Failed to allocate memory\n";
		return nullptr;
	}
	memcpy(content, buf, content_len);
	content[content_len] = '\0';
	return content;
}

bool post(char *path, char *content) {
	int fd = lockFile(path, 1);
	if (fd < 0) {
		return 1;
	}

	std::ofstream file(path, std::ios::trunc);
	if (!file) {
		std::cerr << "Error creating/opening file " << path << "\n";
		unlockFile(path, fd);
		return 1;
	}

	file << content;

	if (!file) {
		std::cerr << "Failed to copy contents to file " << path << "\n";
		unlockFile(path, fd);
		return 1;
	}

	if (unlockFile(path, fd)) {
		return 1;
	}
	
	return 0;
}

char* skipWord(char *buf) {
	while (*buf != '\0' && *buf != ' ' && *buf != '\n') {
		++buf;
	}
	return buf;
}

char* skipWhitespace(char *buf) {
	while (*buf == ' ' || *buf == '\n') {
		++buf;
	}
	return buf;
}

bool isRequestValid(char *buf, bool path_needed, bool additional_data_needed) {
	buf = skipWord(buf);
	buf = skipWhitespace(buf);
	if (*buf == '\0') {
		return !path_needed;
	}
	else if (!path_needed) {
		return 0;
	}

	buf = skipWord(buf);
	buf = skipWhitespace(buf);
	if (*buf == '\0') {
		return !additional_data_needed;
	}
	else if (!additional_data_needed) {
		return 0;
	}

	return 1;
}


char* extractPath(char *buf) {
	buf = skipWord(buf);
	buf = skipWhitespace(buf);

	char *start_of_path = buf;
	char *end_of_path = skipWord(buf);
	ptrdiff_t path_len = end_of_path - start_of_path;

	char *path = (char *)malloc((path_len + 1) * sizeof(char));
	if (!path) {
		std::cerr << "Failed to allocate memory\n";
		return nullptr;
	}
	memcpy(path, start_of_path, path_len);
	path[path_len] = '\0';
	return path;
}

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
				char *content = extractContentToPost(buf);
				char *response = formResponse( (!content || post(path, content)) ? 500 : 200, nullptr);
				free(buf);
				free(path);
				free(content);
				if (!response || sendResponse(client_fd, response)) {
					break;
				}
			}
			else {
				char *body = myStrcmp(buf, "GET") ? getFileContent(path) : listDirectory(path);
				char *response = formResponse(body ? 200 : 500, body);
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
