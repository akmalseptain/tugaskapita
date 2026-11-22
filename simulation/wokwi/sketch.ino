#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <ESP32Servo.h>
#include <WiFi.h>

// Pinout sesuai SmartParking_OTA.ino & Skematik KiCad
#define BTN_RFID_VALID   33
#define BTN_RFID_INVALID 35
#define BTN_EXIT_GATE    34
#define BTN_SLOT_1       27
#define BTN_SLOT_2       26
#define BTN_SLOT_3       25

#define LED_GREEN_1      12
#define LED_RED_1        13
#define BUZZER_1         14
#define SERVO_1_PIN      32
#define SERVO_2_PIN      4

LiquidCrystal_I2C lcd(0x27, 16, 2);
Servo servoEntry;
Servo servoExit;

bool slotOccupied[3] = {false, false, false};
bool otaV2Enabled = false; // Dapat diaktifkan via Serial: ketik "OTA_V2"

void showSlots() {
  int freeCount = (!slotOccupied[0]) + (!slotOccupied[1]) + (!slotOccupied[2]);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(otaV2Enabled ? "FW:v2.0 [OTA OK]" : "FW:v1.0 SmartPrk");
  lcd.setCursor(0, 1);
  lcd.print("Free Slot: ");
  lcd.print(freeCount);
  lcd.print("/3");
}

void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22);
  lcd.init();
  lcd.backlight();

  pinMode(BTN_RFID_VALID, INPUT_PULLUP);
  pinMode(BTN_RFID_INVALID, INPUT);
  pinMode(BTN_EXIT_GATE, INPUT);
  pinMode(BTN_SLOT_1, INPUT_PULLUP);
  pinMode(BTN_SLOT_2, INPUT_PULLUP);
  pinMode(BTN_SLOT_3, INPUT_PULLUP);

  pinMode(LED_GREEN_1, OUTPUT);
  pinMode(LED_RED_1, OUTPUT);
  pinMode(BUZZER_1, OUTPUT);

  servoEntry.attach(SERVO_1_PIN);
  servoExit.attach(SERVO_2_PIN);
  servoEntry.write(0);
  servoExit.write(0);

  WiFi.begin("Wokwi-GUEST", "", 6);
  Serial.println("=== Smart Parking ESP32 + OTA Simulator ===");
  Serial.println("Ketik 'OTA_V2' di Serial Monitor untuk simulasi Update OTA v2.0!");
  showSlots();
}

void loop() {
  // Simulasi perintah OTA via Serial Monitor
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    if (cmd == "OTA_V2") {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("OTA Updating...");
      lcd.setCursor(0, 1);
      lcd.print("Flashing v2.0");
      delay(1200);
      otaV2Enabled = true;
      Serial.println("[OTA] Firmware updated to v2.0 (Full-Slot Lock enabled)!");
      showSlots();
    }
  }

  // Toggle Slot 1, 2, 3
  int slotPins[3] = {BTN_SLOT_1, BTN_SLOT_2, BTN_SLOT_3};
  for (int i = 0; i < 3; i++) {
    if (digitalRead(slotPins[i]) == LOW) {
      slotOccupied[i] = !slotOccupied[i];
      Serial.printf("Slot %d -> %s\n", i + 1, slotOccupied[i] ? "TERISI" : "KOSONG");
      showSlots();
      delay(350);
    }
  }

  // Simulasi Tap RFID Valid
  if (digitalRead(BTN_RFID_VALID) == LOW) {
    bool allFull = slotOccupied[0] && slotOccupied[1] && slotOccupied[2];
    if (otaV2Enabled && allFull) {
      lcd.clear();
      lcd.setCursor(0, 0); lcd.print("Parking Full!");
      lcd.setCursor(0, 1); lcd.print("v2.0 Gate Locked");
      digitalWrite(LED_RED_1, HIGH);
      tone(BUZZER_1, 400, 400);
      delay(1200);
      digitalWrite(LED_RED_1, LOW);
    } else {
      lcd.clear();
      lcd.setCursor(0, 0); lcd.print("Access Granted");
      lcd.setCursor(0, 1); lcd.print("Entry Gate Open");
      digitalWrite(LED_GREEN_1, HIGH);
      tone(BUZZER_1, 1200, 150);
      servoEntry.write(90);
      delay(1800);
      servoEntry.write(0);
      digitalWrite(LED_GREEN_1, LOW);
    }
    showSlots();
  }

  // Simulasi Tap RFID Invalid (GPIO35 input-only, aktif LOW saat ditekan)
  if (digitalRead(BTN_RFID_INVALID) == LOW) {
    lcd.clear();
    lcd.setCursor(0, 0); lcd.print("Access Denied!");
    lcd.setCursor(0, 1); lcd.print("Invalid Card");
    digitalWrite(LED_RED_1, HIGH);
    tone(BUZZER_1, 300, 500);
    delay(1200);
    digitalWrite(LED_RED_1, LOW);
    showSlots();
  }

  // Simulasi Exit Gate (GPIO34 input-only)
  if (digitalRead(BTN_EXIT_GATE) == LOW) {
    lcd.clear();
    lcd.setCursor(0, 0); lcd.print("Exit Gate Open");
    lcd.setCursor(0, 1); lcd.print("Thank You!");
    servoExit.write(90);
    delay(1800);
    servoExit.write(0);
    showSlots();
  }

  delay(50);
}
