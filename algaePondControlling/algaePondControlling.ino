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
#define emptyString String()

#if !(ARDUINOJSON_VERSION_MAJOR == 6 and ARDUINOJSON_VERSION_MINOR >= 7)
#error "Install ArduinoJson v6.7.0-beta or higher"
#endif

#include "secrets.h" // ada cert
char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
const long utcOffsetInSeconds = 25200; //WIB +07.00
const int MQTT_PORT = 8883;

// AWS IoT Core Topic
#define kolamId "1"
const char MQTT_SUB_TOPIC[] = "algaepond/" kolamId "/monitoring";
const char MQTT_SUB_TOPIC2[] = "algaepond/" kolamId "/relay_controlling";
//const char MQTT_SUB_TOPIC3[] = "algaepond/" kolamId "/servoNutrisi_controlling";
//const char MQTT_SUB_TOPIC4[] = "algaepond/" kolamId "/stepperPipaAir_controlling";
const char MQTT_PUB_TOPIC[] = "algaepond/" kolamId "/monitoring";
const char MQTT_PUB2_TOPIC[] = "$aws/things/" THINGNAME "/shadow/update";

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

// fungsi controlling
String stateRelay = "on";
String stateNow = "on";
String saveMode;
const int pinRelay = 13;
const char* control_mode;
const char* control_manual;
String manual = "off";

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
  Serial.printf("Sending  [%s]: ", MQTT_PUB2_TOPIC);
  serializeJson(root, Serial);
  Serial.println();
  char shadow[measureJson(root) + 1];
  serializeJson(root, shadow, sizeof(shadow));
  if (!client.publish(MQTT_PUB2_TOPIC, shadow, false))
    pubSubErr(client.state());
}

void RelayControlManual() {
  if (strcmp(control_manual, "on") == 0) {
    digitalWrite(pinRelay, LOW);
    stateNow = "on";
    if (stateNow != stateRelay) {
      stateRelay = stateNow;
      Serial.println(stateRelay);
      sendData();
      Serial.println("Relay on");
    }
  }

  else if (strcmp(control_manual, "off") == 0) {
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


void setup() {
  Serial.begin(115200);
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
  // while (!Serial) continue;
}

void loop()
{
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
    
  }
}