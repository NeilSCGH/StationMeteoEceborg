#include <Wire.h> //I2C needed for sensors
#include "SoftwareSerial.h"
#include "SparkFunMPL3115A2.h" //Pressure sensor - Search "SparkFun MPL3115" and install from Library Manager
#include "SparkFun_Si7021_Breakout_Library.h" //Humidity sensor - Search "SparkFun Si7021" and install from Library Manager

String ssid ="Bluetooth";
String password="blueblue3";

//Initializing objects
SoftwareSerial esp(15,14);// RX, TX
MPL3115A2 myPressure; //Create an instance of the pressure sensor
Weather myHumidity;//Create an instance of the humidity sensor

//Hardware pin definitions
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
const byte STAT_BLUE = 7;
const byte STAT_GREEN = 8;

const byte REFERENCE_3V3 = A3;
const byte LIGHT = A1;
const byte BATT = A2;

//Global Variables
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
long lastSecond; //The millis counter to see when a second rolls by

String data;
String server = "192.168.???"; // www.example.com
String uri = "/esp.php";// our example is /esppost.php

void setup() {
  esp.begin(115200);
  Serial.begin(115200);
  Serial.println("hey");

  pinMode(STAT_BLUE, OUTPUT); //Status LED Blue
  pinMode(STAT_GREEN, OUTPUT); //Status LED Green

  pinMode(REFERENCE_3V3, INPUT);
  pinMode(LIGHT, INPUT);
  
  //Configure the pressure sensor
  startPressureSensor();
  
  lastSecond = millis();
  Serial.println("Weather Shield online!");
  
  reset();
  connectWifi();
}

void loop()
{
  //Print readings every second
  if (millis() - lastSecond >= 1000)
  {
    digitalWrite(STAT_BLUE, HIGH); //Blink stat LED

    lastSecond += 1000;

    //Check Humidity Sensor
    float humidity = myHumidity.getRH();

    if (humidity == 998) //Humidty sensor failed to respond
    {
      Serial.println("I2C communication to sensors is not working. Check solder connections.");

      //Try re-initializing the I2C comm and the sensors
      startPressureSensor();
    }
    else
    {
      float temp_h = myHumidity.getTempF();//Check Temperature Sensor
      float pressure = myPressure.readPressure();//Check Pressure Sensor
      float tempf = myPressure.readTempF();//Check tempf from pressure sensor
      float light_lvl = get_light_level();//Check light sensor

      data = printValues(humidity, temp_h, pressure, tempf, light_lvl);
      
      Serial.println(data);
      httppost();
    }

    digitalWrite(STAT_BLUE, LOW); //Turn off stat LED
  }

  delay(100);
}

void startPressureSensor(){
  //Configure the pressure sensor
  myPressure.begin(); // Get sensor online
  myPressure.setModeBarometer(); // Measure pressure in Pascals from 20 to 110 kPa
  myPressure.setOversampleRate(7); // Set Oversample to the recommended 128
  myPressure.enableEventFlags(); // Enable all three pressure and temp event flags
  myHumidity.begin();//Configure the humidity sensor
}

String printValues(float humidity, float temp_h, float pressure, float tempf, float light_lvl){
  Serial.print("Humidity = ");
  Serial.print(humidity);
  Serial.print("%,");
  
  Serial.print(" temp_h = ");
  Serial.print(temp_h, 2);
  Serial.print("F,");
  
  //Check Pressure Sensor
  Serial.print(" Pressure = ");
  Serial.print(pressure);
  Serial.print("Pa,");
  
  //Check tempf from pressure sensor
  Serial.print(" temp_p = ");
  Serial.print(tempf, 2);
  Serial.print("F,");
  
  //Check light sensor
  Serial.print(" light_lvl = ");
  Serial.print(light_lvl);
  Serial.println("V");

  return ("hum=" + String(humidity) + "&temph=" + String(temp_h) + "&press=" + String(pressure) + "&tempf=" + String(tempf) + "&light=" + String(light_lvl));
}

//reset the esp8266 module
void reset() {
  esp.println("AT+RST");
  delay(1000);
  
  while (esp.available()){
     String inData = esp.readStringUntil('\n');
     Serial.println("Got reponse from ESP8266: " + inData);
  }
  Serial.println("Reset done");
}

//connect to the wifi network
void connectWifi() {
  String cmd = "AT+CWJAP=\"" +ssid+"\",\"" + password + "\"";
  esp.println(cmd);
  delay(4000);
  Serial.println("Connection done");
}

void httppost () {
  Serial.println("POST METHOD RUNNING");
  
  esp.println("AT+CIPSTART=\"TCP\",\"" + server + "\",80");//start a TCP connection.
  Serial.println("AT+CIPSTART=\"TCP\",\"" + server + "\",80");
  
  delay(1000);
  String postRequest =
    "POST " + uri + " HTTP/1.0\r\n" +
    "Host: " + server + "\r\n" +
    "Accept: *" + "/" + "*\r\n" +
    "Content-Length: " + data.length() + "\r\n" +
    "Content-Type: application/x-www-form-urlencoded\r\n" +
    "\r\n" + data;
    
  String sendCmd = "AT+CIPSEND=";//determine the number of caracters to be sent.
  esp.print(sendCmd);
  esp.println(postRequest.length());
  delay(500);
  if(esp.find(">")) { 
    Serial.println("Sending.."); esp.print(postRequest);
    if( esp.find("SEND OK")) { 
      Serial.println("Packet sent");
      while (esp.available()) {
        String tmpResp = esp.readString();
        Serial.println(tmpResp);
      }
      // close the connection
      esp.println("AT+CIPCLOSE");
    }
  }
}

//Returns the voltage of the light sensor based on the 3.3V rail
//This allows us to ignore what VCC might be (an Arduino plugged into USB has VCC of 4.5 to 5.2V)
float get_light_level()
{
  float operatingVoltage = analogRead(REFERENCE_3V3);

  float lightSensor = analogRead(LIGHT);

  operatingVoltage = 3.3 / operatingVoltage; //The reference voltage is 3.3V

  lightSensor = operatingVoltage * lightSensor;

  return (lightSensor);
}

//Returns the voltage of the raw pin based on the 3.3V rail
//This allows us to ignore what VCC might be (an Arduino plugged into USB has VCC of 4.5 to 5.2V)
//Battery level is connected to the RAW pin on Arduino and is fed through two 5% resistors:
//3.9K on the high side (R1), and 1K on the low side (R2)
float get_battery_level()
{
  float operatingVoltage = analogRead(REFERENCE_3V3);

  float rawVoltage = analogRead(BATT);

  operatingVoltage = 3.30 / operatingVoltage; //The reference voltage is 3.3V

  rawVoltage = operatingVoltage * rawVoltage; //Convert the 0 to 1023 int to actual voltage on BATT pin

  rawVoltage *= 4.90; //(3.9k+1k)/1k - multiple BATT voltage by the voltage divider to get actual system voltage

  return (rawVoltage);
}
