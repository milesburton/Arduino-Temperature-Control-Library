#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <OneWire.h>
#include <DallasTemperature.h>

/*
  SETUP INSTRUCTIONS
  
  1) Change WiFi SSID and Password:
      const char* ssid = "YourSSID";
      const char* password = "YourPassword";

  2) Polling Interval (milliseconds):
      const unsigned long READ_INTERVAL = 10000; // 10 seconds

  3) Number of Readings (History Length):
      const int HISTORY_LENGTH = 360; // 1 hour at 10-second intervals
*/

const char* ssid = "YourSSID";
const char* password = "YourPassword";

const int oneWireBus = 4;
const int MAX_SENSORS = 8;
const int HISTORY_LENGTH = 360; 
const unsigned long READ_INTERVAL = 10000;

DeviceAddress sensorAddresses[MAX_SENSORS];
float tempHistory[MAX_SENSORS][HISTORY_LENGTH];
int historyIndex = 0;
int numberOfDevices = 0;
unsigned long lastReadTime = 0;

OneWire oneWire(oneWireBus);
DallasTemperature sensors(&oneWire);
ESP8266WebServer server(80);

String getAddressString(DeviceAddress deviceAddress);
void handleRoot();
void handleSensorList();
void handleTemperature();
void handleHistory();
void updateHistory();

const char MAIN_page[] PROGMEM = R"=====(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport"
        content="width=device-width, initial-scale=1.0">
  <title>Arduino Temperature Control Library - Sensor Data Graph</title>
  <script src="https://cdn.jsdelivr.net/npm/chart.js@3.9.1"></script>
  <link href="https://cdn.jsdelivr.net/npm/tailwindcss@2.1.2/dist/tailwind.min.css"
        rel="stylesheet">
