// ESP32_MCP4725_SinusGenerator.ino

#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <Adafruit_MCP4725.h>
#include <math.h>

/* ===================== WiFi ===================== */
const char* ssid     = "ASUS_98_2G";
const char* password = "LiDoDa#959285$";

/* ===================== Web Server ===================== */
WebServer server(80);

/* ===================== DAC ===================== */
Adafruit_MCP4725 dac;
#define DAC_ADDR 0x60

/* ===================== Signal ===================== */
volatile float signalPhase = 0.0f;   // 0–360 deg
volatile float frequency   = 100.0f; // Hz
volatile uint32_t millisTIT   = 0; // msec
volatile uint16_t dacValue   = 0.0f; // 0 V

/* ===================== Timer ===================== */
esp_timer_handle_t sinus_timer;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;

const float TIMER_PERIOD_US = 100.0f;   // 100 µs
const float TIMER_FREQ_HZ   = 1000000.0f / TIMER_PERIOD_US;

/* ===================== HTML ===================== */
const char webpage[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>Sinus generator</title>
</head>
<body>
  <h2>Sinus generator</h2>

  <form action="/" method="GET">
    <label>IP Address:</label><br>
    <input type="text" name="ip" value="%IP%"><br><br>

    <label>frequency (Hz)</label><br>
    <input type="number" step="any" name="Freq" id="Freq" value="%FR%"><br><br>

    <input type="submit" value="Send">
    <button type="button" onclick="mul()">*1.41</button>
    <button type="button" onclick="divv()">/1.41</button>
  </form>

<script>
function mul() {
  let f = document.getElementById("Freq");
  f.value = (parseFloat(f.value) * 1.41).toFixed(2);
  window.location = "/?Freq=" + f.value;
}
function divv() {
  let f = document.getElementById("Freq");
  f.value = (parseFloat(f.value) / 1.41).toFixed(2);
  window.location = "/?Freq=" + f.value;
}
</script>
</body>
</html>
)rawliteral";

/* ===================== Timer ISR ===================== */
void sinusTimerCallback(void *arg) {
  portENTER_CRITICAL_ISR(&timerMux);

  // phase increment
  float phaseStep = 360.0f * frequency / TIMER_FREQ_HZ;
  signalPhase += phaseStep;
  if (signalPhase >= 360.0f) signalPhase -= 360.0f;

  // sine calculation (0–4095)
  float rad = signalPhase * DEG_TO_RAD;
  dacValue = (uint16_t)((sin(rad) * 0.5f + 0.5f) * 4095.0f);

  millisTIT = millis ();
  //dac.setVoltage(dacValue, false);

  portEXIT_CRITICAL_ISR(&timerMux);
}

/* ===================== HTTP Handler ===================== */
void handleRoot() {
  if (server.hasArg("Freq")) {
    float f = server.arg("Freq").toFloat();
    if (f > 0 && f < 5000) {
      portENTER_CRITICAL(&timerMux);
      frequency = f;
      portEXIT_CRITICAL(&timerMux);
    }
  }

  String page = webpage;
  page.replace("%IP%", WiFi.localIP().toString());
  page.replace("%FR%", String(frequency, 2));
  server.send(200, "text/html", page);
  Serial.print("Connect:");
  Serial.println(WiFi.localIP().toString());
}

/* ===================== Setup ===================== */
void setup() {
  Serial.begin(115200);

  signalPhase = 0.0f;
  frequency   = 100.0f;

  /* I2C */
  Wire.begin(8, 9);   // SDA, SCL
  dac.begin(DAC_ADDR);

  /* WiFi */
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());

  /* Web server */
  server.on("/", handleRoot);
  server.begin();

  /* Timer */
  esp_timer_create_args_t timer_args = {
    .callback = &sinusTimerCallback,
    .arg = NULL,
    .dispatch_method = ESP_TIMER_TASK,
    .name = "sinus_timer"
  };

  esp_timer_create(&timer_args, &sinus_timer);
  esp_timer_start_periodic(sinus_timer, 100); // 100 us
}

/* ===================== Loop ===================== */
void loop() {
  server.handleClient();

  {
    static uint32_t lastMS = 0;
    uint16_t dacInt16 = 0;
    portENTER_CRITICAL(&timerMux);
    if (lastMS != millisTIT) {
      lastMS = millisTIT;
      dacInt16 = dacValue;
    }
    portEXIT_CRITICAL(&timerMux);
    dac.setVoltage(dacInt16, false);

  }

  static uint32_t lastPrint = 0;
  if (millis() - lastPrint >= 1000) {
    lastPrint = millis();
    portENTER_CRITICAL(&timerMux);
    float  sph =  signalPhase;
    float  fr =  frequency;
    portEXIT_CRITICAL(&timerMux);

    Serial.print("Phase: ");
    Serial.print(sph);
    Serial.print(" deg, Frequency: ");
    Serial.print(fr);
    Serial.println(" Hz");
  }
}
