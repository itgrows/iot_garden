#include <SPI.h>
#include <WiFiNINA.h>
//#include "Adafruit_seesaw.h"
#include <I2CSoilMoistureSensor.h>
#include <Wire.h>

//the following contains WIFI_* and SOLID_* CREDENTIALS
#include "arduino_secrets.h"

//WIFI id and password
char wifi_ssid[] = WIFI_SSID;
char wifi_pass[] = WIFI_PASS;
//SOLID id and password
char solid_username[] = SOLID_USERNAME;
char solid_password[] = SOLID_PASSWORD;
char solid_url[] = SOLID_URL;
int port = 443;

//STARTING MOISTURE VALUES SO PUMP DEFAULTS TO OFF
int moisture_min = 1000;
int moisture_max = 1000;
int motor_status = 0;

int wifi_status = WL_IDLE_STATUS;
WiFiSSLClient client;
String cookieStr = "";

//CATNIP MOISTURE SENSOR
I2CSoilMoistureSensor sensor;
//SEESAW MOISTURE SENSOR
//Adafruit_seesaw ss;

void setup() {
  //Initialize serial data and wait for port to open.  This is for writing data back to Arduino IDE for testing/debugging.
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port connection
  }

  // check for the WiFi module:
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Communication with WiFi module failed!");
    while (true); //stop
  }
  String fv = WiFi.firmwareVersion();
  if (fv < WIFI_FIRMWARE_LATEST_VERSION) {
    Serial.println("Please upgrade the firmware");
  }
  Wire.begin();
  sensor.begin();

  //Declare that pin 12 is to be used for output to drive pump transistor gate
  pinMode(12, OUTPUT);
}

void loop() {
  //Verify WiFi connection.
  while (wifi_status != WL_CONNECTED) {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(wifi_ssid);
    wifi_status = WiFi.begin(wifi_ssid, wifi_pass);
    delay(10000);
  }
  
  //If no cookie then sign in
  if(cookieStr == NULL){
    String signinHeader = signIn(solid_username,solid_password);
    cookieStr = getCookie(signinHeader);
    Serial.println("COOKIE: "+cookieStr);
  }

  //Check Solid Pod for Commands in CSV file.
  String commandStr = getFile(cookieStr,"/commands.csv");
  processCommands(commandStr);

  //Redundant check for data file to get date from file header
  String headerStr = getFileHeader(cookieStr,"/commands.csv");
  String dateNumericStr = getDateNumeric(headerStr);

  //FSS
  String df = getFileHeader(cookieStr,"/garden_"+dateNumericStr+".ttl");
  int idx = df.indexOf("HTTP/1.1 200 OK");
  Serial.println("\n INDEX: "+String(idx));

  //Get WiFi Relative Signal Strength.  Set LED blinks to show WiFi strength bars for field placement.
  int rssi = getWifiStatus();
  blinkBars(rssi);

  //Get sensor and other values
  int moisture = soil_sensor_moisture();
  float temperature = soil_sensor_temperature();
  int free_ram = freeRam();

  //Compare moisture level with min and max levels to set pump status
  if((motor_status == 0) && (moisture < moisture_min)){
    motor_status = 1;
  }
  if((motor_status == 1) &&(moisture >= moisture_max)){
    motor_status = 0;
  }
  if(motor_status == 0){
    motorOff();
  }else{
    motorOn();
  }
  //Serial.println("MOTOR STATUS START: "+String(motor_status)+"   TEMP: "+String(temperature)+"  MOIST: "+String(moisture)+"  MIN: "+String(moisture_min)+"  MAX: "+String(moisture_max));

  //Send data to date specific file on Solid Pod
  String dateStr = getDate(headerStr);
  patchFile(cookieStr,"/garden_"+dateNumericStr+".ttl",dateStr,rssi,moisture,temperature,free_ram);

  //Wait until next time
  //delay(900000); //15 minutes for production
  delay(60000); //1 minute for testing
}


void processCommands(String commandStr){
  //Parse CSV file for datestamp, and minimum and maximum moisture value settings
  //Serial.println(commandStr);
  int startLoc = commandStr.indexOf("SETTING,");
  int ind1 = commandStr.indexOf(",",startLoc+7);
  int ind2 = commandStr.indexOf(",",ind1+1);
  int ind3 = commandStr.indexOf(",",ind2+1);
  String dateStr = commandStr.substring(ind1+1,ind2);
  String minStr = commandStr.substring(ind2+1,ind3);
  String maxStr = commandStr.substring(ind3+1);
  moisture_min = minStr.toInt();
  moisture_max = maxStr.toInt();
}


