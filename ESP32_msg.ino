// ESP32_msg.ino
// dew.ninja  Mar 2023
// communication between 2 ESP32's  
// using @msg/ topic
// this one is named device1

#include <WiFi.h>
#include <PubSubClient.h>

#define LOOP_PERIOD 2000  // millisecond
#define BUTTON 0  // on-board button

const char* ssid = "";
const char* password = "";
const char* mqtt_server = "broker.netpie.io";
const int mqtt_port = 1883;
const char* mqtt_Client = "";
const char* mqtt_username = "";
const char* mqtt_password = "";

WiFiClient espClient;
PubSubClient client(espClient);

char msg[100];
unsigned long last_millis=0, new_millis=0;
bool online = 1;

// -------- global variables for command interpreter ----------
String rcvdstring;   // string received from serial
String ledstring;   // string contains LED status
String periodstring; // string contrins period
int my_led_status=0, your_led_status = 0;  // 0 = off, 1 = on
int my_blink_period=100, your_blink_period=100; // LED blink period (in millisecs)

bool newcmd = 0;     // flag when new command received
int sepIndex;       // index of seperator
int button_status = 1;
bool publish_request = false; 
unsigned long button_time = 0, last_button_time = 0;  // variables for switch debounce

String datastr;       // data string used in command section
void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection…");
    if (client.connect(mqtt_Client, mqtt_username, mqtt_password)) {
      Serial.println("connected");
      // subscribe to command topics
      client.subscribe("@msg/#");      
    }
    else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println("try again in 5 seconds");
      delay(5000);
    }
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  String message;
  for (int i = 0; i < length; i++) {
    message = message + (char)payload[i];
  }
  Serial.println(message);
  if(String(topic) == "@msg/device1") {
    rcvdstring=message; 
    cmdInt();  // call command interpreter
  }
}

// external interrrupt routnine
void IRAM_ATTR publish_msg()   {

  button_time = millis();
  if (button_time - last_button_time > 250)   {  // switch debounce
    last_button_time = button_time;
    publish_request = true;
  }
}

void setup() {
  pinMode(BUTTON, INPUT_PULLUP);
  pinMode(LED_BUILTIN,OUTPUT);
  Serial.begin(115200);
  Serial.println();
  if (online)  {
    Serial.print("Connecting to ");
    Serial.println(ssid);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
    client.setServer(mqtt_server, mqtt_port);
    client.setCallback(callback);
  }
  attachInterrupt(BUTTON, publish_msg, FALLING);
   
}

void loop() {
  // blink LED if led_status = 1
  if (my_led_status)  {
    digitalWrite(LED_BUILTIN,!digitalRead(LED_BUILTIN));
    delay(my_blink_period);
  }
  else digitalWrite(LED_BUILTIN,0);  // turn off LED
  if (online)  {  // maintain NETPIE connection
    new_millis = millis();
    if(new_millis - last_millis > LOOP_PERIOD) {
        last_millis = new_millis;
        if (!client.connected()) {
          reconnect();
        }
        client.loop();
        if (publish_request)   {
          your_led_status = !your_led_status;
          if (your_led_status) your_blink_period = (int) 100*random(1,10);  // choose period randomlyh
          datastr = String(your_led_status)+","+String(your_blink_period);
          datastr.toCharArray(msg, (datastr.length() + 1));
          Serial.print("Publishing --> ");
          Serial.println(msg);          
          client.publish("@msg/device2", msg);
          publish_request = false;
        }
    }
  }
  // ---- this code block is for initial test via serial port ----
  // while (Serial.available() > 0)   {  // detect new input
  //    rcvdstring = Serial.readString();
  //    newcmd = 1;
  // }
  // if (newcmd)   { // execute this part only when new command received
  //   cmdInt();   // invoke the interpreter
  //   newcmd = 0;
  // } 
  // ------------------------------------------------------------   
  if(!my_led_status) delay(100);  // add some delay if not blinking

}

void cmdInt(void)
{
    Serial.println("Received -->"+rcvdstring);    
    rcvdstring.trim();  // remove leading&trailing whitespace, if any
    // find index of separator (',')
    sepIndex = rcvdstring.indexOf(',');
    ledstring = rcvdstring.substring(0, sepIndex);
    ledstring.trim();
    periodstring =  rcvdstring.substring(sepIndex+1);   
    periodstring.trim();
    my_led_status = ledstring.toInt();
    my_blink_period = periodstring.toInt();
    Serial.println(my_led_status);
    Serial.println(my_blink_period);

 }
