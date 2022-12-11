# 24-channel DAC
This is hardware project of a 24-channel DAC with isolated output controlled via Ethernet interface. 

<img src="/photos/photo_2020-09-07_16-47-34.jpg" alt="drawing" width="200"/> <img src="/photos/photo_2020-09-07_16-48-04.jpg" alt="drawing" width="200"/> <img src="/photos/photo_2020-09-07_16-48-26.jpg" alt="drawing" width="200"/>

# Specs
- Number of outputs: 24
- Resolution: 10 bit
- Output voltage: from -4.096 V to +4.096 V
- Output current: 10 mA max
- Output configuration: 24 channels with common rail, isolated from power supply
- Control: Ethernet with web interface and SCPI commands
- Power: 12V

# Details
The schematics and PCB files can be found in the /kicad folder, they are created using KiCAD. The schematics are also presented in PDF. There are 2 boards: the controller bord and the DAC board. The controller board contains a STM32 MCU, an isolated power coverter and an Ethernet PHY module. An of-the-shelf ENC28J60 module is used for the Ethernet. The DAC board is connected via isolated I2C and contains four 8-channel DAC ICs, reference source and output buffers.

The firmware for STM32 MCU can be found in the /STM32_firmware folder. The project is for STM32CubeIDE.
