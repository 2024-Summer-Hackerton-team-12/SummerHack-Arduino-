//chatgpt유노?

#include "DHT.h"
#include "Adafruit_Sensor.h"
#include <WiFi.h>
#include <PubSubClient.h>

#define DHTPIN 25       // DHT 센서가 연결된 핀
#define DHTTYPE DHT11   // DHT11을 사용할 경우

DHT dht(DHTPIN, DHTTYPE);
WiFiClient espClient;
PubSubClient client(espClient);

const char* ssid = "bssm_free";
const char* password = "bssm_free";
const char* mqtt_server = "broker.mqtt-dashboard.com";
const int ledPin = 16; 
const int ledChannel = 0;
const int freq = 50;
const int resolution = 16;

unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE  (50)
char msg[MSG_BUFFER_SIZE];
int value = 0;

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      client.subscribe("bssm/hino");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  dht.begin();
  setup_wifi();

  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  
  pinMode(33, OUTPUT); // 빨간불
  pinMode(32, OUTPUT); // 노란불
  pinMode(17, OUTPUT); // 초록불
}

void loop() {
  delay(100); 
  float humidity = dht.readHumidity();    // 습도 읽기
  float temperature = dht.readTemperature(); // 섭씨 온도 읽기
  int sensorVal = analogRead(34); // 조도 읽기
  
  if (isnan(humidity) || isnan(temperature)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }

  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  
  Serial.print("Humidity: ");
  Serial.print(humidity);
  Serial.print(" %\t");
  Serial.print("Temperature: ");
  Serial.print(temperature);
  Serial.println(" *C");

  Serial.println(sensorVal);

  // LED 제어 및 MQTT 전송
  const char* ledStatus = "";
  if (sensorVal >= 2000) {
    Serial.print("초록불!");
    digitalWrite(17, HIGH);
    digitalWrite(32, LOW);
    digitalWrite(33, LOW);
    ledStatus = "Green";
  } else if (sensorVal >= 1000 && sensorVal <= 1999) {
    Serial.print("노란불!");
    digitalWrite(17, LOW);
    digitalWrite(32, HIGH);
    digitalWrite(33, LOW);
    ledStatus = "Yellow";
  } else if (sensorVal >= 0 && sensorVal <= 999) {
    Serial.print("빨간불!");
    digitalWrite(17, LOW);
    digitalWrite(32, LOW);
    digitalWrite(33, HIGH);
    ledStatus = "Red";
  }

  // 온습도 및 LED 상태 MQTT로 전송
  snprintf(msg, MSG_BUFFER_SIZE, "Humidity: %.2f %% Temperature: %.2f *C LED: %s", humidity, temperature, ledStatus);
  client.publish("bssm/hino/data", msg);
}
