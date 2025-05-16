
# 🔧 Smart Attendance System - Arduino & ESP32 Sketches

---

## 📝 Overview  
This repo holds the **Arduino and ESP32 sketches** powering the Smart Attendance System hardware.

---

### 🖥️ Arduino Uno Sketch  
- Handles fingerprint enrollment & verification with the **R307 sensor**  
- Controls the **16x2 I2C LCD** to show user-friendly messages  
- Talks to ESP32-CAM over serial to sync commands & results  

---

### 📸 ESP32-CAM Sketch  
- Captures face images & sends to backend for recognition  
- Communicates with Arduino Uno to coordinate fingerprint checks  

---

## 🚦 Features  
- Enroll & verify fingerprints on Arduino  
- Display messages on LCD for smooth user experience  
- Serial communication between Arduino & ESP32-CAM  
- Mandatory fingerprint auth + optional face recognition support  

---

## ⚡ How to Use  
1. Wire components as per the wiring diagram  
2. Upload the Arduino sketch via Arduino IDE  
3. Flash ESP32-CAM sketch using ESP32 toolchain  
4. Power up with a stable 5V source  
5. Use frontend app to trigger enroll & check commands  

---

## 📚 Dependencies  
- Adafruit Fingerprint Library  
- LiquidCrystal_I2C Library  
- SoftwareSerial Library  

---

## ⚠️ Notes  
- Make sure all grounds (Arduino, ESP32, sensors, power supply) are common  
- Double-check wiring to avoid communication issues  

---

Happy coding & hacking! ⚡😎
