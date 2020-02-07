#include "user.h"
#include <string.h>

#define userEncodeNybbleHex(c) ((c) <= 9 ? '0' + (c) : 'A' + (c) - 10)

#define userConvertHexToByte(high, low, byte) \
	*(byte) = (((high) - ((high) <= '9' ? '0' : 'A')) * 16) + ((low) - ((low) <= '9' ? '0' : 'A'))

void userParseURIString(const char *read, const char *read_end, char *write, const char *const write_end){

	unsigned int hex = 0;

	while(read < read_end && write < write_end){

		const char r = *read;

		if(hex > 0 || r == '%'){
			switch(hex){
				case 2:
					*write = r;
					hex = 1;
				break;
				case 1:
					userConvertHexToByte(*write, r, write);
					hex = 0;
					++write;
				break;
				default:
					hex = 2;
			}
		}else{
			*write = r;
			++write;
		}

		++read;

	}

}

void userEncodeName(const char *name, const char *const end, char *encoded){

	// Encodes a username in a very strange hexadecimal format
	// that effectively doubles the length of the name.
	// This is so that Unicode characters may be supported
	// in usernames without having to set up a costly lookup
	// table between user names and IDs.
	//
	// The name string is not necessarily null-terminated.
	//
	// The reason last_byte is stored is so that this code
	// will work even when name is equal to encoded.

	char last_byte = *name;
	while(name < end){  //while(last_byte != '\0'){
		const char next_byte = *(++name);
		*encoded = userEncodeNybbleHex((last_byte & 0xF0) >> 4);
		++encoded;
		*encoded = userEncodeNybbleHex(last_byte & 0x0F);
		++encoded;
		last_byte = next_byte;
	}

}

void userHashPassword(const char *const password, char *const hash){
	// Plain-text passwords!
	memcpy(hash, password, USER_PASSWORD_BYTES);
}
