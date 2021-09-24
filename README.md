
## Gardening with Arduino

This is an Internet of Things garden watering project which uses a Solid Pod as a time series data repository and a store for relaying operational commands.  An Arduino posts soil moisture and other data to a Solid Pod and retrieves its new commands and threshold levels for the control of a water pump.  A Python client is used to set Arduino bound commands in CSV and provide graphical representation of the incoming RDF sensor data.  The graph displays soil moisture, soil temperature, pump operation, and wifi signal strength over time.  This design can serve as a base for future automation.

This project was presented to https://solidproject.org on Sept 2nd.  The recording is at: https://vimeo.com/596683888


### Arduino Software
  The arduino_solid project contains the code used by the Arduino to read CSV commands from the solid pod and write RDF data to a different file on the solid pod.  This is written for the Arduino WiFi Rev 2.  Libraries for WiFi and moisture sensors will need to be added. Set solid pod credentials in the arduino_secrets.h file.


### Client
This is a headless (without UI) python3 client which writes commands and reads RDF data on a solid pod.  For now it is manually run as this is a work in progress.  See comments in code.  Set solid pod signin info in a credentials.yaml file based on the _example file.


### Next
Some things I hope to do next include:
  * Run a dedicated Solid Pod using the new Community Solid Server from https://github.com/solid/community-server .
  * Use Linked Data Notification to drive client side processing.
  * Concatenate data from multiple day oriented files.


### Hardware used:
  * Arduino Uno WiFi Rev2 https://store-usa.arduino.cc/products/arduino-uno-wifi-rev2
  * I2C Moisture Sensor (Rugged version) https://www.tindie.com/products/miceuz/i2c-soil-moisture-sensor/
  * 12V DC Diaphragm Pump: https://www.amazon.com/gp/product/B01N75ZIXF/
  * 12V 9AH battery: https://www.mightymaxbattery.com/shop/12v-sla-batteries/ml9-12-12-volt-9-ah-sla-battery/
  * Power Transistor RFP30N06LE MOSFET 30A 60V N-Channel: https://www.amazon.com/gp/product/B07WR86ZGS/
  * Polyethylene Tubing 1/2" OD x 3/8" ID x 25': https://www.menards.com/main/plumbing/hoses-tubing/polyethylene-tubing/sioux-chief-1-2-od-x-3-8-id-polyethylene-tubing/901-03163w0025m/p-1470660864120-c-8581.htm

The links refer to the particular sources and products I used but there are many other options and vendors for most of these products.

For more information on moisture sensors see:  https://flashgamer.com/blog/comments/testing-capacitive-soil-moisture-sensors

The battery I used was a bit underpowered for the task.  A few half hour sessions of pump operation significantly drained the battery.  The arduino still had enough voltage to operate even when the pump could not.  In the future I will add a larger capacity 12V battery and provide battery voltage reporting.

Voltage reporting - this could be done with a simple resistor circuit and one of the A/D pins on the arduino.  For example, a large resistor (e.g. 2M Ohm) in serial with one half that size (e.g. 1M Ohm) could span the 12V battery output to the ground.  This would create a point between the 2 resistors which would measure 1/3 of the voltage.  This would provide a proxy for the 12V-0V in a range suitable to be read by the 5V Analog to Digital pin (4V-0V).  The digital value of that pin could be added to the RDF reported to the solid pod.  (Note that the '12V' battery can often output a little more than 12V so 1/3 measurement is safer than trying to have 12V result in a 5V pin input.)


### Connections:
  * Battery Positive Terminal - Arduino Positive via barrel connector
  * Battery Negative Terminal - Arduino Negative via barrel connector

  * Battery Positive Terminal - Pump Positive (red line)
  * Pump Negative (yellow line) - Transistor Drain 
  * Transistor Source - Battery Negative Terminal
  * Transistor Gate - Arduino Digital Pin 12

  * Moisture Sensor 5V - Arduino 5V pin
  * Moisture Sensor Gnd - Arduino Gnd pin
  * Moisture Sensor SDA - Arduino SDA
  * Moisture Sensor ICL - Arduino ICL


### Notes:
  * Arduino barrel connector is 2.1mm ID, 5.5mm OD.
  * Transistor datasheet: https://cdn.sparkfun.com/datasheets/Components/General/RFP30N06LE.pdf



