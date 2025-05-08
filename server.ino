#include <WiFi.h>
#include <WebServer.h>
#include <time.h>

// ==== WiFi credentials ====
const char* ssid     = "Tele2_f08006";
const char* password = "zgzwemn5";

// ==== NTP settings ====
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 3600;
const int daylightOffset_sec = 3600;

// ==== Web server ====
WebServer server(80);

// ==== Logging system ====
#define MAX_LOGS 50
struct LogEntry {
  String uid;
  String status;
  String timestamp;
};
LogEntry logs[MAX_LOGS];
int logIndex = 0;

int totalGranted = 0;
int totalDenied = 0;

// ==== Device Location ====
float deviceLat = 59.3293;  // default Stockholm
float deviceLng = 18.0686;

void addLog(String uid, String status, String timestamp) {
  logs[logIndex] = {uid, status, timestamp};
  logIndex = (logIndex + 1) % MAX_LOGS;

  if (status == "GRANTED") totalGranted++;
  else if (status == "DENIED") totalDenied++;
}