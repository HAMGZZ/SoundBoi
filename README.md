# SoundBoi
SoundBoi is the codename for a project which is intended to control the exhaust valves in my commodore.
The project originaly used a teensy 3.2 to talk directly to a car's can bus. However, this became too costly, thus, I am using an ESP32 module (through the df-robot beetle for prototyping) to connect to a cheap, rip-off elm327 dongle. These dongles are extreamly cheap and easy to program.

## Rules
The SoundBoi uses 5 states and a set of rules it follows to determine if the valves should be oppened or closed.
The valve will be open for the first 5 seconds since the car's startup.

### Rules 1 (Default state)
The valve will open if:
 - Speed < 90 AND:
 	- RPM 		> 3000
 	- Load 		> 50%
 	- Throttle 	> 50%
 - Speed > 90 AND:
	- RPM 		> 4000
	- Load		> 70%
	- Throttle	> 80%
 - Speed < 5 AND engine is up to temperature.

### Rules 2
The valve will open if:
 - Speed < 90 AND:
	- RPM		> 1900
	- Load		> 30%
	- Throttle	> 30%
 - Speed > 90:
	- Revert back to Rules set 1.
 - Speed < 15 AND engine is up to temperature.

### Rules 3
The valve is always open.

### Rules 4
The valve is always closed.

### Rules 5
The exhaust valve will open the same as the Throttle.

