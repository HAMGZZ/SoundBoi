# SoundBoi
SoundBoi is the codename for a porject which is intended to use with the [BLAT!](https://github.com/hamgzz/BLAT) module for the holden commodored OEM bimodel exhaust. However, it can be extended for use with other cars and valved mufflers. This project originally started when I still had my Holden Barina and wanted an automatic exhaust valve. Over time from trying to write my own CAN transceiver using the teensy line of transceivers and using dodgy rip-off ELM327s with ESP32s, I finally bit the bullet and invested in a few actual ELM327 chips... This is where SoundBoi comes in. SoundBoi is a complete custom solution using the ELM327 chip and an atTiny1614 to communicate with BLAT!. BLAT! is the module the creates the specific PWM and voltages needed to control the OEM Holden mufflers... And yes, all this could be built into one board, but BLAT! came before SoundBoi...

The board follow's a specific set of rules which can be found in the source code.

## Rules
The SoundBoi uses 5 states and a set of rules it follows to determine if the valves should be oppened or closed.
The valve will be open for the first 5 seconds since the car's startup.

### Rules 1 (Default state)
The valve will open if:
 - Speed < 90 AND:
 	- RPM       > 3000
 	- Load      > 50%
 	- Throttle  > 50%
 - Speed > 90 AND:
	- RPM       > 4000
	- Load      > 70%
	- Throttle  > 80%
 - Speed < 5 AND engine is up to temperature.

### Rules 2
The valve will open if:
 - Speed < 90 AND:
	- RPM       > 1900
	- Load      > 30%
	- Throttle  > 30%
 - Speed > 90:
	- Revert back to Rules set 1.
 - Speed < 15 AND engine is up to temperature.

### Rules 3
The valve is always open.

### Rules 4
The valve is always closed.

## PCB

The PCB is meant to be attached to the BLAT! module back-to-back. This is to keep the boards small and modular. The SoundBoi PCB takes it's 5V from the Pololu [regulator](https://www.pololu.com/product/2843) and reads the battery voltage from the 12v coming from the relay on the BLAT! module. Because of the ignition relay, there is no need for low power cutoff on the SoundBoi module, but it might come in future revisions....

### Schematic

<object data="PCB/SoundBoi/SoundBoi.pdf" type="application/pdf" width="700px" height="700px">
    <embed src="PCB/SoundBoi/SoundBoi.pdf">
        <p>This browser does not support PDFs. Please download the PDF to view it: <a href="PCB/SoundBoi/SoundBoi.pdf">Download PDF</a>.</p>
    </embed>
</object>

### 3D Models
![alt text](PCB1.png)

### BOM
