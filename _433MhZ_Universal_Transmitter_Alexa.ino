/****** 02.07.2017 attempting to add JSON binding with Domoticz ************/
/*** 11.07.2017 - domoticz binding now works, however, all other callback functions stopped working ****/
/**** 16.07.2017 - trying to enable Alexa control for all lights ******/


/**** inlude all necessary libraries*/

#include <RCSwitch.h> 
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include "WemoSwitch.h"
#include "WemoManager.h"
#include "CallbackFunction.h"

// prototypes
boolean connectWifi();

//on/off callbacks
void lowLightOn();
void lowLightOff();
void mainLightOn();
void mainLightOff();
void kitchenLightOn();
void kitchenLightOff();
void ambientLightOn();
void ambientLightOff();
void kidsLightOn();
void kidsLightOff();

WemoManager wemoManager;
WemoSwitch *lowLight = NULL;
WemoSwitch *mainLight = NULL;
WemoSwitch *kitchenLight = NULL;
WemoSwitch *ambientLight = NULL;
WemoSwitch *kidsLight = NULL;

const int ledPin = 12;


RCSwitch mySwitch = RCSwitch();

long last_mls = millis();     //time function in miliseconds

/*Wi-Fi setting*/
const char* ssid = "BTHub5-PJ58";
const char* password = "YuriMeiran465859";
const char* mqtt_server = "192.168.1.75"; //broker address

/* MQTT Settings */

const char* kids_topic = "LIGHT/kids_room";

const char* domo_topic = "domoticz/out";
String domoticz = String(domo_topic);

const char* domo_in = "domoticz/in";

const char* Topic = "RF_Socket/Socket1"; //RF sockets topic
String topicSockets = String(Topic);

const char* topic_ks = "RF_Socket/kitchen"; //RF sockets topic
String topicKS = String(topic_ks);

const char* topic_ls = "RF_Socket/living"; //RF sockets topic
String topicLS = String(topic_ls);

const char* Light_topic = "LIGHT/Kitchen_main";
String topicKitchen = String(Light_topic);

const char* Ambient_topic = "LIGHT/Ambient_led";
String topicAmbient = String(Ambient_topic);

const char* Light1_topic = "LIGHT/Living";
String topicLiving = String(Light1_topic);

const char* Light2_topic = "LIGHT/Living1";
String topicLiving1 = String(Light2_topic);

char* hellotopic = "test_topic"; 
String CLIENT_ID;       // Client ID to send to the broker
  

WiFiClient espClient;                        //initialize WiFi client
PubSubClient client(espClient);              //initialize MQTT client

//char data[100];  //used if data needs to be resent
//StaticJsonBuffer<2000> jsonBuffer;

void setup()

{

Serial.begin(115200); //set the port speed to 9600 bit/s

  //Set WiFi to station mode and disconnect from an AP if it was Previously
  // connected
 WiFi.mode(WIFI_STA);
 WiFi.disconnect();
/********************************************* setup data to be sent to alexa ************************/
  wemoManager.begin();
  // Format: Alexa invocation name, local port no, on callback, off callback
  lowLight = new WemoSwitch("low light", 80, lowLightOn, lowLightOff);
  mainLight = new WemoSwitch("main light", 81, mainLightOn, mainLightOff);
  ambientLight = new WemoSwitch("ambient light", 82, ambientLightOn, ambientLightOff);
  kitchenLight = new WemoSwitch("kitchen light", 83, kitchenLightOn, kitchenLightOff);
  kidsLight = new WemoSwitch("kids light", 84, kidsLightOn, kidsLightOff);
  wemoManager.addDevice(*lowLight);
  wemoManager.addDevice(*mainLight);
  wemoManager.addDevice(*kitchenLight);
  wemoManager.addDevice(*ambientLight);
  wemoManager.addDevice(*kidsLight);

  pinMode(ledPin, OUTPUT); // initialize digital ledPin as an output.
  delay(10);
  digitalWrite(ledPin, LOW); // Wemos BUILTIN_LED is active Low, so high is off

/******************************************************************************************************/

 CLIENT_ID += "ESP_RF_CONTROL";
 // Transmitter is connected to pin 5 in this setup, however it can be any available GPIO pin
  mySwitch.enableTransmit(5);

  client.setServer(mqtt_server, 1883);       //connecting to MQTT
  client.setCallback(callback);              //function to receive messages from broker
  delay(100);                               

 Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  Serial.print("Connecting to ");
  Serial.print(mqtt_server);
  Serial.print(" as ");
  Serial.println(CLIENT_ID);

  if (client.connect((char*) CLIENT_ID.c_str())) {
    Serial.println("Connected to MQTT broker");
    Serial.print("Topic is: ");
    Serial.println(domo_topic);
    
    if (client.publish(hellotopic, "test message")) {
      Serial.println("Publish ok");
    }

 
    else {
      Serial.println("Failed to publish");
    }
  }
  else {
    Serial.println("MQTT server connection failed");
    Serial.println("Will reset and try again...");
    abort();
  }

  client.connect((char*) CLIENT_ID.c_str());
 //subscribe to topics you want receive messages from
  /*client.subscribe(Topic);
  client.subscribe(topic_ks);
  client.subscribe(topic_ls);
  client.subscribe(Light_topic);
  client.subscribe(Light1_topic);
  client.subscribe(Light2_topic);
  client.subscribe(Ambient_topic);*/
  client.subscribe(domo_topic);
  //client.subscribe(domo_in);

}

