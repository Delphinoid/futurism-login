#ifndef HTTPRESPONSE_H
#define HTTPRESPONSE_H

#include "../shared/qualifiers.h"
#include <stddef.h>

size_t httpResponse(char *const __RESTRICT__ response, const size_t contentLength, const char *const __RESTRICT__ headers, const size_t headersLength, const char *const __RESTRICT__ type, const size_t typeLength);

#endif
