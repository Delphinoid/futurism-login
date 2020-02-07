#ifndef HELPERS_H
#define HELPERS_H

#include "qualifiers.h"
#include <stddef.h>

#ifdef _WIN32

	#include <windows.h>

	#define FILE_PATH_DELIMITER_CHAR   '\\'
	#define FILE_PATH_DELIMITER_STRING "\\"
	#define FILE_PATH_DELIMITER_CHAR_UNUSED   '/'
	#define FILE_PATH_DELIMITER_STRING_UNUSED "/"

	#define S_IREAD _S_IREAD
	#define S_IWRITE _S_IWRITE
	#define S_IRUSR _S_IREAD
	#define S_IWUSR _S_IWRITE

	#define access(path, mode) _access(path, mode)
	#define mkdir(path, mode) _mkdir(path)
	#define rmdir(path) _rmdir(path)
	#define chmod(path, mode) _chmod(path, mode)

#else

	#include <sys/stat.h>
	#include <unistd.h>

	#define FILE_PATH_DELIMITER_CHAR   '/'
	#define FILE_PATH_DELIMITER_STRING "/"
	#define FILE_PATH_DELIMITER_CHAR_UNUSED   '\\'
	#define FILE_PATH_DELIMITER_STRING_UNUSED "\\"

#endif

#define FILE_MAX_PATH_LENGTH 4096

#define ULTOSTR_MAX_LENGTH 20  // Max number length for ultostr (discluding the null terminator). 64-bit just in case.

void fileProcessPath(char *const __RESTRICT__ path);
size_t ultostr(unsigned long n, char *const __RESTRICT__ s);
int strncasecmp(const char *str1, const char *str2, size_t num);
size_t tokenLength(const char *const str, const size_t strLength, const char *const delims);

#endif

