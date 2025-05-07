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