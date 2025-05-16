#include <SoftwareSerial.h>
#include <Adafruit_Fingerprint.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

const uint8_t FP_RX_PIN = 2, FP_TX_PIN = 3;
SoftwareSerial fpSerial(FP_RX_PIN, FP_TX_PIN);
Adafruit_Fingerprint finger(&fpSerial);
LiquidCrystal_I2C lcd(0x27, 16, 2);

bool inEnrollMode = false;

void setup() {
  Serial.begin(57600);      // from ESP
  while (!Serial);

  fpSerial.begin(57600);    // to fingerprint sensor
  Wire.begin();
  lcd.begin(16, 2);
  lcd.backlight();
  lcd.clear();
  lcd.print("Welcome Everyone!");

  finger.begin(57600);
  if (!finger.verifyPassword()) {
    lcd.clear();
    lcd.print("FP sensor FAIL");
    while (1);
  }
}

int enrolFinger(int slot) {
  lcd.clear();
  lcd.print("Put finger");
  while (finger.getImage() != FINGERPRINT_OK);
  finger.image2Tz(1);

  lcd.clear();
  lcd.print("Remove");
  delay(2000);
  while (finger.getImage() != FINGERPRINT_NOFINGER);

  lcd.clear();
  lcd.print("Put again");
  while (finger.getImage() != FINGERPRINT_OK);
  finger.image2Tz(2);

  if (finger.createModel() != FINGERPRINT_OK) return -1;
  if (finger.storeModel(slot)  != FINGERPRINT_OK) return -2;
  return 0;
}

bool checkFinger(int slot) {
  lcd.clear();
  lcd.print("Scan finger");
  while (finger.getImage() != FINGERPRINT_OK);
  finger.image2Tz();
  if (finger.fingerFastSearch() != FINGERPRINT_OK) return false;
  return (finger.fingerID == slot);
}

void loop() {
  // Only act when ESP sends a command
  if (!Serial.available()) return;
  String cmd = Serial.readStringUntil('\n');
  cmd.trim();

  // Toggle enroll off (should reset to idle)
  if (cmd == "ENROL_OFF") {
    inEnrollMode = false;
    lcd.clear();
    lcd.print("Welcome Everyone!");
    return;
  }

  // Toggle enroll on (enter enroll-waiting state)
  if (cmd == "ENROL_ON") {
    inEnrollMode = true;
    lcd.clear();
    lcd.print("Enroll mode");
    return;
  }

  // ESP is sending ENROL:<sapId> or CHECK:<slot>
  if (cmd.startsWith("ENROL:") || cmd.startsWith("CHECK:")) {
    bool isEnroll = cmd.startsWith("ENROL:");
    int slot;
    String payload = cmd.substring(isEnroll ? 6 : 6);
    if (isEnroll) {
      // Extract sapId if you need it (payload) but slot will be -1
      slot = -1;
    } else {
      slot = payload.toInt();
    }

    // Enrollment flow
    if (slot < 0) {
      inEnrollMode = true;
      // choose a template slot
      int newSlot = finger.getTemplateCount() + 1;
      int rc = enrolFinger(newSlot);

      lcd.clear();
      if (rc == 0) {
        lcd.print("Enrolled ID ");
        lcd.print(newSlot);
        Serial.print("ENROL_OK:");
        Serial.print(payload);     // sapId
        Serial.print(':');
        Serial.println(newSlot);
      } else {
        lcd.print("Enroll failed");
        Serial.println("ENROL_FAIL");
      }
      delay(2000);
      // back to idle
      inEnrollMode = false;
      lcd.clear();
      lcd.print("Welcome Everyone!");
      return;
    }

    // Check flow
    {
      inEnrollMode = false;
      bool ok = checkFinger(slot);

      lcd.clear();
      if (ok) {
        lcd.print("Attendance OK");
        Serial.print("FINGER_OK:");
        Serial.println(slot);
      } else {
        lcd.print("No Match!");
        Serial.println("FINGER_BAD");
      }
      delay(2000);
      lcd.clear();
      lcd.print("Welcome!");
      return;
    }
  }
}
