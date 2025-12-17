# Embedded Software fo the IoT - project group 23
Wrist joint with two axes, controlled by mobile device or automatic positioning. Demo using solar panels

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
- 3D prited parts: ** link coming soon **

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

### Sources:
- ESP32-S3 documentation:
  - [ESP32-S3-DevKitC-1](https://docs.espressif.com/projects/esp-dev-kits/en/latest/esp32s3/esp32-s3-devkitc-1/user_guide_v1.1.html)
  - [ESP-IDF Programming Guide](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/get-started/index.html)
  - [Arduino ESP32](https://docs.espressif.com/projects/arduino-esp32/en/latest/index.html)
- Webserver & websocket for esp32: https://lastminuteengineers.com/esp32-websocket-tutorial/
- Esp32 PWM Basicis: https://lastminuteengineers.com/esp32-pwm-tutorial/
- ledc docs: https://espressif-docs.readthedocs-hosted.com/projects/arduino-esp32/en/latest/api/ledc.html#

### Note
- circa 6s 20cs per fare un giro intero (base)
