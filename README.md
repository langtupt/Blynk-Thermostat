# Blynk-Thermostat
IoT thermostat using Blynk Platform

- How it works:
	- It Triggers a relay connected to a heating device in order to reach and keep the desired temperature.
	- As soon as the temperature is reached it stopps and it only starts again after the temp dropped by at least one deg.
	- Tempset can be updated by blynk app or hardware buttons , the rest of the inputs are by Blynk App only
	- In Manual Mode, if you leave the residence, the device will stop heating.
	

- Features:
	- Manual - 24/7  
	- Scheduled interval - time interval
	- Oled Display - display the actual temp,temp set,and some debugging info
	- Hardware Buttons for changing the temperature
	- Schedule working interval
	- Auto disable when it's on Manual
	
- Known Issues:
	- Successfull Temperature read rate is low . Data reading from DHT Sensor Fails too often