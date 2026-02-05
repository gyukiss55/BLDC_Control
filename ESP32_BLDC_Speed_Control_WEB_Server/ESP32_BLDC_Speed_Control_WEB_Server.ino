// ESP32_BLDC_Speed_Control_WEB_Server.ino

#include <WiFi.h>
#include <WebServer.h>
#include <Adafruit_NeoPixel.h>
#include <Adafruit_MCP4725.h>

/* ========= USER CONFIG ========= */
const char* ssid     = "ASUS_98_2G";
const char* password = "LiDoDa#959285$";
 
/* ========= PINS ========= */
#define PWM_PIN     21
#define DIR_PIN     47
#define RGB_PIN     8

/* ========= PWM CONFIG ========= */
#define PWM_CHANNEL     0
#define PWM_FREQ        39000      // 78 kHz
#define PWM_RESOLUTION  10         // 0–1023

/* ========= RGB ========= */
#define NUM_LEDS 1
Adafruit_NeoPixel rgb(NUM_LEDS, RGB_PIN, NEO_GRB + NEO_KHZ800);

/* ========= WEB SERVER ========= */
WebServer server(80);

/* ===================== DAC ===================== */
Adafruit_MCP4725 dac;
#define DAC_ADDR 0x60

/* ========= GLOBALS ========= */
uint16_t speedValue = 20;
bool updateSpeed = false;

/* Blink state */
unsigned long lastBlink = 0;
bool ledState = false;

/* ========= HTML PAGE ========= */
const char HTML_PAGE[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<title>BLDC Speed Control</title>
<style>
body{font-family:Arial;max-width:400px;margin:40px auto}
label{display:inline-block;width:100px}
button{width:70px;margin:4px}
#status{margin-top:15px;font-size:0.9em;word-break:break-all}
</style>
</head>
<body>

<h1>BLDC Speed Control</h1>

<label>IP Address:</label>
<input id="ip" value="%IP%"><br><br>

<label>Speed:</label>
<input type="number" id="speed" min="0" max="1023" value="0"><br><br>

<input type="range" id="slider" min="0" max="1023" value="%SP%" style="width:100%"><br><br>

<button onclick="chg(1)">+1</button>
<button onclick="chg(-1)">-1</button><br>
<button onclick="chg(10)">+10</button>
<button onclick="chg(-10)">-10</button><br>
<button onclick="chg(100)">+100</button>
<button onclick="chg(-100)">-100</button>

<div id="status">Status: —</div>

<script>
const s=document.getElementById("speed");
const r=document.getElementById("slider");
const st=document.getElementById("status");

function clamp(v){return Math.min(1023,Math.max(0,v));}

function send(v){
  const ip=document.getElementById("ip").value;
  const url=`http://${ip}/?Speed=${v}`;
  st.textContent="Status: Sent → "+url;
  fetch(url).catch(()=>{st.textContent="Status: Failed → "+url;});
}

function set(v){
  v=clamp(v);
  s.value=v;
  r.value=v;
  send(v);
}

function chg(d){ set(parseInt(s.value)+d); }

s.onchange=()=>set(parseInt(s.value));
r.oninput=()=>set(parseInt(r.value));
</script>

</body>
</html>
)rawliteral";

/* ========= FUNCTIONS ========= */

void setPWM(uint16_t value) {
  ledcWrite(PWM_CHANNEL, value);
}

void setRGBfromSpeed(uint16_t speed) {
  uint8_t r, g, b;

  if (speed < 512) {
    r = 255 - (speed / 2);
    g = (speed / 2);
    b = 0;
  } else {
    r = 0;
    g = 255 - ((speed - 512) / 2);
    b = (speed - 512) / 2;
  }

  rgb.setPixelColor(0, rgb.Color(r, g, b));
  rgb.show();
}

void handleNotFound() {
  //digitalWrite(led, 1);

  rgb.setPixelColor(0, rgb.Color(128, 128, 128));
  rgb.show();

  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
  Serial.println("Handle not found:");
  Serial.print(message);
  //digitalWrite(led, 0);
}

void readSpeedValue ()
{
  speedValue = constrain(server.arg("Speed").toInt(), 0, 1023);
  updateSpeed = true;
  ledcWrite(PWM_PIN, speedValue);
  dac.setVoltage(speedValue, false);
  setRGBfromSpeed(speedValue);

  Serial.print("Speed=");
  Serial.println(speedValue);

}

void handleRoot() {
  if (server.hasArg("Speed")) {
    readSpeedValue ();
    server.send(200, "text/plain", "OK");
    return;
  }
  String page = HTML_PAGE;
  page.replace("%IP%", WiFi.localIP().toString());
  page.replace("%SP%", String(speedValue));
  server.send(200, "text/html", page);
  Serial.print("Connect:");
  Serial.println(WiFi.localIP().toString());
}

void handleSpeed() {
  Serial.println("Handle Speed:");
  if (server.hasArg("Speed")) {
    readSpeedValue ();
  }
  server.send(200, "text/plain", "OK");
}

void blinkStartupLED() {
  if (millis() - lastBlink >= 500) {
    lastBlink = millis();
    ledState = !ledState;

    if (ledState)
      rgb.setPixelColor(0, rgb.Color(255, 0, 0));
    else
      rgb.setPixelColor(0, 0);

    rgb.show();
  }
}

void testSpeed ()
{
  int halfcycle =100;
  speedValue = 350;
  ledcWrite(PWM_PIN, speedValue);
  dac.setVoltage(speedValue, false);
  delay (1000);

  bool dir = false;
  for (int i = 0; i<10000; i++)
   {
    speedValue = 350;
    ledcWrite(PWM_PIN, speedValue);
    dac.setVoltage(speedValue, false);
    delay (halfcycle);

    speedValue = 200;
    ledcWrite(PWM_PIN, speedValue);
    dac.setVoltage(speedValue, false);
    delay (halfcycle/50);

    digitalWrite(DIR_PIN, dir?HIGH:LOW);
    dir = !dir;

  }

}


/* ========= SETUP ========= */
void setup() {
  Serial.begin(115200);

  /* PWM */

  ledcAttach(PWM_PIN, PWM_FREQ, PWM_RESOLUTION);
  ledcWrite(PWM_PIN, 0);

  pinMode(DIR_PIN, OUTPUT);
  digitalWrite(DIR_PIN, LOW);

  /* RGB */
  rgb.begin();
  rgb.clear();
  rgb.show();

  /* WiFi */
  WiFi.begin(ssid, password);
  Serial.print("Connecting");
  while (WiFi.status() != WL_CONNECTED) {
    delay(300);
    Serial.print(".");
  }

  Serial.println("\nConnected!");
  Serial.println(WiFi.localIP());

  /* Web server */
  server.on("/", handleRoot);
  server.on("/?", handleSpeed);
  server.onNotFound(handleNotFound);
  server.begin();

    /* I2C */
  Wire.begin(8, 9);   // SDA, SCL
  dac.begin(DAC_ADDR);

  ledcWrite(PWM_PIN, speedValue);
  dac.setVoltage(speedValue, false);

  testSpeed ();

}

/* ========= LOOP ========= */
void loop() {
  server.handleClient();

  // Blink red only when speed = 0
  if (!updateSpeed) {
    blinkStartupLED();
  }
}
