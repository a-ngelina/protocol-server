#include<arpa/inet.h>
#include<iostream>
#include<string.h>
#include<sys/socket.h>
#include<unistd.h>

char *intToStr(int x) {
	if (x < 0) {
		return nullptr;
	}

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

bool sendRequest(int server_fd, const char *request) {
	int request_len = strlen(request), request_sent = 0;
	char *len_str = intToStr(request_len);
	if (!len_str) {
		return 1;
	}
	int len_str_len = strlen(len_str), len_sent = 0, bytes_sent = 0;

	while (len_sent < len_str_len && (bytes_sent = send(server_fd, len_str + len_sent, len_str_len - len_sent, 0)) >= 0) {
		len_sent += bytes_sent;
	}
	free(len_str);
	if (bytes_sent < 0) {
		std::cerr << "Failed to send request to server\n";
		close(server_fd);
		return 1;
	}

  while (request_sent < request_len && (bytes_sent = send(server_fd, request + request_sent, request_len - request_sent, 0)) >= 0) {
    request_sent += bytes_sent;
  }
  if (bytes_sent < 0) {
    std::cerr << "Failed to send request to server\n";
    close(server_fd);
    return 1;
  }

#ifdef DEBUG
	std::cout << "Sent request to server: \"" << request << "\"\n";
#endif

	return 0;
}

bool receiveResponse(int server_fd) {
  ssize_t bytes_read = 0;
	int request_len = 0;
	char tmp[1];
	while ((bytes_read = recv(server_fd, tmp, 1, 0)) > 0 && tmp[0] != '\n') {
		if (!(tmp[0] - '0' >= 0 && tmp[0] - '0' < 10)) {
			std::cerr << "Response length not valid\n";
			close(server_fd);
			return 1;
		}
		request_len = request_len * 10 + tmp[0] - '0';
	}

	char *buf = (char *)malloc((request_len + 1) * sizeof(char));
	if (!buf) {
		std::cerr << "Failed to allocate memory for response\n";
		close(server_fd);
		return 1;
	}

	int left_to_read = request_len, read = 0;
	while (left_to_read > 0 && (bytes_read = recv(server_fd, buf + read, left_to_read, 0)) > 0) {
		left_to_read -= bytes_read;
		read += bytes_read;
	}
  if (bytes_read < 0) {
    std::cerr << "Failed to receive response from server\n";
		free(buf);
    close(server_fd);
    return 1;
  }
  buf[read] = '\0';

#ifdef DEBUG
  std::cout << "Client received response: \"" << buf << "\"\n";
#endif

	free(buf);
	return 0;
}

void quitServer(int server_fd) {
	if (sendRequest(server_fd, "QUIT")) {
		return;
	}

	if (receiveResponse(server_fd)) {
		return;
	}

	close(server_fd);
	
#ifdef DEBUG
	std::cout << "Server quit\n";
#endif
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

	if (sendRequest(server_fd, "POST test/output1.out\nposting to output1.out file")) {
		return 1;
	}

	if (receiveResponse(server_fd)) {
		return 1;
	}

	quitServer(server_fd);

	return 0;
}
