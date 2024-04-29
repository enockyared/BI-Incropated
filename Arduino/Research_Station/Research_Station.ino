/*
 *  This code flashes a WiFi capable Arduino Uno r4 to run as a headless research station.
 *  This device requires a X-NUCLEO-IKS01A3 shield plugged into the device, along with a
 *   GPS-19166 GPS module plugged into ground, 3.3v, and pin 8 on the shield.
 *  
 *  It first prompts the user via Serial (9600 Baud rate) for an ID (range 1000-9999), 
 *  SSID, network password, and device name; and then if the device is able to connect with
 *  that info to both WiFi and MQTT it will start sending measured location and atmospheric
 *  data to the given MQTT server. This prompt can be bypassed by filling out the tempconfig()
 *  function beforehand. 
 *  
 *  The data sent can be pressure, temperature, and humidity data; along 
 *  with the device's IP address and given name. It also delivers gps data measured by a connected
 *  GPS Module. The delivered data can be chosen by the state variable, using the 5 given message 
 *  type definitions below. The message will always include the GPS Data.
 *
 *  Sent GPS data must be converted from DMS to DD to be used in the database. The data is also converted
 *  When displayed over serial.
 */
#include "src/config.h"
#include "src/sensitive.h"
#include "src/certs.h"
#include "src/serializer.h"
#include "src/cbor.h"

#include <LPS22HHSensor.h>
#include <STTS751Sensor.h>
#include <HTS221Sensor.h>
#include <SoftwareSerial.h>
#include <WiFiS3.h>
#include <PubSubClient.h>

#define GPS_RX_PIN 8
#define BAUD_RATE 9600
#ifdef ARDUINO_SAM_DUE
#define DEV_I2C Wire1
#elif defined(ARDUINO_ARCH_STM32)
#define DEV_I2C Wire
#elif defined(ARDUINO_ARCH_AVR)
#define DEV_I2C Wire
#else
#define DEV_I2C Wire
#endif
#define SerialPort Serial

//Edit to change the time between message pulses
#define MeasureDelay 1000

//Below are the message types being sent
#define Pressure 0
#define Humidity 1
#define Temperature 2
#define IP 3
#define Name 4

//Below are variables which assure the device connects to the internet successfully
int timeOut = -1;
int status = WL_IDLE_STATUS;

//Change state to change message type
int state = Name;

//change to alter where the device sends messages to
char* topicPath = "/test/cbor";

//Below are the variables used for sending and displaying data
int id = -1;
float weatherData[3];
float GPSData[2] = {0.0, 0.0};
String SSID = "";
String password = "";
String name = "";
String ipAddress = "";
char utilstr[128];

// Components
SoftwareSerial gpsSerial(GPS_RX_PIN,13);
LPS22HHSensor PressTemp(&DEV_I2C);
HTS221Sensor HumTemp(&DEV_I2C);
STTS751Sensor Temp3(&DEV_I2C);

//Encoding and communication components
CBOR Encoder;
WiFiSSLClient net;
PubSubClient client(net);

//Runs once on power-on. Sets up all measurement devices, and connects to wifi and MQTT. Promts for setup info if needed.
void setup() 
{
  tempconfig();
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
  Serial.begin(9600);
  SerialPort.begin(9600);
  gpsSerial.begin(BAUD_RATE);
  DEV_I2C.begin();
  PressTemp.begin();
  PressTemp.Enable();
  HumTemp.begin();
  HumTemp.Enable();
  Temp3.begin();
  Temp3.Enable();
  if(id == -1){
    prompt();
  }
  else{
  connectToWifi(SSID, password);
  }
  connectToMQTT();
}

//Runs continuously while the device has power. Reads in data and publishes it to MQTT and Serial.
void loop() 
{
  //First makes sure the device is connected to WiFi and MQTT.
  checkIfConnected();
  readWeatherData();
  readGPSData();
  displayToSerial();
  //cleans out some common GPS errors before delivering data
  if(!GPSData[0] <0.001 && GPSData[1] > 0.01 && GPSData[0] != GPSData[1]){
    deliver();
  }
  delay(MeasureDelay);
}

