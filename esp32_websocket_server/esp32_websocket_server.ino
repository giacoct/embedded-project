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

unsigned long int t, t2;

const int motPinForward = 16;
const int motPinBackwards = 17;
const int motPinSpeed = 18;
const int servoPin = 19;

int motSpeed = 0, speedVecchia = 0, servoSpeed = 0;
int servoPos = 90;
Servo servo1;

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {  // called on websocket's incoming message
  AwsFrameInfo *info = (AwsFrameInfo *)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    data[len] = 0;

    /* CODICE ESEGUITO QUANDO RICEVO UN MESSAGGIO DAL WS */
    String payload = String((char *)data);

    if (payload.startsWith("#cord#")) {
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

  pinMode(motPinForward, OUTPUT);
  pinMode(motPinBackwards, OUTPUT);
  pinMode(motPinSpeed, OUTPUT);
  digitalWrite(motPinForward, LOW);
  digitalWrite(motPinBackwards, LOW);
  digitalWrite(motPinSpeed, LOW);

  servo1.attach(servoPin);  // servo setup

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

  // Start server
  server.begin();

  // Initialize timer
  t = millis();
  t2 = millis();
  servo1.write(servoPos);
}

void loop() {
  ws.cleanupClients();  // delete disconnected clients

  // motor drive with h-bridge
  if (t2 + 200 < millis()) {
    if (motSpeed > 0) {
      digitalWrite(motPinForward, HIGH);
      digitalWrite(motPinBackwards, LOW);
      analogWrite(motPinSpeed, map(motSpeed, 0, 10, 125, 255));
    } else if (motSpeed < 0) {
      digitalWrite(motPinBackwards, HIGH);
      digitalWrite(motPinForward, LOW);
      analogWrite(motPinSpeed, map(motSpeed, 0, -10, 125, 255));
    }
    if (motSpeed * speedVecchia <= 0) {
      t2 = millis();
      digitalWrite(motPinBackwards, LOW);
      digitalWrite(motPinForward, LOW);
      digitalWrite(motPinSpeed, LOW);
    }
    speedVecchia = motSpeed;
  }

  // servo drive
  int interval = map(abs(servoSpeed), 0, 10, 200, 1);
  if (t + interval < millis()) {
    if (servoSpeed > 0) {
      servoPos = (servoPos < 180) ? servoPos + 1 : 180;
    } else if (servoSpeed < 0) {
      servoPos = (servoPos > 0) ? servoPos - 1 : 0;
    }
    servo1.write(servoPos);
    t = millis();
  }
}
