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
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27,16,2);  // set the LCD address to 0x3F for a 16 chars and 2 line display


#define GPS_RX_PIN 8
#define BUTTON_PIN 7
#define BAUD_RATE 9600
#define MeasureDelay 100
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
#define BOUNCE 5

int Delay = 0;
int bounce = 0;
int pressed = LOW;
int id = -1;
int timeOut = -1;
int status = WL_IDLE_STATUS;
float weatherData[3];
char utilstr[128];
String SSID = "";
String password = "";
String name = "";
String ipAddress = "";
String GPSData[2];
int state = Name;
float rawGPSData[2] = {0.0, 0.0};

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
  lcd.init();
  lcd.clear();
  lcd.backlight();
  tempconfig();
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
  pinMode(BUTTON_PIN, INPUT);
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

void loop() {
  // put your main code here, to run repeatedly:
  checkIfConnected();
  processButton();
  readWeatherData(weatherData); 
  // readGPSData();
  readRawGPSData();
  // displayToSerial(id, weatherData, GPSData);
  if(Delay % 20 == 0){
    displayToLCD(id, weatherData, GPSData);
  }
  if(Delay <= 0){
    displayToSerialRaw(id, weatherData);
  if(rawGPSData[0] != rawGPSData[1]){
    deliverRaw(id, weatherData, rawGPSData);
  }
  Delay = MeasureDelay;
  }
  else{
    Delay -=1;
  }
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
  id = 4460;
  SSID = "";
  password = "";
  name = "Button Station";
}

void processButton()
{
  pressed = digitalRead(BUTTON_PIN);
  if(bounce > 0 && pressed == LOW){
    bounce -= 1;
  }
  else{
    if(bounce <= 0 && pressed == HIGH){
      bounce = BOUNCE;
      if(state == Temperature){
        state = Pressure;
      }
      else{
        state +=1;
      }
    }
  }

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
  id = Serial.readStringUntil('\n').toInt(); // Read the data until a newline character is received
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

void readRawGPSData()
{
  if (gpsSerial.available() > 0) {
    String sentence = gpsSerial.readStringUntil('\n');
    if (sentence.startsWith("$GPGGA")) {
      //Parse the sentence to extract latitude and longitude
      rawGPSData[0] = parseCoordinate(sentence, ',', 2);
      rawGPSData[1] = parseCoordinate(sentence, ',', 4);
    }
  }
  
}
void readGPSData(String GPSData[])
{
  if (gpsSerial.available() > 0) {
    String sentence = gpsSerial.readStringUntil('\n');
    if (sentence.startsWith("$GPGGA")) {
      //Parse the sentence to extract latitude and longitude
      float latitude = parseCoordinate(sentence, ',', 2);
      float longitude = parseCoordinate(sentence, ',', 4);
      int degLat = int(latitude / 100);
      float minLat = fmod(latitude, 100.0);
      int degLon = int(longitude / 100);
      float minLon = fmod(longitude, 100.0);
      float secLat = (minLat - int(minLat)) * 60.0;
      float secLon = (minLon - int(minLon)) * 60.0;
      if(latitude > 30 && longitude > 30 && latitude != longitude){
        GPSData[0] = String(dms_to_dd(degLat, minLat, secLat));
        GPSData[1] = String(-dms_to_dd(degLon, minLon, secLon));
      }

    }
  }
  
}

float makeCoordsReadable(float coord){
      int deg = int(coord / 100);
      float min = fmod(coord, 100.0);
      float sec = (min - int(min));
      return float(deg) + min/60.0 + sec/3600.0;
}

float dms_to_dd(float degrees, float minutes, float seconds) {
    float dd = degrees + (minutes / 60.0) + (seconds / 3600.0);
    return dd;
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

void deliver(int ID, float weatherData[], String GPSData[]){
  String deliverString = "DeviceID: " + String(ID);
  deliverString += ", la: " + GPSData[0];
  deliverString += ", lo: " + GPSData[1];
  deliverString += ", hu: " + String(weatherData[0], 2);
  deliverString += ", pr: " + String(weatherData[1], 2);
  deliverString += ", te: " + String(weatherData[2], 2);
  deliverString += ", ST: " + String(state);
  deliverString.toCharArray(utilstr, 128);
    Serial.readBytesUntil('\n',utilstr,128);
    client.publish("/test/topic", utilstr); 

}
void deliverRaw(int ID, float weatherData[], float rawGPSData[]){
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
      Encoder.write_integer_value(ID, true);
      Encoder.write_utf8_string("Measurements", 12);
      Encoder.declare_variable_length_map();
          Encoder.write_utf8_string("la", 2); Encoder.write_float_value(rawGPSData[0]);
          Encoder.write_utf8_string("lo", 2); Encoder.write_float_value(rawGPSData[1]);
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

void displayToSerial(int ID, float weatherData[], String GPSData[])
{
  Serial.println("DeviceID: " + String(id));
  Serial.println("Humidity: " + String(weatherData[0]));
  Serial.println("Pressure: " + String(weatherData[1]));
  Serial.println("Temperature: " + String(weatherData[2]));
  Serial.println("Latitude: " + String(GPSData[0]));
  Serial.println("Longitude: " + String(GPSData[1]));
}

void displayToSerialRaw(int ID, float weatherData[])
{
  Serial.println("DeviceID: " + String(id));
  Serial.println("Humidity: " + String(weatherData[0]));
  Serial.println("Pressure: " + String(weatherData[1]));
  Serial.println("Temperature: " + String(weatherData[2]));
  Serial.println("Latitude: " + String(rawGPSData[0]));
  Serial.println("Longitude: " + String(rawGPSData[1]));
}
void displayToLCD(int ID, float weatherData[], String GPSData[])
{
  lcd.clear();
  switch(state){
    case Pressure:
      lcd.setCursor(0, 0);
      lcd.print("Pres.: " + String(weatherData[1]));
      break;
    case Humidity:
      lcd.setCursor(0, 0);
      lcd.print("Humi.: " + String(weatherData[0]));
      break;
    case Temperature:
      lcd.setCursor(0, 0);
      lcd.print("Temp.: " + String(weatherData[2]));
      break;
    case IP:
      lcd.setCursor(0, 0);
      lcd.print("IP: " + ipAddress);
      break;
    case Name:
      lcd.setCursor(0, 0);
      lcd.print("Name: " + name);
      break;
  }
  lcd.setCursor(0,1);
  lcd.print(String(makeCoordsReadable(rawGPSData[0]), 2) + ", " + String(-makeCoordsReadable(rawGPSData[1]), 2));

}
