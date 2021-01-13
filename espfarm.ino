#include <Arduino.h>

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include "SoftwareSerial.h"
#include <WebSocketsClient.h>

#include <Hash.h>

ESP8266WiFiMulti WiFiMulti;

#define USE_SERIAL Serial
SoftwareSerial NodeMCU(D7, D8);

bool connected = false;

template<unsigned interval, typename Fn>
void timer(Fn && f) {
  unsigned time = millis();
  static unsigned lastTime = 0;

  if (time - lastTime > interval) {
    lastTime = time;
    f();
  }
}

WebSocketsClient webSocket;
String lastCommand;

char buf[256];

void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
  String command;
  char* pos = nullptr;
  int minutes = 0;
  int begI = 0, endI = 0;
  switch (type) {
    case WStype_DISCONNECTED:
      connected = false;
      USE_SERIAL.printf("[WSc] Disconnected!\n");
      break;
    case WStype_CONNECTED: {
        connected = true;
        delay(1000);
        USE_SERIAL.printf("[WSc] Connected to url: %s\n", payload);
        webSocket.sendTXT("sign_in farm_rapid 1707");
        // send message to server when Connected
        webSocket.sendTXT("Connected");
      }
      break;
    case WStype_TEXT:
      USE_SERIAL.printf("[WSc] get text: %s\n", payload);
      for (int i = 0; i != length + 1; ++i) {
          command.concat(char(payload[i]));
      }
      NodeMCU.println(command);
      lastCommand = command;
      //
      //      char buf[256];
      //      command.toCharArray(buf, command.length());
      //      pos = strtok(buf, " ");
      //
      //      if (strcmp(pos, "set_time") == 0) {
      //        pos = strtok(NULL, " ");
      //        pos = strtok(NULL, " ");
      //        pos = strtok(NULL, " ");
      //        pos = strtok(NULL, " "); // hours
      //        if (!pos) {
      //          USE_SERIAL.println("fail to set time");
      //          return;
      //        }
      //        minutes += atoi(pos) * 60;
      //
      //        pos = strtok(NULL, " "); // minutes
      //
      //        if (!pos) {
      //          USE_SERIAL.println("fail to set time");
      //          return;
      //        }
      //        minutes += atoi(pos);
      //        setWorldTime(minutes);
      //
      //        pos = strtok(NULL, " ");
      //      } else if (strcmp(pos, "set_light_interval") == 0) {
      //        pos = strtok(NULL, " ");
      //        if (!pos) {
      //          USE_SERIAL.println("fail to set light interval");
      //          return;
      //        }
      //        begI = atoi(pos);
      //
      //        pos = strtok(NULL, " ");
      //
      //        if (!pos) {
      //          USE_SERIAL.println("fail to set light interval");
      //          return;
      //        }
      //
      //        endI = atoi(pos);
      //
      //        beginLightTime = begI;
      //        endLightTime = endI;
      //
      //        USE_SERIAL.print("new light interval is: ");
      //        USE_SERIAL.print(beginLightTime);
      //        USE_SERIAL.print(" ");
      //        USE_SERIAL.println(endLightTime);
      //      } else if (strcmp(pos, "set_pump_interval") == 0) {
      //        pos = strtok(NULL, " ");
      //        if (!pos) {
      //          USE_SERIAL.println("fail to set pump interval");
      //          return;
      //        }
      //        begI = atoi(pos);
      //
      //        pos = strtok(NULL, " ");
      //
      //        if (!pos) {
      //          USE_SERIAL.println("fail to set pump interval");
      //          return;
      //        }
      //
      //        endI = atoi(pos);
      //
      //        workPumpTime = begI;
      //        relaxPumpTime = endI;
      //
      //        USE_SERIAL.print("new pump interval is: ");
      //        USE_SERIAL.print(workPumpTime);
      //        USE_SERIAL.print(" ");
      //        USE_SERIAL.println(relaxPumpTime);

      break;
    case WStype_BIN:
      USE_SERIAL.printf("[WSc] get binary length: %u\n", length);
      hexdump(payload, length);

      // send data to server
      // webSocket.sendBIN(payload, length);
      break;
    case WStype_PING:
      // pong will be send automatically
      USE_SERIAL.printf("[WSc] get ping\n");
      break;
    case WStype_PONG:
      // answer to a ping we send
      USE_SERIAL.printf("[WSc] get pong\n");
      break;
  }
}

void setup() {
  // USE_SERIAL.begin(921600);
  NodeMCU.begin(4800);
  USE_SERIAL.begin(115200);

  //Serial.setDebugOutput(true);
  USE_SERIAL.setDebugOutput(true);

  USE_SERIAL.println();
  USE_SERIAL.println();
  USE_SERIAL.println();

  for (uint8_t t = 4; t > 0; t--) {
    USE_SERIAL.printf("[SETUP] BOOT WAIT %d...\n", t);
    USE_SERIAL.flush();
    delay(1000);
  }

  WiFiMulti.addAP("rapid_farm", "internet");

  //WiFi.disconnect();
  while (WiFiMulti.run() != WL_CONNECTED) {
    delay(100);
  }
  USE_SERIAL.println("connected to wifi");

  // server address, port and URL
  webSocket.begin("5.167.50.115", 9003);

  // event handler
  webSocket.onEvent(webSocketEvent);

  // use HTTP Basic Authorization this is optional remove if not needed


  // try ever 5000 again if connection has failed
  webSocket.setReconnectInterval(5000);

  // start heartbeat (optional)
  // ping server every 15000 ms
  // expect pong from server within 3000 ms
  // consider connection disconnected if pong is not received 2 times
}

void loop() {
  webSocket.loop();

  if (connected && NodeMCU.available()) {
    String message = NodeMCU.readStringUntil('\n');

    for (char c : message) {
      if (!isalpha(c) && !isdigit(c) && !isspace(c) && c != '_' && c != '.') {
        USE_SERIAL.print("error_str: ");
        USE_SERIAL.println(message);
        NodeMCU.flush();
        return;
      }
    }

    USE_SERIAL.println(message);
    if(message.substring(0, 4) == "none") {
      NodeMCU.println(lastCommand);
    }
    webSocket.sendTXT(message.c_str());
  }

}
