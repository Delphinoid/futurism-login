#include "httpRequest.h"
#include "../shared/helpers.h"
#include <stdlib.h>
#include <string.h>

void httpRequestInit(httpRequest *const __RESTRICT__ request){
	request->methodStart = NULL;
	request->methodLength = 0;
	request->targetStart = NULL;
	request->targetLength = 0;
	request->headersStart = NULL;
	request->headersLength = 0;
	request->contentTypeStart = NULL;
	request->contentTypeLength = 0;
	request->contentStart = NULL;
	request->contentLength = 0;
}

return_t httpRequestValid(httpRequest *const __RESTRICT__ request, char *const str, const size_t strLength){

	// Check if a request is valid, storing the type and target.
	// Returns 0 on failure or the character we reached if successful.
	char *tokStart = str;
	size_t tokLength;

	httpRequestInit(request);

	// Get the type.
	if(tokStart >= str+strLength){
		return 0;
	}else{
		tokLength = tokenLength(tokStart, strLength - (tokStart - str), " ");
		if(tokLength == 4 && memcmp(tokStart, "POST", 4) == 0){
			request->methodStart = tokStart;
			request->methodLength = 4;
		}else if(tokLength == 3 && memcmp(tokStart, "GET", 3) == 0){
			request->methodStart = tokStart;
			request->methodLength = 3;
		}else{
			return 0;
		}
		tokStart += tokLength + 1;
	}

	// Get the target.
	if(tokStart >= str+strLength){
		return 0;
	}else{
		tokLength = tokenLength(tokStart, strLength - (tokStart - str), " ");
		request->targetStart = tokStart;
		request->targetLength = tokLength;
		tokStart += tokLength + 1;
	}

	// Get the version.
	if(
		tokStart >= str+strLength &&
		(
			tokLength = tokenLength(tokStart, strLength - (tokStart - str), " \r\n"),
			memcmp(tokStart, "HTTP/1.1", 8) != 0
		)
	){
		return 0;
	}else{
		tokStart += 10;
		request->headersStart = tokStart;
	}

	// Get the headers.
	while(tokStart < str+strLength){

		tokLength = tokenLength(tokStart, strLength - (tokStart - str), "\r\n");

		if(request->methodLength == 4){

			if(tokLength > 14 && strncasecmp(tokStart, "content", 7) == 0){
				// Content type and length.
				if(strncasecmp(tokStart + 7, "-type: ", 7) == 0){
					request->contentTypeStart = tokStart + 14;
					request->contentTypeLength = tokLength - 14;
				}else if(tokLength > 16 && strncasecmp(tokStart + 7, "-length: ", 9) == 0){
					request->contentLength = strtol(tokStart + 16, NULL, 10);
				}
			}

		}

		if(tokLength == 0){
			request->headersLength = tokStart - request->headersStart - 2;
			if(request->contentLength > 0){
				// Make sure we skip trailing CRLF characters.
				request->contentStart = tokStart + 2;
			}
			return 1;
		}

		// Make sure we skip trailing CRLF characters.
		tokStart += tokLength + 2;

	}

	return 0;

}

char *httpRequestFindHeader(httpRequest *const __RESTRICT__ request, char *const start, const char *const __RESTRICT__ header, const size_t headerLength, size_t *const __RESTRICT__ length){

	// Find a HTTP header in a request.
	char *const strStart = start > request->headersStart ? start : request->headersStart;
	const size_t strLength = request->headersLength - (strStart - request->headersStart);
	char *tokStart = strStart;
	size_t tokLength;

	// Get the headers.
	while(tokStart < strStart+strLength){

		tokLength = tokenLength(tokStart, strLength - (tokStart - strStart), "\r\n");

		if(tokLength > headerLength+2 && strncasecmp(tokStart, header, headerLength) == 0 && memcmp(tokStart+headerLength, ": ", 2) == 0){
			if(length != NULL){
				*length = tokLength - headerLength - 2;
			}
			return tokStart + headerLength + 2;
		}

		// Make sure we skip trailing CRLF characters.
		tokStart += tokLength + 2;

	}

	return NULL;

}
