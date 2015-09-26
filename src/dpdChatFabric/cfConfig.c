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
#include "cfConfig.h"

void CP_ICACHE_FLASH_ATTR
cfConfigInit(chatFabricConfig *config) {

	config->configfile = NULL;
	config->newconfigfile = NULL;
	config->pairfile = NULL;
	config->ip = NULL;
	config->msg = NULL;
	config->controlers = NULL;
	config->numOfControllers = -1;
	config->callback = NULL;

	config->port = -1;
	config->type = -1; // SOCK_STREAM SOCK_DGRAM SOCK_RAW SOCK_RDM SOCK_SEQPACKET
	config->hasPairs = 0;

	uuidCreateNil( &(config->uuid.u0));
	uuidCreate( &(config->uuid.u1));

	//uuidCreateNil( &(config->to.u0));
	//uuidCreateNil( &(config->to.u1));

	config->debug = 0;
	config->writeconfig = 1;
	config->mode = 0;

	config->ipv4 = 0; // 1+4
	config->ipv4netmask = 0; // 1+4
	config->ipv4gw  = 0; // 1+4
	config->ipv4ns1 = 0; // 1+4
	config->ipv4ns2 = 0; // 1+4	

	static const unsigned char basepoint[32] = {9};
		
	arc4random_buf((unsigned char *)&(config->privatekey), crypto_box_SECRETKEYBYTES);
	curve25519_donna((unsigned char *)&config->publickey, (unsigned char *)&config->privatekey, (unsigned char *)&basepoint);	

}


void CP_ICACHE_FLASH_ATTR
cfConfigRead(chatFabricConfig *config) {

	struct stat fs;
	uint32_t ni;
	int len =0, i=0, filesize=0;
	unsigned char *str;
	unsigned char t;


#ifdef ESP8266
	int fp=0;
	if ( system_param_load (CP_ESP_PARAM_START_SEC, 0, &(flashConfig), 4096) == FALSE ) {
		CHATFABRIC_DEBUG_FMT(config->debug, "Read from flash failed." ); 
		return;
	}

	if ( flashConfig[0] == cftag_header ) {
		filesize=4096;	
		config->configfile = "1";
		str = &(flashConfig[0]);
	} else {

		CHATFABRIC_DEBUG_FMT(config->debug,  
			"[DEBUG][%s:%s:%d] No Saved configuration in flash. \n", 
			__FILE__, __FUNCTION__, __LINE__ );
		return;

	}
#else

	FILE *fp=0;
	
#endif

	if ( config->configfile == NULL ) 
	{
		CHATFABRIC_DEBUG_FMT(config->debug,  
			"[DEBUG][%s:%s:%d] config->configfile is not set. \n", 
			__FILE__, __FUNCTION__, __LINE__ );
			return;
		
	}
#ifndef ESP8266
	bzero(&fs, sizeof(fs));		
	fp = fopen(config->configfile,"r");	
	if ( fp == NULL ) {
		CHATFABRIC_DEBUG_FMT(config->debug,  
			"[DEBUG][%s:%s:%d] Cannot open configfile %s \n", 
			__FILE__, __FUNCTION__, __LINE__, config->configfile );
			return;
	} else {
		stat(config->configfile, &fs);
		str=(unsigned char *)calloc(fs.st_size,sizeof(unsigned char));
		fread(str, sizeof (unsigned char), fs.st_size, fp );
		filesize=fs.st_size;
		fclose(fp);
	}			
#endif
			
	i=0;
	
	while (i<filesize) 
	{
		memcpy(&t, str+i, 1);
		++i;
					
		switch (t){
			case cftag_header:
				i+=4;
			break;
			
			case cftag_hasPairs:
				memcpy(&ni, str+i, 4);
				i+=4;
				config->hasPairs = ntohl(ni);
			break;

			case cftag_mode:
				memcpy(&ni, str+i, 4);
				i+=4;
				config->mode = ntohl(ni);
			break;
			case cftag_ipv4:
				memcpy(&ni, str+i, 4);
				i+=4;
				config->ipv4 = ntohl(ni);
			break;
			case cftag_ipv4netmask:
				memcpy(&ni, str+i, 4);
				i+=4;
				config->ipv4netmask = ntohl(ni);
			break;
			case cftag_ipv4gw:
				memcpy(&ni, str+i, 4);
				i+=4;
				config->ipv4gw = ntohl(ni);
			break;
			case cftag_ipv4ns1:
				memcpy(&ni, str+i, 4);
				i+=4;
				config->ipv4ns1 = ntohl(ni);
			break;
			case cftag_ipv4ns2:
				memcpy(&ni, str+i, 4);
				i+=4;
				config->ipv4ns2 = ntohl(ni);
			break;
						
			case cftag_configLength:
				memcpy(&ni, str+i, 4);
				i+=4;
				filesize = ntohl(ni);
			break;
			case cftag_publickey:	// 1+crypto_box_SECRETKEYBYTES
				memcpy(&(config->publickey), str+i, crypto_box_PUBLICKEYBYTES);
				i+=crypto_box_PUBLICKEYBYTES;
			break;		
			case cftag_privatekey:	// 1+crypto_box_SECRETKEYBYTES
				memcpy(&(config->privatekey), str+i, crypto_box_SECRETKEYBYTES);
				i+=crypto_box_SECRETKEYBYTES;
			break;		
			case cftag_uuid0:			// 1+16
				uuidFromBytes(str+i, &config->uuid.u0);
				i+=16;
			break;		
			case cftag_uuid1:			// 1+16
				uuidFromBytes(str+i, &config->uuid.u1);
				i+=16;
			break;		
			default:
				CHATFABRIC_DEBUG_FMT(config->debug,  
					"[DEBUG][%s:%s:%d] Bad Config File Tag : %02x \n", 
					__FILE__, __FUNCTION__, __LINE__,  t );
			break;
		}
	}
}

