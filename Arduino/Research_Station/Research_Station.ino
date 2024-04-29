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
#define MeasureDelay 1000
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
#define Pressure 0
#define Humidity 1
#define Temperature 2
#define IP 3
#define Name 4

int state = Name;
int id = -1;
int timeOut = -1;
int status = WL_IDLE_STATUS;
float weatherData[3];
char utilstr[128];
String SSID = "";
String password = "";
String name = "";
String ipAddress = "";
float GPSData[2] = {0.0, 0.0};

// Components
SoftwareSerial gpsSerial(GPS_RX_PIN,13);
LPS22HHSensor PressTemp(&DEV_I2C);
HTS221Sensor HumTemp(&DEV_I2C);
STTS751Sensor Temp3(&DEV_I2C);

CBOR Encoder;

WiFiSSLClient net;
PubSubClient client(net);


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

void loop() 
{
  // put your main code here, to run repeatedly:
  checkIfConnected();
  readWeatherData(weatherData);
  readGPSData();
  displayToSerial();
  if(!GPSData[0] <0.001 && GPSData[1] > 0.01 && GPSData[0] != GPSData[1]){
    deliver();
  }
  delay(MeasureDelay);
}

void checkIfConnected()
{
  if(!client.connected())
    reconnectToMQTT();
  client.loop();
}

void tempconfig()
{
  //temp config
  id = 5858;
  SSID = "";
  password = "";
  name = "Elevator Test";
}

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

void callback(char* topic, byte* payload, unsigned int length) 
{
  Serial.print("Message received on topic");
  Serial.println(topic);

  // copy into buffer and slap a 00 on instead?
  for(int i = 0; i < length; i++) 
    Serial.print((char)payload[i]);

  Serial.println();
}

void connectToMQTT()
{
  // Set server and port
  client.setServer(MQTT_SERVER, MQTT_PORT);
  client.setCallback(callback);

  // Set SSL/TLS certificate
  net.setCACert(rootCACertificate);
}

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
      client.publish("/test", "Hello from ESP32!!!"); 
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

void readWeatherData(float weatherData[])
{
 float humidity = 0;
  HumTemp.GetHumidity(&humidity);

  // Read pressure and temperature.
  float pressure = 0;
  PressTemp.GetPressure(&pressure);

  //Read temperature
  float temperature = 0;
  Temp3.GetTemperature(&temperature);

  weatherData[0] = humidity;
  weatherData[1] = pressure;
  weatherData[2] = temperature;
}

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

float dmsToDd(float coord)
{
  float degree = floor(coord/100.0);
  float minute = coord - 100.0 * degree;
  float second = minute - floor(minute);
  return degree + minute/60.0 + second/3600.0;
}

void deliver()
{
  String Data = "";
  switch(state){
    case Pressure:
      Data = String(weatherData[1]);
      break;
    case Humidity:
      Data = String(weatherData[0]);
      break;
    case Temperature:
      Data = String(weatherData[2]);
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
  client.publish("/test/cbor", (const uint8_t *)(utilstr), Encoder.report_size());

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

void displayToSerial()
{
  float readableGPSData[] = {dmsToDd(GPSData[0]), dmsToDd(-GPSData[1])};
  Serial.println("DeviceID: " + String(id));
  Serial.println("Humidity: " + String(weatherData[0]));
  Serial.println("Pressure: " + String(weatherData[1]));
  Serial.println("Temperature: " + String(weatherData[2]));
  Serial.println("Latitude: " + String(readableGPSData[0], 2));
  Serial.println("Longitude: " + String(readableGPSData[1], 2));
}
