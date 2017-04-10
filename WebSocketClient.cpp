//#define DEBUGGING

#include "global.h"
#include "WebSocketClient.h"

#include "sha1.h"
#include "Base64.h"

WebSocketClient::WebSocketClient(char *WsPath, char *WsHost, char *WsHeaders, char *WsProtocol){
	path = WsPath;
	host = WsHost;
	headers = WsHeaders;
	protocol = WsProtocol;
#ifdef WS_BUFFERED_SEND	
	bufferIndex = 0;
#endif
}

void WebSocketClient::setHost(char* WsHost) {
	host = WsHost;
}
void WebSocketClient::setProtocol(char* WsProtocol) {
	protocol = WsProtocol;
}
void WebSocketClient::setPath(char* WsPath) {
	path = WsPath;
}
void WebSocketClient::setHeaders(char* WsHeaders) {
	headers = WsHeaders;
}


int WebSocketClient::handshake(Client &client) {

    socket_client = &client;

    // If there is a connected client->
    if (socket_client->connected()) {
        // Check request and look for websocket handshake
#ifdef DEBUGGING
            Serial.println(F("Client connected"));
#endif
		int res = analyzeRequest();
        if (res==101) {
#ifdef DEBUGGING
                Serial.println(F("Websocket established"));
#endif

                return res;

        } else {
            // Might just need to break until out of socket_client loop.
#ifdef DEBUGGING
            Serial.println(F("Invalid handshake"));
#endif
            disconnectStream();

            return res;
        }
    } else {
        return -1;
    }
}