void CP_ICACHE_FLASH_ATTR
cfConfigWrite(chatFabricConfig *config) {

	struct stat fs;
	uint32_t ni;
	int len =0, i=0, filesize=0;
	unsigned char *str;
	unsigned char t;

#ifdef ESP8266
	int fp=0;
#else 	
	FILE *fp=0;
#endif

	len+=1+crypto_box_PUBLICKEYBYTES;
	len+=1+crypto_box_SECRETKEYBYTES;
	len+=1+16; // uuid
	len+=1+16;
	len+=1+4; // header
	len+=1+4; // length
	len+=1+4; // haspairs
	
	len+=1+4; // mode
	len+=1+4; // ipv4
	len+=1+4; // ipv4nm
	len+=1+4; // ipv4gw
	len+=1+4; // ipv4ns1
	len+=1+4; // ipv4ns2
		
		
#ifdef ESP8266
	i=0;
	str = &(flashConfig[0]);
#else
	fp = fopen(config->newconfigfile,"w");
	if ( fp == 0 ) { 
		CHATFABRIC_DEBUG_FMT(config->debug,  
			"[DEBUG][%s:%s:%d] Cannot open configfile for writing %s \n", 
			__FILE__, __FUNCTION__, __LINE__, config->configfile );
		return;
	}
	i=0;
	str=(unsigned char *)calloc(len,sizeof(unsigned char));
#endif 	

	cfTagEncoder ( CP_INT32, str, (uint32_t *)&i, cftag_header, 0, NULL, 0, NULL);
	cfTagEncoder ( CP_INT32, str, (uint32_t *)&i, cftag_configLength, len, NULL, 0, NULL);
	cfTagEncoder ( CP_INT32, str, (uint32_t *)&i, cftag_hasPairs, config->hasPairs, NULL, 0, NULL);
	cfTagEncoder ( CP_UUID, str, (uint32_t *)&i, cftag_uuid0, 0, NULL, 0,  &config->uuid.u0);
	cfTagEncoder ( CP_UUID, str, (uint32_t *)&i, cftag_uuid1, 0, NULL, 0,  &config->uuid.u1);
	
	cfTagEncoder ( CP_INT32, str, (uint32_t *)&i, cftag_mode, config->mode, NULL, 0, NULL);

	cfTagEncoder ( CP_INT32, str, (uint32_t *)&i, cftag_ipv4, config->ipv4, NULL, 0, NULL);
	cfTagEncoder ( CP_INT32, str, (uint32_t *)&i, cftag_ipv4netmask, config->ipv4netmask, NULL, 0, NULL);
	cfTagEncoder ( CP_INT32, str, (uint32_t *)&i, cftag_ipv4gw, config->ipv4gw, NULL, 0, NULL);
	cfTagEncoder ( CP_INT32, str, (uint32_t *)&i, cftag_ipv4ns1, config->ipv4ns1, NULL, 0, NULL);
	cfTagEncoder ( CP_INT32, str, (uint32_t *)&i, cftag_ipv4ns2, config->ipv4ns2, NULL, 0, NULL);
	
	cfTagEncoder ( CP_DATA8, str, (uint32_t *)&i, cftag_publickey, 0,(unsigned char *)&(config->publickey), crypto_box_PUBLICKEYBYTES, NULL);
	cfTagEncoder ( CP_DATA8, str, (uint32_t *)&i, cftag_privatekey, 0,(unsigned char *)&(config->privatekey), crypto_box_SECRETKEYBYTES, NULL);

#ifdef ESP8266
	if ( system_param_save_with_protect (CP_ESP_PARAM_START_SEC, &(flashConfig[0]), 4096) == FALSE ) {
		CHATFABRIC_DEBUG_FMT(1,  
			"[DEBUG][%s:%s:%d] Failed to Save Config to Flash\n", 
			__FILE__, __FUNCTION__, __LINE__ );
			return;
	} else {
		CHATFABRIC_DEBUG_FMT(1,  
			"[DEBUG][%s:%s:%d] Save Succesful.\n", 
			__FILE__, __FUNCTION__, __LINE__ );						
	}

#else
	int fwi = fwrite (str, sizeof (unsigned char), len, fp );
	
	if ( fwi == 0 ) {
		CHATFABRIC_DEBUG_FMT(config->debug,  
			"[DEBUG][%s:%s:%d] cf Config Write ERROR  errno %d, %s, =%s=\n",
			__FILE__, __FUNCTION__, __LINE__,  errno, strerror(errno), config->configfile  );	
	}

	int fci = fclose(fp);

	if ( fci != 0 ) {
		CHATFABRIC_DEBUG_FMT(config->debug,  
			"[DEBUG][%s:%s:%d] cf Config fclose ERROR  errno %d, %s, =%s=\n",
			__FILE__, __FUNCTION__, __LINE__,  errno, strerror(errno), config->configfile  );		
	}
	
	CHATFABRIC_DEBUG_FMT(config->debug,  
		"[DEBUG][%s:%s:%d] cf Config Write (%d):(%d) \n",
		__FILE__, __FUNCTION__, __LINE__, fwi, fci );
	
	free(str);

#endif


}

