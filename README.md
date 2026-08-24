# Robot Phone Controller

A ROS 2 and Gazebo mobile robot controlled remotely from an Android phone through a USB connection.

The project demonstrates communication between an Android application and a ROS 2 mobile robot running in Gazebo. The Android phone acts as the remote controller, while Ubuntu handles the ROS 2 and Gazebo simulation.

## Features

- Differential-drive mobile robot simulated in Gazebo
- ROS 2 Jazzy control node written in C++
- Android controller application built with Android Studio
- USB communication between Android and Ubuntu
- ADB reverse port forwarding for communication
- Real-time robot movement commands from the Android phone
- ROS 2 integration between the Android controller and Gazebo simulation

## Demo

### Robot Control Demo

Click the image below to watch the demonstration video:

[![Robot Phone Controller Demo](photo.jpg)](Robot-mobilephone.mp4)

> The video demonstrates controlling the simulated mobile robot in Gazebo using the Android phone controller.

<!--
Replace `Robot-mobilephone.mp4` with the GitHub-hosted video URL after uploading
the video through a GitHub Issue, Release, or another GitHub-supported asset upload method.
-->

## Project Architecture

```text
┌──────────────────────┐
│     Android Phone    │
│   Rover Controller   │
└──────────┬───────────┘
           │
           │ USB
           │ ADB Reverse
           ▼
┌──────────────────────┐
│     Ubuntu 24.04     │
│                      │
│  TCP Server :5000    │
│          │           │
│          ▼           │
│    ROS 2 Jazzy       │
│          │           │
│      /cmd_vel        │
│          │           │
│          ▼           │
│    ros_gz_bridge     │
│          │           │
│          ▼           │
│   Gazebo Harmonic    │
└──────────┬───────────┘
           │
           ▼
     Differential
      Drive Robot
```

## Features

* Forward, backward, left and right control
* Dedicated stop button
* Touch-and-hold movement control
* USB communication through ADB
* ROS 2 `geometry_msgs/msg/Twist` control
* Gazebo differential-drive simulation
* TCP command bridge written in C++
* Command timeout safety mechanism
* Automatic stop when the Android connection is lost
* Automatic stop when the Android application leaves the foreground

## Requirements

### Ubuntu

* Ubuntu 24.04 LTS
* ROS 2 Jazzy
* Gazebo Harmonic
* `ros_gz_bridge`
* `colcon`
* Android Debug Bridge (`adb`)

### Android

* Android Studio
* Android device with USB debugging enabled
* USB cable

## Repository Structure

```text
robot_phone_controller/
│
├── android/
│   └── RoverController/
│       ├── app/
│       ├── gradle/
│       ├── build.gradle.kts
│       ├── gradle.properties
│       ├── gradlew
│       ├── gradlew.bat
│       └── settings.gradle.kts
│
├── ros_ws/
│   └── src/
│       └── robot_mobile_control/
│           ├── CMakeLists.txt
│           ├── package.xml
│           │
│           ├── config/
│           │   └── bridge.yaml
│           │
│           ├── description/
│           │   └── rover.sdf
│           │
│           ├── launch/
│           │   └── sim.launch.py
│           │
│           └── src/
│               └── mobile_bridge.cpp
│
├── .gitignore
└── README.md
```

## ROS 2 Package

ROS 2 package:

```text
robot_mobile_control
```

Main C++ node:

```text
mobile_bridge
```

The node listens for commands from the Android application through TCP and converts them to:

```text
geometry_msgs/msg/Twist
```

published on:

```text
/cmd_vel
```

## Control Protocol

The Android application sends single-character commands:

| Command | Function     |
| ------- | ------------ |
| `F`     | Forward      |
| `B`     | Backward     |
| `L`     | Rotate left  |
| `R`     | Rotate right |
| `S`     | Stop         |

The ROS 2 node converts these commands to linear and angular velocity commands.

## Safety

The C++ bridge contains a command watchdog.

If no valid command is received within the configured timeout, the robot is commanded to stop.

The Android application also sends a stop command when the application leaves the foreground or the connection is lost.

The TCP server is bound to:

