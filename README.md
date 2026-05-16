
# 🔔 Automatic School Bell System

A smart, wireless school bell system built with ESP32, DS3231 RTC, CD4017 counter, and a custom Android app — developed as a Digital Logic Design (DLD) semester project.

---

## 📌 What It Does

- Rings the bell automatically at user-defined times (e.g. 08:00, 09:30, 11:00)
- Tracks class periods using a CD4017 decade counter
- Displays the current period number on a 7-segment display
- Fully controllable from an Android phone via Bluetooth
- Keeps accurate time even after power cuts using DS3231 RTC module
- Manual bell ring via push button or mobile app

---

## 🧩 Components Used

| Component | Purpose |
|---|---|
| ESP32 DevKit V1 | Main controller, Bluetooth communication |
| DS3231 RTC Module | Real-time clock, keeps time after power off |
| CD4017 Decade Counter IC | Sequential logic — counts class periods |
| 7447 BCD to 7-Segment Driver IC | Combinational logic — decodes period number |
| 7-Segment Display (Common Cathode) | Shows current period number |
| Active Buzzer | Bell output |
| Push Button | Manual bell trigger |
| Resistors (330Ω x7) | Current limiting for display segments |
| Breadboard + Jumper Wires | Prototyping |

---

## 📱 Mobile App

Built with **MIT App Inventor**. Features:

- Bluetooth connect to ESP32
- Add bell times (e.g. 08:30)
- View current schedule
- Manual ring, stop, reset
- Sync time to ESP32

### Bluetooth Commands

| Command | Action |
|---|---|
| `ADD:HH:MM` | Add a bell time |
| `RING` | Ring bell immediately |
| `RESET` | Reset period counter |
| `STOP` | Disable automatic bells |
| `START` | Enable automatic bells |
| `CLEAR` | Clear all bell times |
| `LIST` | Get current schedule |
| `TIME` | Get current time from ESP32 |
| `SET:HH:MM:SS` | Set RTC clock time |

---

## ⚡ Circuit Connections

### ESP32 → DS3231
| ESP32 | DS3231 |
|---|---|
| GPIO 21 | SDA |
| GPIO 22 | SCL |
| 3.3V | VCC |
| GND | GND |

### ESP32 → CD4017
| ESP32 | CD4017 |
|---|---|
| GPIO 27 | Clock (Pin 14) |
| GPIO 26 | Reset (Pin 15) |
| 3.3V | VDD (Pin 16) |
| GND | VSS (Pin 8) |

### ESP32 → Buzzer
| ESP32 | Buzzer |
|---|---|
| GPIO 18 | I/O |
| 3.3V | VCC |
| GND | GND |

### ESP32 → Push Button
| ESP32 | Button |
|---|---|
| GPIO 19 | One side |
| GND | Other side |

---

## 🛠️ How to Use

### Setting Up the Code
1. Install **Arduino IDE**
2. Add ESP32 board support via Boards Manager
3. Install **RTClib by Adafruit** via Library Manager
4. Open `school_bell_esp32.ino`
5. Select **Tools → Board → ESP32 Dev Module**
6. Upload to ESP32

### Using the App
1. Install the APK on your Android phone
2. Pair your phone with "SchoolBell" in Bluetooth settings
3. Open the app and tap **Connect to SchoolBell**
4. Add bell times in HH:MM format
5. The system will ring automatically at those times

---

## 🧠 DLD Concepts Demonstrated

- **Sequential Logic** — CD4017 decade counter advances period count on each clock pulse
- **Combinational Logic** — 7447 BCD decoder converts binary period number to 7-segment display output
- **Binary Coded Decimal (BCD)** — Period numbers encoded in 4-bit BCD format

---

## 🔧 Tools Used

- Arduino IDE — ESP32 programming
- MIT App Inventor — Android app
- Cirkit Designer — Circuit diagram
- RTClib (Adafruit) — DS3231 library

---

## 📄 License

This project was built for educational purposes as part of a Digital Logic Design course.
