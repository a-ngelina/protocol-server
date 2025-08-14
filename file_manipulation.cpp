#include<fcntl.h>
#include<sys/file.h>
#include<unistd.h>

#include<cstdlib>
#include<cstring>
#include<filesystem>
#include<fstream>
#include<iostream>

#include"file_manipulation.h"

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