```text
127.0.0.1:5000
```

and is accessed from Android through ADB reverse over USB.

## ROS 2 Installation

Install ROS 2 Jazzy and the required Gazebo packages on Ubuntu 24.04.

After installation:

```bash
source /opt/ros/jazzy/setup.bash
```

## Build the ROS 2 Workspace

Clone the repository:

```bash
git clone YOUR_GITHUB_REPOSITORY_URL
cd robot_phone_controller/ros_ws
```

Install package dependencies:

```bash
source /opt/ros/jazzy/setup.bash

rosdep install \
    --from-paths src \
    --ignore-src \
    -r \
    -y
```

Build:

```bash
colcon build --symlink-install
```

Source the workspace:

```bash
source install/setup.bash
```

## Run Gazebo

Run the simulation:

```bash
source /opt/ros/jazzy/setup.bash
source ~/robot_phone_controller/ros_ws/install/setup.bash

ros2 launch robot_mobile_control sim.launch.py
```

The Gazebo world and the ROS 2 control components are started by the launch file.

## Test the Robot Without Android

Before connecting the Android application, the ROS 2 control pipeline can be tested directly.

Forward:

```bash
ros2 topic pub /cmd_vel geometry_msgs/msg/Twist \
"{linear: {x: 0.8}, angular: {z: 0.0}}"
```

Rotate left:

```bash
ros2 topic pub /cmd_vel geometry_msgs/msg/Twist \
"{linear: {x: 0.0}, angular: {z: 1.5}}"
```

Rotate right:

```bash
ros2 topic pub /cmd_vel geometry_msgs/msg/Twist \
"{linear: {x: 0.0}, angular: {z: -1.5}}"
```

Stop the command publisher with:

```text
Ctrl+C
```

## Android Application

The Android application is located at:

```text
android/RoverController/
```

Open this directory in Android Studio.

Build and install the application on the Android device.

The application provides a robotic-style controller interface:

```text
              ▲

       ◀    STOP    ▶

              ▼
```

## USB Connection

Enable USB debugging on the Android phone.

Check the device:

```bash
adb devices
```

The device should appear as:

```text
List of devices attached
XXXXXXXXXXXX    device
```

Create the reverse TCP connection:

```bash
adb reverse tcp:5000 tcp:5000
```

Check the connection:

```bash
adb reverse --list
```

The Android application connects to:

```text
127.0.0.1:5000
```

The ROS 2 C++ bridge listens on:

```text
127.0.0.1:5000
```

The USB connection is therefore:

```text
Android
   │
   │ USB
   ▼
ADB Reverse
   │
   ▼
127.0.0.1:5000
   │
   ▼
mobile_bridge
```

## Full Startup Sequence

### Terminal 1

```bash
source /opt/ros/jazzy/setup.bash
source ~/robot_phone_controller/ros_ws/install/setup.bash

ros2 launch robot_mobile_control sim.launch.py
```

### Terminal 2

Connect the phone:

```bash
adb devices
```

Create the reverse connection:

```bash
adb reverse tcp:5000 tcp:5000
```

Verify:

```bash
adb reverse --list
```

### Android Studio

Run the `RoverController` application on the connected Android device.

The application should display:

```text
USB LINK: CONNECTED
```

The buttons can then be used to control the simulated robot.

## ROS Topics

Main command topic:

```text
/cmd_vel
```

Message type:

```text
geometry_msgs/msg/Twist
```

Gazebo bridge:

```text
ros_gz_bridge
```

Odometry topic:

```text
/odom
```

## Development Environment

This project was developed and tested with:

```text
Ubuntu 24.04 LTS
ROS 2 Jazzy
Gazebo Harmonic
C++
Kotlin
Android Studio
ADB
```

## Project Goals

This project is intended as a foundation for a larger mobile robotics platform.

Possible future extensions include:

* Camera integration
* OpenCV image processing
* LiDAR
* Odometry visualization
* RViz 2
* SLAM
* Autonomous navigation
* Object detection
* YOLO integration
* Touch joystick control
* Robot telemetry
* Battery and sensor monitoring
* Real robot hardware integration



