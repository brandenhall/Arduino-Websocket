#include <ESP8266WiFi.h>

// Here we define a maximum framelength to 64 bytes. Default is 256.
#define MAX_FRAME_LENGTH 64

// Define how many callback functions you have. Default is 1.
#define CALLBACK_FUNCTIONS 1

#define MESSAGE_INTERVAL 30000
#define HEARTBEAT_INTERVAL 25000

#include <WebSocketClient.h>

WiFiClient client;
WebSocketClient webSocketClient;

uint64_t messageTimestamp = 0;
uint64_t heartbeatTimestamp = 0;

void setup() {

  Serial.begin(115200);

  Serial.print("Connecting to Wifi");
  WiFi.begin("ssid", "password");

  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
    Serial.print(".");
  }
  Serial.println("WiFi connected");

  // This is how you get the local IP as an IPAddress object
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

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
