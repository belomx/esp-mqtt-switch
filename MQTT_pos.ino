#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include "SSD1306Wire.h"

/* Definitions ------------------------------------- */
/* MQTT topics */

#define TOPIC_LED_CTRL       "home/bedroom/switch1/set"
#define TOPIC_LED_STATE      "home/bedroom/switch1"
#define TOPIC_LED_AVAILABLE  "home/bedroom/switch1/available"
#define TOPIC_SENSOR         "PosInatel/sensor"
#define TOPIC_LCD            "PosInatel/lcd"

/* MQTT unique ID */
#define ID_MQTT  "InatelPos_IoT_15"
 
/* Pin map for ESP8266 */
#define GPIO0       16
#define GPIO1       5
#define GPIO2       4
#define GPIO3       0
#define GPIO4       2
#define GPIO5       14
#define LED_GREEN   12
#define LED_BLUE    16//13
#define LED_RED     15
#define GPIO9       3
#define GPIO10      1
#define LDR_SENSOR  A0

/* Variables --------------------------------------- */
/* Wi-Fi configuration */
const char* SSID = "virus";
const char* PASSWORD = "Plm741852$";
 
/* MQTT configuration */
const char* BROKER_MQTT = "192.168.1.101"; /* "iot.eclipse.org" */ /* Broker URL or IP */
int BROKER_PORT = 1883; /* MQTT broker port */

/* Wi-Fi and MQTT instantiation */
WiFiClient espClient; /* ESP8266 client */
PubSubClient MQTT(espClient); /* MQTT instance with Wi-Fi client */

/* LCD instantiation */
SSD1306Wire  display(0x3c, D1, D2);

/* Leds state */
char RedState = '0',GreenState = '0',BlueState = '0';
 
/* Prototypes */
void initSerial();
void initWiFi();
void initMQTT();
void reconectWiFi(); 
void mqtt_callback(char* topic, byte* payload, unsigned int length);
void verifyConnections(void);
void sendSensorState(int iLed, char cState);
char updateLedState(int iLed, char* pcState);
void initOutput(void);
void initLCD(void);

/* Methods ----------------------------------------- */
void setup() 
{
  initOutput();
  initSerial();
  initLCD();
  initWiFi();
  initMQTT();

  char online[7];
  snprintf (online, 7, "online");
  MQTT.publish(TOPIC_LED_AVAILABLE, online);
}
 
/**
  Serial init
 */
void initSerial() 
{
  Serial.begin(115200);
}

/**
  Wi-Fi init
 */
void initWiFi() 
{
  delay(10);
  Serial.print("Connecting: ");
  Serial.println(SSID);
  Serial.println("...");
  
  reconectWiFi();
}
 
/**
  MQTT init
 */
void initMQTT() 
{
  MQTT.setServer(BROKER_MQTT, BROKER_PORT); 
  MQTT.setCallback(mqtt_callback);
}
 
/**
  MQTT callback
 */
void mqtt_callback(char* topic, byte* payload, unsigned int length) 
{
  char msg[10];
  
  /* Reads received packet */
  for(int i = 0; i < length; i++) 
  {
    msg [i] = (char)payload[i];
  }
  msg [length] = 0;

  String displayMsg = String ("");

  Serial.println(topic);

  if(!strncmp(topic,TOPIC_LED_CTRL,sizeof(TOPIC_LED_CTRL)))
  {
    if(!strncmp(msg,"ON",2))
    {
      char state = updateLedState(LED_RED, '1');
      displayMsg = String ("Switch1 received ON");
      sendSensorState(LED_RED, state);
    }
    else if(!strncmp(msg,"OFF",3))
    {
      char state = updateLedState(LED_RED, '0');
      displayMsg = String ("Switch1 received OFF");
      sendSensorState(LED_RED, state);
    }

    if (displayMsg != "")
    {
      display.clear();
      display.drawString(70, 5, displayMsg);
      display.display();
      Serial.println(msg);
    }    
  }
}
 
/**
  MQTT reconnect
 */
void reconnectMQTT() 
{
  if (MQTT.connected())
    return;

  while (!MQTT.connected()) 
  {
    Serial.print("* Connecting to Broker: ");
    Serial.println(BROKER_MQTT);
    if (MQTT.connect(ID_MQTT)) 
    {
      Serial.println("Connected!");
      MQTT.subscribe(TOPIC_LED_CTRL);
      MQTT.subscribe(TOPIC_LCD);
    } 
    else 
    {
      Serial.println("Connection failed");
      Serial.println("Retrying in 2s...");
      delay(2000);
    }
  }
}
 
/**
  Wi-Fi reconnect
 */
void reconectWiFi() 
{
  if (WiFi.status() == WL_CONNECTED)
    return;
  
  WiFi.begin(SSID, PASSWORD);
  
  while (WiFi.status() != WL_CONNECTED) 
  {
    delay(100);
    Serial.print(".");
  }
  
  Serial.println();
  Serial.print("Connected:");
  Serial.print(SSID);
  Serial.println("IP: ");
  Serial.println(WiFi.localIP());
}
 
/**
  Connection verify
 */
void verifyConnections(void)
{
  reconnectMQTT();
  
  reconectWiFi();
}

/**
  Update led state
 */
char updateLedState(int iLed, char cState)
{
  if(cState == '1')
  {
    digitalWrite(iLed, HIGH);
    return '1';
  }
  else if(cState == '0')
  {
    digitalWrite(iLed, LOW);
    return '0';
  }
}

/**
  MQTT send led state
 */
void sendSensorState(int iLed, char cState)
{
  char stringPub[4];
  if(cState == '1')
    snprintf (stringPub, 4, "ON");
  else if (cState == '0')
    snprintf (stringPub, 4, "OFF");
    
  MQTT.publish(TOPIC_LED_STATE, stringPub);
}
 
/**
  MQTT send sensor state
 */
void sendSensorState(void)
{
  char stringPub[10];
  int sensorValue = analogRead(LDR_SENSOR);
  snprintf (stringPub, 10, "%d", sensorValue);
  MQTT.publish(TOPIC_SENSOR, stringPub);
  
  delay(1000);
}
 
/**
  Init outputs
 */
void initOutput(void)
{
  pinMode(LED_RED, OUTPUT);
  digitalWrite(LED_RED, LOW);
  pinMode(LED_GREEN, OUTPUT);
  digitalWrite(LED_GREEN, LOW);
  pinMode(LED_BLUE, OUTPUT);
  digitalWrite(LED_BLUE, LOW);
}

/**
  Draw progress bar on LCD
 */
void ProgressBar()
{
  for (int counter = 0; counter <= 100; counter++)
  {
    display.clear();
    // Draws progress bar 
    display.drawProgressBar(0, 32, 120, 10, counter);
    // Updates the complete percentage
    display.setTextAlignment(TEXT_ALIGN_CENTER);
    display.drawString(64, 15, String(counter) + "%");
    display.display();
    delay(10);
  }
}


/**
  Init LCD
 */
void initLCD(void)
{
  display.init();
  display.flipScreenVertically();

  ProgressBar();
  delay(3000);
}
 
/**
  Main loop
 */
void loop() 
{
  verifyConnections();
  
  sendSensorState();

  MQTT.loop();
}
