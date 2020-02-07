#ifndef HTTPREQUEST_H
#define HTTPREQUEST_H

#include "../shared/return.h"
#include "../shared/qualifiers.h"
#include <stddef.h>

typedef struct httpRequest {

	char *methodStart;
	size_t methodLength;

	char *targetStart;
	size_t targetLength;

	char *headersStart;
	size_t headersLength;

	char *contentTypeStart;
	size_t contentTypeLength;

	char *contentStart;
	size_t contentLength;

} httpRequest;

void httpRequestInit(httpRequest *const __RESTRICT__ request);
return_t httpRequestValid(httpRequest *const __RESTRICT__ request, char *const str, const size_t strLength);
char *httpRequestFindHeader(httpRequest *const __RESTRICT__ request, char *const start, const char *const __RESTRICT__ header, const size_t headerLength, size_t *const length);

#endif
