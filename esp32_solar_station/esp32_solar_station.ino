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

// servo and base controller
MotorController mc = MotorController(8, 3, 0.0005);  // pin servo; pin base; kServo (higher is faster)
double kpBase = 0.2;
double kpServo = 0.1;

// photoresistors
LightControl tl(6, 1.300);
LightControl tr(5, 0.955);
LightControl bl(4, 0.860);
LightControl br(7, 1.008);

// joystick pins
const uint8_t joystickPin_x = 9;
const uint8_t joystickPin_y = 10;
const uint8_t joystickButtonPin = 11;
// joystick control
float joystickOld_x = 0.0, joystickOld_y = 0.0;
bool buttonPressed = false;
uint64_t t0 = 0;

// FSM state
int state = 0;

// AsyncWebServer object on port 80
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");


// functions prototypes
void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len);
void handleWebSocketMessage(void *arg, uint8_t *data, size_t len);
long mapWithCenter(long x, long in_min, long in_center, long in_max, long out_min, long out_max);
void IRAM_ATTR joystickClick();
void joystickControl();


void setup() {
  // Serial port for debugging purposes
  Serial.begin(115200);

  // light control init
  tl.begin();
  tr.begin();
  bl.begin();
  br.begin();

  // motor control init
  mc.begin();
  mc.tune(kpBase, kpServo);

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

  switch (state) {
    case 0:  // autonomous control
      {
        tl.update();
        tr.update();
        bl.update();
        br.update();
        //Serial.printf("TL:%04d  TR:%04d  BL:%04d  BR:%04d", tl_val, tr_val, bl_val, br_val);

        int baseErr = (tl.read() + bl.read() - tr.read() - br.read());
        int servoErr = (tl.read() + tr.read() - bl.read() - br.read());

        mc.moveAuto(baseErr, -servoErr);
        // serial output inside moveWithPID for debug purpose
        // Serial.println();

        if (buttonPressed) {
          buttonPressed = false;
          state = 1;
          mc.stopAll();
          Serial.printf("da 0 passato allo stato 3 (controllo manuel)\n");
        }
      }
      break;
    case 1:  // manual control with joystick
      {
        joystickControl();
        mc.moveServo();
        mc.moveBase();

        if (buttonPressed) {
          buttonPressed = false;
          state = 0;
          mc.stopAll();
          Serial.printf("da 3 passato allo stato 0 (controllo automatico)\n");
        }
      }
      break;
    case 2:  // manual control thru websocket
      {
        mc.moveServo();
        mc.moveBase();
        if (buttonPressed) {
          buttonPressed = false;
          state = 1;
          ws.textAll("#no-web#");
          mc.stopAll();
          Serial.printf("da 4 passato allo stato 3 (controllo manuel)\n");
        }
      }
      break;
    default:
      {
        state = 0;
      }
      break;
  }
}



/* * * * * * * * * * *  FUNCTIONS  * * * * * * * * * * */

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

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {  // called on websocket's incoming message
  AwsFrameInfo *info = (AwsFrameInfo *)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    data[len] = 0;

    // code executed when a new message arrives from the ws
    String payload = String((char *)data);
    if (payload.startsWith("#KB#")) {  // PID base calibration
      kpBase = payload.substring(4).toFloat();
      mc.tune(kpBase, kpServo);
    } else if (payload.startsWith("#KS#")) {  // PID servo calibration
      kpServo = payload.substring(4).toFloat();
      mc.tune(kpBase, kpServo);
    } else if (payload.startsWith("#cord#") && state == 2) {  // catch control data for the motors
      mc.setBaseSpeed(payload.substring(7, payload.indexOf(';')).toInt());
      mc.setServoSpeed(payload.substring(payload.indexOf(';') + 2).toInt());
      //Serial.printf("Mot:%f\tServo:%f\n", baseSpeed, servoSpeed);
    } else if (payload.startsWith("#toggle#") && state == 2) {
      state = 0;
      Serial.printf("da 4 passato allo stato 0 (controllo automatico)\n");
    } else if (payload.startsWith("#control#")) {
      Serial.printf("passato allo stato 2 (controllo web)\n");
      state = 2;
    } else {
      Serial.println(payload);  // print data recived from websocket
    }
  }
}

long mapWithCenter(long x, long in_min, long in_center, long in_max, long out_min, long out_max) {
  const long center = (out_min + out_max) / 2;
  if (x < in_center)
    return map(x, in_min, in_center, out_min, center);
  else
    return map(x, in_center, in_max, center, out_max);
}

// interrupt
void IRAM_ATTR joystickClick() {
  unsigned long t1 = millis();
  if (t1 - t0 > 500) {  // 500 = debounce
    buttonPressed = true;
    t0 = t1;
  }
}

void joystickControl() {
  long read_x = mapWithCenter(analogRead(joystickPin_x), 0, 1773, 4095, -100, 100);
  long read_y = mapWithCenter(analogRead(joystickPin_y), 0, 1751, 4095, -100, 100);

  // dead zone
  read_x = (abs(read_x) <= 10) ? 0 : read_x;
  read_y = (abs(read_y) <= 10) ? 0 : read_y;

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