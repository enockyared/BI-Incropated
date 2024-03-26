
// #include "src/config.h"
// #include "src/sensitive.h"
// #include "src/certs.h"
// #include "src/serializer.h"
#include "WiFiS3.h"
#include <LPS22HHSensor.h>
#include <STTS751Sensor.h>
#include <HTS221Sensor.h>
#include <SoftwareSerial.h>
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

// Components
SoftwareSerial gpsSerial(GPS_RX_PIN,13);
LPS22HHSensor PressTemp(&DEV_I2C);
HTS221Sensor HumTemp(&DEV_I2C);
STTS751Sensor Temp3(&DEV_I2C);

String latitudeString = "";
String longitudeString = "";
int led =  LED_BUILTIN;
int status = WL_IDLE_STATUS;
String data = "" ;
int id = -1;
String username = "";
String password = "";
bool wifiConnected = false;
void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  SerialPort.begin(9600);
  DEV_I2C.begin();
  PressTemp.begin();
  PressTemp.Enable();
  HumTemp.begin();
  HumTemp.Enable();
  Temp3.begin();
  Temp3.Enable();
  Serial.begin(9600);
  gpsSerial.begin(BAUD_RATE);
  digitalWrite(LED_BUILTIN, LOW);
}

void loop() {
  if (Serial.available() > 0 ) { // Check if data is available to read
    id = Serial.readStringUntil('\n').toInt(); // Read the data until a newline character is received
    Serial.println("Awaiting SSID");
   while (Serial.available() <= 0 ) {
    delay(100);
   }
    username = Serial.readStringUntil('\n');
    Serial.println("Awaiting Password");
    while (Serial.available() <= 0 ) { 
    delay(100);
   }
    password = Serial.readStringUntil('\n');
    wifiConnected = true;
    char ssid[username.length() + 1];
    char pass[password.length() + 1];
    username.toCharArray(ssid, username.length() + 1);
    password.toCharArray(pass, password.length() + 1);
    Serial.print("Attempting to connect to Network named: ");
    Serial.println(ssid);    
    if(password.length() > 0){
      status = WiFi.begin(ssid, pass);
    }
    else{
      status = WiFi.begin(ssid);
    }
    delay(10000); 
  }

  // Read humidity and temperature.
  float humidity = 0;
  HumTemp.GetHumidity(&humidity);

  // Read pressure and temperature.
  float pressure = 0;
  PressTemp.GetPressure(&pressure);

  //Read temperature
  float temperature = 0;
  Temp3.GetTemperature(&temperature);

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
      latitudeString = String(degLat) + "° " + String(int(minLat)) + "' " + String(secLat, 5) + "\" " + ((latitude > 0) ? "N" : "S");
      longitudeString = String(degLon) + "° " + String(int(minLon)) + "' " + String(secLon, 5) + "\" " + ((longitude > 0) ? "W" : "E");      
    }
  }
  // Deliverables
  SerialPort.print("Latitude: ");
  SerialPort.println(latitudeString);
  SerialPort.print("Longitude: ");
  SerialPort.println(longitudeString);
  SerialPort.print("| Hum[%]: ");
  SerialPort.print(humidity, 2);
  SerialPort.print(" | Pres[hPa]: ");
  SerialPort.print(pressure, 2);
  SerialPort.print(" | Temp[C]: ");
  SerialPort.print(temperature, 2);
  SerialPort.println(" |");
  if(status == WL_CONNECTED){
    SerialPort.print("ID: ");
    SerialPort.print(id);
    SerialPort.print(", Connected to: ");
    SerialPort.print(username);
    SerialPort.println();
    digitalWrite(LED_BUILTIN, HIGH);
    delay(250);
    digitalWrite(LED_BUILTIN, LOW);
    delay(250);
  }
  delay(1000);
}

float parseCoordinate(String data, char separator, int index) {
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

void deliver(String latitude, String longitude, float humidity, float pressure, float temperature){
  //TODO
}
