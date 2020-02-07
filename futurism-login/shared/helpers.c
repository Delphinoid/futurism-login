#include "helpers.h"
#include <string.h>
#include <ctype.h>

void fileProcessPath(char *const __RESTRICT__ path){
	// Replace Windows delimiters with Linux delimiters or vice versa.
	char *delimiter = path;
	while(*delimiter != '\0'){
		if(*delimiter == FILE_PATH_DELIMITER_CHAR_UNUSED){
			*delimiter = FILE_PATH_DELIMITER_CHAR;
		}
		++delimiter;
	}
}

size_t ultostr(unsigned long n, char *const __RESTRICT__ s){
	// Converts a long to a C-string.
	size_t l;  // Length of the ouput (discluding null terminator).
	if(n == 0){
		s[0] = '0';
		l = 1;
	}else{
		const size_t m = ULTOSTR_MAX_LENGTH;
		l = m;
		s[0] = '\0';
		// Loop through the number backwards.
		while(n > 0){
			--l;
			s[l] = '0' + n % 10;
			n /= 10;
		}
		// Shift everything from the end of the array over to the beginning.
		memcpy(&s[0], &s[l], m-l);
		// Add a null terminator.
		l = m-l;
		s[l+1] = '\0';
	}
	return l;
}

int strncasecmp(const char *str1, const char *str2, size_t num){
	if(num == 0){
		return 0;
	}
	while(num-- != 0 && tolower(*str1) == tolower(*str2)){
		if(num == 0 || *str1 == '\0' || *str2 == '\0'){
			break;
		}
		++str1;
		++str2;
	}
	return(tolower(*(unsigned char *)str1) - tolower(*(unsigned char *)str2));
}

size_t tokenLength(const char *const str, const size_t strLength, const char *const delims){
	// Keep looping until we get to the end of the string or a null terminator.
	const char *tokStart = str;
	while(*tokStart != '\0' && tokStart - str < strLength){
		const char *curDelim = delims;
		while(*curDelim != '\0'){
			if(*tokStart == *curDelim){
				return(tokStart - str);
			}
			++curDelim;
		}
		++tokStart;
	}
	return strLength;
}