int WebSocketClient::analyzeRequest() {
    String temp;

    int bite;
    bool foundupgrade = false;
    unsigned long intkey[2];
	String sAnswerCode;
	int answerCode=0;
    String serverKey;
    char keyStart[17];
    char b64Key[25];
    String key = "------------------------";

    randomSeed(analogRead(0));

    for (int i=0; i<16; ++i) {
        keyStart[i] = (char)random(1, 256);
    }

    base64_encode(b64Key, keyStart, 16);

    for (int i=0; i<24; ++i) {
        key[i] = b64Key[i];
    }

#ifdef DEBUGGING
    Serial.println(F("Sending websocket upgrade headers"));
#endif    
#ifndef WS_BUFFERED_SEND
    socket_client->print(F("GET "));
    socket_client->print(path);
    socket_client->print(F(" HTTP/1.1\r\n"));
    socket_client->print(F("Upgrade: websocket\r\n"));
    socket_client->print(F("Connection: Upgrade\r\n"));
    socket_client->print(F("Host: "));
    socket_client->print(host);
    socket_client->print(F(CRLF)); 
	if(headers) {
		socket_client->print(headers);
		socket_client->print(F(CRLF));
	}	
    socket_client->print(F("Sec-WebSocket-Key: "));
    socket_client->print(key);
    socket_client->print(F(CRLF));
	if(protocol!=NULL) {
		socket_client->print(F("Sec-WebSocket-Protocol: "));
		socket_client->print(protocol);
		socket_client->print(F(CRLF));
	}	
    socket_client->print(F("Sec-WebSocket-Version: 13\r\n"));
    socket_client->print(F(CRLF));
#else
	bufferIndex = 0;
	strncpy_P((char *)&buffer[bufferIndex],(const char *)F("GET "), 4);
	bufferIndex+=4;
	strcpy((char *)&buffer[bufferIndex],path);
	bufferIndex+=strlen(path);
    strncpy_P((char *)&buffer[bufferIndex],(const char *)F(" HTTP/1.1\r\n"), 11);
	bufferIndex+=11;
    strncpy_P((char *)&buffer[bufferIndex],(const char *)F("Upgrade: websocket\r\n"), 20);
	bufferIndex+=20;
    strncpy_P((char *)&buffer[bufferIndex],(const char *)F("Connection: Upgrade\r\n"), 21);
	bufferIndex+=21;
    strncpy_P((char *)&buffer[bufferIndex],(const char *)F("Host: "), 6);
	bufferIndex+=6;
    strcpy((char *)&buffer[bufferIndex],host);
	bufferIndex+=strlen(host);
    strncpy_P((char *)&buffer[bufferIndex],(const char *)F(CRLF), 2);
	bufferIndex+=2;	
	if(headers) {
		strcpy((char *)&buffer[bufferIndex],headers);
		bufferIndex+=strlen(headers);
		strncpy_P((char *)&buffer[bufferIndex],(const char *)F(CRLF), 2);
		bufferIndex+=2;	
	}	
    strncpy_P((char *)&buffer[bufferIndex],(const char *)F("Sec-WebSocket-Key: "), 19);
	bufferIndex+=19;	
    strcpy((char *)&buffer[bufferIndex],&key[0]);
	bufferIndex+=key.length();
	strncpy_P((char *)&buffer[bufferIndex],(const char *)F(CRLF), 2);
	bufferIndex+=2;	
	if(protocol!=NULL) {
		strncpy_P((char *)&buffer[bufferIndex],(const char *)F("Sec-WebSocket-Protocol: "), 24);
		bufferIndex+=24;
		strcpy((char *)&buffer[bufferIndex],protocol);
		bufferIndex+=strlen(protocol);
		strncpy_P((char *)&buffer[bufferIndex],(const char *)F(CRLF), 2);
		bufferIndex+=2;	
	}	
    strncpy_P((char *)&buffer[bufferIndex],(const char *)F("Sec-WebSocket-Version: 13\r\n"), 27);
	bufferIndex+=27;	
	strncpy_P((char *)&buffer[bufferIndex],(const char *)F(CRLF), 2);
	bufferIndex+=2;
	if(socket_client->write(buffer, bufferIndex)){
#ifdef DEBUGGING
		Serial.print("Sending: ");
		int i;
		for(i=0; i<bufferIndex; i++) Serial.write(buffer[i]);
#endif		
		bufferIndex = 0;
	} else {
#ifdef DEBUGGING
		Serial.println("Error Sending");
#endif		
		return -1;
	}

#endif
#ifdef DEBUGGING
    Serial.println(F("Analyzing response headers"));
#endif    

    while (socket_client->connected() && !socket_client->available()) {
        delay(100);
#ifdef DEBUGGING
        Serial.println("Waiting...");
#endif
    }
#ifdef DEBUGGING
	if(!socket_client->connected()) Serial.println("Error. Broken connection");
#endif
    // TODO: More robust string extraction
    while ((bite = socket_client->read()) != -1) {

        temp += (char)bite;
        if ((char)bite == '\n') {
			if(temp.startsWith("\r\n")) {
#ifdef DEBUGGING
				Serial.println("End of headers");
#endif			
				break;
			} else if (!foundupgrade && temp.startsWith("Upgrade: websocket")) {
				foundupgrade = true;
			} else if (temp.startsWith("Sec-WebSocket-Accept: ")) {
				serverKey = temp.substring(22,temp.length() - 2); // Don't save last CR+LF
			} else if(temp.startsWith("HTTP/1.1 ")){
				int i;
				for(i=9; i< temp.length(); i++) if (!isDigit(temp[i])) break;
				sAnswerCode = temp.substring(9,i);
				answerCode = atoi(&sAnswerCode[0]);
#ifdef DEBUGGING
				Serial.print("Answer Code: ");
				Serial.println(answerCode);
#endif
			} 
#ifdef DEBUGGING
			Serial.print("Got Header: " + temp);
#endif			
            temp = "";		
        }

        if (!socket_client->available()) {
          delay(20);
        }
    }

    key += "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
    uint8_t *hash;
    char result[21];
    char b64Result[30];

    Sha1.init();
    Sha1.print(key);
    hash = Sha1.result();

    for (int i=0; i<20; ++i) {
        result[i] = (char)hash[i];
    }
    result[20] = '\0';

    base64_encode(b64Result, result, 20);

    // if the keys match, good to go
	if (answerCode==101 && serverKey.equals(String(b64Result)) || answerCode!=101 && answerCode!=0) return answerCode;
    else return -1;
}

