/*
 * Bluetooth Measuring Robot
 * --------------------------
 * A 4-wheel Arduino robot controlled from a phone (Dabble app, Gamepad module).
 * A servo-mounted ultrasonic sensor can be aimed at two points, and the robot
 * uses the Law of Cosines to calculate the straight-line distance between them.
 * Live readings are shown on a 128x64 OLED display.
 *
 * Controls (Dabble Gamepad):
 *   Right / Left  -> drive forward / backward (release to stop)
 *   Up / Down     -> rotate the sensor servo (hold to keep moving)
 *   Triangle      -> save Point 1 (angle + distance)
 *   Circle        -> save Point 2 and calculate the distance between P1 and P2
 *
 * Author: Ali Imran
 */

// Only load the Gamepad module from the Dabble library (saves memory)
#define CUSTOM_SETTINGS
#define INCLUDE_GAMEPAD_MODULE

#include <Servo.h>
#include <Dabble.h>
#include <U8g2lib.h>
#include <AFMotor.h>

// ---------------- OLED Display ----------------
// 128x64 SSD1306 OLED over hardware I2C
U8G2_SSD1306_128X64_NONAME_1_HW_I2C u8g2(U8G2_R0);

// ---------------- Servo ----------------
Servo myServo;
uint8_t angle = 90;                    // current servo angle (degrees)
unsigned long servoLastMove = 0;       // time of the last repeated servo step
const uint16_t SERVO_DELAY = 300;      // ms a button must be held before repeating
const uint16_t SERVO_INTERVAL = 50;    // ms between repeated steps while held

// ---------------- Motors ----------------
// Four DC motors on the motor shield (ports M1-M4)
AF_DCMotor motor1(1);
AF_DCMotor motor2(2);
AF_DCMotor motor3(3);
AF_DCMotor motor4(4);
bool motorsActive = false;

// ---------------- Ultrasonic Sensor ----------------
const uint8_t trigPin = A0;
const uint8_t echoPin = A1;

// ---------------- Timing ----------------
unsigned long lastUpdate = 0;
const uint16_t UPDATE_INTERVAL = 20;   // ms between control/display updates

// ---------------- Measurement Storage ----------------
int dist1 = -1, dist2 = -1;     // distances to Point 1 and Point 2 (mm), -1 = no reading
int angle1 = -1, angle2 = -1;   // servo angles for Point 1 and Point 2
float delta = -1;               // calculated distance between the two points (mm)
bool firstCaptured = false;     // true once Point 1 has been saved

void setup() {
  Serial.begin(9600);
  Dabble.begin(9600, 16, 17);   // Bluetooth module: baud 9600, RX pin 16 (A2), TX pin 17 (A3)
  Serial.println(F("Booted!"));

  myServo.attach(9);
  myServo.write(angle);

  pinMode(trigPin, OUTPUT);
  digitalWrite(trigPin, LOW);
  pinMode(echoPin, INPUT);

  u8g2.begin();
  u8g2.setFont(u8g2_font_5x7_tf);

  stopMotors();
  motor1.setSpeed(150);
  motor2.setSpeed(150);
  motor3.setSpeed(150);
  motor4.setSpeed(150);
}

void loop() {
  Dabble.processInput();   // read the latest input from the phone

  // Run controls and display every UPDATE_INTERVAL ms
  if (millis() - lastUpdate >= UPDATE_INTERVAL) {
    lastUpdate = millis();

    handleServo();
    handleMotors();
    handleMeasurementButtons();
    updateDisplay();
  }
}

// Up/Down rotate the sensor servo by 2 degrees per press.
// Holding a button keeps moving it after a short delay.
void handleServo() {
  static bool upPressed = false, downPressed = false;
  static unsigned long holdStart = 0;

  bool currentUp = GamePad.isUpPressed();
  bool currentDown = GamePad.isDownPressed();

  // New press: move one step
  if (currentUp && !upPressed) {
    angle = min(angle + 2, 180);
    myServo.write(angle);
    holdStart = millis();
  }
  if (currentDown && !downPressed) {
    angle = max(angle - 2, 0);
    myServo.write(angle);
    holdStart = millis();
  }

  // Button held: keep stepping every SERVO_INTERVAL ms
  if ((currentUp || currentDown) && (millis() - holdStart > SERVO_DELAY)) {
    if (millis() - servoLastMove > SERVO_INTERVAL) {
      if (currentUp) angle = min(angle + 2, 180);
      if (currentDown) angle = max(angle - 2, 0);
      myServo.write(angle);
      servoLastMove = millis();
    }
  }

  upPressed = currentUp;
  downPressed = currentDown;
}

