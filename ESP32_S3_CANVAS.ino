/*
  ESP32-S3 XIAO + 1.44 inch ST7735 drawing pad

  The ESP32 creates a Wi-Fi network. Connect a phone to it, open
  http://192.168.4.1, and draw on the web canvas. Each stroke is mirrored
  on the 128 x 128 display.

  Libraries required:
    - WiFi and WebServer (included with the ESP32 board package)
    - Adafruit GFX Library
    - Adafruit ST7735 and ST7789 Library
*/

#include <WiFi.h>
#include <WebServer.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>

// ---------------------------------------------------------------------------
// Wi-Fi access point shown on the phone
// ---------------------------------------------------------------------------
const char *AP_SSID = "XIAO-DRAW";
const char *AP_PASSWORD = "drawpad1";  // Must be at least 8 characters

// ---------------------------------------------------------------------------
// XIAO ESP32-S3 -> ST7735 1.44 inch display
// ---------------------------------------------------------------------------
#define TFT_CS    2   // D1 / GPIO2
#define TFT_DC    1   // D0 / GPIO1
#define TFT_RST   3   // D2 / GPIO3
#define TFT_SCLK  7   // D8 / GPIO7
#define TFT_MOSI  9   // D10 / GPIO9

Adafruit_ST7735 tft(TFT_CS, TFT_DC, TFT_RST);
WebServer server(80);