bool WebSocketClient::handleMessageHeader(uint8_t *msgtype, unsigned int *length, bool *hasMask, uint8_t *mask, uint8_t *opcode) {
    if (!socket_client->connected() || !socket_client->available()) {
        return false;
    }      

    *msgtype = timedRead();
    if (!socket_client->connected()) {
        return false;
    }

    *length = timedRead();

    if (*length & WS_MASK) {
        *hasMask = true;
        *length = *length & ~WS_MASK;
    }


    if (!socket_client->connected()) {
        return false;
    }

    if (*length == WS_SIZE16) {
        *length = timedRead() << 8;
        if (!socket_client->connected()) {
            return false;
        }
            
        *length |= timedRead();
        if (!socket_client->connected()) {
            return false;
        }   

    } else if (*length == WS_SIZE64) {
#ifdef DEBUGGING
        Serial.println(F("No support for over 16 bit sized messages"));
#endif
        return false;
    }

    if (*hasMask) {
        // get the mask
        mask[0] = timedRead();
        if (!socket_client->connected()) {
            return false;
        }

        mask[1] = timedRead();
        if (!socket_client->connected()) {

            return false;
        }

        mask[2] = timedRead();
        if (!socket_client->connected()) {
            return false;
        }

        mask[3] = timedRead();
        if (!socket_client->connected()) {
            return false;
        }
    }
              
    if (opcode != NULL)
    {
      *opcode = *msgtype & ~WS_FIN;
    }
	
	return true;
}

bool WebSocketClient::handleStream(String& data, uint8_t *opcode) {
    uint8_t msgtype;
    unsigned int length;
    uint8_t mask[4];
    bool hasMask = false;

	if(!handleMessageHeader(&msgtype, &length, &hasMask, mask, opcode)) return false;	
	    
    data = "";
        
    if (hasMask) {
        for (int i=0; i<length; ++i) {
            data += (char) (timedRead() ^ mask[i % 4]);
            if (!socket_client->connected()) {
                return false;
            }
        }
    } else {
        for (int i=0; i<length; ++i) {
            data += (char) timedRead();
            if (!socket_client->connected()) {
                return false;
            }
        }            
    }
    
    return true;
}

bool WebSocketClient::handleStream(char *data, unsigned int dataLen, uint8_t *opcode) {
    uint8_t msgtype;
    unsigned int length;
    uint8_t mask[4];
    bool hasMask = false;

	if(!handleMessageHeader(&msgtype, &length, &hasMask, mask, opcode)) return false;	
	
	int i;
	int limit = length>dataLen?dataLen:length;
    if (hasMask) {
        for (i=0; i<limit; ++i) {
            data[i] = (char) (timedRead() ^ mask[i % 4]);
            if (!socket_client->connected()) {
                return false;
            }
        }
    } else {
        for (i=0; i<limit; ++i) {
            data[i] = (char) timedRead();
            if (!socket_client->connected()) {
                return false;
            }
        }            
    }
    data[i] = '\0';
    return true;
}

void WebSocketClient::disconnectStream() {
#ifdef DEBUGGING
    Serial.println(F("Terminating socket"));
#endif
    // Should send 0x8700 to server to tell it I'm quitting here.
    socket_client->write((uint8_t) 0x87);
    socket_client->write((uint8_t) 0x00);
    
    socket_client->flush();
    delay(10);
    socket_client->stop();
}

int WebSocketClient::connected(void) {	
	return socket_client->connected();
}

bool WebSocketClient::getData(String& data, uint8_t *opcode) {
    return handleStream(data, opcode);
}    

bool WebSocketClient::getData(char *data, unsigned int dataLen, uint8_t *opcode) {
    return handleStream(data, dataLen, opcode);
}    

