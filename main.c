#include <Wire.h>
#include <MPU6050.h>

MPU6050 mpu;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  Wire.begin(21, 22); // SDA = 21, SCL = 22
  mpu.initialize();
  mpu.setDLPFMode(3);

  if (mpu.testConnection()) {
    Serial.println("MPU6050 connected successfully.");
  } else {
    Serial.println("MPU6050 connection failed.");
  }

  pinMode(16, OUTPUT);
  pinMode(2, OUTPUT);
  pinMode(23, OUTPUT);
  pinMode(17, OUTPUT);
  pinMode(12, OUTPUT);
  pinMode(25, OUTPUT);
}

void loop() {
  // put your main code here, to run repeatedly:
  int16_t ax, ay, az;
  int16_t gx, gy, gz;

  mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);

  Serial.print("a/g:\t");
  Serial.print(ax - 2500); Serial.print("\t");
  Serial.print(ay - 600); Serial.print("\t");
  Serial.print((az - 18000) * 360 / 30000); Serial.print("\t");
  Serial.print(gx - 90); Serial.print("\t");
  Serial.print(gy + 600); Serial.print("\t");
  Serial.println(gz - 60);

  delay(500);


  // digitalWrite(25, HIGH);
  // digitalWrite(23, HIGH);
  // digitalWrite(17, HIGH);
  // digitalWrite(12, HIGH);
  // digitalWrite(2, HIGH);
  // delay(1000);
  // digitalWrite(2, LOW);
  // digitalWrite(25, LOW);
  // digitalWrite(23, LOW);
  // digitalWrite(17, LOW);
  // digitalWrite(12, LOW);
  // delay(1000);

  // digitalWrite(23, HIGH);
  // digitalWrite(17, HIGH);
  // digitalWrite(12, HIGH);
  // digitalWrite(25, HIGH);
  // delay(1000);
  // digitalWrite(23, LOW);
  // digitalWrite(17, LOW);
  // digitalWrite(12, LOW);
  // digitalWrite(25, LOW);
  // delay(1000);
}
