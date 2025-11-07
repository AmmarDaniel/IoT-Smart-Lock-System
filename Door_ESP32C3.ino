//ESP8266 Chip id_prototype =  75550e
//Door ID_Coexys: e101c2
////Door ID_Prototype: e185f8
// ID extra :e0b268
// Prototype me: 600163b0e4




/*Dev Log
1. Use HiveMq as MQTT broker
2. Simple IoT system to open/off solenoid door
3. WifiManager lib for wifi webclient config
4. 4/2/2023: Add delay for relay input HIGH (Door open), buzz alarm/send warning message to HiveMQ if relay input HIGH (door is still open) after delay 
5. 10/2/2023: Add buzzer, manual switch + 5 seconds delay, secure connection
6. Remove active buzzer, replace with passive and directly connect to relay = buzz on each time door open + off when door close
7. Add support for magnetic contact switch = indicate open/close door
8. Get nodeMCU UID to assign as door UID
9. Fix door logic = door always close and auto close after 10 seconds, add 2 UID to distinguished 2 doors ID-DCS= e101c2, , ID-DLIS=4a6c4d , 3 prototype: 8984b0
*/

#if defined(ESP32)
#include <WiFi.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#endif
#include <WiFiManager.h>
#include <PubSubClient.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"


//MQTT Broker -for security make seperate file
//const char* mqtt_server = "0270aa60965f44f3b52ea4a75ff28d99.s2.eu.hivemq.cloud";  // HiveMQ broker URL
const char* mqtt_server = "broker.hivemq.com";  // HiveMQ broker URL
const int mqtt_port = 1883;

//const char* mqtt_username = "smartdoor";
//const char* mqtt_password = "password1234";

const char* mqtt_username = "";
const char* mqtt_password = "";
const char* doorAmmar1_status = "doorAmmar1";
const char* doorAmmar2_status = "doorAmmar2";
const char* doorstat_topic = "doorstatus";  //old" magstatus
const char* door_uidAmmar = "dooruidAmmar";

const int relay1 = 6;
const int relay2 = 7;
const int buzzer = 10;
const int swtch = 2;
const int rstSwitch = 19;
const int deskSW = 4;
int manualSWCurrent, manualSWLast, deskSWCurrent, deskSWLast, rstStateCurrent, rstStateLast;
String doorUID;

/*For secure MQTT connection: Root Cert*/
static const char* root_ca PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
MIIFazCCA1OgAwIBAgIRAIIQz7DSQONZRGPgu2OCiwAwDQYJKoZIhvcNAQELBQAw
TzELMAkGA1UEBhMCVVMxKTAnBgNVBAoTIEludGVybmV0IFNlY3VyaXR5IFJlc2Vh
cmNoIEdyb3VwMRUwEwYDVQQDEwxJU1JHIFJvb3QgWDEwHhcNMTUwNjA0MTEwNDM4
WhcNMzUwNjA0MTEwNDM4WjBPMQswCQYDVQQGEwJVUzEpMCcGA1UEChMgSW50ZXJu
ZXQgU2VjdXJpdHkgUmVzZWFyY2ggR3JvdXAxFTATBgNVBAMTDElTUkcgUm9vdCBY
MTCCAiIwDQYJKoZIhvcNAQEBBQADggIPADCCAgoCggIBAK3oJHP0FDfzm54rVygc
h77ct984kIxuPOZXoHj3dcKi/vVqbvYATyjb3miGbESTtrFj/RQSa78f0uoxmyF+
0TM8ukj13Xnfs7j/EvEhmkvBioZxaUpmZmyPfjxwv60pIgbz5MDmgK7iS4+3mX6U
A5/TR5d8mUgjU+g4rk8Kb4Mu0UlXjIB0ttov0DiNewNwIRt18jA8+o+u3dpjq+sW
T8KOEUt+zwvo/7V3LvSye0rgTBIlDHCNAymg4VMk7BPZ7hm/ELNKjD+Jo2FR3qyH
B5T0Y3HsLuJvW5iB4YlcNHlsdu87kGJ55tukmi8mxdAQ4Q7e2RCOFvu396j3x+UC
B5iPNgiV5+I3lg02dZ77DnKxHZu8A/lJBdiB3QW0KtZB6awBdpUKD9jf1b0SHzUv
KBds0pjBqAlkd25HN7rOrFleaJ1/ctaJxQZBKT5ZPt0m9STJEadao0xAH0ahmbWn
OlFuhjuefXKnEgV4We0+UXgVCwOPjdAvBbI+e0ocS3MFEvzG6uBQE3xDk3SzynTn
jh8BCNAw1FtxNrQHusEwMFxIt4I7mKZ9YIqioymCzLq9gwQbooMDQaHWBfEbwrbw
qHyGO0aoSCqI3Haadr8faqU9GY/rOPNk3sgrDQoo//fb4hVC1CLQJ13hef4Y53CI
rU7m2Ys6xt0nUW7/vGT1M0NPAgMBAAGjQjBAMA4GA1UdDwEB/wQEAwIBBjAPBgNV
HRMBAf8EBTADAQH/MB0GA1UdDgQWBBR5tFnme7bl5AFzgAiIyBpY9umbbjANBgkq
hkiG9w0BAQsFAAOCAgEAVR9YqbyyqFDQDLHYGmkgJykIrGF1XIpu+ILlaS/V9lZL
ubhzEFnTIZd+50xx+7LSYK05qAvqFyFWhfFQDlnrzuBZ6brJFe+GnY+EgPbk6ZGQ
3BebYhtF8GaV0nxvwuo77x/Py9auJ/GpsMiu/X1+mvoiBOv/2X/qkSsisRcOj/KK
NFtY2PwByVS5uCbMiogziUwthDyC3+6WVwW6LLv3xLfHTjuCvjHIInNzktHCgKQ5
ORAzI4JMPJ+GslWYHb4phowim57iaztXOoJwTdwJx4nLCgdNbOhdjsnvzqvHu7Ur
TkXWStAmzOVyyghqpZXjFaH3pO3JLF+l+/+sKAIuvtd7u+Nxe5AW0wdeRlN8NwdC
jNPElpzVmbUq4JUagEiuTDkHzsxHpFKVK7q4+63SM1N95R1NbdWhscdCb+ZAJzVc
oyi3B43njTOQ5yOf+1CceWxG1bQVs5ZufpsMljq4Ui0/1lvh+wjChP4kqKOJ2qxq
4RgqsahDYVvTH9w7jXbyLeiNdd8XM2w9U/t7y0Ff/9yi0GE44Za4rF2LN9d11TPA
mRGunUHBcnWEvgJBQl9nJEiU0Zsnvgc/ubhPgXRR4Xq37Z0j4r7g1SgEEzwxA57d
emyPxgcYxn/eR44/KJ4EBs+lVDR3veyJm+kXQ99b21/+jh5Xos1AnX5iItreGCc=
-----END CERTIFICATE-----
)EOF";