void loop()
{

  if (client.connected())
  {
    client.loop();
  }
  
  delay(200);
  
  if (millis() - last_mls > 300000)     //check for connection every 5 mins
  {
    last_mls = millis();
    reconnect();
  }
  
  wemoManager.serverLoop();
}


void callback(char* topic, byte* payload, unsigned int length) 
{
  char data[256];
  const size_t bufferSize = JSON_OBJECT_SIZE(12) + 160;
  DynamicJsonBuffer jsonBuffer(bufferSize);
  
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print(" ] ");
  String string = String(topic);

  //char message[length + 1];
  for (int i = 0; i < length; i++) {
    //Serial.print((char)payload[i]);

    data[i] = (char)payload[i];
  }
 data[length] = '\0';
 Serial.println(data);


/*************************** handle json message from domoticz *****************************************/

JsonObject& root = jsonBuffer.parseObject(data); 

if (!root.success()) {
    Serial.println("parseObject() failed");             // if error - print out                                         
    return;                                             
  }

int nvalue = root["nvalue"];
int svalue = root["svalue1"];
int idx = root["idx"];

/*if (idx == 20){
if(nvalue == 1){

  client.publish(Light_topic,"light is on"); 
  
  }
else if (nvalue == 0)
{
client.publish(Light_topic,"light is off");  

}
}
*/
/*******************************************************************************************************/
//when message received, execute matching fuction
//04.05.2017 - now added if statement to identify payload by topic

/******************************* SOCKET #1 *************************************************************/
if (string.equals(topicSockets)){
  if ((char)payload[0] == '1') {
    
    mySwitch.send(9797711, 24);  //Radio Frequency 433mhz socket #1 code 9797711, to switch on
    }
  else if ((char)payload[0] == '2') {
    
    mySwitch.send(9797710, 24); //Radio Frequency 433mhz socket #1 code 9797710, to switch off
    }
/*******************************************************************************************************/
 
/******************************* SOCKET #2 *************************************************************/

 else if ((char)payload[0] == '3') {
    
    mySwitch.send(9797703, 24);  //Radio Frequency 433mhz socket #2 code 9797703, to switch on
    }
  else if ((char)payload[0] == '4') {
    
    mySwitch.send(9797702, 24); //Radio Frequency 433mhz socket #2 code 9797702, to switch off
    }
/*******************************************************************************************************/ 

/******************************* SOCKET #3 *************************************************************/

 else if ((char)payload[0] == '5') {
    
    mySwitch.send(9797707, 24);  //Radio Frequency 433mhz socket #3 code 9797707, to switch on
    }
  else if ((char)payload[0] == '6') {
    
    mySwitch.send(9797706, 24); //Radio Frequency 433mhz socket #3 code 9797706, to switch off
    }
/*******************************************************************************************************/ 

/******************************* SOCKET #4 *************************************************************/

 else if ((char)payload[0] == '7') {
    
    mySwitch.send(9797699, 24);  //Radio Frequency 433mhz socket #4 code 9797699, to switch on
    }
  else if ((char)payload[0] == '8') {
    
    mySwitch.send(9797698, 24); //Radio Frequency 433mhz socket #4 code 9797698, to switch off
    }
/*******************************************************************************************************/ 

/******************************* SOCKET-1 MASTER BEDROOM *************************************************/

 else if ((char)payload[0] == 'K') {
    
    mySwitch.send(10745055, 24);  
    }
  else if ((char)payload[0] == 'L') 
  {
      mySwitch.send(10745054, 24); 
    }
/*******************************************************************************************************/ 

/******************************* ALL SOCKETS ON OR OFF *************************************************/

 else if ((char)payload[0] == '9') {
    
    mySwitch.send(9797709, 24);  //Radio Frequency 433mhz all sockets code 9797709, to switch on
    }
  else if ((char)payload[0] == 'A') //"A" has no meaning
  {
      mySwitch.send(9797708, 24); //Radio Frequency 433mhz all sockets code 9797708, to switch off
    }
}
/*******************************************************************************************************/ 

/******************************* KITCHEN LIGHTS *************************************************/

/*else if (string.equals(topicKitchen)){
   if ((char)payload[0] == '1') {                              //B
    
    mySwitch.send(14426536, 24);  // light on
   
    }
 else if ((char)payload[0] == '0')                            //M
  {
      mySwitch.send(14426532, 24);  //light off
  }
}*/

else if (idx == 20){
if(nvalue == 1){

  mySwitch.send(14426536, 24);  // light on
  client.publish(Light_topic, "1"); //send message back to 
}

else if(nvalue == 0){

  mySwitch.send(14426532, 24);  //light off
  client.publish(Light_topic, "0");
}

}

//else if(idx == 11){
//
//  if (svalue == 10){
//  
//  mySwitch.send(14426536, 24);  // light on
//  client.publish(domo_topic, "test");
//  }
//  else {
//    
//  mySwitch.send(14426532, 24);  //light off
//  client.publish(domo_topic, "test off");
//    }
//}
/*******************************************************************************************************/ 

/******************************* AMBIENT LIGHT LED *************************************************/

else if (string.equals(topicAmbient)){
 if ((char)payload[0] == '1') {
    
    mySwitch.send(10745053, 24); 
    client.publish(Ambient_topic, "1"); 
    }
  else if ((char)payload[0] == '0') 
  {
      mySwitch.send(10745052, 24); 
      client.publish(Ambient_topic, "0"); 
    }
}
/*******************************************************************************************************/ 


/******************************* LIVING MAIN LIGHT *************************************************/

// else if (string.equals(topicLiving)) { 
//  if ((char)payload[0] == '1') 
//    {  
//    mySwitch.send(10745047, 24);  
//    }
//  else if ((char)payload[0] == '0') 
//    {
//      mySwitch.send(10745046, 24); 
//    }
//
// }

else if (idx == 48){
if(nvalue == 1){

   mySwitch.send(10745047, 24);  // light on
   client.publish(Light1_topic, "1");
}

else if(nvalue == 0){

  mySwitch.send(10745046, 24); //light off
  client.publish(Light1_topic, "0");
}

}

/*******************************************************************************************************/ 

/******************************* LIVING LOW LIGHT *************************************************/

// else if (string.equals(topicLiving1)) {
//  if ((char)payload[0] == '1') 
//    {  
//    mySwitch.send(10745051, 24);  
//    }
//  else if ((char)payload[0] == '0') 
//    {
//      mySwitch.send(10745050, 24); 
//    }
//
// }

 else if (idx == 53){
if(nvalue == 1){

   mySwitch.send(10745051, 24);  // light on
   client.publish(Light2_topic, "1");
}

else if(nvalue == 0){

  mySwitch.send(10745050, 24); //light off
  client.publish(Light2_topic, "0");
}

}

/*******************************************************************************************************/ 


/******************************* kitchen sockets ************ socket 1 *********************************/

else if (string.equals(topicKS)) { 
  if ((char)payload[0] == '1') 
    {  
    mySwitch.send(7871119, 24);  
    }
  else if ((char)payload[0] == '0') 
    {
      mySwitch.send(7871118, 24); 
    }
/************************************************ socket 2 *********************************************/
 
  else if ((char)payload[0] == '2') 
    {  
    mySwitch.send(7871111, 24);  
    }
  else if ((char)payload[0] == '3') 
    {
      mySwitch.send(7871110, 24); 
    }

}
/*******************************************************************************************************/

/******************************* living room  socket 1 *************************************************/

else if (string.equals(topicLS)) { 
  if ((char)payload[0] == '1') 
    {  
    mySwitch.send(7871107, 24);  
    }
  else if ((char)payload[0] == '0') 
    {
      mySwitch.send(7871106, 24); 
    }
/************************************************ socket 2 *********************************************/
 
  else if ((char)payload[0] == '2') 
    {  
    mySwitch.send(1234567, 24);  
    }
  else if ((char)payload[0] == '3') 
    {
      mySwitch.send(1234567, 24); 
    }
/*******************************************************************************************************/

 else if ((char)payload[0] == '4') 
    {  
    mySwitch.send(1234567, 24);  
    }
  else if ((char)payload[0] == '5') 
    {
      mySwitch.send(1234567, 24); 
    }

}
/******************************************************************************************************/

/******************************************************************************************************/
else if (idx == 52){
if(nvalue == 1){

client.publish(kids_topic, "1");  
}

else if(nvalue == 0){

  client.publish(kids_topic, "0");
}

}

/******************************************************************************************************/
  }