// Right = forward, Left = backward, release both = stop.
void handleMotors() {
  static bool lastForward = false, lastBackward = false;
  bool currentForward = GamePad.isRightPressed();
  bool currentBackward = GamePad.isLeftPressed();

  if (currentForward && !lastForward) {
    motorsActive = true;
    moveForward();
  } else if (currentBackward && !lastBackward) {
    motorsActive = true;
    moveBackward();
  } else if (!currentForward && !currentBackward && motorsActive) {
    motorsActive = false;
    stopMotors();
  }

  lastForward = currentForward;
  lastBackward = currentBackward;
}

// Triangle saves Point 1. Circle saves Point 2 and calculates
// the distance between the two points.
void handleMeasurementButtons() {
  if (GamePad.isTrianglePressed()) {
    dist1 = readDistance();
    angle1 = angle;
    firstCaptured = true;
    delta = -1;
    Serial.println(F("P1 saved"));
    delay(100);
  }

  if (GamePad.isCirclePressed() && firstCaptured) {
    dist2 = readDistance();
    angle2 = angle;
    Serial.println(F("P2 saved"));

    if (dist1 == -1 || dist2 == -1) {
      delta = -1;   // one of the readings failed
    } else {
      // Angle between the two readings, in radians
      float theta = abs(angle1 - angle2) * (3.14159265 / 180.0);

      // Law of Cosines: c = sqrt(a^2 + b^2 - 2ab*cos(theta))
      // (float) cast prevents integer overflow on the Arduino Uno
      delta = sqrt(
        pow(dist1, 2) +
        pow(dist2, 2) -
        (2.0 * (float)dist1 * (float)dist2 * cos(theta))
      );
    }

    firstCaptured = false;
    delay(100);
  }
}

// Draws the servo angle, live distance, saved points and result on the OLED.
void updateDisplay() {
  int dist = readDistance();

  u8g2.firstPage();
  do {
    // Current servo angle
    u8g2.setCursor(0, 10);
    u8g2.print(F("Angle: "));
    u8g2.print(angle);
    u8g2.print(F("\xB0"));

    // Live distance reading
    u8g2.setCursor(0, 20);
    u8g2.print(F("Dist: "));
    if (dist == -1) {
      u8g2.print(F("N/A"));
    } else {
      u8g2.print(dist);
      u8g2.print(F(" mm"));
    }

    // Saved Point 1
    u8g2.setCursor(0, 35);
    u8g2.print(F("P1: "));
    if (angle1 >= 0 && dist1 >= 0) {
      u8g2.print(angle1);
      u8g2.print(F("\xB0 "));
      u8g2.print(dist1);
      u8g2.print(F("mm"));
    } else {
      u8g2.print(F("N/A"));
    }

    // Saved Point 2
    u8g2.setCursor(0, 45);
    u8g2.print(F("P2: "));
    if (angle2 >= 0 && dist2 >= 0) {
      u8g2.print(angle2);
      u8g2.print(F("\xB0 "));
      u8g2.print(dist2);
      u8g2.print(F("mm"));
    } else {
      u8g2.print(F("N/A"));
    }

    // Distance between Point 1 and Point 2
    u8g2.setCursor(0, 58);
    u8g2.print(F("Delta: "));
    if (delta >= 0) {
      u8g2.print(delta, 1);
      u8g2.print(F(" mm"));
    } else {
      u8g2.print(F("N/A"));
    }

  } while (u8g2.nextPage());
}

// Reads the HC-SR04 ultrasonic sensor. Returns distance in mm, or -1 if no echo.
int readDistance() {
  // Send a 10 microsecond trigger pulse
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  long duration = pulseIn(echoPin, HIGH, 30000);  // timeout 30 ms (about 5 m)
  if (duration <= 0) return -1;

  // Speed of sound = 0.343 mm/us; divide by 2 because the sound goes there and back
  float distance_mm = (duration * 0.343) / 2.0;
  return round(distance_mm);
}

// ---------------- Motor helpers ----------------

void moveForward() {
  motor1.run(FORWARD);
  motor2.run(FORWARD);
  motor3.run(FORWARD);
  motor4.run(FORWARD);
}

void moveBackward() {
  motor1.run(BACKWARD);
  motor2.run(BACKWARD);
  motor3.run(BACKWARD);
  motor4.run(BACKWARD);
}

void stopMotors() {
  motor1.run(RELEASE);
  motor2.run(RELEASE);
  motor3.run(RELEASE);
  motor4.run(RELEASE);
}
