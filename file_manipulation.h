#ifndef PROTOCOL_SERVER_FILE_MANIP_H
#define PROTOCOL_SERVER_FILE_MANIP_H

int lockFile(char *path, bool exclusive);

bool unlockFile(char *path, int fd);

char* listDirectory(char *path);

char* getFileContent(char *path);

bool post(char *path, char *content);

#endif
