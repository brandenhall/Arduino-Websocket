#include <SPI.h>
#include <SC16IS750.h>
#include <WiFly.h>

// Here we define a maximum framelength to 64 bytes. Default is 256.
#define MAX_FRAME_LENGTH 64

// Define how many callback functions you have. Default is 1.
#define CALLBACK_FUNCTIONS 1

#define MESSAGE_INTERVAL 30000
#define HEARTBEAT_INTERVAL 25000

#include <WebSocketClient.h>

WiFlyClient client = WiFlyClient();
WebSocketClient webSocketClient;

uint64_t messageTimestamp = 0;
uint64_t heartbeatTimestamp = 0;

void setup() {


  Serial.begin(9600);
  SC16IS750.begin();

  WiFly.setUart(&SC16IS750);

  WiFly.begin();

  // This is for an unsecured network
  // For a WPA1/2 network use auth 3, and in another command send 'set wlan phrase PASSWORD'
  // For a WEP network use auth 2, and in another command send 'set wlan key KEY'
  WiFly.sendCommand(F("set wlan auth 1"));
  WiFly.sendCommand(F("set wlan channel 0"));
  WiFly.sendCommand(F("set ip dhcp 1"));

  Serial.println(F("Joining WiFi network..."));


  // Here is where you set the network name to join
  if (!WiFly.sendCommand(F("join arduino_wifi"), "Associated!", 20000, false)) {
    Serial.println(F("Association failed."));
    while (1) {
      // Hang on failure.
    }
  }

  if (!WiFly.waitForResponse("DHCP in", 10000)) {
    Serial.println(F("DHCP failed."));
    while (1) {
      // Hang on failure.
    }
  }

  // This is how you get the local IP as an IPAddress object
  Serial.println(WiFly.localIP());

  // This delay is needed to let the WiFly respond properly
  delay(100);

  // Connect to the websocket server
  if (client.connect("echo.websocket.org", 80)) {
    Serial.println("Connected");
  } else {
    Serial.println("Connection failed.");
    while(1) {
      // Hang on failure
    }
  }

  // Handshake with the server
  webSocketClient.path = "/";
  webSocketClient.host = "echo.websocket.org";

  if (webSocketClient.handshake(client, true)) {
    Serial.println("Handshake successful");
    // socket.io upgrade confirmation message (required)
    webSocketClient.sendData("5");
  } else {
    Serial.println("Handshake failed.");
    while(1) {
      // Hang on failure
    }
  }
}

void loop() {
  String data;

  if (client.connected()) {

    webSocketClient.getData(data);

    if (data.length() > 0) {
      Serial.print("Received data: ");
      Serial.println(data);
    }

    uint64_t now = millis();

    if(now - messageTimestamp > MESSAGE_INTERVAL) {
        messageTimestamp = now;
        // capture the value of analog 1, send it along
        pinMode(1, INPUT);
        data = String(analogRead(1));

        // example socket.io message with type "messageType" and JSON payload
        char message[128];
        sprintf(message, "42[\"messageType\",{\"data\":\"%s\"}]", data.c_str());
        webSocket.sendData(message);
    }
    if((now - heartbeatTimestamp) > HEARTBEAT_INTERVAL) {
        heartbeatTimestamp = now;
        // socket.io heartbeat message
        webSocket.sendData("2");
    }

  } else {

    Serial.println("Client disconnected.");
    while (1) {
      // Hang on disconnect.
    }
  }

  // wait to fully let the client disconnect
  delay(3000);
}