void WebSocketClient::sendData(const char *str, uint8_t opcode) {
#ifdef DEBUGGING
    if((char)str[0]!=0) { 
		Serial.print(F("Sending data: "));
		Serial.println(str);
	}	
#endif
    if (socket_client->connected()) {
        sendEncodedData(str, opcode);       
    }
}

void WebSocketClient::sendData(String str, uint8_t opcode) {
#ifdef DEBUGGING
    if((char)str[0]!=0) { 
		Serial.print(F("Sending data: "));
		Serial.println(str);
	}	
#endif
    if (socket_client->connected()) {
        sendEncodedData(str, opcode);
    }
}

int WebSocketClient::timedRead() {
  while (!socket_client->available()) {
    delay(20);  
  }
#ifdef DEBUGGING
  char c = socket_client->read();
  Serial.println(c);
  return c;
#else  
  return socket_client->read();
#endif
}

#ifdef WS_BUFFERED_SEND
int WebSocketClient::bufferedSend(uint8_t c) {
	if(bufferIndex<MAX_FRAME_LENGTH) { //Add byte to buffer if space available
		 buffer[bufferIndex++] = c;
		 return 1;
	} else { //Buffer is full
		if(process()) {
			buffer[bufferIndex++] = c;
			return 1;
		} else return 0;
	}
}

int WebSocketClient::process(void) {
	if(bufferIndex>0) {
		if(socket_client->write(buffer, bufferIndex)) {
			bufferIndex = 0;
			return 1;
		} else {
			//Error sending. Most probable thing is socket disconnection
			//Serial.println("######################################################################SEND FAILED");
			return 0;
		}
	}
}
#endif

void WebSocketClient::sendEncodedData(char *str, uint8_t opcode) {
    uint8_t header[8];
    int size = strlen(str);
	int i = 0;

    // Opcode; final fragment
	header[i++] = opcode | WS_FIN;

    // NOTE: no support for > 16-bit sized messages
    if (size > 125) {
		header[i++] = WS_SIZE16 | WS_MASK;
		header[i++] = (uint8_t) (size >> 8);
		header[i++] = (uint8_t) (size & 0xFF);
    } else {
		header[i++] = (uint8_t) size | WS_MASK;
    }
#ifdef DEBUGGING
	Serial.print("Sending message. Header: ");
	int j;
	for(j=0; j<i; j++){
		Serial.print(header[j]);
		Serial.print(" ");
	}	
	Serial.println();
#endif
	int mask_base = i;
    header[i++] = random(0, 127);
    header[i++] = random(0, 127);
    header[i++] = random(0, 127);
    header[i++] = random(0, 127);

#ifdef WS_BUFFERED_SEND	
#ifdef DEBUGGING
	Serial.print("Sending: ");
#endif
	for(int k=0; k<i; k++) {
		if(!bufferedSend(header[k])) break;
#ifdef DEBUGGING
		Serial.write(header[k]);
#endif			
	}
	for(int k=0; k<size; k++) {
		char c = str[k] ^ header[mask_base + (k % 4)];
		if(!bufferedSend(c)) break;
#ifdef DEBUGGING
		Serial.write(c);
#endif			
	}
#else /*WS_BUFFERED_SEND*/
#ifdef DEBUGGING
	Serial.print("Sending: ");
	for(int k=0; k<i; k++) Serial.write(header[k]);
#endif
	if(socket_client->write(header, i)) {
		for (int j=0; j<size; ++j) {
			char c = str[j] ^ header[mask_base + (j % 4)];
#ifdef DEBUGGING
			Serial.write(c);
#endif			
			socket_client->write(c);
		}
    }
#ifdef DEBUGGING
	Serial.println();
#endif
#endif /*WS_BUFFERED_SEND*/	
}

void WebSocketClient::sendEncodedData(String str, uint8_t opcode) {
    int size = str.length() + 1;
    char cstr[size];

    str.toCharArray(cstr, size);

    sendEncodedData(cstr, opcode);
}
