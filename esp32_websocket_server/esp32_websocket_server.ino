/* Requirements
 *  => ESPAsyncWebServer: https://github.com/ESP32Async/ESPAsyncWebServer/
 *  => AsyncTCP: https://github.com/ESP32Async/AsyncTCP/
 *  => ESP32Servo [version=3.0.3]: https://github.com/madhephaestus/ESP32Servo
*/
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ESP32Servo.h>  // version=3.0.3
#include "website.cpp"

const char *ssid = "TORRETTA_MOBILE";
const char *password = "12345678";

// timers - for millis()
uint64_t t1, t2;
// motor & servo pins
const uint8_t motPinForward = 16;
const uint8_t motPinBackwards = 17;
const uint8_t motPinSpeed = 18;
const uint8_t servoPin = 19;
// motor & servo control variables
int motSpeed = 0, motSpeedOld = 0, servoSpeed = 0;
int servoPos = 90;
Servo servo1;

// AsyncWebServer object on port 80
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {  // called on websocket's incoming message
  AwsFrameInfo *info = (AwsFrameInfo *)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    data[len] = 0;

    // code executed when a new message arrives from the ws
    String payload = String((char *)data);
    if (payload.startsWith("#cord#")) {   // catch control data from the motors
      motSpeed = payload.substring(7, payload.indexOf(';')).toInt();
      servoSpeed = payload.substring(payload.indexOf(';') + 2).toInt();
      Serial.printf("Mot:%d\tServo:%d\n", motSpeed, servoSpeed);
    } else { 
      Serial.println(payload);  // print data recived from websocket
    }

    // ws.textAll("hello world");  // send a broadcast message
  }
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
  switch (type) {
    case WS_EVT_CONNECT:
      Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
      break;
    case WS_EVT_DISCONNECT:
      Serial.printf("WebSocket client #%u disconnected\n", client->id());
      break;
    case WS_EVT_DATA:
      handleWebSocketMessage(arg, data, len);
      break;
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
      break;
  }
}


void setup() {
  // Serial port for debugging purposes
  Serial.begin(115200);

  // pin setup
  pinMode(motPinForward, OUTPUT);
  pinMode(motPinBackwards, OUTPUT);
  pinMode(motPinSpeed, OUTPUT);
  digitalWrite(motPinForward, LOW);
  digitalWrite(motPinBackwards, LOW);
  digitalWrite(motPinSpeed, LOW);
  
  // servo setup
  servo1.attach(servoPin);
  servo1.write(servoPos);

  // create Wi-Fi
  WiFi.mode(WIFI_AP);
  IPAddress local_IP(192, 168, 4, 1);
  IPAddress gateway(192, 168, 4, 1);
  IPAddress subnet(255, 255, 255, 0);
  WiFi.softAPConfig(local_IP, gateway, subnet);
  WiFi.softAP(ssid, password);

  Serial.println("Access Point attivo!");
  Serial.print("IP: ");
  Serial.println(WiFi.softAPIP());

  // initialize websocket
  ws.onEvent(onEvent);
  server.addHandler(&ws);

  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html", index_html);
  });
  server.begin();   // Start server

  // Initialize timer
  t1 = millis();
  t2 = millis();
}

void loop() {
  ws.cleanupClients();  // delete disconnected clients

  // motor drive with h-bridge
  if (t2 + 150 < millis()) {      // t2 + 'delay'
    if (motSpeed > 0) {
      digitalWrite(motPinForward, HIGH);
      digitalWrite(motPinBackwards, LOW);
      analogWrite(motPinSpeed, map(motSpeed, 0, 10, 125, 255));
    } else if (motSpeed < 0) {
      digitalWrite(motPinBackwards, HIGH);
      digitalWrite(motPinForward, LOW);
      analogWrite(motPinSpeed, map(motSpeed, 0, -10, 125, 255));
    }
    // when the motor changes direction it should have time to stop
    if (motSpeed * motSpeedOld <= 0) {
      t2 = millis();
      digitalWrite(motPinBackwards, LOW);
      digitalWrite(motPinForward, LOW);
      digitalWrite(motPinSpeed, LOW);
    }
    motSpeedOld = motSpeed;
  }

  // servo drive
  int interval = map(abs(servoSpeed), 0, 10, 200, 1);
  if (t1 + interval < millis()) {
    if (servoSpeed > 0) {
      servoPos = (servoPos < 180) ? servoPos + 1 : 180;
    } else if (servoSpeed < 0) {
      servoPos = (servoPos > 0) ? servoPos - 1 : 0;
    }
    servo1.write(servoPos);
    t1 = millis();
  }
}
