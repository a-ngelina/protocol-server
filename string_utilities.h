#ifndef PROTOCOL_SERVER_STRING_UTILITIES_H
#define PROTOCOL_SERVER_STRING_UTILITIES_H

bool myStrcmp(const char *s1, const char *s2);

char *intToStr(int x);

char* skipWord(char *buf);

char* skipWhitespace(char *buf);

#endif
