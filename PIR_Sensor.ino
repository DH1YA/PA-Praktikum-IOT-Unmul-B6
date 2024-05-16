#include <ESP8266WiFi.h>
#include <PubSubClient.h>

//Wifi
const char *ssid = "A54 milik Rian Syaputra";
const char *password = "12345678";

// MQTT Broker
const char *mqtt_broker = "broker.emqx.io";
const char *topicUtama = "PAGroup6/SmartDoor";
const char *topic2 = "PAGroup6/SmartDoor/controlling";
const char *mqtt_username = "";
const char *mqtt_password = "";
const int mqtt_port = 1883;

const int PIR_PIN = D6;

WiFiClient espClient;
PubSubClient client(espClient);

String security = "OFF";

void setup() {
  delay(1000);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to the WiFi network");

  // pinMode( BUZZER_PIN, OUTPUT);
  // pinMode( LED_PIN, OUTPUT);
  pinMode(PIR_PIN, INPUT);
  Serial.begin(9600);

  // Connecting to an MQTT broker
  client.setServer(mqtt_broker, mqtt_port);
  while (!client.connected()) {
    String client_id = "mqttx_pa_group6";
    client_id += String(WiFi.macAddress());
    Serial.printf("The client %s connects to the public MQTT broker\n", client_id.c_str());
    if (client.connect(client_id.c_str(), mqtt_username, mqtt_password)) {
      Serial.println("Public EMQX MQTT broker connected");
    } else {
      Serial.print("Failed with state ");
      Serial.print(client.state());
      delay(2000);
    }
  }

  // Publish and subscribe
  client.setCallback(callback);  //1
  client.subscribe(topic2);      //2
  client.publish(topicUtama, "NodeMCU Succes Connected");
}

void callback(char *topic, byte *payload, unsigned int length) {
  Serial.print("Pesan diterima [");
  Serial.print(topic);
  Serial.print("] ");
  String data = "";  // variabel untuk menyimpan data yang berbentuk array char
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
    data += (char)payload[i];  // menyimpan kumpulan char kedalam string
  }
  Serial.println();
  if (strcmp(topic, topic2) == 0) {
    Serial.println("masuk");
    if (data == "ON") {
      security = "ON";
    } else {
      security = "OFF";
    }
  }
}

void loop() {
  if (security == "ON") {
    Serial.println(".");
    String message;
    delay(2000);
    int motion = digitalRead(PIR_PIN);
    if (motion == HIGH) {
      Serial.println(security);
      Serial.println("Motion detected");
      message = String(motion);
      client.publish(topicUtama, message.c_str());
      delay(1000);
    }  // if motion detected
    else {
      message = String(motion);
      client.publish(topicUtama, message.c_str());
      delay(2000);
    }
    
  }


  client.loop();
}