</head>
<body class="bg-gray-100 font-sans min-h-screen flex flex-col">
  <div class="container mx-auto p-6 flex-grow">
    <h1 class="text-2xl font-semibold text-gray-800 mb-4">
      Arduino Temperature Control Library - Sensor Data
    </h1>

    <div class="flex mb-6">
      <div class="cursor-pointer px-4 py-2 bg-blue-500 text-white rounded-lg shadow
                  hover:bg-blue-400 active:scale-95"
           onclick="showTab('dashboard')">
        Dashboard
      </div>
      <div class="cursor-pointer px-4 py-2 bg-gray-200 rounded-lg shadow
                  hover:bg-gray-300 active:scale-95 ml-4"
           onclick="showTab('api')">
        API Docs
      </div>
      <div class="cursor-pointer px-4 py-2 bg-gray-200 rounded-lg shadow
                  hover:bg-gray-300 active:scale-95 ml-4"
           onclick="showTab('setup')">
        Setup
      </div>
    </div>

    <div id="dashboard" class="tab-content">
      <button class="px-6 py-2 bg-green-500 text-white rounded-lg shadow
                     hover:bg-green-400 active:scale-95"
              onclick="refreshData()">
        Refresh Data
      </button>
      <div id="sensors" class="mt-6">
        <div class="text-gray-600">Loading sensor data...</div>
      </div>
    </div>

    <div id="api" class="tab-content hidden mt-8">
      <h2 class="text-xl font-semibold text-gray-800 mb-4">API</h2>
      <div class="bg-white p-6 rounded-lg shadow">
        <div class="mb-4">
          <span class="font-semibold text-blue-500">GET</span>
          <a href="/temperature" class="text-blue-500 hover:underline">/temperature</a>
          <pre class="bg-gray-100 p-4 rounded mt-2">
{
  "sensors": [
    {
      "id": 0,
      "address": "28FF457D1234AB12",
      "celsius": 23.45,
      "fahrenheit": 74.21
    }
  ]
}
          </pre>
        </div>
        <div class="mb-4">
          <span class="font-semibold text-blue-500">GET</span>
          <a href="/sensors" class="text-blue-500 hover:underline">/sensors</a>
          <pre class="bg-gray-100 p-4 rounded mt-2">
{
  "sensors": [
    {
      "id": 0,
      "address": "28FF457D1234AB12"
    }
  ]
}
          </pre>
        </div>
        <div>
          <span class="font-semibold text-blue-500">GET</span>
          <a href="/history" class="text-blue-500 hover:underline">/history</a>
          <pre class="bg-gray-100 p-4 rounded mt-2">
{
  "interval_ms": 10000,
  "sensors": [
    {
      "id": 0,
      "address": "28FF457D1234AB12",
      "history": [23.45, 23.50, 23.48]
    }
  ]
}
          </pre>
        </div>
      </div>
    </div>

    <div id="setup" class="tab-content hidden mt-8">
      <h2 class="text-xl font-semibold text-gray-800 mb-4">Setup Instructions</h2>
      <div class="bg-white p-6 rounded-lg shadow leading-relaxed">
        <p class="mb-4">
          Edit the .ino code to change SSID, password, read interval, and number
          of stored readings (HISTORY_LENGTH).
        </p>
      </div>
    </div>
  </div>

  <footer class="bg-gray-800 text-white p-4 mt-auto text-center">
    <p>&copy; 2025 Miles Burton. All Rights Reserved.</p>
    <p>
      Licensed under the 
      <a href="https://opensource.org/licenses/MIT" class="text-blue-400 hover:underline">
        MIT License
      </a>.
    </p>
  </footer>

  <script>
    const showTab = (name) => {
      document.querySelectorAll('.tab-content').forEach(c => c.classList.add('hidden'));
      document.getElementById(name).classList.remove('hidden');
    };
    const buildSensorsHTML = (sensors) => {
      return sensors.map(s => {
        const chartId = "chart-" + s.id;
        return `
          <div class="bg-white p-6 rounded-lg shadow mb-6">
            <div class="text-lg font-semibold text-blue-500">
              ${s.celsius.toFixed(2)}°C / ${s.fahrenheit.toFixed(2)}°F
            </div>
            <div class="text-sm text-gray-600 mt-2">
              Sensor ID: ${s.id} (${s.address})
            </div>
            <div class="mt-4" style="height: 300px;">
              <canvas id="${chartId}"></canvas>
            </div>
          </div>
        `;
      }).join('');
    };
    const drawChart = (chartId, dataPoints) => {
      const ctx = document.getElementById(chartId);
      if (!ctx) return;
      new Chart(ctx, {
        type: 'line',
        data: {
          labels: dataPoints.map(p => p.x),
          datasets: [{
            label: 'Temp (°C)',
            data: dataPoints,
            borderColor: 'red',
            backgroundColor: 'rgba(255,0,0,0.1)',
            borderWidth: 2,
            pointRadius: 3,
            lineTension: 0.1,
            fill: true
          }]
        },
        options: {
          responsive: true,
          maintainAspectRatio: false,
          plugins: {
            tooltip: {
              mode: 'index',
              intersect: false,
              callbacks: {
                label: (ctx) => {
                  const t = ctx.parsed.x;
                  const d = new Date(t);
                  return `${ctx.dataset.label}: ${ctx.parsed.y.toFixed(2)}°C at ${d.toLocaleTimeString()}`;
                }
              }
            }
          },
          interaction: { mode: 'nearest', intersect: true },
          scales: {
            x: {
              type: 'linear',
              position: 'bottom',
              ticks: {
                autoSkip: true,
                maxTicksLimit: 10,
                callback: (v) => new Date(v).toLocaleTimeString()
              }
            },
            y: {
              grid: { color: 'rgba(0,0,0,0.05)' }
            }
          }
        }
      });
    };
    const refreshData = async () => {
      try {
        const sensorsDiv = document.getElementById('sensors');
        sensorsDiv.innerHTML = '<div class="text-gray-600">Loading sensor data...</div>';
        const td = await fetch('/temperature');
        const tempData = await td.json();
        const hd = await fetch('/history');
        const historyData = await hd.json();
        sensorsDiv.innerHTML = buildSensorsHTML(tempData.sensors);
        tempData.sensors.forEach(s => {
          const chartId = "chart-" + s.id;
          const sensorHist = historyData.sensors.find(h => h.id === s.id);
          if (!sensorHist) return;
          const total = sensorHist.history.length;
          const arr = sensorHist.history.map((v, i) => {
            return { x: Date.now() - (total - 1 - i)*10000, y: v };
          });
          drawChart(chartId, arr);
        });
      } catch(e) {
        console.error(e);
        document.getElementById('sensors').innerHTML = 
          '<div class="text-gray-600">Error loading data.</div>';
      }
    };
    refreshData();
    setInterval(refreshData, 30000);
  </script>
