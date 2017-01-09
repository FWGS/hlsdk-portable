#ifndef MP_PARSE_H
#define MP_PARSE_H

char *MP_COM_GetToken();
char *MP_COM_Parse(char *data);
int MP_COM_TokenWaiting(char *buffer);

#endif // MP_PARSE_H