String signIn(char username[], char password[]) { //also disengages latch
  //Serial.println("\nStarting connection to server...");
  String respStr = "";
  String signinStr = "username="+String(username)+"&password="+String(password);
  int contentLen = signinStr.length();
  Serial.println("CONT LEN "+contentLen);
  if (client.connectSSL(solid_url, port)) {
    Serial.println("signing in: "+String(contentLen));
    client.println("POST /login/password HTTP/1.1");
    client.print("Host: "); client.println(solid_url);
    client.println("User-Agent: arduino");
    client.println("Accept-Encoding: gzip, deflate");
    client.println("Accept: */*");
    client.println("Connection: keep-alive");
    client.println("Content-Length: "+String(contentLen));
    client.println("Content-type: application/x-www-form-urlencoded");
    client.println();
    client.println(signinStr);
    client.println();

    while(respStr.length() < 1){
      delay(10000);
      while (client.available() >=1) {
        char c = client.read();
        respStr = respStr+c;
      }
    }
  } else {
    Serial.println("signIn() didn't connect");
  }
  return respStr;
}

String getFileHeader(String cookieStr,String filepath) {
  String respStr = "";
  if (client.connectSSL(solid_url, port)) {
    //Serial.println("HEAD "+filepath);
    client.println("HEAD "+filepath+" HTTP/1.1");
    client.print("Host: "); client.println(solid_url);
    client.println("User-Agent: arduino");
    client.println("Accept-Encoding: gzip, deflate");
    client.println("Accept: */*");
    client.println("Connection: keep-alive");
    client.println("Cookie: "+cookieStr);
    client.println();

    while(respStr.length() < 1){
      delay(10000);
      while (client.available() >=1) {
        char c = client.read();
        respStr = respStr+c;
      }
    }
  } else {
    Serial.println("didn't HEAD connect");
  }
  return respStr;
}

String getFile(String cookieStr,String filepath) {
  String respStr = "";
  if (client.connectSSL(solid_url, port)) {
    Serial.println("GET "+filepath);
    client.println("GET "+filepath+" HTTP/1.1");
    client.print("Host: "); client.println(solid_url);
    client.println("User-Agent: arduino");
    client.println("Accept-Encoding: gzip, deflate");
    client.println("Accept: */*");
    client.println("Connection: keep-alive");
    client.println("Cookie: "+cookieStr);
    client.println();

    while(respStr.length() < 1){
      delay(10000);
      while (client.available() >=1) {
        char c = client.read();
        respStr = respStr+c;
      }
    }
  } else {
    Serial.println("didn't GET connect");
  }
  return respStr;
}

String patchFile(String cookieStr,String filepath,String the_date,int the_rssi,int the_moisture, int the_temperature,int freeram) { //also disengages latch
  String respStr = "";
  String payloadStr = "INSERT DATA {";
  payloadStr += "\n  <:"+the_date+"> <extern:rssi> \""+the_rssi+"\"^^<xs:int>.";
  payloadStr += "\n  <:"+the_date+"> <extern:moisture> \""+the_moisture+"\"^^<xs:int>.";
  payloadStr += "\n  <:"+the_date+"> <extern:temperature> \""+the_temperature+"\"^^<xs:int>.";
  payloadStr += "\n  <:"+the_date+"> <extern:moisture_min> \""+moisture_min+"\"^^<xs:int>.";
  payloadStr += "\n  <:"+the_date+"> <extern:moisture_max> \""+moisture_max+"\"^^<xs:int>.";
  payloadStr += "\n  <:"+the_date+"> <extern:motor_status> \""+motor_status+"\"^^<xs:int>.";
  payloadStr += "\n  <:"+the_date+"> <extern:freeram> \""+freeram+"\"^^<xs:int>.";
  payloadStr += "\n};";
  
  int plen = payloadStr.length();
  if (client.connectSSL(solid_url, port)) {
    client.println("PATCH "+filepath+" HTTP/1.1");
    client.print("Host: "); client.println(solid_url);
    client.println("User-Agent: arduino");
    client.println("Cookie: "+cookieStr);
    client.println("Accept: */*");
    client.println("Connection: keep-alive");
    client.print("Content-Length: "); client.println(String(plen));
    client.println("Content-type: application/sparql-update");
    client.println();
    client.println(payloadStr);
    client.println();

    while(respStr.length() < 1){
      delay(10000);
      while (client.available() >=1) {
        char c = client.read();
        respStr = respStr+c;
      }
    }
  } else {
    Serial.println("didn't PATCH connect");
  }
  return respStr;
}

