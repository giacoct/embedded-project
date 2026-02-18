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
const char *password = "password";

// servo and base controller
MotorController mc(8, 3, 0.0005);  // pin servo; pin base; kServo (higher is faster)
                                   // see the setup() to modify the mc constants

// photoresistors
LightControl tl(6, 1.300);    // LDR pin; gain
LightControl tr(5, 0.955);
LightControl bl(4, 0.860);
LightControl br(7, 1.008);

// joystick pins
const uint8_t joystickPinX = 9;
const uint8_t joystickPinY = 10;
const uint8_t joystickButtonPin = 11;

// joystick control
float joystickOldX = 0.0, joystickOldY = 0.0;
bool buttonPressed = false;
uint64_t t0 = 0;

// FSM state
uint8_t state = 0;

// AsyncWebServer object on port 80
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");


// functions prototypes
void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len);
void handleWebSocketMessage(void *arg, uint8_t *data, size_t len);
long mapWithCenter(long x, long inMin, long inCenter, long inMax, long outMin, long outMax);
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
  mc.kpBase = 0.5;
  mc.kpServo = 2.5;
  mc.deadzone = 80;

  // joystick pin setup
  pinMode(joystickPinX, INPUT);
  pinMode(joystickPinY, INPUT);
  pinMode(joystickButtonPin, INPUT_PULLUP);

  // interrupt on joystick button pin
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
  server.begin();  // start webserver
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

        int baseErr = (tl.read() + bl.read() - tr.read() - br.read());
        int servoErr = (bl.read() + br.read() - tl.read() - tr.read());

        mc.moveAuto(baseErr, servoErr);

        if (buttonPressed) {
          buttonPressed = false;
          state = 1;
          mc.stopMotors();
          Serial.println("State 3 - manual control");
        }
      }
      break;
    case 1:  // manual control with joystick
      {
        joystickControl();
        mc.moveMotors();

        if (buttonPressed) {
          buttonPressed = false;
          state = 0;
          mc.stopMotors();
          Serial.println("State 0 - autonomous control");
        }
      }
      break;
    case 2:  // manual control through websocket
      {
        mc.moveMotors();
        if (buttonPressed) {
          buttonPressed = false;
          state = 1;
          ws.textAll("#no-web#");
          mc.stopMotors();
          Serial.println("State 3 - manual control");
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

// handle client connections
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

// called on websocket's incoming message
void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) { 
  AwsFrameInfo *info = (AwsFrameInfo *)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    data[len] = 0;
    String payload = String((char *)data);

    // UNCOMMENT TO USE 'PID tuning.py' -> 'testing' folder
    /*
    if (payload.startsWith("#KB#")) {
      // base constant calibration
      mc.kpBase = payload.substring(4).toDouble();
    } else if (payload.startsWith("#KS#")) {
      // servo constant calibration
      mc.kpServo = payload.substring(4).toDouble();
    } else if (payload.startsWith("#DZ#")) {
      // deadzone calibration
      mc.deadzone = payload.substring(4).toInt();
    }
    */ 
    if (payload.startsWith("#cord#") && state == 2) {
      // catch control data for the motors
      mc.setBaseSpeed(payload.substring(7, payload.indexOf(';')).toInt());
      mc.setServoSpeed(payload.substring(payload.indexOf(';') + 2).toInt());
      //Serial.printf("Mot:%f\tServo:%f\n", baseSpeed, servoSpeed);
    } else if (payload.startsWith("#toggle#") && state == 2) {
      state = 0;
      //Serial.printf("Passato allo stato 0 (controllo automatico)\n");
    } else if (payload.startsWith("#control#")) {
      //Serial.printf("Passato allo stato 2 (controllo web)\n");
      state = 2;
    } else {
      Serial.println(payload);  // print data recived from websocket
    }
  }
}

int mapWithCenter(int x, int inMin, int inCenter, int inMax, int outMin, int outMax) {
  const int center = (outMin + outMax) / 2;
  if (x < inCenter)
    return map(x, inMin, inCenter, outMin, center);
  else
    return map(x, inCenter, inMax, center, outMax);
}

// joystick button interrupt
void IRAM_ATTR joystickClick() {
  unsigned long t1 = millis();
  if (t1 - t0 > 500) {  // 500 = debounce
    buttonPressed = true;
    t0 = t1;
  }
}

void joystickControl() {
  int read_x = mapWithCenter(analogRead(joystickPinX), 0, 1773, 4095, -100, 100);
  int read_y = mapWithCenter(analogRead(joystickPinY), 0, 1751, 4095, -100, 100);

  // dead zone
  read_x = (abs(read_x) <= 10) ? 0 : read_x;
  read_y = (abs(read_y) <= 10) ? 0 : read_y;

  // update base speed only if joystick_x moved
  if (read_x != joystickOldX) {
    joystickOldX = float(read_x);
    mc.setBaseSpeed(read_x);
  }
  // update servo speed only if joystick_y moved
  if (read_y != joystickOldY) {
    joystickOldY = float(read_y);
    mc.setServoSpeed(read_y);
  }
}