</body>
</html>
)=====";

void setup() {
  Serial.begin(115200);
  Serial.println(WiFi.localIP());
  sensors.begin();

  for (int i = 0; i < MAX_SENSORS; i++) {
    for (int j = 0; j < HISTORY_LENGTH; j++) {
      tempHistory[i][j] = 0;
    }
  }

  numberOfDevices = sensors.getDeviceCount();
  if (numberOfDevices > MAX_SENSORS) {
    numberOfDevices = MAX_SENSORS;
  }

  for (int i = 0; i < numberOfDevices; i++) {
    sensors.getAddress(sensorAddresses[i], i);
  }

  sensors.setResolution(12);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }

  server.on("/", HTTP_GET, handleRoot);
  server.on("/temperature", HTTP_GET, handleTemperature);
  server.on("/sensors", HTTP_GET, handleSensorList);
  server.on("/history", HTTP_GET, handleHistory);

  server.begin();
}

void loop() {
  server.handleClient();
  unsigned long t = millis();
  if (t - lastReadTime >= READ_INTERVAL) {
    updateHistory();
    lastReadTime = t;
  }
}

void updateHistory() {
  sensors.requestTemperatures();
  for (int i = 0; i < numberOfDevices; i++) {
    float tempC = sensors.getTempC(sensorAddresses[i]);
    tempHistory[i][historyIndex] = tempC;
  }
  historyIndex = (historyIndex + 1) % HISTORY_LENGTH;
}

void handleRoot() {
  server.send(200, "text/html", MAIN_page);
}

void handleSensorList() {
  String json = "{\"sensors\":[";
  for (int i = 0; i < numberOfDevices; i++) {
    if (i > 0) json += ",";
    json += "{\"id\":" + String(i) + ",\"address\":\"" + getAddressString(sensorAddresses[i]) + "\"}";
  }
  json += "]}";
  server.send(200, "application/json", json);
}

void handleTemperature() {
  sensors.requestTemperatures();
  String json = "{\"sensors\":[";
  for (int i = 0; i < numberOfDevices; i++) {
    if (i > 0) json += ",";
    float c = sensors.getTempC(sensorAddresses[i]);
    float f = sensors.toFahrenheit(c);
    json += "{\"id\":" + String(i) + ",\"address\":\"" + getAddressString(sensorAddresses[i]) + "\",";
    json += "\"celsius\":" + String(c) + ",\"fahrenheit\":" + String(f) + "}";
  }
  json += "]}";
  server.send(200, "application/json", json);
}

void handleHistory() {
  String json = "{\"interval_ms\":" + String(READ_INTERVAL) + ",\"sensors\":[";
  for (int i = 0; i < numberOfDevices; i++) {
    if (i > 0) json += ",";
    json += "{\"id\":" + String(i) + ",\"address\":\"" + getAddressString(sensorAddresses[i]) + "\",\"history\":[";
    for (int j = 0; j < HISTORY_LENGTH; j++) {
      int idx = (historyIndex - j + HISTORY_LENGTH) % HISTORY_LENGTH;
      if (j > 0) json += ",";
      json += String(tempHistory[i][idx]);
    }
    json += "]}";
  }
  json += "]}";
  server.send(200, "application/json", json);
}

String getAddressString(DeviceAddress deviceAddress) {
  String addr;
  for (uint8_t i = 0; i < 8; i++) {
    if (deviceAddress[i] < 16) addr += "0";
    addr += String(deviceAddress[i], HEX);
  }
  return addr;
}
