#include "httpResponse.h"
#include "../shared/helpers.h"
#include <string.h>

size_t httpResponse(char *const __RESTRICT__ response, const size_t contentLength, const char *const __RESTRICT__ headers, const size_t headersLength, const char *const __RESTRICT__ type, const size_t typeLength){

	char conLen[ULTOSTR_MAX_LENGTH + 1];
	const size_t conLenSize = ultostr(contentLength, conLen);
	size_t responseLength = 17 + headersLength + 16 + conLenSize + 16 + typeLength + 4;

	// Move the content.
	memmove(response + responseLength, response, contentLength);

	// Set up the header.
	memcpy(response, "HTTP/1.1 200 OK\r\n", 17);
	responseLength = 17;
	memcpy(response + responseLength, headers, headersLength);
	responseLength += headersLength;
	memcpy(response + responseLength, "Content-Length: ", 16);
	responseLength += 16;
	memcpy(response + responseLength, conLen, conLenSize);
	responseLength += conLenSize;
	memcpy(response + responseLength, "\r\nContent-Type: ", 16);
	responseLength += 16;
	memcpy(response + responseLength, type, typeLength);
	responseLength += typeLength;
	memcpy(response + responseLength, "\r\n\r\n", 4);

	// Add a null terminator.
	responseLength += 4 + contentLength;
	response[responseLength] = '\0';

	return responseLength;

}
