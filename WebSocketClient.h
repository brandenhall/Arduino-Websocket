/*
Websocket-Arduino, a websocket implementation for Arduino
Copyright 2011 Per Ejeklint

Based on previous implementations by
Copyright 2010 Ben Swanson
and
Copyright 2010 Randall Brewer
and
Copyright 2010 Oliver Smith

Some code and concept based off of Webduino library
Copyright 2009 Ben Combee, Ran Talbott

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

-------------
Now based off
http://www.whatwg.org/specs/web-socket-protocol/

- OLD -
Currently based off of "The Web Socket protocol" draft (v 75):
http://tools.ietf.org/html/draft-hixie-thewebsocketprotocol-75
*/


#ifndef WEBSOCKETCLIENT_H_
#define WEBSOCKETCLIENT_H_

#include <Arduino.h>
#include <Stream.h>
#include "string.h"
#include "Client.h"

//Uncoment the following line for debug output ot serial port
//#define DEBUGGING

#if defined ESP8266 || defined ESP32 || defined ARDUINO_SAMD_MKR1000
#define WS_BUFFERED_SEND
#endif

// CRLF characters to terminate lines/handshakes in headers.
#define CRLF "\r\n"

// Amount of time (in ms) a user may be connected before getting disconnected 
// for timing out (i.e. not sending any data to the server).
#define TIMEOUT_IN_MS 10000

// ACTION_SPACE is how many actions are allowed in a program. Defaults to 
// 5 unless overwritten by user.
#ifndef CALLBACK_FUNCTIONS
#define CALLBACK_FUNCTIONS 1
#endif

// Don't allow the client to send big frames of data. This will flood the Arduinos
// memory and might even crash it.
#ifndef MAX_FRAME_LENGTH

#if defined ESP8266 || defined ESP32 || defined ARDUINO_SAMD_MKR1000
#define MAX_FRAME_LENGTH 2048
#else
#define MAX_FRAME_LENGTH 256
#endif

#endif

#define SIZE(array) (sizeof(array) / sizeof(*array))

// WebSocket protocol constants
// First byte
#define WS_FIN            0x80
#define WS_OPCODE_TEXT    0x01
#define WS_OPCODE_BINARY  0x02
#define WS_OPCODE_CLOSE   0x08
#define WS_OPCODE_PING    0x09
#define WS_OPCODE_PONG    0x0a
// Second byte
#define WS_MASK           0x80
#define WS_SIZE16         126
#define WS_SIZE64         127

  
class WebSocketClient {
public:
	WebSocketClient(char *WsPath = NULL, char *WsHost = NULL, char *WsHeaders = NULL, char *WsProtocol = NULL);
	void setPath(char* WsPath);
	void setHeaders(char *WsHeaders);
	void setHost(char * WsHost);
	void setProtocol(char * WsProtocol);
    // Handle connection requests to validate and process/refuse
    // connections.
    int handshake(Client &client);
    //Check if socket os connected
	int connected();
    // Get data off of the stream
    bool getData(String& data, uint8_t *opcode = NULL);
	bool getData(char *data, unsigned int dataLen, uint8_t *opcode = NULL);

    // Write data to the stream
    void sendData(const char *str, uint8_t opcode = WS_OPCODE_TEXT);
    void sendData(String str, uint8_t opcode = WS_OPCODE_TEXT);

#ifdef WS_BUFFERED_SEND
	int process(void);
#endif	

	void disconnect(void) {disconnectStream();};
	

private:
    Client *socket_client;
	
    const char *socket_urlPrefix;
    char *path;
    char *host;
    char *protocol;
	char *headers;

#ifdef WS_BUFFERED_SEND
	uint8_t buffer[MAX_FRAME_LENGTH];
	unsigned int bufferIndex; 
	int bufferedSend(uint8_t c);
#endif	

    // Discovers if the client's header is requesting an upgrade to a
    // websocket connection.
    int analyzeRequest();

	bool handleMessageHeader(uint8_t *msgtype, unsigned int *length, bool *hasMask, uint8_t *mask, uint8_t *opcode);

    bool handleStream(String& data, uint8_t *opcode);    
    bool handleStream(char *data, unsigned int dataLen, uint8_t *opcode);
	
    // Disconnect user gracefully.
    void disconnectStream();
    
    int timedRead();

    void sendEncodedData(char *str, uint8_t opcode);
    void sendEncodedData(String str, uint8_t opcode);
};



#endif
