# Embedded Software fo the IoT - project group 23
Wrist joint with two axes, controlled by mobile device or automatic positioning. Demo using solar panels

# Project Managers
Matteo Zambon
Alessandro Weber
Giacomo Castellan

### Utilities [internal tools]:
- Remove line breaks: https://www.sfu.ca/~mjordan/remove-line-breaks.htm
- Gears generator: https://www.stlgears.com/generators/3dprint
  - M=1, PA=20, GL=10.

### Bill of materials:
- 1 _ESP32 S3_ board
- 1 Positional servo motor
- 1 Continuous servo motor
- 1 Slip ring (8 wires)
- 1 Solar panel
- 4 photoresistors
- 3D prited parts: ** link coming soon **2

### Instructions:
- You just have to print your 3D model from the link and assemble them as seen in the presentation.
- Connect the cables as seen in the electrical scheme
![alt text](wiring-diagram.png)
- Upload the libraries and the main code with your arduino ide having the following parameters:

### Arduino IDE Setup
- Board: "ESP32S3 Dev Module"
- **USB CDC On Boot**: "Enabled"
- **CPU Frequency**: "240MHz (WiFi)"
- Core Debug Level: "None"
- USB DFU On Boot: "Disabled"
- Erase All Flash Before Sketch Upload: "Disabled"
- Events Run On: "Core 1"
- **Flash Mode**: "QIO 80MHz"
- **Flash Size**: "16MB (128Mb)"
- JTAG Adapter: "Disabled"
- Arduino Runs On: "Core 1"
- USB Firmware MSC On Boot: "Disabled"
- **Partition Scheme**: "Default 4MB with spiffs (1.2MB APP/1.5MB SPIFFS)"
- **PSRAM**: "OPI PSRAM"
- **Upload Mode**: "UART0 / Hardware CDC"
- Upload Speed: "921600"
- USB Mode: "Hardware CDC and JTAG"
- Zigbee Mode: "Disabled"

### Code description
We have our main that works as a finite state machine that is linked to a manual,automatic and web control.
You can access to the web mode via the wifi "SOLAR_MOBILE" created by the esp (you need to install the specific library) and then access to the web page on adress 192.168.4.1 with the wifi password on the code.
Here you can see that you can switch between modalities with the various buttons and switches.
The porpouse of the automatic mode is to get the better illumination on our panel.
You can customize the various parameters to modify how fast the turret moves.

### Requirments
The libraries you need to have are:
- WiFi.h
- AsyncTCP.h
- ESPAsyncWebServer.h
- "website.cpp"
- "motorController.h"
- "lightControl.h"


### Who did what
- Giacomo
  - esp code(libraries and main)
  - 3D model and printing
  - finite state machine diagram
  - PID tuning tool(vibe coded)
- Matteo
  - esp code(libraries and main)
  - html development
  - finite state machine diagram
  - presentation
- Alessandro
  - esp code(libraries and main)
  - html development
  - presentation
  - electrical scheme


### Sources:
- ESP32-S3 documentation:
  - [ESP32-S3-DevKitC-1](https://docs.espressif.com/projects/esp-dev-kits/en/latest/esp32s3/esp32-s3-devkitc-1/user_guide_v1.1.html)
  - [ESP-IDF Programming Guide](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/get-started/index.html)
  - [Arduino ESP32](https://docs.espressif.com/projects/arduino-esp32/en/latest/index.html)
- Webserver & websocket for esp32: https://lastminuteengineers.com/esp32-websocket-tutorial/
- Esp32 PWM Basicis: https://lastminuteengineers.com/esp32-pwm-tutorial/
- ledc docs: https://espressif-docs.readthedocs-hosted.com/projects/arduino-esp32/en/latest/api/ledc.html#
