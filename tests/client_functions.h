#ifndef PROTOCOL_SERVER_CLIENT_FUNCTIONS_H
#define PROTOCOL_SERVER_CLIENT_FUNCTIONS_H

char *intToStr(int x);

bool sendRequest(int server_fd, const char *request);

bool receiveResponse(int server_fd);

void quitServer(int server_fd);

#endif