/*

void CP_ICACHE_FLASH_ATTR
cfConfigParse(chatFabricConfig *config) 
{

#ifdef ESP8266
	int fp=0;
#else
	FILE *fp=0;
#endif
	struct stat fs;
	uint32_t ni;
	int len =0, i=0, filesize=0;
	unsigned char *str;
	unsigned char t;
//	enum chatFabricConfigTags t;
#ifdef ESP8266

	if ( system_param_load (CP_ESP_PARAM_START_SEC, 0, &(flashConfig), 4096) == FALSE ) {
		CHATFABRIC_DEBUG_FMT(config->debug, "Read from flash failed." ); 	
	}

	if ( flashConfig[0] == cftag_header ) {
		filesize=4096;	
		config->configfile = "1";
	} else {

		CHATFABRIC_DEBUG_FMT(config->debug,  
			"[DEBUG][%s:%s:%d] Unconfigured. \n", 
			__FILE__, __FUNCTION__, __LINE__ );

		uint32_t status;
		static const unsigned char basepoint[32] = {9};
        config->writeconfig = 1;
		config->hasPairs = 0;
        config->configfile = NULL;
		config->pairfile = NULL;
		config->callback = NULL;
		
		filesize=0;
		// These are created in args, so for the embedded solution
		// putting here.
		uuidCreateNil( &(config->to.u0));
		uuidCreateNil( &(config->to.u1));
		uuidCreateNil( &(config->uuid.u0));
		uuidCreate( &(config->uuid.u1));
		arc4random_buf((unsigned char *)&(config->privatekey), crypto_box_SECRETKEYBYTES);
		curve25519_donna((unsigned char *)&config->publickey, (unsigned char *)&config->privatekey, (unsigned char *)&basepoint);	

	}
	str = &(flashConfig[0]);
	
#endif

	
	if ( config->configfile != NULL ) 
	{
#ifndef ESP8266
			bzero(&fs, sizeof(fs));		
			fp = fopen(config->configfile,"r");	
			if ( fp == NULL ) {
				fprintf(stderr, " Error, can't open file %s \n", config->configfile );
				filesize=0;
			} else {
				stat(config->configfile, &fs);
				str=(unsigned char *)calloc(fs.st_size,sizeof(unsigned char));
				fread(str, sizeof (unsigned char), fs.st_size, fp );
				filesize=fs.st_size;
				fclose(fp);
			}			
#endif
			
			i=0;
			
			while (i<filesize) 
			{
				memcpy(&t, str+i, 1);
				++i;
							
				switch (t){
					case cftag_header:
						i+=4;
					break;
					
					case cftag_hasPairs:
						memcpy(&ni, str+i, 4);
						i+=4;
						config->hasPairs = ntohl(ni);
					break;

					case cftag_mode:
						memcpy(&ni, str+i, 4);
						i+=4;
						config->mode = ntohl(ni);
					break;
					case cftag_ipv4:
						memcpy(&ni, str+i, 4);
						i+=4;
						config->ipv4 = ntohl(ni);
					break;
					case cftag_ipv4netmask:
						memcpy(&ni, str+i, 4);
						i+=4;
						config->ipv4netmask = ntohl(ni);
					break;
					case cftag_ipv4gw:
						memcpy(&ni, str+i, 4);
						i+=4;
						config->ipv4gw = ntohl(ni);
					break;
					case cftag_ipv4ns1:
						memcpy(&ni, str+i, 4);
						i+=4;
						config->ipv4ns1 = ntohl(ni);
					break;
					case cftag_ipv4ns2:
						memcpy(&ni, str+i, 4);
						i+=4;
						config->ipv4ns2 = ntohl(ni);
					break;
								
					case cftag_configLength:
						memcpy(&ni, str+i, 4);
						i+=4;
						filesize = ntohl(ni);
					break;
					case cftag_publickey:	// 1+crypto_box_SECRETKEYBYTES
						memcpy(&(config->publickey), str+i, crypto_box_PUBLICKEYBYTES);
						i+=crypto_box_PUBLICKEYBYTES;
					break;		
					case cftag_privatekey:	// 1+crypto_box_SECRETKEYBYTES
						memcpy(&(config->privatekey), str+i, crypto_box_SECRETKEYBYTES);
						i+=crypto_box_SECRETKEYBYTES;
					break;		
					case cftag_uuid0:			// 1+16
						uuidFromBytes(str+i, &config->uuid.u0);
						i+=16;
					break;		
					case cftag_uuid1:			// 1+16
						uuidFromBytes(str+i, &config->uuid.u1);
						i+=16;
					break;		
					default:
						CHATFABRIC_DEBUG_FMT(config->debug,  
							"[DEBUG][%s:%s:%d] Bad Config File Tag : %02x \n", 
							__FILE__, __FUNCTION__, __LINE__,  t );
					break;
				}
			}
		}

	if ( config->writeconfig ) 
	{
		len+=1+crypto_box_PUBLICKEYBYTES;
		len+=1+crypto_box_SECRETKEYBYTES;
		len+=1+16; // uuid
		len+=1+16;
		len+=1+4; // header
		len+=1+4; // length
		len+=1+4; // haspairs
		
		len+=1+4; // mode
		len+=1+4; // ipv4
		len+=1+4; // ipv4nm
		len+=1+4; // ipv4gw
		len+=1+4; // ipv4ns1
		len+=1+4; // ipv4ns2
		
		
		

#ifdef ESP8266
		if ( 1 ) 
		{
			i=0; // FIXME: need to make this the correct offset in flash.

#else
		fp = fopen(config->newconfigfile,"w");		
		if ( fp != 0 )
		{
			i=0;
			str=(unsigned char *)calloc(len,sizeof(unsigned char));
#endif 	

			cfTagEncoder ( CP_INT32, str, (uint32_t *)&i, cftag_header, 0, NULL, 0, NULL);
			cfTagEncoder ( CP_INT32, str, (uint32_t *)&i, cftag_configLength, len, NULL, 0, NULL);
			cfTagEncoder ( CP_INT32, str, (uint32_t *)&i, cftag_hasPairs, config->hasPairs, NULL, 0, NULL);
			cfTagEncoder ( CP_UUID, str, (uint32_t *)&i, cftag_uuid0, 0, NULL, 0,  &config->uuid.u0);
			cfTagEncoder ( CP_UUID, str, (uint32_t *)&i, cftag_uuid1, 0, NULL, 0,  &config->uuid.u1);
			
			cfTagEncoder ( CP_INT32, str, (uint32_t *)&i, cftag_mode, config->mode, NULL, 0, NULL);

			cfTagEncoder ( CP_INT32, str, (uint32_t *)&i, cftag_ipv4, config->ipv4, NULL, 0, NULL);
			cfTagEncoder ( CP_INT32, str, (uint32_t *)&i, cftag_ipv4netmask, config->ipv4netmask, NULL, 0, NULL);
			cfTagEncoder ( CP_INT32, str, (uint32_t *)&i, cftag_ipv4gw, config->ipv4gw, NULL, 0, NULL);
			cfTagEncoder ( CP_INT32, str, (uint32_t *)&i, cftag_ipv4ns1, config->ipv4ns1, NULL, 0, NULL);
			cfTagEncoder ( CP_INT32, str, (uint32_t *)&i, cftag_ipv4ns2, config->ipv4ns2, NULL, 0, NULL);
			
			cfTagEncoder ( CP_DATA8, str, (uint32_t *)&i, cftag_publickey, 0,(unsigned char *)&(config->publickey), crypto_box_PUBLICKEYBYTES, NULL);
			cfTagEncoder ( CP_DATA8, str, (uint32_t *)&i, cftag_privatekey, 0,(unsigned char *)&(config->privatekey), crypto_box_SECRETKEYBYTES, NULL);

#ifdef ESP8266
		if ( system_param_save_with_protect (CP_ESP_PARAM_START_SEC, &(flashConfig[0]), 4096) == FALSE ) {
			CHATFABRIC_DEBUG_FMT(1,  
				"[DEBUG][%s:%s:%d] Failed to Save Config to Flash\n", 
				__FILE__, __FUNCTION__, __LINE__ );
		} else {
			CHATFABRIC_DEBUG_FMT(1,  
				"[DEBUG][%s:%s:%d] Save Succesful.\n", 
				__FILE__, __FUNCTION__, __LINE__ );		
//			util_print_bin2hex((unsigned char *)&flashConfig, 256);	
				
		}

#else
			int fwi = fwrite (str, sizeof (unsigned char), len, fp );
			int fci = fclose(fp);
			CHATFABRIC_DEBUG_FMT(config->debug,  
				"[DEBUG][%s:%s:%d] cf Config Write (%d):(%d) \n",
				__FILE__, __FUNCTION__, __LINE__, fwi, fci );
			
			free(str);
#endif
		} else {
		CHATFABRIC_DEBUG_FMT(config->debug,  
			"[DEBUG][%s:%s:%d] cf Config Write ERROR  errno %d, %s, =%s=\n",
			__FILE__, __FUNCTION__, __LINE__,  errno, strerror(errno), config->configfile  );
		
		}
	}

}

*/
