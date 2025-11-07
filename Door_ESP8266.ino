#include <SPI.h>
#include <MFRC522.h>
#include <Adafruit_Fingerprint.h>
#include <SoftwareSerial.h>

// RFID setup
#define RST_PIN 4  // Reset pin
#define SS_PIN 15  // SDA pin
MFRC522 mfrc522(SS_PIN, RST_PIN);

// Fingerprint setup
#define FINGERPRINT_RX 0 // D3 (BLACK wire)
#define FINGERPRINT_TX 2 // D4 (YELLOW wire)
SoftwareSerial mySerial(FINGERPRINT_RX, FINGERPRINT_TX);
Adafruit_Fingerprint finger(&mySerial);

// Pin for signaling ESP32-C3
#define AUTH_PIN 5 // D1 pin to signal ESP32-C3
// Authorized card UIDs
const byte authorizedCards[][4] = {
  {0x73, 0x03, 0xEB, 0x13},
  {0x34, 0x56, 0x78, 0x90}
};
const int numAuthorizedCards = sizeof(authorizedCards) / 4;

void setup() {
  Serial.begin(9600);
  while (!Serial);

  // Initialize RFID
  SPI.begin();
  mfrc522.PCD_Init();
  Serial.println("RFID and Fingerprint authentication system ready!");

  // Initialize fingerprint sensor
  mySerial.begin(57600);
  finger.begin(57600);
  if (finger.verifyPassword()) {
    Serial.println("Fingerprint sensor found!");
  } else {
    Serial.println("Fingerprint sensor not detected. Check connections.");
    while (1);
  }

  // Initialize AUTH_PIN as OUTPUT
  pinMode(AUTH_PIN, OUTPUT);
  digitalWrite(AUTH_PIN, LOW); // Ensure AUTH_PIN starts LOW
}

void loop() {
  // Check for RFID card
  if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
    handleRFID();
  }

  // Check for fingerprint
  int fingerprintID = getFingerprintID();
  if (fingerprintID != -1) {
    Serial.print("Fingerprint ID ");
    Serial.print(fingerprintID);
    Serial.println(" recognized! Door unlocked.");
    signalESP32(); // Signal ESP32-C3 for authentication
    delay(1500);
    Serial.println("Door locked.");
  }

  // Read from AUTH_PIN as INPUT
  pinMode(AUTH_PIN, INPUT); // Switch AUTH_PIN to INPUT mode
  int pinState = digitalRead(AUTH_PIN);
  Serial.print("AUTH_PIN state: ");
  Serial.println(pinState);

  delay(1000); // Delay to allow reading
  
  pinMode(AUTH_PIN, OUTPUT); // Switch back to OUTPUT mode
  digitalWrite(AUTH_PIN, LOW); // Ensure AUTH_PIN remains LOW when not signaling
}

// Handle RFID logic
void handleRFID() {
  Serial.print("Card UID:");
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    Serial.print(" ");
    Serial.print(mfrc522.uid.uidByte[i], HEX);
  }
  Serial.println();

  if (isCardAuthorized(mfrc522.uid.uidByte, mfrc522.uid.size)) {
    Serial.println("Authorized card! Door unlocked.");
    signalESP32(); // Signal ESP32-C3 for authentication
    delay(1500);
    Serial.println("Door locked.");
  } else {
    Serial.println("Unauthorized card! Door remains locked.");
  }

  mfrc522.PICC_HaltA();  // Halt RFID card
}

// Check if RFID card is authorized
bool isCardAuthorized(byte *uid, byte size) {
  for (int i = 0; i < numAuthorizedCards; i++) {
    bool match = true;
    for (byte j = 0; j < size; j++) {
      if (authorizedCards[i][j] != uid[j]) {
        match = false;
        break;
      }
    }
    if (match) {
      return true;
    }
  }
  return false;
}

// Get fingerprint ID
int getFingerprintID() {
  int result = finger.getImage();
  if (result != FINGERPRINT_OK) {
    if (result == FINGERPRINT_NOFINGER) {
      return -1;  // No fingerprint detected
    } else {
      Serial.println("Error reading fingerprint image.");
      return -1;
    }
  }

  result = finger.image2Tz();
  if (result != FINGERPRINT_OK) {
    if (result == FINGERPRINT_IMAGEMESS) {
      Serial.println("Fingerprint image is too messy.");
    } else if (result == FINGERPRINT_PACKETRECIEVEERR) {
      Serial.println("Error in receiving fingerprint data.");
    } else {
      Serial.println("Fingerprint conversion failed.");
    }
    return -1;
  }

  result = finger.fingerSearch();
  if (result != FINGERPRINT_OK) {
    if (result == FINGERPRINT_NOTFOUND) {
      Serial.println("Unrecognized fingerprint! Access denied.");
    } else {
      Serial.println("Error searching fingerprint.");
    }
    return -1;
  }

  return finger.fingerID;  // Return matched ID
}

// Signal ESP32-C3
void signalESP32() {
  pinMode(AUTH_PIN, OUTPUT);   // Ensure AUTH_PIN is in OUTPUT mode
  digitalWrite(AUTH_PIN, HIGH);  // Set AUTH_PIN HIGH
}