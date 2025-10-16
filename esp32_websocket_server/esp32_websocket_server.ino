#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "website.cpp"

const char *ssid = "ALEWEB";
const char *password = "password";

const int motPinForward = 16;
const int motPinBackwards = 17;
int motSpeed = 0, servoSpeed = 0;

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

      Serial.printf("%d \t %d\n", motSpeed, servoSpeed);
    } else {
      Serial.println(payload);  // print data recived from websocket
    }


    // ws.textAll(String(ledState));  // send a broadcast message
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
  digitalWrite(motPinForward, LOW);
  digitalWrite(motPinBackwards, LOW);

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }

  // Print ESP Local IP Address
  Serial.println(WiFi.localIP());

  // initialize websocket
  ws.onEvent(onEvent);
  server.addHandler(&ws);

  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html", index_html);
  });

  // Start server
  server.begin();
}

void loop() {
  ws.cleanupClients();  // delete disconnected clients

  // motor drive with h-bridge
  if (motSpeed > 0) {
    analogWrite(motPinForward, map(motSpeed, 0, 10, 0, 255));
    analogWrite(motPinBackwards, 0);
  } else {
    analogWrite(motPinForward, 0);
    analogWrite(motPinBackwards, map(motSpeed, 0, -10, 0, 255));
  }
}
