String doorUID;

void setup() {
  // Initialize serial communication
  Serial.begin(9600);

  // Generate a unique door UID based on the ESP32-C3's MAC address
  uint64_t chipId = ESP.getEfuseMac(); // Get unique chip ID
  doorUID = String((uint32_t)(chipId >> 32), HEX) + String((uint32_t)chipId, HEX);

  // Print the Door ID
  Serial.print("Door ID: ");
  Serial.println(doorUID);
}

void loop() {
  // Main loop code
}
