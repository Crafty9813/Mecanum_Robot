#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

RF24 radio(7, 8);

const byte address[6] = "00001";

struct Data {
  int joy1X; // Forward/reverse
  int joy1Y; // Strafe
  int joy2X; // Rotation
};

Data data;

const int FL_IN1 = 2;
const int FL_IN2 = 4;
const int FL_EN  = 5;

const int RL_IN1 = A3;
const int RL_IN2 = A4;
const int RL_EN  = 6;

const int FR_IN1 = A1;
const int FR_IN2 = A2;
const int FR_EN  = 3;

const int RR_IN1 = A5;
const int RR_IN2 = A0;
const int RR_EN  = 9;

// Joystick settings
const int JOY_CENTER = 512;
const int DEADZONE = 70;

const int MIN_PWM = 35; // Prevents buzzing

void setup() {
  Serial.begin(9600);

  radio.begin();

  radio.openReadingPipe(0, address);

  radio.setPALevel(RF24_PA_MIN);

  radio.startListening();

  pinMode(FL_IN1, OUTPUT);
  pinMode(FL_IN2, OUTPUT);
  pinMode(FL_EN, OUTPUT);

  pinMode(RL_IN1, OUTPUT);
  pinMode(RL_IN2, OUTPUT);
  pinMode(RL_EN, OUTPUT);

  pinMode(FR_IN1, OUTPUT);
  pinMode(FR_IN2, OUTPUT);
  pinMode(FR_EN, OUTPUT);

  pinMode(RR_IN1, OUTPUT);
  pinMode(RR_IN2, OUTPUT);
  pinMode(RR_EN, OUTPUT);

  stopAllMotors();
}

void loop() {
  if (radio.available()) {

    radio.read(&data, sizeof(data));

    int forward = mapJoystick(data.joy1X);
    int strafe = -mapJoystick(data.joy1Y);
    int rotation = mapJoystick(data.joy2X);

    // Mecanum mixing
    //
    //                FRONT
    //
    //          FL             FR
    //
    //
    //          RL             RR
    //
    //                 REAR
    //
    //
    // Forward: all wheels same dir
    //
    // Strafe: FR = BL, FL = BR
    //
    // Rot: left opp to right

    int frontLeft = forward + strafe + rotation;
    int rearLeft = forward - strafe + rotation;
    int frontRight = forward - strafe - rotation;
    int rearRight = forward + strafe - rotation;

    // Normalize (if combined cmds > 255 scale motors proportionally)
    int maxSpeed = max(
      max(abs(frontLeft), abs(rearLeft)),
      max(abs(frontRight), abs(rearRight))
    );

    if (maxSpeed > 255) {
      frontLeft = (long)frontLeft * 255 / maxSpeed;
      rearLeft = (long)rearLeft * 255 / maxSpeed;
      frontRight = (long)frontRight * 255 / maxSpeed;
      rearRight = (long)rearRight * 255 / maxSpeed;
    }

    setMotor(
      FL_IN1,
      FL_IN2,
      FL_EN,
      frontLeft
    );

    setMotor(
      RL_IN1,
      RL_IN2,
      RL_EN,
      rearLeft
    );

    setMotor(
      FR_IN1,
      FR_IN2,
      FR_EN,
      frontRight
    );

    setMotor(
      RR_IN1,
      RR_IN2,
      RR_EN,
      rearRight
    );
  }
}

/*
void loop() {

  if (radio.available()) {

    radio.read(&data, sizeof(data));

    int strafe   = mapJoystick(data.joy1X);
    int forward  = mapJoystick(data.joy1Y);
    int rotation = mapJoystick(data.joy2X);


    // Print everything
    Serial.print("RAW  X=");
    Serial.print(data.joy1X);

    Serial.print(" Y=");
    Serial.print(data.joy1Y);

    Serial.print(" R=");
    Serial.print(data.joy2X);

    Serial.print("   |   MAPPED  S=");
    Serial.print(strafe);

    Serial.print(" F=");
    Serial.print(forward);

    Serial.print(" R=");
    Serial.println(rotation);


    // Mecanum calculation
    int frontLeft =
      forward +
      strafe +
      rotation;

    int rearLeft =
      forward -
      strafe +
      rotation;

    int frontRight =
      forward -
      strafe -
      rotation;

    int rearRight =
      forward +
      strafe -
      rotation;


    // Print motor commands
    Serial.print("MOTORS  FL=");
    Serial.print(frontLeft);

    Serial.print(" RL=");
    Serial.print(rearLeft);

    Serial.print(" FR=");
    Serial.print(frontRight);

    Serial.print(" RR=");
    Serial.println(rearRight);


    // Normalize
    int maxSpeed = max(
      max(abs(frontLeft), abs(rearLeft)),
      max(abs(frontRight), abs(rearRight))
    );

    if (maxSpeed > 255) {

      frontLeft =
        (long)frontLeft * 255 / maxSpeed;

      rearLeft =
        (long)rearLeft * 255 / maxSpeed;

      frontRight =
        (long)frontRight * 255 / maxSpeed;

      rearRight =
        (long)rearRight * 255 / maxSpeed;
    }


    // Drive motors
    setMotor(
      FL_IN1,
      FL_IN2,
      FL_EN,
      frontLeft
    );

    setMotor(
      RL_IN1,
      RL_IN2,
      RL_EN,
      rearLeft
    );

    setMotor(
      FR_IN1,
      FR_IN2,
      FR_EN,
      frontRight
    );

    setMotor(
      RR_IN1,
      RR_IN2,
      RR_EN,
      rearRight
    );

    delay(1000);
  }
}*/

int mapJoystick(int value) {
  int difference = value - JOY_CENTER;

  if (abs(difference) <= DEADZONE) {
    return 0;
  }

  if (difference > 0) {
    return map(
      value,
      JOY_CENTER + DEADZONE,
      1023,
      0,
      255
    );
  }

  return map(
    value,
    0,
    JOY_CENTER - DEADZONE,
    -255,
    0
  );
}

void setMotor(
  int in1,
  int in2,
  int enable,
  int speed
) {

  speed = constrain(speed, -255, 255);

  if (abs(speed) < MIN_PWM) {
    // Turn PWM off
    analogWrite(enable, 0);

    // Coast/stop
    digitalWrite(in1, LOW);
    digitalWrite(in2, LOW);

    return;
  }

  if (speed > 0) {
    digitalWrite(in1, HIGH);
    digitalWrite(in2, LOW);

    analogWrite(enable, speed);
  }

  else {
    digitalWrite(in1, LOW);
    digitalWrite(in2, HIGH);

    analogWrite(enable, -speed);
  }
}

void stopAllMotors() {
  analogWrite(FL_EN, 0);
  analogWrite(RL_EN, 0);
  analogWrite(FR_EN, 0);
  analogWrite(RR_EN, 0);

  digitalWrite(FL_IN1, LOW);
  digitalWrite(FL_IN2, LOW);

  digitalWrite(RL_IN1, LOW);
  digitalWrite(RL_IN2, LOW);

  digitalWrite(FR_IN1, LOW);
  digitalWrite(FR_IN2, LOW);

  digitalWrite(RR_IN1, LOW);
  digitalWrite(RR_IN2, LOW);
}
