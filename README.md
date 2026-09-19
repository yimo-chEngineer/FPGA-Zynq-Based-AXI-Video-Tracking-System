# FPGA Zynq-Based-AXI-Video-Detection-System

## Status
**In progress.**
- Working: [camera I2C/SCCB config, basic pixel capture onto AXI-Stream, asynchronous FIFO VDMA pipeline]
- In development: [DDR3 write path, ping-pong buffering]
- Not started: [Display output]

## Overview
This repository contains the custom RTL and bare-metal C firmware for a real-time video tracking system developed on a Zynq-7000 SoC. The project interfaces a raw OV7670 image sensor, with the goal of streaming pixel data through a custom AXI4-Stream IP into DDR3 memory for processing and display output through HDMI.

## Hardware Requirements
* **System-on-Chip (SoC):** Xilinx Zynq-7000 (ARM Cortex-A9)
* **FPGA Model:** Microphase Z7-Lite 7010
* **Vision Sensor:** OV7670 Camera

## Architecture
* **Hardware/Software Co-Design:** Bare-metal C code running on the ARM processor through AMD Vitis handles I2C/SCCB and VDMA register configuration, while custom pure Verilog logic handles the high-speed pixel stream.
* **Clock Domain Crossing (CDC):** Custom rtl synchronizes data across asynchronous interfaces between the camera's pixel clock, the AXI system clock, and HDMI pixel clock.
* **High-Speed AXI Pipeline:** Video Direct Memory Access (VDMA) transfers video data between the AXI stream and the DDR3 memory without requiring the Zynq processor to copy each pixel.
* **Ping-Pong Buffering:** A double-buffering memory architecture in DDR3, intended to eliminate screen tearing during frame reads and writes. This architecture will be used for video processing.

## Repository Structure
* `rtl/`: Custom Verilog modules and top-level HDL.
* `sim/`: Testbenches and simulation scripts.
* `sw/`: Bare-metal Zynq firmware and Vitis source files.
* `vivado/`: Vivado project files, block designs, and build scripts.
* `constraints/`: FPGA pin assignments and timing constraints (`.xdc`).
* `docs/`: System diagrams, hardware documentation, and design notes.
* `ip/`: Custom packaged IP used by the Vivado block design.
