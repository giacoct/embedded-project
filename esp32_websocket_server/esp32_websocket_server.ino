/* Requirements
 *  => ESPAsyncWebServer: https://github.com/ESP32Async/ESPAsyncWebServer/
 *  => AsyncTCP: https://github.com/ESP32Async/AsyncTCP/
*/
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "website.cpp"
#include "motorController.h"
#include "lightControl.h"

const char *ssid = "TORRETTA_MOBILE";
const char *password = "12345678";

// base and servo controller
MotorController mc = MotorController(21, 23, 0.004);
// photoresistors
LightControl tl = LightControl(32, 2150);
LightControl tr = LightControl(33, 2385);
LightControl bl = LightControl(34, 2050);
LightControl br = LightControl(35, 1620);

// joystick pins
const uint8_t joystickPin_x = 36;
const uint8_t joystickPin_y = 39;
const uint8_t joystickButtonPin = 25;

// solar Tracking Logic
int threshold = 100;  // Sensitivity: minimum difference in light to move
uint64_t t_auto = 0;

// joystick control
float joystickOld_x = 0.0, joystickOld_y = 0.0;
bool buttonPressed = false;
uint64_t t0 = 0;
// state variable
int state = 0;  // 0-auto,1-move to optimal,2-reset threshold,3-joystick,4-websocket

// AsyncWebServer object on port 80
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

// function prototypes - optimization
void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len);
void handleWebSocketMessage(void *arg, uint8_t *data, size_t len);
void joystickControl();
long mapWithCenter(long x, long in_min, long in_center, long in_max, long out_min, long out_max);
void IRAM_ATTR joystickClick();
void solarTrackerLogic();


void solarTrackerLogic() {
  // Low value = light
  tl.sample();
  tr.sample();
  bl.sample();
  br.sample();

  // Serial.printf("TL: %d \tTR: %d \tBL: %d \tBR: %d \t", tl.read(), tr.read(), bl.read(), br.read());

  // averages of the sides
  int avgTop = (tl.read() + tr.read()) / 2;
  int avgBot = (bl.read() + br.read()) / 2;
  int avgLeft = (tl.read() + bl.read()) / 2;
  int avgRight = (tr.read() + br.read()) / 2;

  // --- CONTROLLO Y (TILT - Servo Standard) ---
  // Se Top è più luminoso di Bottom (valore numerico PIÙ BASSO = più luce)
  // avgTop < avgBot
  if (abs(avgTop - avgBot) > threshold) {
    if (avgTop < avgBot) {
      Serial.printf("move up \t");
      // mc.setServoSpeed(1);
    } else {
      Serial.printf("move down \t");
      // mc.setServoSpeed(-1);
    }
  } else {
    mc.setServoSpeed(0);
  }

  // --- CONTROLLO X (BASE - Servo Continuo) ---
  // avgLeft < avgRight
  if (abs(avgLeft - avgRight) > threshold) {
    if (avgLeft < avgRight) {
      Serial.printf("move left \t");
      // mc.setBaseSpeed(-1);
    } else {
      Serial.printf("move right \t");
      // mc.setBaseSpeed(1); // ruota a destra
    }
  } else {
    mc.setBaseSpeed(0);
  }
  Serial.println();
}

// interrupt
void IRAM_ATTR joystickClick() {
  unsigned long t1 = millis();
  if (t1 - t0 > 500) {  // 500 = debounce
    buttonPressed = true;
    t0 = t1;
  }
}

long mapWithCenter(long x, long in_min, long in_center, long in_max, long out_min, long out_max) {
  const long center = (out_min + out_max) / 2;
  if (x < in_center)
    return map(x, in_min, in_center, out_min, center);
  else
    return map(x, in_center, in_max, center, out_max);
}

void joystickControl() {
  long read_x = mapWithCenter(analogRead(joystickPin_x), 0, 1773, 4095, -10, 10);
  long read_y = mapWithCenter(analogRead(joystickPin_y), 0, 1751, 4095, -10, 10);

  // dead zone
  read_x = (abs(read_x) <= 1) ? 0 : read_x;
  read_y = (abs(read_y) <= 1) ? 0 : read_y;

  // update base speed only if joystick_x moved
  if (read_x != joystickOld_x) {
    joystickOld_x = float(read_x);
    mc.setBaseSpeed(read_x);
  }
  // update servo speed only if joystick_y moved
  if (read_y != joystickOld_y) {
    joystickOld_y = float(read_y);
    mc.setServoSpeed(float(read_y));
  }
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {  // called on websocket's incoming message
  AwsFrameInfo *info = (AwsFrameInfo *)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    data[len] = 0;

    // code executed when a new message arrives from the ws
    String payload = String((char *)data);
    if (payload.startsWith("#cord#") && state == 4) {  // catch control data for the motors
      mc.setBaseSpeed(payload.substring(7, payload.indexOf(';')).toFloat());
      mc.setServoSpeed(payload.substring(payload.indexOf(';') + 2).toFloat());
      //Serial.printf("Mot:%f\tServo:%f\n", baseSpeed, servoSpeed);
    } else if (payload.startsWith("#toggle#") && state == 4) {
      state = 0;
      Serial.printf("da 4 passato allo stato 0 (controllo automatico)\n");
    } else if (payload.startsWith("#control#")) {
      Serial.printf("passato allo stato 4 (controllo web)\n");
      state = 4;
    } else {
      Serial.println(payload);  // print data recived from websocket
    }
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

  mc.begin();
  tl.begin();
  tr.begin();
  br.begin();
  bl.begin();

  // joystick pin setup
  pinMode(joystickPin_x, INPUT);
  pinMode(joystickPin_y, INPUT);
  pinMode(joystickButtonPin, INPUT_PULLUP);

  // interrupt on (joystick) button pin
  attachInterrupt(digitalPinToInterrupt(joystickButtonPin), joystickClick, FALLING);

  // initialize Wi-Fi
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
  server.begin();  // Start webserver
}

void loop() {
  ws.cleanupClients();  // delete disconnected clients

  // keep the motors moving
  mc.moveServo();
  mc.moveBase();

  switch (state) {
    case 0:  // autonomous control
      if (buttonPressed) {
        buttonPressed = false;
        state = 3;
        Serial.printf("da 0 passato allo stato 3 (controllo manuel)\n");
        mc.stopAll();
      }
      // if (millis() > t_auto + 100) {
      solarTrackerLogic();
      //   t_auto = millis();
      // }
      break;
    case 1:  // movement to optimal position
      if (buttonPressed) {
        buttonPressed = false;
        state = 3;
        Serial.printf("da 1 passato allo stato 3 (controllo manuel)\n");
        mc.stopAll();
      }
      break;
    case 2:  // reset threshold

      break;
    case 3:  // manual control with joystick
      joystickControl();
      if (buttonPressed) {
        buttonPressed = false;
        state = 0;
        Serial.printf("da 3 passato allo stato 0 (controllo automatico)\n");
        mc.stopAll();
      }
      break;
    case 4:  // manual control thru websocket
      if (buttonPressed) {
        buttonPressed = false;
        state = 3;
        Serial.printf("da 4 passato allo stato 3 (controllo manuel)\n");
        ws.textAll("#no-web#");
        mc.stopAll();
      }
      break;
    default:
      state = 0;
      break;
  }
}
