#ifndef PROTOCOL_SERVER_REQUEST_RESPONSE_H
#define PROTOCOL_SERVER_REQUEST_RESPONSE_H

char* formResponse(int status, char *body);

char* checkPathValidity(char *path, int& status, bool dir, bool creat);

const char* getMessage(int status);

char* extractContentToPost(char *buf);

bool isRequestValid(char *buf, bool path_needed, bool additional_data_needed);

char* extractPath(char *buf);

#endif
