#ifdef ESP32
#include <WiFi.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#else
#error Platform not supported
#endif
#include <ArduinoJson.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <NTPClient.h>
#include <SoftwareSerial.h>
#include <WiFiUdp.h>
#include <DNSServer.h>
#include <SPI.h>
#include <LiquidCrystal_I2C.h>
#define emptyString String()

#if !(ARDUINOJSON_VERSION_MAJOR == 6 and ARDUINOJSON_VERSION_MINOR >= 7)
#error "Install ArduinoJson v6.7.0-beta or higher"
#endif
 
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
#define RELAY 15
#define RELAY 15
#define RELAY 15
#define RELAY 15

// Serial UART for GSM
#define RXD 16
#define TXD 17

// liquid I2C  
LiquidCrystal_I2C  lcd(0x27,4,20);

#include "secrets.h" // ada cert
char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
const long utcOffsetInSeconds = 25200; //WIB +07.00
const int MQTT_PORT = 8883;

// AWS IoT Core Topic
#define kolamId "1"
const char MQTT_SUB_TOPIC2[] = "algaepond/" kolamId "/relay_controlling";
const char MQTT_PUB_TOPIC[] = "algaepond/" kolamId "/monitoring";
const char MQTT_PUB_TOPIC2[] = "$aws/things/" THINGNAME "/shadow/update";

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);

#ifdef USE_SUMMER_TIME_DST
uint8_t DST = 1;
#else
uint8_t DST = 0;
#endif

WiFiClientSecure net;

#ifdef ESP8266

// timeClient.begin();

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
      if (!client.subscribe(MQTT_SUB_TOPIC2))
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

// fungsi controlling
String stateRelay = "panen:1";
String stateNow = "panen:1";
String saveMode;
const int pinRelay = 26;
const char* control_mode;
const char* control_manual;
String manual = "panen:0";

void messageReceived(char *topic, byte *payload, unsigned int length)
{
  if (strcmp(topic, MQTT_SUB_TOPIC2) == 0) {
    Serial.print("Received [");
    Serial.print(topic);
    String stateRelay = "";
    Serial.print("]: ");
    for (int i = 0; i < length; i++)
    {
      Serial.print((char)payload[i]);
      stateRelay += (char)payload[i];
    }
    const size_t capacity = JSON_OBJECT_SIZE(3) + 100;
    DynamicJsonDocument doc(capacity);
    deserializeJson(doc, stateRelay);
    control_mode = doc["mode"];
    control_manual = doc["manual"];
    Serial.println();
    saveMode = String(control_mode);
  }

  else Serial.println("Unknown topic");

  if (saveMode == "manual") {
    RelayControlManual();
    Serial.println("Manual");
  }
  else Serial.println("Unknown mode");


}

void sendData(void)
{
  String stateRelayNow = "";
  stateRelayNow = String(digitalRead(pinRelay));
  DynamicJsonDocument jsonBuffer(JSON_OBJECT_SIZE(3) + 100);
  JsonObject root = jsonBuffer.to<JsonObject>();
  JsonObject state = root.createNestedObject("state");
  JsonObject state_reported = state.createNestedObject("reported");
  state_reported["stateRelayDevice"] = stateRelayNow;
  Serial.printf("Sending  [%s]: ", MQTT_PUB_TOPIC2);
  serializeJson(root, Serial);
  Serial.println();
  char shadow[measureJson(root) + 1];
  serializeJson(root, shadow, sizeof(shadow));
  if (!client.publish(MQTT_PUB_TOPIC2, shadow, false))
    pubSubErr(client.state());
}

void RelayControlManual() {
  if (strcmp(control_manual, "panen:1") == 0) {
    digitalWrite(pinRelay, LOW);
    stateNow = "on";
    if (stateNow != stateRelay) {
      stateRelay = stateNow;
      Serial.println(stateRelay);
      sendData();
      Serial.println("Relay on");
    }
  }

  else if (strcmp(control_manual, "panen:0") == 0) {
    digitalWrite(pinRelay, HIGH);
    stateNow = "off";
    if (stateNow != stateRelay) {
      stateRelay = stateNow;
      sendData();
      Serial.println("Relay off");
    }
  }

  else Serial.println(stateRelay);
}

String inputText[2];
 
SoftwareSerial serial(7, 6); // RX: 10, TX:11
SoftwareSerial serial1(13, 15);
SoftwareSerial serial2(16, 17);

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
  client.setCallback(messageReceived);
  
  connectToMqtt();  
  pinMode(pinRelay, OUTPUT);
  while (!Serial) continue;
}

void sendDataSensor() {
  now = time(nullptr);
  timeClient.update();
  // because fcking corona, data from sensor is dummy
  srand(time(NULL));
  String id_kolam = String(kolamId);
  float keruh = random(1000,3000);
  String kekeruhan = String(keruh);
  float ketinggian = random(10,14);
  String ketinggian_air = String(ketinggian);
  float ph = random(7.5,9.5);
  String ph_air = String(ph);
  float aliran = random(3.33,25);
  String aliran_udara = String(aliran);
  float kecepatan = random(16,32);
  String kecepatan_air = String(kecepatan);
  float intensitas = random(5000,30000);
  String intensitas_cahaya = String(intensitas);
  float energi = random(110,200);
  String energi_listrik = String(energi);
  float suhu = random(25,35);
  String suhu_air = String(suhu);
  
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
  if(inputText[0]=="ph")  ph_air = inputText[1];
  if(inputText[0]=="mpx") aliran_udara = inputText[1];
  if(inputText[0]=="wfl") kecepatan_air = inputText[1];

  // data faiq
  stringToArray(inputText,' ',S2);
  if(inputText[0]=="lux") intensitas_cahaya = inputText[1];
  if(inputText[0]=="va") energi_listrik = inputText[1];
  if(inputText[0]=="tmp") suhu_air = inputText[1];

  // data mesi
  stringToArray(inputText,' ',S3);
  if(inputText[0]=="tb") kekeruhan = inputText[1];
  if(inputText[0]=="wl") ketinggian_air = inputText[1];
  
  String created_at = String(timeClient.getFullFormattedTime());
  
  // parameter yang akan dipublish
  message += id_kolam + "," + ph_air + "," + aliran_udara + "," + kecepatan_air + "," + 
             intensitas_cahaya + "," + energi_listrik + "," + suhu_air + "," + 
             kekeruhan + "," + ketinggian_air + "," + created_at;
  // publish data ke broker AWS
  if (!client.publish(MQTT_PUB_TOPIC, message.c_str(), false))
    //contoh data yg terkirim (100, 2019-10-30 00:00:00)
    pubSubErr(client.state());

  // menampilkan data ke LCD
  lcd.setCursor(0,0);
  lcd.print("PH: "+ph_air+"  Lux:"+intensitas_cahaya);
  // lcd.print(String(S3));
  lcd.setCursor(0,1);
  lcd.print("Tb: "+kekeruhan+"  Wlv:"+ketinggian_air);
  // lcd.print(String(S2));
  lcd.setCursor(0,2);
  lcd.print("VA: "+energi_listrik+"  Tmp:"+suhu_air);
  // lcd.print(String(S1));
  lcd.setCursor(0,3);
  lcd.print("Fl: "+kecepatan_air+"  Mpx:"+aliran_udara);

  delay(500);
  lcd.clear();
  
}

void clearRow(int x)
{ lcd.setCursor(0,x);
  lcd.print("                    ");
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
    // delay publish data
    if (millis() - lastMillis > 30000)
    {
      sendDataSensor();
      lastMillis = millis();
    }
       
  }
  }