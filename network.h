#ifndef PROTOCOL_SERVER_NETWORK_H
#define PROTOCOL_SERVER_NETWORK_H

char *receiveRequest(int client_fd);

bool sendResponse(int client_fd, char *response);

#endif
