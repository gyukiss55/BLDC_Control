#include <WiFi.h>
#include <WebServer.h>
#include <math.h>

// ================= WiFi =================
const char* ssid     = "ASUS_98_2G";
const char* password = "LiDoDa#959285$";

// ================= Globals =================
volatile float signalPhase = 0.0f;   // 0–360 deg
volatile float frequency   = 100.0f; // Hz

esp_timer_handle_t sinus_timer;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;

// ================= PWM =================
const int pwmPin        = 10;   // GPIO 10
const int pwmResolution = 10;   // 0–1023

WebServer server(80);

unsigned long lastPrint = 0;

// ================= HTML =================
const char HTML_PAGE[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>Sinus generator</title>
</head>
<body>
  <h2>Sinus generator</h2>

  <label>IP Address:</label>
  <input type="text" id="ip" value="%IP%"><br><br>

  <label>frequency (Hz):</label>
  <input type="text" id="freq" value="%FR%"><br><br>

  <button onclick="location.href='/?Freq=100'">100 Hz</button>

  <button onclick="
    let f = parseFloat(document.getElementById('freq').value) * 1.41;
    document.getElementById('freq').value = f.toFixed(2);
    location.href='/?Freq=' + f;
  ">*1.41</button>

  <button onclick="
    let f = parseFloat(document.getElementById('freq').value) / 1.41;
    document.getElementById('freq').value = f.toFixed(2);
    location.href='/?Freq=' + f;
  ">/1.41</button>

</body>
</html>
)rawliteral";

// ================= Timer ISR =================
void sinusTimerCallback(void *arg)
{
  portENTER_CRITICAL(&timerMux);

  signalPhase += (360.0f * frequency * 0.0001f);
  if (signalPhase >= 360.0f) signalPhase -= 360.0f;

  float rad = signalPhase * PI / 180.0f;
  float s   = (sinf(rad) + 1.0f) * 0.5f;
  int pwmVal = (int)(s * 1023.0f);

  ledcWrite(pwmPin, pwmVal);

  portEXIT_CRITICAL(&timerMux);
}

void handleRoot() {
  if (server.hasArg("Freq")) {
    float f = server.arg("Freq").toFloat();
    if (f > 0.0f) {
      portENTER_CRITICAL(&timerMux);
      frequency = f;
      portEXIT_CRITICAL(&timerMux);
    }
  }
  String page = HTML_PAGE;
  page.replace("%IP%", WiFi.localIP().toString());
  page.replace("%FR%", String(frequency, 2));
  server.send(200, "text/html", page);
  Serial.print("Connect:");
  Serial.println(WiFi.localIP().toString());
}

void setup() {
  Serial.begin(115200);

  // Initial values
  signalPhase = 0.0f;
  frequency   = 100.0f;

  // ================= WiFi =================
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("Connected. IP: ");
  Serial.println(WiFi.localIP());

  // ================= PWM =================
  ledcAttach(pwmPin, 5000, pwmResolution);
  ledcWrite(pwmPin, 0);

  // ================= Timer =================
/**/
  esp_timer_create_args_t timer_args = {
    .callback = &sinusTimerCallback,
    .arg = NULL,
    .dispatch_method = ESP_TIMER_TASK,
    .name = "sinus_timer"
  };

  esp_timer_create(&timer_args, &sinus_timer);
  esp_timer_start_periodic(sinus_timer, 100); // 100 us



  // ================= Web Server =================
  server.on("/", handleRoot);

  server.begin();
}

void loop() {
  // Handle HTTP clients
  server.handleClient();

  // Print status every second
  unsigned long now = millis();
  if (now - lastPrint >= 1000) {
    lastPrint = now;

    float ph, fr;
    portENTER_CRITICAL(&timerMux);
    ph = signalPhase;
    fr = frequency;
    portEXIT_CRITICAL(&timerMux);

    Serial.print("signalPhase: ");
    Serial.print(ph);
    Serial.print(" deg | frequency: ");
    Serial.print(fr);
    Serial.println(" Hz");
  }
}
