**Arduino Websocket Server with Multiple Client Connection**

This is a demo to show Websocket Server running on Arduino can handle 
concurrent multiple connection from clients.

It also handles broadcast to let all clients to know updates about the states
they're watching and synchronizing these updates to UI on the webpage.

You can open multiple WebSocketServer_MulConn_ClientWebpage.html in your browser
to see how it working and synchronize the states together!
(Don't forget to change websocket IP server corresponding to your arduino IP in the webpage!)

This example running with Arduino Websocket Server library getting from
brandenhall/Arduino-Websocket. Thanks for your great lib.
 
Created on: Apr 8, 2015
Author: HieuNT | hieucdtspk@gmail.com