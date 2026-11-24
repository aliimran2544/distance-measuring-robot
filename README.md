# Bluetooth Measuring Robot

A 4-wheel Arduino robot you drive from your phone. It can also measure the distance between two objects without touching them.

A servo rotates an ultrasonic sensor so you can aim it at two points. The robot records the angle and distance to each one, then uses the **Law of Cosines** to calculate how far apart they are. All readings are shown live on an OLED screen.

<!-- Add a photo or GIF of the robot here -->
<!-- ![Robot](images/robot.jpg) -->

## Features

- **Phone control** over Bluetooth with the [Dabble](https://thestempedia.com/product/dabble/) app (Gamepad mode)
- **Rotating sensor**: tap to move 2°, or hold to sweep
- **Two-point measurement**: save Point 1 and Point 2, and the robot calculates the distance between them
- **Live OLED display** of sensor angle, current distance, both saved points, and the result

## How the measurement works

The sensor measures distance `a` to Point 1 and distance `b` to Point 2. The servo angles give the angle `θ` between the two readings. The distance between the points is:

```
c = √(a² + b² − 2ab·cos θ)
```

## Controls (Dabble Gamepad)

| Button | Action |
|---|---|
| Right / Left | Drive forward / backward (release to stop) |
| Up / Down | Rotate the sensor (hold to keep moving) |
| Triangle | Save Point 1 |
| Circle | Save Point 2 and calculate the distance |

## Hardware

| Part | Notes |
|---|---|
| Arduino Uno | |
| L293D Motor Driver Shield (Adafruit Motor Shield v1 compatible) | Drives the 4 DC motors |
| 4× DC gear motors + wheels | Connected to M1–M4 |
| HC-SR04 ultrasonic sensor | Trig → A0, Echo → A1 |
| Servo motor (SG90 or similar) | Signal → pin 9 |
| HC-05 Bluetooth module | RX/TX on pins 16 (A2) and 17 (A3) |
| 0.96" SSD1306 OLED (128×64, I2C) | SDA → A4, SCL → A5 |
| Battery pack | Powers the motors |

## Libraries

Install these from the Arduino IDE Library Manager:

- `Dabble`
- `U8g2`
- `Adafruit Motor Shield library` (AFMotor, v1)
- `Servo` (included with the Arduino IDE)

## Getting started

1. Clone this repo and open `measuring_robot/measuring_robot.ino` in the Arduino IDE.
2. Install the libraries above.
3. Select **Arduino Uno** and upload.
4. Open the Dabble app, connect to the Bluetooth module, and choose **Gamepad**.

## What I'd improve next

- Add turning (left/right steering) so it can drive freely, not just forward and back
- Take several readings per point and average them to reduce sensor noise
- Send measurements to the phone app as well as the OLED

## Author

**Ali Imran**, CS & Math student at the University of Toronto Scarborough