// The web page is stored in the ESP32's program memory.
const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!doctype html>
<html lang="en">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width,initial-scale=1,viewport-fit=cover">
  <meta name="theme-color" content="#10182e">
  <title>XIAO Draw</title>
  <style>
    :root {
      color-scheme: dark;
      --ink: #f4f7ff;
      --muted: #a6b2ce;
      --card: #18233e;
      --edge: #304267;
      --accent: #68d7ff;
      --active: #1e8ac4;
    }
    * { box-sizing: border-box; -webkit-tap-highlight-color: transparent; }
    body {
      margin: 0;
      min-height: 100vh;
      display: grid;
      place-items: center;
      background: radial-gradient(circle at top, #263b68 0, #10182e 47%, #080c18 100%);
      color: var(--ink);
      font-family: system-ui, -apple-system, BlinkMacSystemFont, "Segoe UI", sans-serif;
    }
    main {
      width: min(100%, 480px);
      padding: 20px 16px 28px;
    }
    header {
      display: flex;
      justify-content: space-between;
      align-items: center;
      margin: 0 3px 16px;
    }
    h1 { margin: 0; font-size: 1.18rem; letter-spacing: .02em; }
    .live {
      display: inline-flex;
      align-items: center;
      gap: 6px;
      color: var(--muted);
      font-size: .78rem;
    }
    .dot {
      width: 8px; height: 8px; border-radius: 50%;
      background: #41e09a; box-shadow: 0 0 12px #41e09a;
    }
    .canvas-wrap {
      padding: 10px;
      border: 1px solid var(--edge);
      border-radius: 20px;
      background: #111a2e;
      box-shadow: 0 18px 45px rgba(0,0,0,.35);
    }
    canvas {
      display: block;
      width: 100%;
      aspect-ratio: 1;
      touch-action: none;
      border-radius: 12px;
      background: #000;
      border: 1px solid #35425e;
      image-rendering: pixelated;
      cursor: crosshair;
    }
    .toolbar {
      display: flex;
      align-items: center;
      justify-content: space-between;
      gap: 10px;
      margin-top: 15px;
    }
    .colours, .tools { display: flex; align-items: center; gap: 8px; }
    button {
      border: 1px solid var(--edge);
      color: var(--ink);
      background: var(--card);
      min-height: 39px;
      border-radius: 10px;
      font: inherit;
      font-size: .82rem;
      padding: 0 13px;
    }
    button:active { transform: scale(.96); }
    .swatch {
      width: 32px; min-width: 32px; height: 32px; min-height: 32px;
      padding: 0; border-radius: 50%;
      border: 3px solid transparent;
    }
    .swatch.selected { border-color: var(--ink); box-shadow: 0 0 0 2px var(--active); }
    #clear { color: #ffb8bd; }
    #status { display: block; margin: 15px 4px 0; color: var(--muted); font-size: .8rem; }
  </style>
</head>
<body>
  <main>
    <header>
      <h1>XIAO Draw</h1>
      <span class="live"><i class="dot"></i> Display connected</span>
    </header>

    <section class="canvas-wrap">
      <canvas id="pad" width="128" height="128" aria-label="Drawing surface"></canvas>
    </section>

    <section class="toolbar">
      <div class="colours" aria-label="Ink colour">
        <button class="swatch selected" data-colour="0" style="background:#ffffff" aria-label="White"></button>
        <button class="swatch" data-colour="1" style="background:#45d9ff" aria-label="Cyan"></button>
        <button class="swatch" data-colour="2" style="background:#ffe94a" aria-label="Yellow"></button>
        <button class="swatch" data-colour="3" style="background:#ff5c72" aria-label="Red"></button>
        <button class="swatch" data-colour="4" style="background:#49e18d" aria-label="Green"></button>
      </div>
      <div class="tools">
        <button id="size">Brush: 2</button>
        <button id="clear">Clear</button>
      </div>
    </section>
    <small id="status">Draw with one finger. Your strokes appear on the display.</small>
  </main>

  <script>
    const canvas = document.getElementById("pad");
    const ctx = canvas.getContext("2d");
    const statusText = document.getElementById("status");
    const sizeButton = document.getElementById("size");
    const clearButton = document.getElementById("clear");

    const colourHex = ["#ffffff", "#45d9ff", "#ffe94a", "#ff5c72", "#49e18d"];
    let colour = 0;
    let brush = 2;
    let drawing = false;
    let localLast = null;
    let sentLast = null;
    let queuedPoint = null;
    let requestInFlight = false;

    function prepareCanvas() {
      ctx.fillStyle = "#000000";
      ctx.fillRect(0, 0, 128, 128);
      ctx.lineCap = "round";
      ctx.lineJoin = "round";
    }

    function pointFromEvent(event) {
      const rect = canvas.getBoundingClientRect();
      return {
        x: Math.max(0, Math.min(127, Math.round((event.clientX - rect.left) * 127 / rect.width))),
        y: Math.max(0, Math.min(127, Math.round((event.clientY - rect.top) * 127 / rect.height)))
      };
    }

    function drawLocal(a, b) {
      ctx.strokeStyle = colourHex[colour];
      ctx.lineWidth = brush;
      ctx.beginPath();
      ctx.moveTo(a.x, a.y);
      ctx.lineTo(b.x, b.y);
      ctx.stroke();
    }

    // Sends only one active request. Fast finger movement is coalesced,
    // keeping the ESP32 responsive while preserving the complete stroke.
    function mirrorNextLine() {
      if (requestInFlight || !queuedPoint || !sentLast) return;

      const start = sentLast;
      const end = queuedPoint;
      queuedPoint = null;
      sentLast = end;
      requestInFlight = true;

      const url = "/line?x0=" + start.x + "&y0=" + start.y +
                  "&x1=" + end.x + "&y1=" + end.y +
                  "&c=" + colour + "&s=" + brush;

      fetch(url, { cache: "no-store" })
        .then(function () { statusText.textContent = "Drawing live on the display"; })
        .catch(function () { statusText.textContent = "Connection lost — reconnect to XIAO-DRAW"; })
        .finally(function () {
          requestInFlight = false;
          if (queuedPoint) setTimeout(mirrorNextLine, 12);
        });
    }

    function beginStroke(event) {
      event.preventDefault();
      drawing = true;
      localLast = pointFromEvent(event);
      sentLast = localLast;
      queuedPoint = localLast; // a zero-length line makes a visible dot
      drawLocal(localLast, localLast);
      canvas.setPointerCapture(event.pointerId);
      mirrorNextLine();
    }

    function moveStroke(event) {
      if (!drawing) return;
      event.preventDefault();
      const current = pointFromEvent(event);
      drawLocal(localLast, current);
      localLast = current;
      queuedPoint = current;
      mirrorNextLine();
    }

    function endStroke(event) {
      if (!drawing) return;
      drawing = false;
      if (queuedPoint) mirrorNextLine();
      try { canvas.releasePointerCapture(event.pointerId); } catch (ignore) {}
    }

    canvas.addEventListener("pointerdown", beginStroke);
    canvas.addEventListener("pointermove", moveStroke);
    canvas.addEventListener("pointerup", endStroke);
    canvas.addEventListener("pointercancel", endStroke);

    document.querySelectorAll(".swatch").forEach(function (button) {
      button.addEventListener("click", function () {
        colour = Number(button.dataset.colour);
        document.querySelectorAll(".swatch").forEach(function (item) {
          item.classList.remove("selected");
        });
        button.classList.add("selected");
      });
    });

    sizeButton.addEventListener("click", function () {
      brush = brush === 1 ? 2 : (brush === 2 ? 3 : 1);
      sizeButton.textContent = "Brush: " + brush;
    });

    clearButton.addEventListener("click", function () {
      if (!confirm("Clear the phone and display canvas?")) return;
      fetch("/clear", { cache: "no-store" })
        .then(function () {
          prepareCanvas();
          statusText.textContent = "Canvas cleared";
        })
        .catch(function () {
          statusText.textContent = "Could not clear the display";
        });
    });

    prepareCanvas();
  </script>
</body>
</html>
)rawliteral";

