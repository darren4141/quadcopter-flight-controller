#include <WiFi.h>
#include <Wire.h>
#include <MPU6050.h>
#include <WebServer.h>
const char* ssid = "Darren’s iPhone";
const char* password = "password";

MPU6050 mpu;
WebServer server(80);

float pitchOffset = 0;
float rollOffset = 0;

void calibrateOffsets() {
  int16_t ax, ay, az;
  float pitchSum = 0, rollSum = 0;

  for (int i = 0; i < 20; i++) {
    mpu.getAcceleration(&ax, &ay, &az);
    float pitch = atan2(ax, sqrt(ay * ay + az * az)) * 180.0 / PI;
    float roll = atan2(ay, sqrt(ax * ax + az * az)) * 180.0 / PI;
    pitchSum += pitch;
    rollSum += roll;
    delay(50);
  }

  pitchOffset = pitchSum / 20.0;
  rollOffset = rollSum / 20.0;
  Serial.println("Calibration done");
}

void handleRoot() {
  server.send(200, "text/html", R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <title>Quadcopter Tilt Visualizer</title>
  <style>
    body {
      margin: 0;
      font-family: sans-serif;
      height: 100vh;
      display: grid;
      grid-template-rows: auto 1fr auto;
      grid-template-columns: 1fr 300px;
      background-color: black;
      color: white;
    }
    h1 {
      grid-column: span 2;
      text-align: center;
      margin: 10px 0;
      font-size: 2em;
    }
    #render-area {
      background-color: black;
      display: flex;
      justify-content: center;
      align-items: center;
      height: 70%;
    }
    #three-canvas {
      width: 100%;
      height: 100%;
    }
    #overlay {
      grid-column: span 2;
      background-color: white;
      color: black;
      padding: 15px;
      text-align: center;
    }
    .info {
      margin: 0 10px;
      display: inline-block;
    }
    button {
      margin: 0 10px;
      padding: 10px 20px;
      font-size: 1em;
    }
    #log-panel {
      background-color: #111;
      padding: 10px;
      overflow-y: auto;
      height: calc(100vh - 200px);
      border-left: 1px solid #444;
    }
    #log-panel table {
      width: 100%;
      border-collapse: collapse;
      color: #ccc;
      font-size: 0.9em;
    }
    #log-panel th, #log-panel td {
      padding: 4px 6px;
      text-align: center;
    }
    #log-panel th {
      background-color: #222;
      position: sticky;
      top: 0;
    }
    #log-panel tr:nth-child(even) {
      background-color: #1a1a1a;
    }
  </style>
</head>
<body>
  <h1>Quadcopter Tilt Visualizer</h1>

  <div id="render-area">
    <canvas id="three-canvas"></canvas>
  </div>

  <div id="log-panel">
    <table>
      <thead>
        <tr>
          <th>Time</th>
          <th>Pitch (°)</th>
          <th>Roll (°)</th>
        </tr>
      </thead>
      <tbody id="log-body">
      </tbody>
    </table>
  </div>

  <div id="overlay">
    <span class="info">Pitch: <span id="pitch">--</span>°</span>
    <span class="info">Roll: <span id="roll">--</span>°</span>
    <span class="info">Motor 1: <span id="m1">0%</span></span>
    <span class="info">Motor 2: <span id="m2">0%</span></span>
    <span class="info">Motor 3: <span id="m3">0%</span></span>
    <span class="info">Motor 4: <span id="m4">0%</span></span>
    <br><br>
    <button onclick="recalibrate()">Recalibrate</button>
    <button onclick="start()">Start</button>
    <button onclick="emergencyStop()">Emergency Stop</button>
  </div>

  <script src="https://cdn.jsdelivr.net/npm/three@0.150.1/build/three.min.js"></script>
  <script>
    let scene = new THREE.Scene();
    let camera = new THREE.PerspectiveCamera(75, window.innerWidth / (window.innerHeight * 0.7), 0.1, 1000);
    let renderer = new THREE.WebGLRenderer({ canvas: document.getElementById('three-canvas') });
    renderer.setSize(window.innerWidth * 0.7, window.innerHeight * 0.7);

    let geometry = new THREE.BoxGeometry(2, 0.2, 2);
    let material = new THREE.MeshNormalMaterial();
    let cube = new THREE.Mesh(geometry, material);
    scene.add(cube);
    camera.position.z = 3;

    function animate() {
      requestAnimationFrame(animate);
      renderer.render(scene, camera);
    }
    animate();

    function updateOrientation(pitch, roll) {
      cube.rotation.z = pitch * Math.PI / 180;
      cube.rotation.x = roll * Math.PI / 180;

      document.getElementById("pitch").textContent = pitch.toFixed(1);
      document.getElementById("roll").textContent = roll.toFixed(1);

      const now = new Date();
      const time = now.toLocaleTimeString();
      const row = document.createElement("tr");
      row.innerHTML = `<td>${time}</td><td>${pitch.toFixed(1)}</td><td>${roll.toFixed(1)}</td>`;

      const logBody = document.getElementById("log-body");
      logBody.appendChild(row);
      if (logBody.rows.length > 10) {
        logBody.removeChild(logBody.firstChild);
      }
      logBody.scrollTop = logBody.scrollHeight;
    }

    function fetchData() {
      fetch("/data")
        .then(res => res.json())
        .then(data => {
          updateOrientation(data.pitch, data.roll);
          document.getElementById("m1").textContent = "65%";
          document.getElementById("m2").textContent = "67%";
          document.getElementById("m3").textContent = "62%";
          document.getElementById("m4").textContent = "70%";
        });
    }

    setInterval(fetchData, 100);

    function recalibrate() {
      const btn = document.querySelector("button");
      btn.disabled = true;
      btn.textContent = "Calibrating...";
      fetch("/recalibrate").then(() => {
        btn.textContent = "Recalibrate";
        btn.disabled = false;
      });
    }

    function start() {
      alert("Start clicked (placeholder)");
    }

    function emergencyStop() {
      alert("Emergency stop clicked (placeholder)");
    }
  </script>
</body>
</html>
  )rawliteral");
}


void handleData() {
  int16_t ax, ay, az;
  mpu.getAcceleration(&ax, &ay, &az);

  float pitch = atan2(ax, sqrt(ay * ay + az * az)) * 180.0 / PI - pitchOffset;
  float roll  = atan2(ay, sqrt(ax * ax + az * az)) * 180.0 / PI - rollOffset;

  String json = "{\"pitch\":" + String(pitch, 2) + ",\"roll\":" + String(roll, 2) + "}";
  server.send(200, "application/json", json);
}


void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22);
  mpu.initialize();
  mpu.setDLPFMode(3);

  if (mpu.testConnection()) {
    Serial.println("MPU6050 connected");
  } else {
    Serial.println("MPU6050 connection failed");
  }

  calibrateOffsets();

  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected, IP address: ");
  Serial.println(WiFi.localIP());

  server.on("/", handleRoot);
  server.on("/data", handleData);
  server.on("/recalibrate", HTTP_GET, []() {
    calibrateOffsets();
    server.send(200, "text/plain", "OK");
  });

  server.begin();
}

void loop() {
  server.handleClient();
}