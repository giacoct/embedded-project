/* Requirements
 *  => ESPAsyncWebServer: https://github.com/ESP32Async/ESPAsyncWebServer/
 *  => AsyncTCP: https://github.com/ESP32Async/AsyncTCP/
*/
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "website.cpp"
#include "motorController.h"

const char *ssid = "TORRETTA_MOBILE";
const char *password = "12345678";

// joystick pins
const uint8_t joystickPin_x = 33;
const uint8_t joystickPin_y = 32;
const uint8_t joystickPulsante = 25;

// Solar Tracking Logic
bool autoMode = false;  // Se true, segue il sole. Se false, usa joystick/wifi
int threshold = 100;    // Sensibilità: differenza minima di luce per muoversi

// joystick control
float joystickOld_x = 0.0, joystickOld_y = 0.0;

//state variable
int state = 3;  // 0-auto,1-move to optimal,2-reset threshold,3-joystick,4-websocket

//roba per interrupt
uint64_t ultimoTempoPressione = 0;
const uint64_t tempoDebounce = 500;  // Semplice debounce: se è passato abbastanza tempo dall'ultimo click
volatile bool pulsantePremuto = false;

MotorController mc = MotorController(21, 23, 0.004);

// AsyncWebServer object on port 80
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");


long mapWithCenter(long x, long in_min, long in_center, long in_max, long out_min, long out_max) {
  const long center = (out_min + out_max) / 2;
  if (x < in_center)
    return map(x, in_min, in_center, out_min, center);
  else
    return map(x, in_center, in_max, center, out_max);
}

void joystickControl() {
  long read_x = mapWithCenter(analogRead(joystickPin_x), 0, 1773, 4095, -10, 10);  // CENTER VALUE TO TUNE FOR EACH AXIS
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

void IRAM_ATTR gestisciClick() {
  unsigned long tempoCorrente = millis();

  if (tempoCorrente - ultimoTempoPressione > tempoDebounce) {
    pulsantePremuto = true;
    ultimoTempoPressione = tempoCorrente;
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
    } else if (payload.startsWith("#control#")) {
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

  // joystick pin setup
  pinMode(joystickPin_x, INPUT);
  pinMode(joystickPin_y, INPUT);
  pinMode(joystickPulsante, INPUT_PULLUP);

  // Collega l'interrupt al pin.
  // digitalPinToInterrupt(pinPulsante): converte il pin in interrupt ID
  // gestisciClick: la funzione da lanciare
  // FALLING: lancia l'evento quando il segnale scende da HIGH a LOW (pressione)
  attachInterrupt(digitalPinToInterrupt(joystickPulsante), gestisciClick, FALLING);

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
}

void loop() {
  ws.cleanupClients();  // delete disconnected clients

  mc.moveServo();
  mc.moveBase();

  switch (state) {
    case 0:  // controllo autonomo
      if (pulsantePremuto) {
        pulsantePremuto = false;
        state = 3;
        Serial.printf("da 0 passato allo stato 3 (controllo manuel)\n");
        mc.stop();
      }
      break;
    case 1:  // movimento verso posizione ottimale
      if (pulsantePremuto) {
        pulsantePremuto = false;
        state = 3;
        Serial.printf("da 1 passato allo stato 3 (controllo manuel)\n");
        mc.stop();
      }
      break;
    case 2:  // reset threshold

      break;
    case 3:  // control emanuel joystick
      joystickControl();
      if (pulsantePremuto) {
        pulsantePremuto = false;
        state = 1;
        Serial.printf("da 3 passato allo stato 1 (controllo manuel)\n");
        mc.stop();
      }
      break;
    case 4:  // controllo manuale websocket
      if (pulsantePremuto) {
        pulsantePremuto = false;
        state = 3;
        ws.textAll("NO_WEB");
        Serial.printf("da 4 passato allo stato 3 (controllo manuel)\n");
        mc.stop();
      }
      break;
    default:
      state = 0;
      break;
  }
}
