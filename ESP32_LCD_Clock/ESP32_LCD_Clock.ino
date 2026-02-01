
#include <WiFi.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <time.h>

#define I2C_SDA 21
#define I2C_SCL 22

// ---------- WIFI SETTINGS ----------
const char* ssid     = "RTAX999";
const char* password = "LiDoDa#959285$";

// ---------- NTP SETTINGS ----------
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 0;        // UTC offset (0 = GMT)
const int   daylightOffset_sec = 0;   // DST offset (seconds)


// ---------- LCD SETTINGS ----------
LiquidCrystal_I2C lcd(0x27, 16, 2);   // Change 0x27 if needed

void setup() {
  Serial.begin(115200);

  // WiFi connect
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.print("Connect:");
  Serial.println(WiFi.localIP().toString());

  Wire.begin(I2C_SDA, I2C_SCL);
// LCD init
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Connecting WiFi");
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("WiFi Connected");
  lcd.setCursor(0, 1);
  lcd.print(WiFi.localIP().toString());

  // Time init
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  delay(5000);
}

void loop() {
  struct tm timeinfo;

  if (!getLocalTime(&timeinfo)) {
    lcd.clear();
    lcd.print("Time Error");
    delay(2000);
    return;
  }

  char timeStr[9];
  char dateStr[17];

  strftime(timeStr, sizeof(timeStr), "%H:%M:%S", &timeinfo);
  strftime(dateStr, sizeof(dateStr), "%Y-%m-%d", &timeinfo);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(timeStr);

  lcd.setCursor(0, 1);
  lcd.print(dateStr);

  Serial.println(timeStr);
  Serial.println(dateStr);

  delay(1000);
}
