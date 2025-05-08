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

void setup() {
    Serial.begin(115200);
    WiFi.begin(ssid, password);
    Serial.print("Connecting to WiFi");
  
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }
  
    Serial.println("\nWiFi connected!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

      // ==== Web Routes ====

server.on("/", []() {
    String html = R"rawliteral(
  <!DOCTYPE html>
  <html lang="sv">
  <head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>RFID IoT Dashboard</title>
    <style>
      body { background: #0b0f1a; color: #e0e0e0; font-family: 'Segoe UI', sans-serif; margin: 0; }
      header {
        background: #141a2e;
        color: #fff;
        padding: 20px;
        text-align: center;
        position: relative;
      }
      header img { height: 50px; vertical-align: middle; animation: pulse 2s infinite; }
      @keyframes pulse {
        0% { transform: scale(1); opacity: 0.9; }
        50% { transform: scale(1.1); opacity: 1; }
        100% { transform: scale(1); opacity: 0.9; }
      }
      .lang-toggle {
        position: absolute;
        right: 20px;
        top: 20px;
      }
      .lang-toggle button {
        background: #1f2a3a;
        border: none;
        color: #fff;
        padding: 6px 12px;
        margin-left: 6px;
        border-radius: 4px;
        cursor: pointer;
      }
      .container {
        padding: 20px;
        max-width: 1200px;
        margin: auto;
      }
      .top-section {
        display: flex;
        flex-wrap: wrap;
        gap: 20px;
      }
      .log-section, .chart-section {
        flex: 1;
        min-width: 300px;
        display: flex;
        flex-direction: column;
      }
      h2 { margin-top: 0; }
      input[type="text"] {
        width: 100%;
        padding: 8px;
        margin-bottom: 10px;
        border: 1px solid #ccc;
        border-radius: 4px;
        font-size: 14px;
      }
      table {
        width: 100%;
        border-collapse: collapse;
        font-size: 13px;
      }
      th, td {
        padding: 6px 8px;
        border: 1px solid #333;
      }
      .granted {
        background-color: #2e7d32;
        transition: background-color 0.2s;
      }
      .granted:hover {
        background-color: #388e3c;
      }
      .denied {
        background-color: #c62828;
        transition: background-color 0.2s;
      }
      .denied:hover {
        background-color: #d32f2f;
      }
      canvas {
        max-width: 100%;
      }
      #map {
        height: 300px;
        margin-top: 20px;
        border-radius: 8px;
      }
      footer {
        margin-top: 40px;
        text-align: center;
        font-size: 14px;
        color: #777;
      }
    </style>
    <script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
    <link rel="stylesheet" href="https://unpkg.com/leaflet@1.9.4/dist/leaflet.css"/>
    <script src="https://unpkg.com/leaflet@1.9.4/dist/leaflet.js"></script>
  </head>
  <body>
    <header>
      <div class="lang-toggle">
        <button onclick="setLang('sv')">🇸🇪 Svenska</button>
        <button onclick="setLang('en')">🇬🇧 English</button>
      </div>
      <img src="https://cdn-icons-png.flaticon.com/512/1048/1048953.png">
      <h1 id="title">RFID IoT Säkerhetsdashboard</h1>
      <p id="founder">Grundare: Hewa Eliassi</p>
    </header>
    <div class="container">
      <div class="top-section">
        <div class="log-section">
          <h2 id="logsTitle">Loggar</h2>
          <input type="text" id="search" placeholder="Sök UID, status eller tid...">
          <table id="logTable">
            <thead>
              <tr><th id="uidLabel">UID</th><th id="statusLabel">Status</th><th id="timeLabel">Tid</th></tr>
            </thead>
            <tbody></tbody>
          </table>
        </div>
        <div class="chart-section">
          <h2 id="statsTitle">Statistik</h2>
          <canvas id="accessChart"></canvas>
          <div id="map"></div>
        </div>
      </div>
    </div>
    <footer>
      Powered by ESP32 | RFID & IoT-säkerhetslösning
    </footer>
  
    <script>
      const soundGranted = new Audio('https://actions.google.com/sounds/v1/cartoon/pop.ogg');
      const soundDenied = new Audio('https://actions.google.com/sounds/v1/alarms/beep_short.ogg');
      let latestTimestamp = "";
  
      function fetchStats() {
        fetch('/stats')
          .then(res => res.json())
          .then(data => {
            chart.data.datasets[0].data = [data.granted, data.denied];
            chart.update();
          });
      }
  
      function fetchLogs() {
        fetch('/logs')
          .then(res => res.json())
          .then(data => {
            const table = document.querySelector("#logTable tbody");
            const firstLog = data.logs[data.logs.length - 1];
            if (firstLog && firstLog.timestamp !== latestTimestamp) {
              if (firstLog.status === "GRANTED") soundGranted.play();
              else soundDenied.play();
              latestTimestamp = firstLog.timestamp;
            }
            table.innerHTML = "";
            data.logs.forEach(log => {
              const row = document.createElement("tr");
              row.className = log.status === "GRANTED" ? "granted" : "denied";
              row.innerHTML = `<td>${log.uid}</td><td>${log.status}</td><td>${log.timestamp}</td>`;
              table.prepend(row);
            });
            applySearch();
          });
      }
  
      function applySearch() {
        const filter = document.getElementById("search").value.toLowerCase();
        const rows = document.querySelectorAll("#logTable tbody tr");
        rows.forEach(row => {
          const text = row.textContent.toLowerCase();
          row.style.display = text.includes(filter) ? "" : "none";
        });
      }
  
      function setLang(lang) {
        if (lang === 'sv') {
          document.getElementById("title").innerText = "RFID IoT Säkerhetsdashboard";
          document.getElementById("founder").innerText = "Grundare: Hewa Eliassi";
          document.getElementById("logsTitle").innerText = "Loggar";
          document.getElementById("statsTitle").innerText = "Statistik";
          document.getElementById("uidLabel").innerText = "UID";
          document.getElementById("statusLabel").innerText = "Status";
          document.getElementById("timeLabel").innerText = "Tid";
          document.getElementById("search").placeholder = "Sök UID, status eller tid...";
        } else {
          document.getElementById("title").innerText = "RFID IoT Security Dashboard";
          document.getElementById("founder").innerText = "Founder: Hewa Eliassi";
          document.getElementById("logsTitle").innerText = "Logs";
          document.getElementById("statsTitle").innerText = "Statistics";
          document.getElementById("uidLabel").innerText = "UID";
          document.getElementById("statusLabel").innerText = "Status";
          document.getElementById("timeLabel").innerText = "Time";
          document.getElementById("search").placeholder = "Search UID, status or time...";
        }
      }
  
      document.getElementById("search").addEventListener("input", applySearch);
  
      const ctx = document.getElementById('accessChart').getContext('2d');
      const chart = new Chart(ctx, {
        type: 'bar',
        data: {
          labels: ['GRANTED', 'DENIED'],
          datasets: [{
            label: 'Access Attempts',
            data: [0, 0],
            backgroundColor: ['#66bb6a', '#ef5350']
          }]
        },
        options: {
          responsive: true,
          scales: {
            y: { beginAtZero: true }
          }
        }
      });
  
      var map = L.map('map').setView([59.3293, 18.0686], 13);
      L.tileLayer('https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png', {
        attribution: '&copy; OpenStreetMap contributors'
      }).addTo(map);
      var marker = L.marker([59.3293, 18.0686]).addTo(map);
  
      function fetchLocation() {
        fetch('/getlocation')
          .then(res => res.json())
          .then(data => {
            marker.setLatLng([data.lat, data.lng]);
            map.setView([data.lat, data.lng]);
          });
      }
  
      setInterval(() => {
        fetchStats();
        fetchLogs();
        fetchLocation();
      }, 2000);
  
      fetchStats();
      fetchLogs();
      fetchLocation();
    </script>
  </body>
  </html>
    )rawliteral";
    server.send(200, "text/html", html);
  });

  server.on("/log", HTTP_POST, []() {
    if (server.hasArg("uid") && server.hasArg("status")) {
      String uid = server.arg("uid");
      String status = server.arg("status");

      struct tm timeinfo;
      getLocalTime(&timeinfo);
      char timestamp[20];
      strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", &timeinfo);

      addLog(uid, status, String(timestamp));
      Serial.println("Log received: " + uid + " - " + status);
      server.send(200, "text/plain", "OK");
    } else {
      server.send(400, "text/plain", "Missing parameters");
    }
});