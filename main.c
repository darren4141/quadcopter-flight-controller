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
  <title>3D Tilt Visualizer</title>
  <style>
    body { margin: 0; overflow: hidden; font-family: sans-serif; }
    canvas { display: block; }
    #recalBtn {
      position: absolute;
      top: 20px; left: 20px;
      z-index: 10;
      padding: 10px;
    }
  </style>
</head>
<body>
  <button id="recalBtn" onclick="recalibrate()">Recalibrate</button>
  <script src="https://cdn.jsdelivr.net/npm/three@0.150.1/build/three.min.js"></script>
  <script>
    let scene = new THREE.Scene();
    let camera = new THREE.PerspectiveCamera(75, window.innerWidth/window.innerHeight, 0.1, 1000);
    let renderer = new THREE.WebGLRenderer();
    renderer.setSize(window.innerWidth, window.innerHeight);
    document.body.appendChild(renderer.domElement);

    let geometry = new THREE.BoxGeometry(2, 0.2, 2);
    let material = new THREE.MeshNormalMaterial();
    let cube = new THREE.Mesh(geometry, material);
    scene.add(cube);

    camera.position.z = 5;

    function animate() {
      requestAnimationFrame(animate);
      renderer.render(scene, camera);
    }
    animate();

    function updateOrientation(pitch, roll) {
      // Convert degrees to radians
      cube.rotation.z = -pitch * Math.PI / 180;
      cube.rotation.x = -roll * Math.PI / 180;
    }

    function fetchData() {
      fetch("/data")
        .then(res => res.json())
        .then(data => {
          updateOrientation(data.pitch, data.roll);
        });
    }

    setInterval(fetchData, 100);

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