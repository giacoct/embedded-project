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

// servo pin
const uint8_t servoPin = 21;
// base pin
const uint8_t basePin = 22; //<---------METTERE PUNG GIUSTO
// joystick pins
const uint8_t joystickPin_x = 33;
const uint8_t joystickPin_y = 32;


// base & servo control
const uint8_t PWMFreq = 50;        // PWM frequency specific to servo motor
const uint8_t PWMResolution = 10;  // PWM resolution 2^10 values
const uint8_t minDutyCycle = 26;
const uint8_t maxDutyCycle = 126;
int servoSpeed = 0;
float servoPos;

// base control
/*const uint8_t PWMFreq = 50;        // PWM frequency specific to servo motor
const uint8_t PWMResolution = 10;*/  // PWM resolution 2^10 values
/*const uint8_t minDutyCycle = 26;
const uint8_t maxDutyCycle = 126;*/
int baseSpeed = 0;
int basePos;

// joystick control
int joystickOld_x = 0, joystickOld_y = 0;
// timers for millis()
uint64_t t1, t2;

// AsyncWebServer object on port 80
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");


long mapWithCenter(long x, long in_min, long in_center, long in_max, long out_min, long out_max) {
  const uint16_t center = (out_min + out_max) / 2;
  if (x < in_center) 
    return map(x, in_min, in_center, out_min, center);
  else 
    return map(x, in_center, in_max, center, out_max);
}

void joystickControl() {
  int16_t read_x = mapWithCenter(analogRead(joystickPin_x), 0, 1773, 4095, -10, 10);  // CENTER VALUE TO TUNE FOR EACH AXIS
  int16_t read_y = mapWithCenter(analogRead(joystickPin_y), 0, 1751, 4095, -10, 10);
  // dead zone
  read_x = (abs(read_x) <= 1) ? 0 : read_x;
  read_y = (abs(read_y) <= 1) ? 0 : read_y;

  // update base speed only if joystick_x moved
  if (read_x != joystickOld_x) {
    joystickOld_x = read_x;
    baseSpeed = read_x;
  }
  // update servo speed only if joystick_y moved
  if (read_y != joystickOld_y) {
    joystickOld_y = read_y;
    servoSpeed = read_y;
  }
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {  // called on websocket's incoming message
  AwsFrameInfo *info = (AwsFrameInfo *)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    data[len] = 0;

    // code executed when a new message arrives from the ws
    String payload = String((char *)data);
    if (payload.startsWith("#cord#")) {  // catch control data for the motors
      baseSpeed = payload.substring(7, payload.indexOf(';')).toInt();
      servoSpeed = payload.substring(payload.indexOf(';') + 2).toInt();
      Serial.printf("Mot:%d\tServo:%d\n", baseSpeed, servoSpeed);
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
  
  // servo setup
  ledcAttach(servoPin, PWMFreq, PWMResolution);
  servoPos = (maxDutyCycle + minDutyCycle) / 2;
  ledcWrite(servoPin, servoPos);

  // base setup
  ledcAttach(basePin, PWMFreq, PWMResolution);
  basePos = 90;
  //ledcWrite(servoPin, servoPos); //-------- not sure if needed when starting


  // joystick pin setup
  pinMode(joystickPin_x, INPUT);
  pinMode(joystickPin_y, INPUT);

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
  joystickControl();    // control motors locally thru joystick

  // base drive, il tutto nella mia testa funziona ma non so se funziona davvero ~matteo

  if (t2 + 5 < millis()) {
  ledcWrite(basePin, map(baseSpeed,-10,10,26,126));

  t2 = millis();
}

  // servo drive
  const float k_servo = 0.05;  // higher is faster
  if (t1 + 5 < millis()) {
    servoPos = servoPos + (servoSpeed * k_servo);
    if (servoPos > maxDutyCycle) servoPos = maxDutyCycle;
    if (servoPos < minDutyCycle) servoPos = minDutyCycle;
    ledcWrite(servoPin, servoPos);

    t1 = millis();
  }
}
