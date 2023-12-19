#include "config.h"
#include "sensitive.h"
#include "certs.h"
#include "serializer.h"

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>

WiFiClientSecure espClient;
PubSubClient client(espClient);
int pub_ct = 0;
char utilstr[128];

void callback(char* topic, byte* payload, unsigned int length) 
{
  Serial.printf("Message received on topic: %s\n", topic);

  // copy into buffer and slap a 00 on instead?
  for(int i = 0; i < length; i++) 
    Serial.print((char)payload[i]);

  Serial.println();
}

void setup() {
  Serial.begin(9600);

  // Connect to WiFi
  delay(1000);
  Serial.println("Connecting to WiFi...");
  WiFi.begin(WIRELESS_SSID, WIRELESS_PASSWD);

  while(WiFi.status() != WL_CONNECTED) 
  {
    delay(1000);
    Serial.print(".");
  }

  Serial.println("Connected!");

  // Set server and port
  client.setServer(MQTT_SERVER, MQTT_PORT);
  client.setCallback(callback);

  // Set SSL/TLS certificate
  espClient.setCACert(rootCACertificate);
}

void reconnect() 
{
  while(!client.connected()) 
  {
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
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void loop() 
{
  if(!client.connected())
    reconnect();

  client.loop();

  sprintf(utilstr, "Hello from ESP32!!!%d", pub_ct++);
  client.publish("/test", utilstr); 

  // measure m = { .type=Temp, .val=132 , .timestamp=456, };

  // char buff[100];
  // memset(buff, 0, 100);
  // Serializer s;

  // StaticJsonDocument<256> doc;
  // doc["foo"] = "bar";
  // JsonArray test = doc.createNestedArray("test");
  // for(int i = 0; i < 10; i++)
  //   test.add("1");

  // serializeJson(doc, buff);

  // Serial.printf("%s\n", buff);
}
