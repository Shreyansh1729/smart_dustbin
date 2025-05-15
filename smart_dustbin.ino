#include <ESP8266WiFi.h>
#include <SoftwareSerial.h>

// WiFi credentials
const char* ssid = "Vivo";
const char* password = "00000000";

// Server details
const char* server = "192.168.130.17";  // Your Flask server IP
const int port = 5000;                  // Flask port

// Sensor Pins
#define ODOR_SENSOR_PIN D0             // MQ135 gas sensor
#define LID_SENSOR_PIN D1              // Proximity sensor for lid
#define FULL_SENSOR_PIN D2             // Proximity sensor for bin full
#define LID_LED_PIN D4                 // Lid indicator LED
#define ODOR_LED_PIN D7                // Odor indicator LED

// SIM800L Serial (for LBS)
SoftwareSerial simSerial(D5, D6);      // RX, TX

WiFiClient client;

// Dustbin ID
const String dustbinID = "Dustbin001";

// --- URL Encoding ---
String urlEncode(const String &str) {
  String encodedString = "";
  char c;
  char code0;
  char code1;
  for (int i = 0; i < str.length(); i++) {
    c = str.charAt(i);
    if (isalnum(c)) {
      encodedString += c;
    } else {
      code1 = (c & 0xf) + '0';
      if ((c & 0xf) > 9) code1 = (c & 0xf) - 10 + 'A';
      c = (c >> 4) & 0xf;
      code0 = c + '0';
      if (c > 9) code0 = c - 10 + 'A';
      encodedString += '%';
      encodedString += code0;
      encodedString += code1;
    }
  }
  return encodedString;
}

// --- Send message to Flask server ---
void sendNotification(String message) {
  if (client.connect(server, port)) {
    Serial.println("Connected to server, sending data...");

    String fullMsg = "Dustbin ID: " + dustbinID + " - " + message;
    String encodedMsg = urlEncode(fullMsg);

    client.print(String("GET /notify?msg=") + encodedMsg + " HTTP/1.1\r\n" +
                 "Host: " + String(server) + "\r\n" +
                 "Connection: close\r\n\r\n");

    Serial.println("Sent: " + fullMsg);  // Debug print

    delay(100);
    client.stop();
  } else {
    Serial.println("Failed to connect to server!");
  }
}

// --- Get LBS Location from SIM800L ---
void getSIM800LLocation() {
  simSerial.println("AT+CIPGSMLOC=1,1");
  long timeout = millis() + 5000;
  String locationData = "";

  while (millis() < timeout) {
    if (simSerial.available()) {
      char c = simSerial.read();
      locationData += c;
    }
  }

  if (locationData.indexOf("+CIPGSMLOC:") != -1) {
    int start = locationData.indexOf(":") + 2;
    int end = locationData.indexOf("\r\n", start);
    String result = locationData.substring(start, end);
    Serial.print("Location: "); Serial.println(result);
    sendNotification("Dustbin Location (LBS): " + result);
  } else {
    Serial.println("LBS location fetch failed.");
  }
}

void setup() {
  Serial.begin(9600);
  simSerial.begin(9600);

  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("\nConnected to WiFi!");

  pinMode(LID_SENSOR_PIN, INPUT);
  pinMode(FULL_SENSOR_PIN, INPUT);
  pinMode(ODOR_SENSOR_PIN, INPUT);
  pinMode(LID_LED_PIN, OUTPUT);
  pinMode(ODOR_LED_PIN, OUTPUT);

  // SIM800L init
  simSerial.println("AT");
  delay(1000);
  simSerial.println("AT+CGATT?");
  delay(1000);
  simSerial.println("AT+CSTT=\"\"");
  delay(1000);
  simSerial.println("AT+CIICR");
  delay(3000);
  simSerial.println("AT+CIFSR");
  delay(1000);
}

unsigned long lastLocationCheck = 0;

void loop() {
  // === Odor Detection ===
  int gasDetected = digitalRead(ODOR_SENSOR_PIN);
  if (gasDetected == LOW) {
    Serial.println("⚠️ Odor detected!");
    sendNotification("Odor detected!");
    digitalWrite(ODOR_LED_PIN, HIGH);
  } else {
    digitalWrite(ODOR_LED_PIN, LOW);
    Serial.println("✅ Odor not detected.");
    sendNotification("Odor not detected.");
    delay(5000);
  }

  // === Lid Open Detection ===
  int lidTriggered = digitalRead(LID_SENSOR_PIN);
  if (lidTriggered == LOW) {
    digitalWrite(LID_LED_PIN, HIGH);
    Serial.println("👋 Hand detected: Lid opening.");
    sendNotification("Hand detected: Lid opening.");
  } else {
    digitalWrite(LID_LED_PIN, LOW);
  }

  // === Bin Full Detection ===
  int binFull = digitalRead(FULL_SENSOR_PIN);
  if (binFull == LOW) {
    Serial.println("🗑️ Dustbin is full!");
    sendNotification("Dustbin is full!");
  } else {
    Serial.println("🗑️ Dustbin is not full.");
    delay(5000);
  }

  // === Send Location every 30 seconds ===
  if (millis() - lastLocationCheck > 30000) {
    getSIM800LLocation();
    lastLocationCheck = millis();
  }

  delay(1000);
}
