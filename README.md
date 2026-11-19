# 3D Model Interactive Slicer (MPU + ToF + VTK)

A C++ application built with the **Visualization Toolkit (VTK)** that reads real-time orientation (MPU) and distance (Time-of-Flight) sensor data over a serial port to dynamically slice a 3D mesh, allowing users to inspect the interior of a 3D model in real time.

## Features

- **Real-Time Serial Communication:** Reads incoming sensor data at `115200` baud using `serialib` (cross-platform support for Windows and Linux/macOS).
- **Dynamic 3D Clipping Plane:** Converts roll, pitch, and yaw angles into a 3D normal vector and adjusts the plane's elevation using ToF distance measurements.
- **Tilt-Compensated Height & Smoothing:** Applies cosine tilt compensation and an exponential moving average filter to keep the clipping plane smooth and stable.
- **Textured 3D Rendering:** Loads `.obj` models with `.png` textures and provides interactive camera controls (`TrackballCamera`) alongside a 3D orientation axes widget.

## Hardware & Serial Data Format

To use this application, connect a microcontroller (e.g., Arduino, ESP32, STM32) wired to an **MPU (IMU)** and a **ToF distance sensor**. 

The microcontroller must transmit comma-separated values terminated by a newline (`\n`) at **115200 baud**:

```text
roll,pitch,yaw,distance\n
```

- **`roll`, `pitch`, `yaw`:** Orientation angles in **degrees**.
- **`distance`:** Distance reading from the ToF sensor in **millimeters** (values `> 600` or `-1` are treated as `0`).

**Example payload:**
```text
12.5,-4.2,0.0,145.0
```

## Prerequisites

- **C++ Compiler** (C++11 or later)
- **VTK (Visualization Toolkit)** (version 8.x or 9.x)
- **CMake** (recommended for building VTK projects)
- **`serialib`** library (included in `serial_input/serialib.h`)

## Setup & Usage

1. **Clone the repository:**
   ```bash
   git clone https://github.com/yourusername/your-repo-name.git
   cd your-repo-name
   ```

2. **Configure the Serial Port:**
   Update the `SERIAL_PORT` macro in `main.cpp` to match your system's port:
   ```cpp
   // Windows
   #define SERIAL_PORT "\\\\.\\COM1"

   // Linux / macOS
   #define SERIAL_PORT "/dev/ttyUSB0"
   ```

3. **Prepare 3D Assets:**
   Ensure your `.obj` model and `.png` texture files are placed in the working directory where the executable runs (default in `main.cpp`: `chibi.obj` and `chibi.png`).

4. **Build and Run:**
   Compile the project using CMake or your preferred build system linked against VTK, then run the executable:
   ```bash
   mkdir build && cd build
   cmake ..
   make
   ./your_executable_name
   ```

## Controls

- **Left Click + Drag:** Rotate the 3D camera around the model.
- **Right Click + Drag / Scroll Wheel:** Zoom in and out to inspect the clipped interior.
- **Middle Click + Drag:** Pan the camera.
- **Physical Sensor Movement:** Tilt and move the sensor closer/farther to control the angle and depth of the cross-section cut.
