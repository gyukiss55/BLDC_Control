// ESP32_ScanI2CAddresses.ino
#include <Wire.h>

#define I2C_SDA 8
#define I2C_SCL 9

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("ESP32 I2C Scanner");
  Serial.println("================");

  Wire.begin(I2C_SDA, I2C_SCL);

  scanI2C();
}

void loop() {
  // Scan only once
}

void scanI2C() {
  byte error, address;
  int devicesFound = 0;

  Serial.println("Scanning I2C bus...");

  for (address = 1; address < 127; address++) {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();

    if (error == 0) {
      Serial.print("I2C device found at address 0x");
      if (address < 16)
        Serial.print("0");
      Serial.println(address, HEX);
      devicesFound++;
    }
  }

  if (devicesFound == 0) {
    Serial.println("No I2C devices found.");
  } else {
    Serial.print("Scan complete. Devices found: ");
    Serial.println(devicesFound);
  }

  Serial.println("Done.");
}
