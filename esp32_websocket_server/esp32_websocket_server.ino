/* Requirements
 *  => ESPAsyncWebServer: https://github.com/ESP32Async/ESPAsyncWebServer/
 *  => AsyncTCP: https://github.com/ESP32Async/AsyncTCP/
*/
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "website.cpp"

const char *ssid = "TORRETTA_MOBILE";
const char *password = "12345678";

// motor pins
const uint8_t motPinForward = 16;
const uint8_t motPinBackwards = 17;
const uint8_t motPinSpeed = 18;
// servo pin
const uint8_t servoPin = 19;
// joystick pins
const uint8_t joystickPin_x = 32;
const uint8_t joystickPin_y = 33;


// servo control
const uint8_t PWMFreq = 50;        // PWM frequency specific to servo motor
const uint8_t PWMResolution = 10;  // PWM resolution 2^10 values
const uint8_t minDutyCycle = 26;
const uint8_t maxDutyCycle = 126;
int servoSpeed = 0;
float servoPos;
// motor control
int motSpeed = 0, motSpeedOld = 0;
// joystick control
int joystickOld_x = 0, joystickOld_y = 0;
// timers for millis()
uint64_t t1, t2;

// AsyncWebServer object on port 80
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");


void joystickControl() {
  int temp_x = map(analogRead(joystickPin_x), 0, 4095, -10, 10);
  int temp_y = map(analogRead(joystickPin_y), 0, 4095, -10, 10);

  // update motor speed only if joystick_x moved
  if (temp_x != joystickOld_x) {
    joystickOld_x = temp_x;
    motSpeed = temp_x;
  }
  // update servo speed only if joystick_y moved
  if (temp_y != joystickOld_y) {
    joystickOld_y = temp_y;
    servoSpeed = temp_y;
  }
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {  // called on websocket's incoming message
  AwsFrameInfo *info = (AwsFrameInfo *)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    data[len] = 0;

    // code executed when a new message arrives from the ws
    String payload = String((char *)data);
    if (payload.startsWith("#cord#")) {  // catch control data from the motors
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

  // motor pin setup
  pinMode(motPinForward, OUTPUT);
  pinMode(motPinBackwards, OUTPUT);
  pinMode(motPinSpeed, OUTPUT);
  digitalWrite(motPinForward, LOW);
  digitalWrite(motPinBackwards, LOW);
  digitalWrite(motPinSpeed, LOW);

  // joystick pin setup
  pinMode(joystickPin_x, INPUT);
  pinMode(joystickPin_y, INPUT);

  // servo setup
  ledcAttach(servoPin, PWMFreq, PWMResolution);
  servoPos = (maxDutyCycle + minDutyCycle) / 2;
  ledcWrite(servoPin, servoPos);

  // create Wi-Fi
  WiFi.mode(WIFI_AP);
  IPAddress local_IP(192, 168, 4, 1);
  IPAddress gateway(192, 168, 4, 1);
  IPAddress subnet(255, 255, 255, 0);
  WiFi.softAPConfig(local_IP, gateway, subnet);
  WiFi.softAP(ssid, password);

  Serial.println("Access Point active!");
  Serial.print("IP: ");
  Serial.println(WiFi.softAPIP());

  // initialize websocket
  ws.onEvent(onEvent);
  server.addHandler(&ws);

  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html", index_html);
  });
  server.begin();  // Start server

  // Initialize timer
  t1 = millis();
  t2 = millis();
}

void loop() {
  ws.cleanupClients();  // delete disconnected clients

  joystickControl();  // control motors locally thru joystick

  // motor drive with h-bridge
  if (t2 + 150 < millis()) {  // t2 + 'delay'
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
  const int k_servo = 0.2;
  servoPos = servoPos + servoSpeed*k_servo;
  if (servoPos > maxDutyCycle) servoPos = maxDutyCycle;
  if (servoPos < minDutyCycle) servoPos = minDutyCycle;
  ledcWrite(servoPin, servoPos);

  // int interval = map(abs(servoSpeed), 0, 10, 200, 50);
  // if (t1 + interval < millis()) {
  //   if (servoSpeed > 0) {
  //     servoPos = (servoPos < 180) ? servoPos + abs(servoSpeed) : 180;
  //   } else if (servoSpeed < 0) {
  //     servoPos = (servoPos > 0) ? servoPos - abs(servoSpeed) : 0;
  //   }
  //   ledcWrite(servoPin, map(servoPos, 0, 180, minDutyCycle, maxDutyCycle));
  //   t1 = millis();
  // }
}
