#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <SPIFFS.h>
#include <MPU6050.h>

const char* ssid = "Darren’s iPhone";
const char* password = "password";

WebServer server(80);
MPU6050 mpu;

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
  Serial.println("Calibration done.");
}

void handleRoot() {
  server.send(200, "text/html", R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <title>ESP32 Tilt 3D Viewer</title>
  <style>
    body { margin: 0; overflow: hidden; }
    button {
      position: absolute;
      top: 20px;
      left: 20px;
      z-index: 10;
      padding: 10px;
    }
  </style>
</head>
<body>
  <button id="recalBtn" onclick="recalibrate()">Recalibrate</button>
  <script type="module">
    import * as THREE from 'https://cdn.jsdelivr.net/npm/three@0.150.1/build/three.module.js';
    import { GLTFLoader } from 'https://cdn.jsdelivr.net/npm/three@0.150.1/examples/jsm/loaders/GLTFLoader.js';

    const scene = new THREE.Scene();
    const camera = new THREE.PerspectiveCamera(75, window.innerWidth/window.innerHeight, 0.1, 1000);
    const renderer = new THREE.WebGLRenderer();
    renderer.setSize(window.innerWidth, window.innerHeight);
    document.body.appendChild(renderer.domElement);

    const loader = new GLTFLoader();
    let model = null;

    loader.load('/droneModel.glb', function(gltf) {
      model = gltf.scene;
      scene.add(model);
    });

    camera.position.z = 5;

    function animate() {
      requestAnimationFrame(animate);
      renderer.render(scene, camera);
    }
    animate();

    function updateOrientation(pitch, roll) {
      if (model) {
        model.rotation.x = pitch * Math.PI / 180;
        model.rotation.z = -roll * Math.PI / 180;
      }
    }

    setInterval(() => {
      fetch("/data")
        .then(res => res.json())
        .then(data => updateOrientation(data.pitch, data.roll));
    }, 1000);

    function recalibrate() {
      const btn = document.getElementById("recalBtn");
      btn.disabled = true;
      const original = btn.textContent;
      btn.textContent = "Calibrating...";
      fetch("/recalibrate").then(() => {
        btn.textContent = original;
        btn.disabled = false;
      });
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
  float roll = atan2(ay, sqrt(ax * ax + az * az)) * 180.0 / PI - rollOffset;
  String json = "{\"pitch\":" + String(pitch, 2) + ",\"roll\":" + String(roll, 2) + "}";
  server.send(200, "application/json", json);
}

void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22);
  mpu.initialize();
  SPIFFS.begin(true);

  if (mpu.testConnection()) {
    Serial.println("MPU6050 connected.");
    calibrateOffsets();
  } else {
    Serial.println("MPU6050 connection failed.");
  }

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("WiFi connected: " + WiFi.localIP().toString());

  server.on("/", handleRoot);
  server.on("/data", handleData);
  server.on("/recalibrate", HTTP_GET, []() {
    calibrateOffsets();
    server.send(200, "text/plain", "OK");
  });

  server.serveStatic("/droneModel.glb", SPIFFS, "/droneModel.glb");
  server.begin();
}

void loop() {
  server.handleClient();
}
