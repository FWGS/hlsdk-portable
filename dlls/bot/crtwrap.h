#ifndef CRTWRAP_H
#define CRTWRAP_H

typedef long long int64;
typedef int int32;
typedef unsigned int uint32;
#define Q_stricmp strcasecmp
#define Q_max( a, b ) (((a) > (b)) ? (a) : (b))
#define Q_min( a, b ) (((a) < (b)) ? (a) : (b))
#define Q_atoi atoi
#define Q_snprintf snprintf
#define Q_strncpy strncpy
#define CloneString strdup
#define Q_atof atof
#define Q_strchr strchr
#define Q_strcmp strcmp
#define Q_strcpy strcpy
#define Q_sprintf sprintf
#define Q_strlen strlen
#define Q_write write
#define Q_close close
#define Q_strcat strcat
#define Q_stricmp strcasecmp
#define Q_strnicmp strncasecmp
#define Q_vsnprintf vsnprintf
template <typename T>
T clamp(T a, T min, T max) { return (a > max) ? max : (a < min) ? min : a; }

#endif // CRTWRAP_H

