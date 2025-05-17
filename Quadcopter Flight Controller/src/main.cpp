#include <WiFi.h>
#include <Wire.h>
#include <WebServer.h>
#include <SPIFFS.h>
#include "I2Cdev.h"
#include "MPU6050_6Axis_MotionApps20.h"


const char* ssid = "Darren’s iPhone";
const char* password = "password";

MPU6050 mpu;
bool dmpReady = false;
uint16_t packetSize;
uint8_t fifoBuffer[64];

float ypr[3];

WebServer server(80);

float yawOffset = 0;
float pitchOffset = 0;
float rollOffset = 0;

int motorPWM[4] = {0, 0, 0, 0};
const int motorPins[4] = {23, 17, 12, 25};
const int pwmChannels[4] = {0, 1, 2, 3};

void calibrateOffsets() {
  float yawSum = 0, pitchSum = 0, rollSum = 0;

  for (int i = 0; i < 20; i++) {
    if (dmpReady){
      if (mpu.dmpGetCurrentFIFOPacket(fifoBuffer)) {
        Quaternion q;
        VectorFloat gravity;
  
        mpu.dmpGetQuaternion(&q, fifoBuffer);
        mpu.dmpGetGravity(&gravity, &q);
        mpu.dmpGetYawPitchRoll(ypr, &q, &gravity);
        
        yawSum += ypr[0];
        pitchSum += ypr[1];
        rollSum += ypr[2];

      }
    }else{
      i--;
    }

    delay(50);
  }

  yawOffset = yawSum / 20.0;
  pitchOffset = pitchSum / 20.0;
  rollOffset = rollSum / 20.0;
  Serial.println("Calibration done");
}

void handleData() {
  float yaw = ypr[0] * 180.0 / M_PI;
  float pitch = ypr[1] * 180.0 / M_PI;
  float roll = ypr[2] * 180.0 / M_PI;

  String json = "{\"yaw\":" + String(yaw, 2)+ ",\"pitch\":" + String(pitch, 2) + ",\"roll\":" + String(roll, 2) + "}";
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
  
  int devStatus = mpu.dmpInitialize();
  if (devStatus == 0) {
    mpu.setDMPEnabled(true);
    dmpReady = true;
    packetSize = mpu.dmpGetFIFOPacketSize();
    Serial.println("DMP Ready!");
  } else {
    Serial.print("DMP Init failed (code ");
    Serial.print(devStatus);
    Serial.println(")");
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

void updateAccel(){
  if (!dmpReady) return;

  if (mpu.dmpGetCurrentFIFOPacket(fifoBuffer)) {
    Quaternion q;
    VectorFloat gravity;

    mpu.dmpGetQuaternion(&q, fifoBuffer);
    mpu.dmpGetGravity(&gravity, &q);
    mpu.dmpGetYawPitchRoll(ypr, &q, &gravity);

    ypr[0] -= yawOffset;
    ypr[1] -= pitchOffset;
    ypr[2] -= rollOffset;

  }
}

void loop() {
  server.handleClient();

  for (int i = 0; i < 4; i++) {
    ledcWrite(pwmChannels[i], motorPWM[i]);
  }

  updateAccel();

  Serial.print(ypr[0]);
  Serial.print(" ");
  Serial.print(ypr[1]);
  Serial.print(" ");
  Serial.println(ypr[2]);

}