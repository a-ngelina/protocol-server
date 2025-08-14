#include<fcntl.h>
#include<sys/file.h>
#include<unistd.h>

#include<cstdlib>
#include<cstring>
#include<filesystem>
#include<fstream>
#include<iostream>

#include"string_utilities.h"
#include"request_parsing_response_preparation.h"

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
			
			if (!std::filesystem::exists(full_path)) {
				std::ofstream new_file(full_path);
				if (!new_file) {
					std::cerr << "Failed to create file\n";
					return nullptr;
				}
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
