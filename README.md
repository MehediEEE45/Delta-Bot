# 🤖 Delta-Bot (Deskbot)

[![PlatformIO](https://img.shields.io/badge/PlatformIO-Build%20%26%20Upload-orange?logo=platformio)](https://platformio.org/)
[![Board](https://img.shields.io/badge/Board-ESP32--C3--DevKitM--1-red?logo=espressif)](https://www.espressif.com/)
[![Framework](https://img.shields.io/badge/Framework-Arduino-blue?logo=arduino)](https://www.arduino.cc/)
[![Language](https://img.shields.io/badge/Language-C%2B%2B17-00599C?logo=cplusplus)](https://isocpp.org/)
[![License](https://img.shields.io/badge/License-MIT-green.svg)](LICENSE)

**Delta-Bot** is an interactive, smart desktop companion robot powered by the **ESP32-C3** microcontroller. Featuring expressive animated OLED faces, Wi-Fi connectivity, real-time clock & weather integration, touch gesture recognition, dual motor drive, and a built-in web dashboard for remote control.

---

## 🌟 Key Features

- 🎭 **Expressive OLED Face System**: Dynamic facial expressions, eye animations, blinking, and surprise reactions rendered on an OLED display via Adafruit GFX/SSD1305.
- 🌐 **Built-in Web Control Dashboard**: Embedded web server hosting a responsive control panel to trigger animations, control motor movements, set clock/weather modes, and configure Wi-Fi.
- ⏰ **NTP Real-Time Clock**: Synchronizes time automatically over Wi-Fi with customizable timezone support.
- 🌤️ **Live Weather Updates**: Background polling for temperature and weather conditions using Open-Meteo API.
- 👆 **Touch Sensor Gestures**: Capacitive touch sensor supporting single tap, double tap, and long press gestures to switch modes or trigger interactions.
- 🚗 **Dual Motor Control**: Motor driver interface for dynamic robot movement and responsive actions.
- 📡 **Smart Wi-Fi & Fallback AP**: Seamlessly connects to home Wi-Fi; if disconnected, automatically launches a fallback Access Point (`Delta` AP at `192.168.4.1`).

---

## 🛠️ Hardware Requirements

| Component | Description / Specification | Pinout / Connection |
| :--- | :--- | :--- |
| **Microcontroller** | ESP32-C3 DevKitM-1 | - |
| **Display** | 128x64 OLED Display (I2C) | SDA: `GPIO 8`, SCL: `GPIO 9` |
| **Touch Sensor** | TTP223 or Capacitive Touch Sensor | `GPIO 4` |
| **Motor Driver** | DRV8833 / TB6612 / L298N | Motor L: PWM `3`, IN1 `5`, IN2 `6`<br>Motor R: PWM `0`, IN1 `7`, IN2 `10`<br>STBY: `GPIO 1` |
| **Power / Battery** | LiPo Battery & Voltage Divider | ADC Pin: `GPIO 2` (Optional) |

---

## 💻 Tech Stack & Libraries

Built with **PlatformIO** and **Arduino Framework** for ESP32-C3:

- [Adafruit GFX Library](https://github.com/adafruit/Adafruit-GFX-Library) - Graphics rendering engine
- [Adafruit SSD1305](https://github.com/adafruit/Adafruit_SSD1305) - OLED hardware driver
- [ArduinoJson](https://arduinojson.org/) - Efficient JSON parsing for weather API & web requests

---

## 📁 Project Structure

```text
Deskbot/
├── include/
│   ├── AppController.h       # Main app state machine & mode manager
│   ├── Config.h              # Pinout, timing, & hardware constants
│   ├── FaceRenderer.h        # OLED face animation engine
│   ├── MotorController.h     # Dual DC motor driver control
│   ├── Secrets.h.example     # Wi-Fi credential template
│   ├── TouchSensor.h         # Touch debouncing & gesture handler
│   ├── Weather.h             # Weather API fetcher
│   └── WebController.h       # Embedded HTTP server & web UI
├── src/
│   ├── AppController.cpp
│   ├── FaceRenderer.cpp
│   ├── MotorController.cpp
│   ├── TouchSensor.cpp
│   ├── Weather.cpp
│   ├── WebController.cpp
│   └── main.cpp              # System initialization & main loop
├── platformio.ini            # PlatformIO build configuration
├── .gitignore                # Git untracked file rules
└── README.md                 # Project documentation
```

---

## 🚀 Getting Started

### 1. Prerequisites

- Install [VS Code](https://code.visualstudio.com/)
- Install the [PlatformIO IDE Extension](https://platformio.org/platformio-ide)

### 2. Clone the Repository

```bash
git clone https://github.com/MehediEEE45/Delta-Bot.git
cd Delta-Bot
```

### 3. Configure Wi-Fi Credentials

Copy `include/Secrets.h.example` to `include/Secrets.h`:

```bash
cp include/Secrets.h.example include/Secrets.h
```

Edit `include/Secrets.h` with your Wi-Fi SSID and Password:

```cpp
#pragma once

namespace Config {
constexpr char WIFI_SSID[] = "YOUR_WIFI_NAME";
constexpr char WIFI_PASSWORD[] = "YOUR_WIFI_PASSWORD";
}
```

> ⚠️ **Note:** `Secrets.h` is excluded in `.gitignore` to keep your credentials safe from being published online.

### 4. Build and Upload

1. Connect your ESP32-C3 board to your PC via USB.
2. Open the project in VS Code with PlatformIO.
3. Click **Build** (`✓`) or **Upload** (`→`) in the PlatformIO toolbar.
4. Open **Serial Monitor** set to `115200` baud rate to view system logs and local IP address.

---

## 🌐 Web Control Dashboard

Once connected to your Wi-Fi network, Delta-Bot prints its IP address to the Serial Monitor (e.g., `http://192.168.1.100`).

Open any web browser on your network to access the web controller interface:
- 🎮 Move motors remotely
- 😊 Change facial expressions on demand
- ⚙️ Manage device settings & network parameters

If Wi-Fi is unavailable or not configured, Delta-Bot automatically creates an Access Point:
- **AP SSID**: `Delta`
- **AP Password**: `delta123`
- **Web Address**: `http://192.168.4.1`

---

## 🤝 Contributing

Contributions, issues, and feature requests are welcome! Feel free to check out the [Issues](https://github.com/MehediEEE45/Delta-Bot/issues) page.

---

## 📄 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.
