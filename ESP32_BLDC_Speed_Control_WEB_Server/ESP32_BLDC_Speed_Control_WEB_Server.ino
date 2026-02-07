// ESP32_BLDC_Speed_Control_WEB_Server.ino

#include <WiFi.h>
#include <WebServer.h>
#include <Adafruit_NeoPixel.h>
#include <Adafruit_MCP4725.h>

/* ================= WIFI ================= */
const char* ssid     = "ASUS_98_2G";
const char* password = "LiDoDa#959285$";

/* ================= GLOBAL PARAMETERS ================= */
int16_t speed = 0;          // 0–1023
bool enableMotor = false;   // default false
bool direction = true;      // true = forward

/* ================= GPIO ================= */
constexpr uint8_t PIN_ENABLE    = 48;
constexpr uint8_t PIN_DIRECTION = 47;

/* ================= OBJECTS ================= */
WebServer server(80);
Adafruit_MCP4725 dac;

/* ================= TIMER ================= */
unsigned long lastPrint = 0;

/* ================= HTML ================= */
String makeHTML()
{
  IPAddress ip = WiFi.localIP();

  String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>Speed Web Server</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">

  <script>
    function setSpeed(val)
    {
      val = Math.max(0, Math.min(1023, val));
      document.getElementById("speed").value = val;
      document.getElementById("slider").value = val;
      send();
    }

    function sliderChanged(val)
    {
      document.getElementById("speed").value = val;
      send();
    }

    function send()
    {
      document.getElementById("form").submit();
    }

    function resetSpeed()
    {
      document.getElementById("speed").value = 0;
      document.getElementById("slider").value = 0;
      send();
    }
  </script>
</head>

<body>
<h1>Speed Web Server</h1>

<form id="form" action="/" method="get">

<label>IP Address:</label><br>
<input type="text" value="%IP%" readonly><br><br>

<label>Enable:</label><br>
<input type="radio" name="Enable" value="1" %EN1% onchange="resetSpeed()"> True<br>
<input type="radio" name="Enable" value="0" %EN0% onchange="resetSpeed()"> False<br><br>

<label>Speed:</label><br>
<input type="number" id="speed" name="Speed" min="0" max="1023" value="%SPD%">
<br>

<button type="button" onclick="setSpeed(parseInt(speed.value)+1)">+1</button>
<button type="button" onclick="setSpeed(parseInt(speed.value)-1)">-1</button>
<button type="button" onclick="setSpeed(parseInt(speed.value)+10)">+10</button>
<button type="button" onclick="setSpeed(parseInt(speed.value)-10)">-10</button>
<button type="button" onclick="setSpeed(parseInt(speed.value)+100)">+100</button>
<button type="button" onclick="setSpeed(parseInt(speed.value)-100)">-100</button>
<br><br>

<input type="range" id="slider" min="0" max="1023" value="%SPD%"
       oninput="sliderChanged(this.value)">
<br><br>

<label>Direction:</label><br>
<input type="radio" name="Direction" value="1" %DIR1% onchange="resetSpeed()"> Forward<br>
<input type="radio" name="Direction" value="0" %DIR0% onchange="resetSpeed()"> Reverse<br>

</form>
</body>
</html>
)rawliteral";

  html.replace("%IP%", ip.toString());
  html.replace("%SPD%", String(speed));
  html.replace("%EN1%", enableMotor ? "checked" : "");
  html.replace("%EN0%", !enableMotor ? "checked" : "");
  html.replace("%DIR1%", direction ? "checked" : "");
  html.replace("%DIR0%", !direction ? "checked" : "");

  return html;
}

/* ================= HTTP HANDLER ================= */
void handleRoot()
{
  bool paramChanged = false;

  if (server.hasArg("Enable")) {
    bool newEnable = server.arg("Enable").toInt();
    if (newEnable != enableMotor) {
      enableMotor = newEnable;
      speed = 0;
      paramChanged = true;
    }
  }

  if (server.hasArg("Direction")) {
    bool newDir = server.arg("Direction").toInt();
    if (newDir != direction) {
      direction = newDir;
      speed = 0;
      paramChanged = true;
    }
  }

  if (server.hasArg("Speed")) {
    speed = constrain(server.arg("Speed").toInt(), 0, 1023);
    paramChanged = true;
  }

  /* GPIO effects */
  digitalWrite(PIN_ENABLE, enableMotor ? HIGH : LOW);
  digitalWrite(PIN_DIRECTION, direction ? HIGH : LOW);

  /* DAC output */
  if (enableMotor) {
    dac.setVoltage(speed << 2, false);  // 10-bit → 12-bit
  } else {
    dac.setVoltage(0, false);
  }

  server.send(200, "text/html", makeHTML());
}

/* ================= SETUP ================= */
void setup()
{
  Serial.begin(115200);

  pinMode(PIN_ENABLE, OUTPUT);
  pinMode(PIN_DIRECTION, OUTPUT);

  enableMotor = false;
  direction   = true;
  speed       = 0;

  digitalWrite(PIN_ENABLE, LOW);
  digitalWrite(PIN_DIRECTION, HIGH);

  Wire.begin();
  dac.begin(0x60);

  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.print("WebServer IP: ");
  Serial.println(WiFi.localIP());

  server.on("/", handleRoot);
  server.begin();

  Serial.println("HTTP server started");
}

/* ================= LOOP ================= */
void loop()
{
  server.handleClient();

  if (millis() - lastPrint >= 1000) {
    lastPrint = millis();
    Serial.print("Enable=");
    Serial.print(enableMotor);
    Serial.print(" | Speed=");
    Serial.print(speed);
    Serial.print(" | Direction=");
    Serial.println(direction);
  }
}


