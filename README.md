# 🌫️ ESP32 Air Quality Monitoring System

## 📊 Overview

This ESP32-based air quality monitoring system measures temperature, humidity, and multiple gas concentrations (CO2, CO, SO2) to calculate a comprehensive Air Quality Index (AQI). The system provides real-time environmental data through a serial monitor interface and can be expanded to connect with various IoT platforms.

## ✨ Features

- **Environmental Monitoring**:
  - Temperature and humidity measurement (DHT22)
  - CO2 detection (MQ135)
  - Carbon Monoxide detection (MQ7)
  - Sulfur Dioxide estimation

- **Air Quality Analysis**:
  - Individual pollutant indices
  - Overall Air Quality Index (AQI)
  - Air quality categorization (Good to Hazardous)
  
- **Smart Functionality**:
  - WiFi connectivity
  - Temperature/humidity compensation for gas readings
  - Configurable measurement intervals
  - Sensor warmup period

## 🔧 Hardware Requirements

- ESP32 Development Board
- DHT22 Temperature and Humidity Sensor
- MQ135 Air Quality Sensor
- MQ7 Carbon Monoxide Sensor
- Jumper Wires
- Breadboard
- Power Supply (USB or external)

## 📋 Circuit Diagram

[Upcoming]
![Circuit Diagram]("Upcoming")

### Connections

| Component | ESP32 Pin |
|-----------|-----------|
| DHT22     | GPIO 4    |
| MQ135     | GPIO 34   |
| MQ7       | GPIO 35   |

*Detailed wiring instructions:*
1. Connect DHT22 data pin to GPIO 4
2. Connect MQ135 analog output to GPIO 34
3. Connect MQ7 analog output to GPIO 35
4. Connect VCC and GND for all components

## 💻 Software Setup

### Prerequisites
- Arduino IDE
- Required Libraries:
  - WiFi.h
  - DHT.h
  - math.h

### Installation
1. Clone this repository:
2. Open the Arduino IDE and install the required libraries through the Library Manager.

3. Open the `Air_Monitoring_FlatOnlyWifi.ino` file.

4. Configure your WiFi credentials:
#define WIFI_SSID "Your SSID Here"
#define WIFI_PASSWORD "Your Password Here"
5. Upload the code to your ESP32 board.

## 📱 Usage
1. Power on your ESP32 device.
2. Open the Serial Monitor (baud rate: 115200).
3. The device will connect to WiFi and begin a 30-second sensor warmup period.
4. After warmup, readings will display every 30 seconds with the following information:
=== Sensor Data ===
Temperature: 25.60°C
Humidity: 45.20%
CO2: 650 ppm
CO: 2.5 ppm
SO2: 0.15 ppm
AQI: 42 - Good
===================

## 🛠️ Calibration
For accurate readings, you may need to calibrate the sensors:

```cpp
// Sensor calibration constants
#define RLOAD 10.0
#define MQ135_RZERO 76.63
#define MQ7_RZERO 5.0
#define MQ135_SCALE 100.0
#define MQ7_SCALE 5.0
#define MQ135_SO2_SCALE 0.5
 ```

Calibration steps:
1. Place sensors in a known clean environment
2. Record baseline readings
3. Adjust RZERO values to match expected concentrations
4. Test with reference equipment if available

## 📜 License
This project is licensed under the MIT License - see the LICENSE file for details.