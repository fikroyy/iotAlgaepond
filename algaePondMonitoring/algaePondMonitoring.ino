#ifdef ESP32
#include <WiFi.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#else
#error Platform not supported
#endif
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <DNSServer.h>
//#include <WiFiManager.h>
#include <EEPROM.h>
#include <FS.h>
//#include <SD.h>
#include <SPI.h>
#include <LiquidCrystal_I2C.h>

// Define port sensor dan relay
#define BUZZ 13   //active high
#define BUTT 12   //active low

// DHT SENSOR
#define DHTPIN 14
#define DHTTYPE DHT22

// RFID SENSOR
#define SS_PIN    21
#define RST_PIN   22

// IR OBS SENSOR
#define IRPIN 27

// RELAY
#define RELAY 26

// Serial UART for GSM
#define RXD 16
#define TXD 17

LiquidCrystal_I2C  lcd(0x27,4,20);

#include "secrets.h" // ada cert
char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
const long utcOffsetInSeconds = 25200; //WIB +07.00
const int MQTT_PORT = 8883;
// AWS IoT Core Topic
const char MQTT_SUB_TOPIC[] = "algaepond/monitoring";//"$aws/things/" THINGNAME "/shadow/update";
const char MQTT_PUB_TOPIC[] = "algaepond/monitoring";//"$aws/things/" THINGNAME "/shadow/update";

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);

#ifdef USE_SUMMER_TIME_DST
uint8_t DST = 1;
#else
uint8_t DST = 0;
#endif

WiFiClientSecure net;

#ifdef ESP8266

   //timeClient.begin();

BearSSL::X509List cert(cacert);
BearSSL::X509List client_crt(client_cert);
BearSSL::PrivateKey key(privkey);
#endif

PubSubClient client(net);

unsigned long lastMillis = 0;
time_t now;
time_t nowish = 1510592825;

void NTPConnect(void)
{
  Serial.print("Setting time using SNTP");
  configTime(TIME_ZONE * 3600, DST * 3600, "pool.ntp.org", "time.nist.gov");
  now = time(nullptr);
  while (now < nowish)
  {
    delay(500);
    Serial.print(".");
    now = time(nullptr);
  }
  Serial.println("done!");
  struct tm timeinfo;
  gmtime_r(&now, &timeinfo);
  Serial.print("Current time: ");
  Serial.print(asctime(&timeinfo));
}

void pubSubErr(int8_t MQTTErr)
{
  if (MQTTErr == MQTT_CONNECTION_TIMEOUT)
    Serial.print("Connection tiemout");
  else if (MQTTErr == MQTT_CONNECTION_LOST)
    Serial.print("Connection lost");
  else if (MQTTErr == MQTT_CONNECT_FAILED)
    Serial.print("Connect failed");
  else if (MQTTErr == MQTT_DISCONNECTED)
    Serial.print("Disconnected");
  else if (MQTTErr == MQTT_CONNECTED)
    Serial.print("Connected");
  else if (MQTTErr == MQTT_CONNECT_BAD_PROTOCOL)
    Serial.print("Connect bad protocol");
  else if (MQTTErr == MQTT_CONNECT_BAD_CLIENT_ID)
    Serial.print("Connect bad Client-ID");
  else if (MQTTErr == MQTT_CONNECT_UNAVAILABLE)
    Serial.print("Connect unavailable");
  else if (MQTTErr == MQTT_CONNECT_BAD_CREDENTIALS)
    Serial.print("Connect bad credentials");
  else if (MQTTErr == MQTT_CONNECT_UNAUTHORIZED)
    Serial.print("Connect unauthorized");
}

void connectToMqtt(bool nonBlocking = false)
{
  Serial.print("MQTT Connecting ");
  while (!client.connected())
  {
    if (client.connect(THINGNAME))
    {
      Serial.println("Connected!");
      if (!client.subscribe(MQTT_SUB_TOPIC))
        pubSubErr(client.state());
    }
    else
    {
      Serial.print("failed, reason -> ");
      pubSubErr(client.state());
      if (!nonBlocking)
      {
        Serial.println(" < try again in 5 seconds");
        delay(5000);
      }
      else
      {
        Serial.println(" <");
      }
    }
    if (nonBlocking)
      break;
  }
}

void connectToWiFi(String init_str)
{
  if (init_str != emptyString)
    Serial.print(init_str);
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(1000);
  }
  if (init_str != emptyString)
    Serial.println("OK!");
}

void checkWiFiThenMQTT(void)
{
  connectToWiFi("Checking WiFi");
  connectToMqtt();
}

unsigned long previousMillis = 0;
const long interval = 5000;

void checkWiFiThenMQTTNonBlocking(void)
{
  connectToWiFi(emptyString);
  if (millis() - previousMillis >= interval && !client.connected()) {
    previousMillis = millis();
    connectToMqtt(true);
  }
}

void checkWiFiThenReboot(void)
{
  connectToWiFi("Checking WiFi");
  Serial.print("Rebooting");
  ESP.restart();
}

String inputText[2];

//mesi
String turbidity   = "0";
String timestamp   = "0";
String water_lv    = "0";

//ilham
String Ph          = "0";
String mpx         = "0";
String water_flow  = "0";

//faiq
String lux         = "0";
String pzem        = "0";
String suhu_air    = "0";

