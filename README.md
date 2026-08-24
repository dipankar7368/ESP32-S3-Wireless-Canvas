# 🎨 ESP32-S3 Wireless Canvas

A wireless drawing canvas project based on the **ESP32-S3 XIAO** and a **1.44-inch ST7735 TFT display**.

The ESP32-S3 creates its own Wi-Fi access point. Connect your phone to the ESP32, open the web interface, and draw directly on your phone. The drawing is then displayed on the TFT screen in real time.

## ✨ Features

- 📱 Wireless drawing from a smartphone
- 📡 ESP32-S3 creates its own Wi-Fi network
- 🎨 Web-based drawing canvas
- 🖥️ Drawing appears on the ST7735 TFT display
- ⚡ Real-time drawing
- 🔌 No internet connection required

## 🧰 Components

- ESP32-S3 XIAO
- 1.44-inch ST7735 TFT Display
- Jumper wires
- USB cable

## 📚 Libraries Required

- WiFi
- WebServer
- Adafruit GFX
- Adafruit ST7735

## 🔌 Display Connection

| ST7735 | ESP32-S3 |
|---|---|
| CS | GPIO 2 |
| DC | GPIO 1 |
| RST | GPIO 3 |
| SCLK | GPIO 7 |
| MOSI | GPIO 9 |

## 🚀 How to Use

1. Upload the code to the ESP32-S3.
2. Power on the ESP32-S3.
3. Connect your phone to the Wi-Fi network created by the ESP32.
4. Open a browser on your phone.
5. Go to:

`http://192.168.4.1`

6. Start drawing on the canvas.
7. Your drawing will appear on the TFT display.

## 📺 YouTube Demo

🎥 Watch the complete project demonstration:

**[▶️ Watch the Project on YouTube](https://youtube.com/shorts/87DYXOui1Us?si=q2Ojm-48lm3F29UG)**

## 👨‍💻 Author

**Dipankar Bhunia**

IoT • Embedded Systems • Robotics • Electronics • DIY

---

⭐ If you find this project useful, consider giving the repository a star!