String getHeaderLine(String headerString,String headerName,String partEnd){
  //Utility for parsing header info
  int startLoc = headerString.indexOf(headerName);
  int endLoc = headerString.indexOf(partEnd,startLoc);
  String baseStr = headerString.substring(startLoc+headerName.length(),endLoc);
  return baseStr;
}

String getCookie(String headerString) {
  //Utility for getting Cookie from Signin header response
  String headerSegment = getHeaderLine(headerString,"Set-Cookie:",";");
  headerSegment.trim();
  return headerSegment;
}

//String getLocation(String headerString) {
//  String headerSegment = getHeaderLine(headerString,"Location:","\r\n");
//  headerSegment.trim();
//  return headerSegment;
//}

String getDateNumeric(String headerString) {
  //Utility for generating date string in a numeric format.  e.g. 20210921 for year 2021, September 21st
  String headerSegment = getHeaderLine(headerString,"Date:","\r\n");
  String monthStr = headerSegment.substring(9,12);
  String moStr = "";
  if(monthStr == "Jan"){
    moStr = "01";
  }else if(monthStr == "Feb"){
    moStr = "02";
  }else if(monthStr == "Mar"){
    moStr = "03";
  }else if(monthStr == "Apr"){
    moStr = "04";
  }else if(monthStr == "May"){
    moStr = "05";
  }else if(monthStr == "Jun"){
    moStr = "06";
  }else if(monthStr == "Jul"){
    moStr = "07";
  }else if(monthStr == "Aug"){
    moStr = "08";
  }else if(monthStr == "Sep"){
    moStr = "09";
  }else if(monthStr == "Oct"){
    moStr = "10";
  }else if(monthStr == "Nov"){
    moStr = "11";
  }else if(monthStr == "Dec"){
    moStr = "12";
  }
  return headerSegment.substring(13,17)+String(moStr)+headerSegment.substring(6,8);
}

String getDate(String headerString) {
  //Create text date representation from header
  String headerSegment = getHeaderLine(headerString,"Date:","\r\n");
  headerSegment.replace(" ","-");
  String dateStr = headerSegment;
  dateStr.trim();
  return dateStr;
}

void blinkBars(int rssi){
  //Show WiFi bars leve lon board for field placement: 4 fast blinks for 4 bars, 3 fast blinks for 3 bars, ...
  int bars = 0;
  if(rssi <= -70){
    bars = 1;
  }else if(rssi <= -60){
    bars = 2;
  }else if(rssi <= -50){
    bars = 3;
  }else{
    bars = 4;
  }
  pinMode(LED_BUILTIN,OUTPUT);
  for(int i=0;i<bars;i++){
    digitalWrite(LED_BUILTIN,HIGH);
    delay(300);
    digitalWrite(LED_BUILTIN,LOW);
    delay(300);
  }
}

int freeRam(){
  //Calculate free RAM on Arduino
  //https://playground.arduino.cc/Code/AvailableMemory/
  extern int __heap_start, *__brkval;
  int v;
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
}

int getWifiStatus(){
  // print the SSID of the network you're attached to:
  //Serial.print("SSID: "); Serial.println(WiFi.SSID());

  // print your board's IP address:
  //IPAddress ip = WiFi.localIP();
  //Serial.print("IP Address: "); Serial.println(ip);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  //Serial.print("signal strength (RSSI):"); Serial.print(rssi); Serial.println(" dBm");
  return rssi;
}

int soil_sensor_moisture(){
  //CATNIP MOISTURE SENSOR
  while(sensor.isBusy()) { delay(50); }
  return (int)sensor.getCapacitance();
  //SEESAW MOISTURE SENSOR
  //if (ss.begin(0x36)) {
  //  uint16_t capread = ss.touchRead(0);
  //  Serial.print("Capacitive: "); Serial.println(capread);
  //  return capread;
  //}
}


float soil_sensor_temperature(){
  //CATNIP MOISTURE SENSOR
  while(sensor.isBusy()) { delay(50); }
  return (float)(sensor.getTemperature()/(float)10);
  //SEESAW MOISTURE SENSOR
  //float tempC = 0.0;
  //if (ss.begin(0x36)) {
  //  tempC = ss.getTemp();
  //  Serial.print("Temperature: "); Serial.print(tempC); Serial.println("*C");
  //}
  //return tempC;
}

void motorOn(){
  //SET PIN 12 HIGH TO TURN PUMP MOTOR ON
  digitalWrite(12,HIGH);
}

void motorOff(){
  //SET PIN 12 LOW TO TURN PUMP MOTOR OFF
  digitalWrite(12,LOW);
}