void reconnect() {
  // Loop until we're reconnected
  
    if (WiFi.status() != WL_CONNECTED)         //if not connected to wi-fi network
  {
    WiFi.begin(ssid, password);
    Serial.println("");
    Serial.println("Connecting to WiFi...");      
  } else {
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
  }
  if (!client.connected() && WiFi.status() == WL_CONNECTED) { // if conected to wi-fi but not connected to broker
    client.connect("ESP03Client");          //client name
   //subscribe to all necessary topics
    client.subscribe(Topic);
    client.subscribe(Light_topic);
    client.subscribe(Light1_topic);
    client.subscribe(Ambient_topic);  
    client.subscribe(domo_topic);           
    Serial.println("Mosquitto connect..."); 
  }
  }

/********** living room low light *********/
void lowLightOn() {
      Serial.print("low light turn on ...");
      mySwitch.send(10745051, 24); 
      client.publish(Light2_topic, "1");
  }
  
void lowLightOff() {
      Serial.print("low light turn off ...");
      mySwitch.send(10745050, 24);
      client.publish(Light2_topic, "0");
  }
/******************************************/

/********** living room main light *********/
void mainLightOn() {
      Serial.print("main light turn on ...");
      mySwitch.send(10745047, 24); 
      client.publish(Light1_topic, "1");
  }
  
void mainLightOff() {
      Serial.print("main light turn off ...");
      mySwitch.send(10745046, 24);
      client.publish(Light1_topic, "0");
  }
/******************************************/

/********** kitchen light *********/
void kitchenLightOn() {
      Serial.print("kitchen turn on ...");
      mySwitch.send(14426536, 24); 
      client.publish(Light_topic, "1");
  }
  
void kitchenLightOff() {
      Serial.print("kitchen light turn off ...");
      mySwitch.send(14426532, 24);
      client.publish(Light_topic, "0");
  }
/******************************************/

/********** ambient light *********/
void ambientLightOn() {
      Serial.print("ambient light turn on ...");
      mySwitch.send(10745053, 24); 
      client.publish(Ambient_topic, "1");
  }
  
void ambientLightOff() {
      Serial.print("ambient light turn off ...");
      mySwitch.send(10745052, 24);
      client.publish(Ambient_topic, "0");
  }
/******************************************/

/********** kids light *********/
void kidsLightOn() {
      Serial.print("kids light turn on ...");
      client.publish(kids_topic, "1");
  }
  
void kidsLightOff() {
      Serial.print("kids light turn off ...");
      client.publish(kids_topic, "0");
  }
/******************************************/


