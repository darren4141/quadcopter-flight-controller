#include <WiFi.h>
#include <Wire.h>
#include <MPU6050.h>
#include <WebServer.h>
#include <SPIFFS.h>

const char* ssid = "Darren’s iPhone";
const char* password = "password";

MPU6050 mpu;
WebServer server(80);

float pitchOffset = 0;
float rollOffset = 0;

int motorPWM[4] = {0, 0, 0, 0};
const int motorPins[4] = {23, 17, 12, 25};
const int pwmChannels[4] = {0, 1, 2, 3};

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
  
  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS mount failed!");
    return;
  }
  
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected, IP address: ");
  Serial.println(WiFi.localIP());


  // Serve index.html from SPIFFS
  server.serveStatic("/", SPIFFS, "/index.html");
  server.serveStatic("/style.css", SPIFFS, "/style.css");
  server.serveStatic("/script.js", SPIFFS, "/script.js");

  for (int i = 0; i < 4; i++) {
    ledcSetup(pwmChannels[i], 5000, 8);           // 5kHz, 8-bit
    ledcAttachPin(motorPins[i], pwmChannels[i]);
    ledcWrite(pwmChannels[i], 0);

  }

  server.on("/data", handleData);

  server.on("/recalibrate", HTTP_GET, []() {
    calibrateOffsets();
    server.send(200, "text/plain", "OK");
  });

  server.on("/setPWM", HTTP_GET, []() {
    for (int i = 0; i < 4; i++) {
      if (server.hasArg("m" + String(i + 1))) {
        motorPWM[i] = constrain(server.arg("m" + String(i + 1)).toInt(), 0, 255);
      }
    }
    server.send(200, "text/plain", "OK");
  });

  server.begin();

  calibrateOffsets();

}

void loop() {
  server.handleClient();
  for (int i = 0; i < 4; i++) {
    ledcWrite(pwmChannels[i], motorPWM[i]);
  }

}