//WiFiClientSecure espClient;
WiFiClient espClient;
PubSubClient client(espClient);

/****Functions****/
// Wifi Setup
void setupWifi() {
  //Init WifiManager
  WiFiManager wifiManager;
  Serial.print("\n\nConnecting Wifi: ");
  //wifiManager.resetSettings();
  wifiManager.autoConnect("Smart Door: WiFI Setup"); //nama wifi esp
  Serial.print("Wifi Status: Connected");
}

//HiveMQ connection setup
void reconnect() 
{
  //Connect HiveMQ server, loop until connected
  while (!client.connected()) 
  {
    String clientId = "ESP8266Client-";  // Create a random client ID - Change to unique ID
    clientId += doorUID;
    Serial.println("\nHiveMQ:" + clientId);
    // Attempt to connect
    //if (client.connect(clientId.c_str(), mqtt_username, mqtt_password)) 
    if (client.connect(clientId.c_str())) 
    {
      Serial.println("\nHiveMQ: Connected");
      client.subscribe(doorAmmar1_status);  // subscribe the topics here
      client.subscribe(doorAmmar2_status);
      client.subscribe(doorstat_topic);
      client.subscribe(door_uidAmmar);

      publishMessage(doorstat_topic, String("Door Initialized"), true);

      if (doorUID == "600163b0e4") 
      {
        publishMessage(door_uidAmmar, String("Smart Door ONLINE"), true);  //push nodeMCU chip UID
      } 
      else 
      {
        publishMessage(door_uidAmmar, String("Smart Door OFFLINE"), true);
      }

    } 
    else 
    {
      Serial.print("Connection failed: ");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

//MQTT callback
void callback(char* topic, byte* payload, unsigned int length) 
{
  String incommingMessage = "";
  for (int i = 0; i < length; i++) incommingMessage += (char)payload[i];
  Serial.println("HiveMQ Incomming Message[" + String(topic) + "]: " + incommingMessage);

  if (doorUID == "600163b0e4") 
  {
    //Check Door 1 Status
    if (strcmp(topic, doorAmmar1_status) == 0) 
    { //Auto lock door after 10 seconds
      if (incommingMessage.equals("1")) 
      { //relay HIGH = OPEN, LOW = CLOSE
        publishMessage(doorstat_topic, String("Door Unlocked"), true);
        digitalWrite(6, LOW);
        digitalWrite(buzzer, HIGH);
        delay(2000);
        digitalWrite(buzzer, LOW);
        //delay(7000);
        publishMessage(doorAmmar1_status, String("0"), true);
      } 
      else 
      {
        digitalWrite(6, HIGH);  //close door
        publishMessage(doorstat_topic, String("Door Locked"), true);
      }
    }
    
    //Check Door 2 Status
    if (strcmp(topic, doorAmmar2_status) == 0) 
    {
      if (incommingMessage.equals("1")) 
      {
        publishMessage(doorstat_topic, String("Door Left OPEN"), true);
        digitalWrite(7, LOW);
        digitalWrite(buzzer, HIGH);
        delay(2000);
        digitalWrite(buzzer, LOW);
        //delay(7000);
        publishMessage(doorAmmar2_status, String("0"), true);
      } 
      else 
      {
        digitalWrite(7, HIGH);  //close door
        publishMessage(doorstat_topic, String("Door Left CLOSED"), true);
      }
    }
  }
}

//MQTT publising as string
void publishMessage(const char* topic, String payload, boolean retained) 
{
  if (client.publish(topic, payload.c_str(), true))
    Serial.println("Message publised [" + String(topic) + "]: " + payload);
}

void onManualSwitch()
{
 //Open door
    //digitalWrite(relay1, HIGH);
    //digitalWrite(relay2, HIGH);
    digitalWrite(buzzer, HIGH);
    Serial.println("Manual Switch ON: Door Open");
    publishMessage(doorstat_topic, String("Switch ON: Door Open"), true);
    delay(2000);
    digitalWrite(buzzer, LOW);

    //Close door after 10 seconds
    delay(10000);
    digitalWrite(relay1, LOW);
    digitalWrite(relay2, LOW);
    Serial.println("Manual Switch OFF: Door Close");
    publishMessage(doorstat_topic, String("Switch OFF: Door Close"), true);
}

// Fingerprint and RFID
void onPin4High() {
    publishMessage(doorstat_topic, String("Door Unlocked"), true);
    digitalWrite(6, LOW); // Door open (relay LOW to activate)
    digitalWrite(buzzer, HIGH); // Turn on buzzer
    delay(2000); // Wait for 2 seconds
    digitalWrite(buzzer, LOW); // Turn off buzzer
    // Add a delay if needed, commented here for simplicity
    // delay(7000); 
    publishMessage(doorAmmar1_status, String("0"), true); // Reset door status
    Serial.println("Pin 4 HIGH: Door Open process completed.");

    // Temporarily switch pin 4 to output to set it LOW
    pinMode(4, OUTPUT); 
    digitalWrite(4, LOW);
    delay(10); // Small delay to ensure state is applied

    // Switch back to input mode
    pinMode(4, INPUT);
}


/*****Setup && Loop*****/
void setup() 
{
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); //disable   detector
  Serial.begin(9600);
  while (!Serial) delay(1);
  doorUID = String(ESP.getEfuseMac(), HEX);
  //Serial.printf("\n ESP32-C3 Chip id = %08X\n", doorUID);  //get nodeMCU UID
  Serial.print("ESP32-C3 Chip id = ");
  Serial.println(doorUID);
  pinMode(4, INPUT);
  pinMode(6, OUTPUT);
  pinMode(10, OUTPUT);
  pinMode(7, OUTPUT);
  pinMode(5, OUTPUT);
  pinMode(buzzer, OUTPUT);
  digitalWrite(buzzer, LOW);
  pinMode(swtch, INPUT_PULLUP);
  pinMode(rstSwitch, INPUT_PULLUP);
  pinMode(deskSW, INPUT_PULLUP);
  setupWifi();
  //WiFi Client Init
/*
#ifdef ESP8266
  espClient.setInsecure();
#else
  espClient.setCACert(root_ca);
#endif
*/
  //MQTT Init
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
}

void loop() {
  digitalWrite(6, HIGH);
  digitalWrite(7, HIGH);

  /*
  //init reset button to restart nodemcu
  rstStateLast = rstStateCurrent;
  rstStateCurrent = digitalRead(rstSwitch);
  if (rstStateLast == HIGH && rstStateCurrent == LOW) {
    Serial.printf("Restart ESP32-C3..");
    ESP.restart();
  }
  
  //Front Desk Switch: Open door for 10 seconds.
  deskSWLast = deskSWCurrent;
  deskSWCurrent = digitalRead(deskSW);
  if (deskSWLast == HIGH && deskSWCurrent == LOW) {
   onManualSwitch();
  }
  */
  // Check if pin 4 is HIGH
  if (digitalRead(4) == HIGH) {
    onPin4High();
  }

  //Manual Switch: Open door for 10 seconds.
  manualSWLast = manualSWCurrent;
  manualSWCurrent = digitalRead(swtch);
  if (manualSWLast == HIGH && manualSWCurrent == LOW) {
    onManualSwitch();
  }
  //Client listener
  if (!client.connected()) reconnect();
  client.loop();
}
