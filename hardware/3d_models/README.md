# 🖨️ Delta-Bot 3D Printable Chassis & Enclosure

This folder contains the 3D printable STL CAD models for **Delta-Bot (Deskbot)**. You can print these parts on any standard FDM or Resin 3D printer.

---

## 📁 Included 3D Models (STL Files)

| Part File | Description | Recommended Material |
| :--- | :--- | :--- |
| `delta_bot_chassis.stl` | Main lower body chassis hosting battery, motors, and PCB | PLA / PETG |
| `delta_bot_head_cover.stl` | Upper head casing holding 128x64 OLED display & Touch Sensor | PLA / PETG |
| `delta_bot_wheel_left.stl` | Left drive wheel with rubber tire groove | PLA / TPU |
| `delta_bot_wheel_right.stl` | Right drive wheel with rubber tire groove | PLA / TPU |

---

## ⚙️ Recommended 3D Printing Settings

- **Layer Height**: `0.2mm` (or `0.16mm` for finer screen bezel detail)
- **Infill Density**: `20%` (Grid or Gyroid infill pattern)
- **Supports**: Enable touching buildplate supports for screen cutouts
- **Wall Loops / Shells**: `3` minimum for structural strength
- **Nozzle Temperature**: `200°C - 210°C` (Standard PLA)
- **Print Bed Temperature**: `60°C`

---

## 🔧 Assembly Guide

1. Insert N20 gear motors into the lower motor slots of `delta_bot_chassis.stl`.
2. Mount the ESP32-C3 PCB board and Li-Po battery inside the main chassis cavity.
3. Snap the 128x64 OLED display into `delta_bot_head_cover.stl`.
4. Secure the TTP223 touch sensor behind the top head panel.
5. Press fit the left and right wheels onto the motor D-shafts.