void setup() {
  Serial.begin(9600);
  Serial1.begin(9600);
  Serial2.begin(9600);
  
  lcd.begin(); 
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("D0 : Sx-Data Out");
  lcd.setCursor(0,1);
  lcd.print("S1 : Serial 1 In");
  lcd.setCursor(0,2);
  lcd.print("S2 : Serial 2 In");
  lcd.setCursor(0,3);
  lcd.print("S3 : Serial 3 In");

  delay(5000);
  lcd.clear();

//  if(!SD.begin()){
  //      lcd.print("Card Mount Failed!");
    //    return;
    //}

//  konek1.begin(115200, SWSERIAL_8N1, 4, 2, true, 256);
//  konek1.begin(115200);

  //writeFile(SD, "/hello.txt", "");
  
  //WiFiManager wifiManager;
  //wifiManager.autoConnect(THINGNAME);
  //reset saved settings
  //wifiManager.resetSettings();
  //Serial.println("Connected!");
  WiFi.setHostname(THINGNAME);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);
  connectToWiFi(String("Attempting to connect to SSID: ") + String(ssid));

  NTPConnect();

#ifdef ESP32
  net.setCACert(cacert);
  net.setCertificate(client_cert);
  net.setPrivateKey(privkey);
#else
  net.setTrustAnchors(&cert);
  net.setClientRSACert(&client_crt, &key);
#endif

  client.setServer(MQTT_HOST, MQTT_PORT);

  connectToMqtt();

  while (!Serial) continue;
}

void sendDataSensor() {
  String S1 = "";
  String S2 = "";
  String S3 = "";

  // serial komunikasi arduino dengan esp32
  while (Serial.available() > 0) {
    char c = Serial.read();
    if(c!=13 && c!=10) S1 += String(c);
  }
  while (Serial1.available() > 0) {
    char c = Serial1.read();
    if(c!=13 && c!=10) S2 += String(c);
  }
  while (Serial2.available() > 0) {
    char c = Serial2.read();
    if(c!=13 && c!=10) S3 += String(c);
    }
  String message = "";
  // data ilham
  stringToArray(inputText,' ',S1);
  if(inputText[0]=="ph")  Ph = inputText[1];
  if(inputText[0]=="mpx") mpx = inputText[1];
  if(inputText[0]=="wfl") water_flow = inputText[1];
  //if(inputText[0]=="ph_min") 
  // Serial.println("6.85");

  // data faiq
  stringToArray(inputText,' ',S2);
  if(inputText[0]=="lux") lux = inputText[1];
  if(inputText[0]=="va") pzem = inputText[1];
  if(inputText[0]=="tmp") suhu_air = inputText[1];
  //if(inputText[0]=="wf_min") 
  // Serial1.println("7.00");

  // data mesi
  stringToArray(inputText,' ',S3);
  if(inputText[0]=="tb") turbidity = inputText[1];
  if(inputText[0]=="wl") water_lv = inputText[1];
  //if(inputText[0]=="time_now") 
  // Serial2.println("23:59");
  String formattedDate = String(timeClient.getFullFormattedTime());
  message += Ph + "," + mpx + "," + water_flow + "," + lux + "," + pzem + "," + suhu_air + "," + turbidity + "," + water_lv + "," + formattedDate;
  // publish data ke broker AWS
  if (!client.publish(MQTT_PUB_TOPIC, message.c_str(), false)) //contoh data yg terkirim (100, 2019-10-30 00:00:00)
    pubSubErr(client.state());

  lcd.setCursor(0,0);
  lcd.print("PH: "+Ph+"  Lux:"+lux);
  // lcd.print(String(S3));
  lcd.setCursor(0,1);
  lcd.print("Tb: "+turbidity+"  Wlv:"+water_lv);
  // lcd.print(String(S2));
  lcd.setCursor(0,2);
  lcd.print("VA: "+pzem+"  Tmp:"+suhu_air);
  // lcd.print(String(S1));
  lcd.setCursor(0,3);
  lcd.print("Fl: "+water_flow+"  Mpx:"+mpx);

  delay(500);
  lcd.clear();
  
  //appendFile(SD, "/hey.txt", "World!\n");
}

void clearRow(int x)
{ lcd.setCursor(0,x);
  lcd.print("");
}

void stringToArray(String hasil[], char delimiter, String input)
{ 
  int index = 0;
  while(input.indexOf(delimiter)>=0)
  { 
    hasil[index] = input.substring(0,input.indexOf(delimiter));
    index++;
    input = input.substring(input.indexOf(delimiter)+1);
  }
  hasil[index] = input;
}

void writeFile(fs::FS &fs, const char * path, const char * message){
    Serial.printf("Writing file: %s\n", path);

    File file = fs.open(path, FILE_WRITE);
    if(!file){
        Serial.println("Failed to open file for writing");
        return;
    }
    if(file.print(message)){
        Serial.println("File written");
    } else {
        Serial.println("Write failed");
    }
    file.close();
}

void appendFile(fs::FS &fs, const char * path, const char * message){
    Serial.printf("Appending to file: %s\n", path);

    File file = fs.open(path, FILE_APPEND);
    if(!file){
        Serial.println("Failed to open file for appending");
        return;
    }
    if(file.print(message)){
        Serial.println("Message appended");
    } else {
        Serial.println("Append failed");
    }
    file.close();
}


void loop(){
  now = time(nullptr);

  if (!client.connected())
  {
    checkWiFiThenMQTT();
    //checkWiFiThenMQTTNonBlocking();
    //checkWiFiThenReboot();
  }
  else
  {
    client.loop();
      // while(s.available())
      sendDataSensor();
      delay(10000);
  }
  }