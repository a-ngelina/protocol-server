#include<cstdlib>
#include<iostream>

#include"string_utilities.h"

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
