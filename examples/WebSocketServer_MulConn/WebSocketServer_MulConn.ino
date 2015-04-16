/*
 * 		This is a demo to show Websocket Server running on Arduino can handle 
 * concurrent multiple connection from clients.
 *
 * 		It also handles broadcast to let all clients to know updates about the states
 * they're watching and synchronizing these updates to UI on the webpage.
 *
 * 		You can open multiple WebSocketServer_MulConn_ClientWebpage.html in your browser
 * to see how it working and synchronize the states together!
 * (Don't forget to change websocket IP server corresponding to your arduino IP in the webpage!)
 *
 * 		This example running with Arduino Websocket Server library getting from
 * brandenhall/Arduino-Websocket. Thanks for your great lib.
 *
 *  Created on: Apr 8, 2015
 *      Author: HieuNT | hieucdtspk@gmail.com
 *
 *  Copyright (c) 2015 HieuNT
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <SPI.h>
#include "Ethernet.h"

// Enabe debug tracing to Serial port.
#define DEBUGGING

// Here we define a maximum framelength to 64 bytes. Default is 256.
#define MAX_FRAME_LENGTH 64

// Define how many callback functions you have. Default is 1.
#define CALLBACK_FUNCTIONS 1

#include <WebSocketServer.h>

EthernetServer server(80);
// WebSocketServer webSocketServer;

#define MAX_CLIENT_NUM   4

typedef struct {
  EthernetClient client;
  WebSocketServer webSocketServer;
  unsigned long previousMillis;
} WebSocketStack_T;

WebSocketStack_T webSocketStack[MAX_CLIENT_NUM];

//byte mac[] = { 0x52, 0x4F, 0x43, 0x4B, 0x45, 0x54 };
//byte ip[] = { 192, 168, 1 , 101 };

byte mac[] = {0x00, 0xAA, 0xBB, 0xCC, 0xDE, 0x02};
IPAddress ip(192, 168, 1, 177);

// Called when a new message from the WebSocket is received
// Looks for a message in this form:
//
// DPV
//
// Where: 
//        D is either 'd' or 'a' - digital or analog
//        P is a pin #
//        V is the value to apply to the pin
//

void handleClientData(String &dataString) {
  bool isDigital = dataString[0] == 'd';
  int pin = dataString[1] - '0';
  int value;

  value = dataString[2] - '0';

    
  pinMode(pin, OUTPUT);
   
  if (isDigital) {
    digitalWrite(pin, value);
  } else {
    analogWrite(pin, value);
  }
    
  Serial.println(dataString);
}

// send the client the analog value of a pin
void sendClientData(int id, int pin) {
  String data = "a";
  
  pinMode(pin, INPUT);
  data += String(pin) + String(analogRead(pin));
  //Serial.print("Send\n\r");
  webSocketStack[id].webSocketServer.sendData(data);  
}

void sendClientData(int id, String s) {
  webSocketStack[id].webSocketServer.sendData(s);  
}

void setup() {
  Serial.begin(57600);
  
  // start the Ethernet connection:
  Serial.println("Trying to get an IP address using DHCP");
  if (Ethernet.begin(mac) == 0) {
    Serial.println("Failed to configure Ethernet using DHCP");
    // initialize the ethernet device not using DHCP:
    Ethernet.begin(mac, ip);
  }
  // print your local IP address:
  Serial.print("My IP address: ");
  ip = Ethernet.localIP();
  for (byte thisByte = 0; thisByte < 4; thisByte++) {
    // print the value of each byte of the IP address:
    Serial.print(ip[thisByte], DEC);
    Serial.print(".");
  }
  Serial.println();
  
  // start listening for clients
  server.begin();
  
  // This delay is needed to let the WiFly respond properly
  delay(100);
}

void webServerStack_Broadcast(int sourceId, String data)
{
  for (int i = 0; i < MAX_CLIENT_NUM; i++){
    if (sourceId == i) continue; // leave as it's original broadcast source!
    if (webSocketStack[i].client){
      if (webSocketStack[i].client.connected()){ 
        sendClientData(i, data);
      }
    }
  }
}


/*
 * This function will check all connected client to check
 * whether if it has available data to read, then pass this data
 * to handle, if not, pass this new connection request to 
 * inactive client, handshake and make this client active 
 * (ready to receive data)
 * Incase of new incoming client data, this also 
 * make a broadcast to all remaining connected 
 * client to let them know about new states.
 */
void webServerStack_ProcessMsgIn()
{ 
  bool inMsgPassed = false;

  // Process for connected client
  for (int i = 0; i < MAX_CLIENT_NUM; i++){
    if (webSocketStack[i].client){
      if (webSocketStack[i].client.connected()){ 
        // Incoming msg
        if (webSocketStack[i].client.available()){
          String data = webSocketStack[i].webSocketServer.getData();
          if (data.length() > 0) {
            Serial.print("data!\n\r");
            handleClientData(data);
            // Broacast
            webServerStack_Broadcast(i, data);
          }
          inMsgPassed = true;
          break;
        }
      }
      else { // client may be closed
        Serial.print("closed!\n\r");
        webSocketStack[i].client.stop();
      }
    }
  }

  // incomming data has been passed thru client ==> no data for new client! ==> Bug found, kakaka...
  // Return now :)
  if (inMsgPassed) return; 

  // Data here is still available and not yet processed ==> pass to new client!

  // Scan for new client
  int j = MAX_CLIENT_NUM;
  for (int i = 0; i < MAX_CLIENT_NUM; i++){
    if (!webSocketStack[i].client){
      j = i;
      break;
    }
  }

  if (j < MAX_CLIENT_NUM){
    webSocketStack[j].client = server.available();
    if (webSocketStack[j].client.connected()) {
      Serial.print("Client connected\r\n");
      if(webSocketStack[j].webSocketServer.handshake(webSocketStack[j].client)){
        Serial.print("Client handshaked\r\n");
      }
      else {
        webSocketStack[j].client.stop();
      }
    }
  }
}

/*
 * This function execute sending data to all
 * connected clients after each interval.
 */
void webServerStack_ProcessMsgOut()
{
  for (int i = 0; i < MAX_CLIENT_NUM; i++){
    if (webSocketStack[i].client){
      if (webSocketStack[i].client.connected()){ 
        // Outgoing msg
        unsigned long currentMillis = millis();
        if(currentMillis - webSocketStack[i].previousMillis >= 1000) {
          // save the last time you blinked the LED 
          webSocketStack[i].previousMillis = currentMillis;
          sendClientData(i, 1);
          sendClientData(i, 2);
          sendClientData(i, 3);
          //sendClientData(i, "d81");
          //sendClientData(i, "d90");
        }
      }
      else { // client may be closed
        Serial.print("closed!\n\r");
        webSocketStack[i].client.stop();
      }
    }
  }
}
 
void loop() {
  if (server.available()) //{
    webServerStack_ProcessMsgIn(); 
  //}
  webServerStack_ProcessMsgOut();
}




