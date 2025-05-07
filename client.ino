#include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <SPI.h>
#include <MFRC522.h>

// ==== WiFi ====
const char* ssid     = "Tele2_f08006";
const char* password = "zgzwemn5";

// ==== Server IP ====
const char* serverIP = "192.168.0.51";
const int serverPort = 80;

// ==== WhatsApp CallMeBot ====
const String phone = "46763282158";
const String apikey = "2673624";

// ==== RFID ====
#define RST_PIN 9
#define SS_PIN  7
MFRC522 mfrc522(SS_PIN, RST_PIN);

// ==== LCD ====
LiquidCrystal_I2C lcd(0x27, 16, 2);

// ==== LEDs ====
#define GREEN_LED 2
#define RED_LED   18

// ==== UID-lista ====
const String authorizedUIDs[] = {
  "3ABED7A3",
  "11223344"
};

// ==== Simulerad GPS-position ====
float latitude = 59.3293;
float longitude = 18.0686;

// ==== Hjälpfunktioner ====
bool isAuthorized(String uid) {
  for (String valid : authorizedUIDs) {
    if (uid == valid) return true;
  }
  return false;
}

void sendLogToServer(String uid, String status) {
    if (WiFi.status() == WL_CONNECTED) {
      HTTPClient http;
      String url = "http://" + String(serverIP) + ":" + String(serverPort) + "/log";
      http.begin(url);
      http.addHeader("Content-Type", "application/x-www-form-urlencoded");
      String postData = "uid=" + uid + "&status=" + status;
      int httpCode = http.POST(postData);
      Serial.printf("Log POST status: %d\n", httpCode);
      http.end();
    }
  }

  void sendLocationToServer(float lat, float lng) {
    if (WiFi.status() == WL_CONNECTED) {
      HTTPClient http;
      String url = "http://" + String(serverIP) + ":" + String(serverPort) + "/location";
      http.begin(url);
      http.addHeader("Content-Type", "application/x-www-form-urlencoded");
      String postData = "lat=" + String(lat, 6) + "&lng=" + String(lng, 6);
      int httpCode = http.POST(postData);
      Serial.printf("Location POST status: %d\n", httpCode);
      http.end();
    }
  }

  void sendWhatsApp(String message) {
    message.replace(" ", "+");
    message.replace("\xE5", "%E5"); // å
    message.replace("\xE4", "%E4"); // ä
    message.replace("\xF6", "%F6"); // ö
  
    String url = "https://api.callmebot.com/whatsapp.php?phone=" + phone + "&text=" + message + "&apikey=" + apikey;
    HTTPClient http;
    http.begin(url);
    int httpCode = http.GET();
    Serial.printf("WhatsApp GET status: %d\n", httpCode);
    http.end();
  }

  // ==== Setup ====
void setup() {
    Serial.begin(115200);
    pinMode(GREEN_LED, OUTPUT);
    pinMode(RED_LED, OUTPUT);
    digitalWrite(GREEN_LED, LOW);
    digitalWrite(RED_LED, LOW);
  
    Wire.begin(1, 0);
    lcd.init();
    lcd.backlight();
    lcd.setCursor(0, 0);
    lcd.print("Connecting WiFi...");
  
    WiFi.begin(ssid, password);
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
      delay(500);
      Serial.print(".");
      attempts++;
    }
  
    lcd.clear();
    if (WiFi.status() == WL_CONNECTED) {
      lcd.print("WiFi connected");
      Serial.println("\nWiFi connected!");
    } else {
      lcd.print("WiFi failed!");
      Serial.println("\nWiFi failed!");
      return;
    }
  
    SPI.begin(6, 4, 5, 7);
    mfrc522.PCD_Init();
    delay(1000);
    lcd.clear();
    lcd.print("Waiting for card...");
  }

// ==== Loop ====
void loop() {
    static unsigned long lastLocationUpdate = 0;
  
    if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
      String uid = "";
      for (byte i = 0; i < mfrc522.uid.size; i++) {
        uid += (mfrc522.uid.uidByte[i] < 0x10 ? "0" : "") + String(mfrc522.uid.uidByte[i], HEX);
      }
      uid.toUpperCase();
  
      bool authorized = isAuthorized(uid);
  
      Serial.printf("UID: %s -> %s\n", uid.c_str(), authorized ? "AUTHORIZED" : "DENIED");
  
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(authorized ? "Access Granted" : "Access Denied");
      lcd.setCursor(0, 1);
      lcd.print("UID: ");
      lcd.print(uid);
  
      digitalWrite(GREEN_LED, authorized ? HIGH : LOW);
      digitalWrite(RED_LED, authorized ? LOW : HIGH);
  
      if (!authorized) {
        sendWhatsApp("ACCESS DENIED | UID: " + uid);
      }
  
      sendLogToServer(uid, authorized ? "GRANTED" : "DENIED");
  
      delay(3000);
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Waiting for card...");
      digitalWrite(GREEN_LED, LOW);
      digitalWrite(RED_LED, LOW);
    }
  
    // Skicka plats var 15:e sekund
    if (millis() - lastLocationUpdate > 15000) {
      sendLocationToServer(latitude, longitude);
      lastLocationUpdate = millis();
    }
  }