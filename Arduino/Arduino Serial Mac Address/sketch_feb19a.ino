#include "WiFiS3.h"
int led =  LED_BUILTIN;
int status = WL_IDLE_STATUS;
String data = "" ;
int id = -1;
String username = "";
String password = "";
bool wifiConnected = false;
WiFiServer server(80);
void setup() {
  Serial.begin(9600); // Initialize serial communication at 9600 baud rate
  pinMode(led, OUTPUT);      // set the LED pin mode
}

void loop() {
  if (Serial.available() > 0 ) { // Check if data is available to read
    id = Serial.readStringUntil('\n').toInt(); // Read the data until a newline character is received
   while (Serial.available() <= 0 ) {
    delay(100);
   }
    username = Serial.readStringUntil('\n');
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
    status = WiFi.begin(ssid, pass);
    delay(10000); 
  }
  printWifiStatus();
}
void printWifiStatus() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your board's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");  byte mac[6];
  WiFi.macAddress(mac);
  Serial.print("MAC address: ");
  printMacAddress(mac);
}
void printMacAddress(byte mac[]) {for (int i = 0; i < 6; i++) {
    if (i > 0) {
      Serial.print(":");
    }
    if (mac[i] < 16) {
      Serial.print("0");
    }
    Serial.print(mac[i], HEX);
  }
  Serial.println();
}

