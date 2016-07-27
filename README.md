## Websocket client for Arduino, with fast data send

This is a simple library that implements a Websocket client running on an Arduino.

### Rationale

For our IoT prototype project based on Arduino, we needed a reliable data transmission protocol, and we thought of Websocket. We then searched for existing client implementations for Arduino, something that was able to handle any subclass of the `Client` one provided by Arduino, and that was essential in implementation but complete as well. We found the excellent code here <https://github.com/brandenhall/Arduino-Websocket>. However, some modifications were needed for our purpose. In particular, we needed max throughput possible, approaching 100 messages/s.

### Features
I added the following:

- Faster data send (`client.sendData(..., true)`, default behaviour): instead of sending a TCP packet per char, everything is sent
  in one shot in a single TCP packet. This makes the implementation much faster. However, take into consideration max string length when using `WiFiClient.write()` method (around 90 bytes, from users experience when googled). Example:

  ```C++
  webSocketClient.sendData("my string to send", WS_OPCODE_TEXT, true);
  ```

- For method `client.getData()`, I created a pure C string implementation, to avoid chances of heap fragmentation due to `String`
  class. Example:

  ```C++
  char msg_in[100]; // should be long enough to hold the longest arriving message
  uint8_t opcode_in;
  ...
  webSocketClient.getData(msg_in, &opcode_in)
  ```

### Tests
The optimized code was tested for:

- `WiFiClient` (`<WiFi.h>` and `<WiFi101.h>`)
- Arduino UNO and ZERO
- WiFi shield (retired) on Arduino UNO and WiFi shield 101 on Arduino ZERO
- `ws` as Node.js websocket server

We were able to obtain to reach the target throughput indicated above, with a message length of around 70 bytes (\*):

(\*) In order to reach that speed, we had to apply the following hack:

1. <https://gist.github.com/u0078867/9df30eb7da64d8f43422faa70b1a9e52>

   We did not want to get the `loop()` stuck if the TCP message was not sent (via WiFi), and we could afford some data lost randomly; although, we wanted our data to be reliable and in time order on the server side, so we excluded UDP packets.
   However, when the message rate you want to have is high, then some TCP packets could be lost (data or ACK); in this case, TCP fast-retransmit might not always be triggered, and this might increase the wait time for next packet to arrive. In this case, UDP protocol is strongly suggested.

2. After point 1, we had to manually disable the mask flag for websocket messages, by replacing this line in src /WebSocketClient.h:

   ```C++
   #define WS_MASK           0x80
   ```

   with this one:

   ```C++
   #define WS_MASK           0x00
   ```

   This modification disables the message mask, which normally is **compulsory**. `ws` tolerates it however.

### MCU compatibility
- Tested: Arduino UNO, ZERO
- Not tested: Arduino DUE; howerer, by searching similar C++ repos on GitHub (`arduino websocket due in:readme,name,description fork:true`), it seems that the conditional inclusion (in src/sha1.cpp) of `#include <avr/io.h>` and `#include <avr/pgmspace.h>` needed for ZERO board, would also fix compilation for DUE board. Any good-soul tester is welcome to feedback.

### Notes
See the original code from Branden for additional notes.

### Credits
This is an optimized version of the client code from the excellent job in <https://github.com/brandenhall/Arduino-Websocket>. Most of the credit goes to Branden.