uint16_t colourFromIndex(int index) {
  switch (index) {
    case 1: return ST77XX_CYAN;
    case 2: return ST77XX_YELLOW;
    case 3: return ST77XX_RED;
    case 4: return ST77XX_GREEN;
    default: return ST77XX_WHITE;
  }
}

int boundedArg(const char *name, int minimum, int maximum) {
  if (!server.hasArg(name)) return minimum;
  return constrain(server.arg(name).toInt(), minimum, maximum);
}

// Adds thickness by drawing neighbouring one-pixel lines.
void drawStroke(int x0, int y0, int x1, int y1, uint16_t colour, int size) {
  int radius = (size - 1) / 2;

  for (int dx = -radius; dx <= radius; dx++) {
    for (int dy = -radius; dy <= radius; dy++) {
      if (dx * dx + dy * dy <= radius * radius) {
        tft.drawLine(x0 + dx, y0 + dy, x1 + dx, y1 + dy, colour);
      }
    }
  }

  // Rounded ends make taps and very short strokes visible.
  tft.fillCircle(x0, y0, radius, colour);
  tft.fillCircle(x1, y1, radius, colour);
}

void handleRoot() {
  server.sendHeader("Cache-Control", "no-store, max-age=0");
  server.send_P(200, "text/html", INDEX_HTML);
}

void handleLine() {
  int x0 = boundedArg("x0", 0, 127);
  int y0 = boundedArg("y0", 0, 127);
  int x1 = boundedArg("x1", 0, 127);
  int y1 = boundedArg("y1", 0, 127);
  int colourIndex = boundedArg("c", 0, 4);
  int size = boundedArg("s", 1, 3);

  drawStroke(x0, y0, x1, y1, colourFromIndex(colourIndex), size);
  server.send(204); // No response body: minimizes drawing delay.
}

void handleClear() {
  tft.fillScreen(ST77XX_BLACK);
  server.send(204);
}

void setup() {
  Serial.begin(115200);

  SPI.begin(TFT_SCLK, -1, TFT_MOSI, TFT_CS);
  tft.initR(INITR_144GREENTAB);
  tft.setRotation(0);
  tft.fillScreen(ST77XX_BLACK);

  WiFi.mode(WIFI_AP);

  if (!WiFi.softAP(AP_SSID, AP_PASSWORD)) {
    tft.setTextColor(ST77XX_RED);
    tft.setTextSize(1);
    tft.setCursor(10, 60);
    tft.print("Wi-Fi AP failed");
    while (true) delay(1000);
  }

  server.on("/", HTTP_GET, handleRoot);
  server.on("/line", HTTP_GET, handleLine);
  server.on("/clear", HTTP_GET, handleClear);
  server.onNotFound([]() {
    server.send(404, "text/plain", "Not found");
  });
  server.begin();

  Serial.println();
  Serial.print("Connect phone to Wi-Fi: ");
  Serial.println(AP_SSID);
  Serial.print("Open: http://");
  Serial.println(WiFi.softAPIP());
}

void loop() {
  server.handleClient();
}
