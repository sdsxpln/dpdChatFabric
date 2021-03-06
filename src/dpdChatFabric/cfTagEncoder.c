/* 
Copyright (c) 2015, David P. Discher
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice, this
list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#include "cfTagEncoder.h"

void CP_ICACHE_FLASH_ATTR
cfTagEncoder( enum chatPacketTagData type, unsigned char *b, uint32_t *i, unsigned char tag,  uint32_t value, unsigned char*s, uint32_t len, uuuid2_t *uuid)
{

	uint32_t x = *i;
	uint32_t ni = 0;
	memcpy(b+x, &tag, 1);
	++x;
	if ( type == CP_INT32 ) {
#ifdef ESP8266	
		ni = ntohl(value);
#else 
		ni = htonl(value);
#endif
		memcpy(b+x, &ni, 4);
		x += 4;		
	} else if (  type == CP_DATA8 ) {
		memcpy(b+x, s, len);
		x += len;
	} else if (  type == CP_UUID ) {
		memcpy(b+x, &uuid->bytes, 16);
		x += 16;
	}

	*i=x;


}

extern uint32_t _data_type;


void CP_ICACHE_FLASH_ATTR
cfTagPrinter( enum chatPacketTagData type, unsigned char tag,  uint32_t value, unsigned char*s, uint32_t len, uuuid2_t *uuid)
{
#ifdef ESP8266	
	char *buf;
	unsigned char *v;
	cfTagLookup(tag);
	CHATFABRIC_DEBUG(1,"Check");	
	if ( type == CP_INT32 ) {
		if (_data_type == 0 ) {
	CHATFABRIC_DEBUG(1,"Check");	
			CHATFABRIC_PRINT_FMT( "[ %04x ] %-20s : %12d",  tag, cfTagLookup(tag), value );	
		} else if (_data_type == 1 ) {
	CHATFABRIC_DEBUG(1,"Check");	
			buf = calloc(16, sizeof(char));			
	CHATFABRIC_DEBUG(1,"Check");	
			os_sprintf(buf, IPSTR, IP2STR(value));
	CHATFABRIC_DEBUG(1,"Check");	
			CHATFABRIC_PRINT_FMT( "[ %04x ] %-20s : %s",  tag, cfTagLookup(tag), buf);	
			free(buf);		
		}		
	} else if (  type == CP_DATA8 ) {
	CHATFABRIC_DEBUG(1,"Check");	
		if (_data_type == 0  || _data_type == 2) {
	CHATFABRIC_DEBUG(1,"Check");	
			buf = calloc(len+1, sizeof(char));
			memcpy(buf, s, len);
	CHATFABRIC_DEBUG(1,"Check");	
			CHATFABRIC_PRINT_FMT( "[ %04x ] %-20s : %s",  tag, cfTagLookup(tag), buf);	
			free(buf);
		} else if (_data_type == 1 ) {
	CHATFABRIC_DEBUG(1,"Check");	
			buf = calloc(16, sizeof(char));
	CHATFABRIC_DEBUG(1,"Check");	
			os_sprintf(buf, IPSTR, IP2STR(value));
	CHATFABRIC_DEBUG(1,"Check");	
			CHATFABRIC_PRINT_FMT( "[ %04x ] %-20s : %s",  tag, cfTagLookup(tag), buf);	
			free(buf);
		}
	} else if (  type == CP_UUID ) {
	CHATFABRIC_DEBUG(1,"Check");	
		buf = calloc(37, sizeof(char));
		uuuid2_to_str(buf, 37, uuid);
		CHATFABRIC_PRINT_FMT( "[ %04x ] %-20s : %s",  tag, cfTagLookup(tag), buf);
		free(buf);
	}
	CHATFABRIC_DEBUG(1,"Check");	
#endif
}
