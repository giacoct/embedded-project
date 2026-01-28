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

const char *ssid = "SOLAR_MOBILE";
const char *password = "militarygrade";

double kBasePID[] = {0.05, 0.0, 0.0};
double kServoPID[] = {0.05, 0.0, 0.0};

// servo and base controller
MotorController mc = MotorController(8, 3, 0.01);
// photoresistors
LightControl tl = LightControl(6, 1788);
LightControl tr = LightControl(5, 2369);
LightControl bl = LightControl(4, 2880);
LightControl br = LightControl(7, 2514);

// joystick pins
const uint8_t joystickPin_x = 9;
const uint8_t joystickPin_y = 10;
const uint8_t joystickButtonPin = 11;

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

// current production mex
uint64_t ts;
int solar = 50;

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

  Serial.printf("TL: %04d  TR: %04d  BL: %04d  BR: %04d \t", tl.read(), tr.read(), bl.read(), br.read());

  double baseErr = (tl.read() + bl.read() - tr.read() - br.read()) / 2.0;
  double servoErr = (tl.read() + tr.read() - bl.read() - br.read()) / 2.0;
  mc.moveWithPID(baseErr, -servoErr);

  // // --- CONTROLLO Y (TILT - Servo Standard) ---
  // // Se Top è più luminoso di Bottom (valore numerico PIÙ BASSO = più luce)
  // // avgTop < avgBot
  // if (abs(avgTop - avgBot) > threshold) {
  //   if (avgTop < avgBot) {
  //     Serial.printf("move up \t");
  //     // mc.setServoSpeed(1);
  //   } else {
  //     Serial.printf("move down \t");
  //     // mc.setServoSpeed(-1);
  //   }
  // } else {
  //   mc.setServoSpeed(0);
  // }

  // // --- CONTROLLO X (BASE - Servo Continuo) ---
  // // avgLeft < avgRight
  // if (abs(avgLeft - avgRight) > threshold) {
  //   if (avgLeft < avgRight) {
  //     Serial.printf("move left \t");
  //     // mc.setBaseSpeed(-1);
  //   } else {
  //     Serial.printf("move right \t");
  //     // mc.setBaseSpeed(1); // ruota a destra
  //   }
  // } else {
  //   mc.setBaseSpeed(0);
  // }
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
    mc.setServoSpeed(read_y);
  }
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {  // called on websocket's incoming message
  AwsFrameInfo *info = (AwsFrameInfo *)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    data[len] = 0;

    // code executed when a new message arrives from the ws
    String payload = String((char *)data);
    if (payload.startsWith("B#")) {  // PID base calibration
      kBasePID[0] = payload.substring(payload.indexOf("#kp=")+4, payload.indexOf("#ki=")).toFloat();
      kBasePID[1] = payload.substring(payload.indexOf("#ki=")+4, payload.indexOf("#kd=")).toFloat();
      kBasePID[2] = payload.substring(payload.indexOf("#kd=")+4, payload.indexOf("##")).toFloat();
      mc.tunePID(kBasePID, kServoPID);
    }
    else if (payload.startsWith("S#")) {  // PID servo calibration
      kServoPID[0] = payload.substring(payload.indexOf("#kp=")+4, payload.indexOf("#ki=")).toFloat();
      kServoPID[1] = payload.substring(payload.indexOf("#ki=")+4, payload.indexOf("#kd=")).toFloat();
      kServoPID[2] = payload.substring(payload.indexOf("#kd=")+4, payload.indexOf("##")).toFloat();
      mc.tunePID(kBasePID, kServoPID);
    }
    else if (payload.startsWith("#cord#") && state == 4) {  // catch control data for the motors
      mc.setBaseSpeed(payload.substring(7, payload.indexOf(';')).toInt());
      mc.setServoSpeed(payload.substring(payload.indexOf(';') + 2).toInt());
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

  // motor control
  mc.begin();
  mc.tunePID(kBasePID, kServoPID);
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

  // time for solar production
  ts = millis();
}

void loop() {
  ws.cleanupClients();  // delete disconnected clients

  switch (state) {
    case 0:  // autonomous control
      // if (millis() > t_auto + 100) {


      solarTrackerLogic();
      // mc.moveBase();
      // mc.moveServo();


      //   t_auto = millis();
      // }
      if (buttonPressed) {
        buttonPressed = false;
        state = 3;
        mc.stopAll();
        Serial.printf("da 0 passato allo stato 3 (controllo manuel)\n");
      }
      break;
    case 1:  // movement to optimal position
      if (buttonPressed) {
        buttonPressed = false;
        state = 3;
        mc.stopAll();
        Serial.printf("da 1 passato allo stato 3 (controllo manuel)\n");
      }
      break;
    case 2:  // reset threshold

      break;
    case 3:  // manual control with joystick
      joystickControl();
      mc.moveServo();
      mc.moveBase();
      if (buttonPressed) {
        buttonPressed = false;
        state = 0;
        mc.stopAll();
        Serial.printf("da 3 passato allo stato 0 (controllo automatico)\n");
      }
      break;
    case 4:  // manual control thru websocket
      mc.moveServo();
      mc.moveBase();
      if (buttonPressed) {
        buttonPressed = false;
        state = 3;
        ws.textAll("#no-web#");
        mc.stopAll();
        Serial.printf("da 4 passato allo stato 3 (controllo manuel)\n");
      }
      break;
    default:
      state = 0;
      break;
  }
  // solar production
  if (ts + 1000 < millis()) {
    String payload = "#solar=";
    payload = payload + String(solar) + "#";
    ws.textAll(payload);
    ts = millis();
  }
}
