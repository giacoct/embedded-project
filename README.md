# Embedded Software fo the IoT - project group 23 - SUNFLOWER
Wrist joint with two axes, controlled by mobile device or automatic positioning.

We have our main that works as a finite state machine that is linked to a manual, automatic and web control.
You can access to the web mode via the wifi "SOLAR_MOBILE" created by the esp (you need to install the specific library) and then access to the web page on adress 192.168.4.1 with the wifi password on the code.
Here you can see that you can switch between modalities with the various buttons and switches.
The porpouse of the automatic mode is to get the better illumination on our panel.
You can customize the various parameters to modify how fast the turret moves.

![alt text](FSM.png)

### Project Members
Matteo Zambon,
Alessandro Weber,
Giacomo Castellan
#
### Bill of materials:
- 1 _ESP32 S3_ board
- 1 Positional servo motor
- 1 Continuous servo motor
- 1 Slip ring (8 wires)
- 1 Solar panel
- 4 photoresistors
- 3D prited parts: [at this link](https://www.printables.com/model/1574943-sunflower-project)

### Instructions:
- You just have to print your 3D model and assemble it.
- Connect the cables as seen in the electrical scheme
![wiring diagram](wiring-diagram.png)
- Upload the libraries and the main code with your arduino ide having the following parameters and [board setup](https://randomnerdtutorials.com/installing-the-esp32-board-in-arduino-ide-windows-instructions/):

### Arduino IDE Setup (tools)
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

### Project Structure

```text
embedded-project
│   README.md
│   website.html
│   wiring-diagram.png
│
├───esp32_solar_station
│       esp32_solar_station.ino
│       lightControl.cpp
│       lightControl.h
│       motorController.cpp
│       motorController.h
│       website.cpp
│
└───testing
        PID tuning.py
        pid_constants.json
        ws_client - send noweb.py
        ws_client - test.js
        ws_server - echo.py
```

### Requirments
The libraries you need to have are:
- [AsyncTCP.h](https://github.com/ESP32Async/AsyncTCP)
- [ESPAsyncWebServer.h](https://github.com/ESP32Async/ESPAsyncWebServer)

#

### Who did what

| Giacomo                      | Matteo                       | Alessandro                   |
|:-----------------------------|:----------------------------:|-----------------------------:|
| esp code(libraries and main) | esp code(libraries and main) | esp code(libraries and main) |
| 3D model and printing        | html development             | html development             |
| finite state machine diagram | finite state machine diagram | video                        |
| video                        | presentation                 | electrical scheme            |

### Links
- [PPT Presentation](https://drive.google.com/file/d/1SDzr-Ak0xUK7H76m8eDZOuBcJwM7BU4j/view?usp=drive_link)
- [Video Presentation]()


### Sources:
- ESP32-S3 documentation:
  - [ESP32-S3-DevKitC-1](https://docs.espressif.com/projects/esp-dev-kits/en/latest/esp32s3/esp32-s3-devkitc-1/user_guide_v1.1.html)
  - [ESP-IDF Programming Guide](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/get-started/index.html)
  - [Arduino ESP32](https://docs.espressif.com/projects/arduino-esp32/en/latest/index.html)
- Webserver & websocket for esp32: https://lastminuteengineers.com/esp32-websocket-tutorial/
- Esp32 PWM Basicis: https://lastminuteengineers.com/esp32-pwm-tutorial/
- ledc docs: https://espressif-docs.readthedocs-hosted.com/projects/arduino-esp32/en/latest/api/ledc.html#

#

### PRESENTATION:
- [Video](https://youtu.be/n94C179-7IY)
- [Presentation](https://drive.google.com/file/d/1SDzr-Ak0xUK7H76m8eDZOuBcJwM7BU4j/view?usp=drive_link)
