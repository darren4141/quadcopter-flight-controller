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
    <!DOCTYPE html><html>
    <head>
      <meta charset="UTF-8">
      <title>ESP32 Quadcopter Tilt</title>
    </head>
    <body>
      <h2>ESP32 Drone Tilt Monitor</h2>
      <p>Pitch: <span id="pitch">--</span>°</p>
      <p>Roll: <span id="roll">--</span>°</p>
      <script>
        setInterval(() => {
          fetch("/data").then(r => r.text()).then(t => {
            let [pitch, roll] = t.split(",");
            document.getElementById("pitch").textContent = pitch;
            document.getElementById("roll").textContent = roll;
          });
        }, 1000);
      </script>
    </body></html>
  )rawliteral");
}

void handleData() {
  int16_t ax, ay, az;
  mpu.getAcceleration(&ax, &ay, &az);
  float pitch = atan2(ax, sqrt(ay * ay + az * az)) * 180.0 / PI - pitchOffset;
  float roll = atan2(ay, sqrt(ax * ax + az * az)) * 180.0 / PI - rollOffset;
  String data = String(pitch, 2) + "," + String(roll, 2);
  server.send(200, "text/plain", data);
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
  server.begin();
}

void loop() {
  server.handleClient();
}