//preloads the device with given config data. disable by setting id to -1
void tempconfig()
{
  //temp config
  id = 5858;
  SSID = "";
  password = "";
  name = "Elevator Test";
}

//Prompts the user over Serial for config data and connects to WiFi with it.
void prompt()
{
  delay(1000);
  do{
    Serial.println("Awaiting ID; must be be 4 integers 1000-9999");
    while (Serial.available() <= 0 ) {
      delay(100);
    }
    id = Serial.readStringUntil('\n').toInt(); // Read the data until a newline character is received
  }while(id < 1000 || id > 9999);
  Serial.println("Awaiting SSID");
  while (Serial.available() <= 0 ) {
    delay(100);
  }
  SSID = Serial.readStringUntil('\n');
  Serial.println("Awaiting Password");
  while (Serial.available() <= 0 ) { 
    delay(100);
  }
  password = Serial.readStringUntil('\n');
  Serial.println("Awaiting System Name");
  while (Serial.available() <= 0 ) { 
    delay(100);
  }
  name = Serial.readStringUntil('\n');
  connectToWifi(SSID, password);
}

//Attempts to connect to WiFi 10 times before timing out and reprompting for config data.
void connectToWifi(String SSID, String password)
{
  char ssid[SSID.length() + 1];
  char pass[password.length() + 1];
  SSID.toCharArray(ssid, SSID.length() + 1);
  password.toCharArray(pass, password.length() + 1);
  Serial.println("Connecting to " + String(ssid));    
    if(password.length() > 0){
      status = WiFi.begin(ssid, pass);
    }
    else{
      status = WiFi.begin(ssid);
    }
  timeOut = 10;
  while(WiFi.status() != WL_CONNECTED && timeOut >= 0) 
  {
    delay(1000);
    Serial.print(".");
    timeOut --;
  }
    Serial.println(".");

  if(WiFi.status() == WL_CONNECTED){
    Serial.println("Connected!");
    ipAddress = WiFi.gatewayIP().toString();
  }
  else
    prompt();
}

//Generic callback function for recieving data from MQTT Server from the given topic.
void callback(char* topic, byte* payload, unsigned int length) 
{
  Serial.print("Message received on topic");
  Serial.println(topic);

  // copy into buffer and slap a 00 on instead?
  for(int i = 0; i < length; i++) 
    Serial.print((char)payload[i]);

  Serial.println();
}

//Connects to MQTT with the server and port and certificate given in the config.h and certs.h files
void connectToMQTT()
{
  // Set server and port
  client.setServer(MQTT_SERVER, MQTT_PORT);
  client.setCallback(callback);

  // Set SSL/TLS certificate
  net.setCACert(rootCACertificate);
}

//Checks to see if the MQTT Client is connected. If not it reconnects to MQTT
void checkIfConnected()
{
  if(!client.connected())
    reconnectToMQTT();
  client.loop();
}

//reconnects to MQTT if not connected, and sends a message to the broker when connected.
void reconnectToMQTT() 
{
  while(!client.connected()) 
  {
     while(WiFi.status() != WL_CONNECTED) 
  {
    delay(1000);
    Serial.print(".");
  }

    Serial.print("Attempting MQTT connection...");
    if(client.connect("ESP32Client", MQTT_USER, MQTT_PASS)) 
    {
      Serial.println("connected");
      client.publish("/station/connect", "Hello from the field"); 
      client.subscribe("/in"); 
    }
    else 
    {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 1 second");
      delay(1000);
    }
  }
}

//reads in humidity, pressure, and temperature data into weatherData using their defined state values
void readWeatherData()
{
  // Read pressure
  float pressure = 0;
  PressTemp.GetPressure(&pressure);

 float humidity = 0;
  HumTemp.GetHumidity(&humidity);

  //Read temperature
  float temperature = 0;
  Temp3.GetTemperature(&temperature);

  weatherData[Pressure] = pressure;
  weatherData[Humidity] = humidity;
  weatherData[Temperature] = temperature;
}

//reads in raw GPS Data (DMS in one full number) into the GPSData array by calling the parseCoordinate function
void readGPSData()
{
  if (gpsSerial.available() > 0) {
    String sentence = gpsSerial.readStringUntil('\n');
    if (sentence.startsWith("$GPGGA")) {
      //Parse the sentence to extract latitude and longitude
      GPSData[0] = parseCoordinate(sentence, ',', 2);
      GPSData[1] = parseCoordinate(sentence, ',', 4);
    }
  }
  
}

//Takes the measured GPGGA data and seperates it into raw GPS Data (DMS in one full number)
float parseCoordinate(String data, char separator, int index) 
{
  int commaIndex = 0;
  int dataIndex = 0;
  for (int i = 0; i < data.length(); i++) {
    if (data[i] == separator) {
      commaIndex++;
      if (commaIndex == index) {
        dataIndex = i + 1;
        break;
      }
    }
  }
  String coordString = data.substring(dataIndex, dataIndex + 10);
  return coordString.toFloat();
}

//Converts the stored dms gpa data to DD gps data for diplay purposes.
float dmsToDd(float coord)
{
  float degree = floor(coord/100.0);
  float minute = coord - 100.0 * degree;
  float second = minute - floor(minute);
  return degree + minute/60.0 + second/3600.0;
}

//Delivers the data corresponding to the current state to the given MQTT topic path, then cycles to another data type. 
void deliver()
{
  String Data = "";
  switch(state){
    case Pressure:
      Data = String(weatherData[Pressure]);
      break;
    case Humidity:
      Data = String(weatherData[Humidity]);
      break;
    case Temperature:
      Data = String(weatherData[Temperature]);
      break;
    case IP:
      Data = ipAddress;
      break;
    case Name:
      Data = name;
      break;
  }
  char dataString[Data.length() + 1];
  Data.toCharArray(dataString, Data.length() + 1);
  Encoder.assign_input_buffer((uint8_t *)(utilstr), 128);
  Encoder.declare_variable_length_map();
      Encoder.write_utf8_string("ID", 2); 
      Encoder.write_integer_value(id, true);
      Encoder.write_utf8_string("Measurements", 12);
      Encoder.declare_variable_length_map();
          Encoder.write_utf8_string("la", 2); Encoder.write_float_value(GPSData[0]);
          Encoder.write_utf8_string("lo", 2); Encoder.write_float_value(GPSData[1]);
          Encoder.write_utf8_string("da", 2); Encoder.write_utf8_string(dataString, sizeof(dataString)-1);
          Encoder.write_utf8_string("ST", 2); Encoder.write_integer_value(state, true);
      Encoder.terminate_variable_length_object();
  Encoder.terminate_variable_length_object();
  client.publish(topicPath, (const uint8_t *)(utilstr), Encoder.report_size());

  switch(state){
    case Pressure:
      state = Humidity;
      break;
    case Humidity:
      state = Temperature;
      break;
    case Temperature:
      state = IP;
      break;
    case IP:
      state = Pressure;
      break;
    case Name:
      state = Pressure;
      break;
  }
}

//Displays the current data to the Serial Monitor
void displayToSerial()
{
  float readableGPSData[] = {dmsToDd(GPSData[0]), dmsToDd(-GPSData[1])};
  Serial.println("DeviceID: " + String(id));
  Serial.println("Device name: " + name);
  Serial.println("Pressure: " + String(weatherData[Pressure]));
  Serial.println("Humidity: " + String(weatherData[Humidity]));
  Serial.println("Temperature: " + String(weatherData[Temperature]));
  Serial.println("Latitude: " + String(readableGPSData[0]));
  Serial.println("Longitude: " + String(readableGPSData[1]));
}
