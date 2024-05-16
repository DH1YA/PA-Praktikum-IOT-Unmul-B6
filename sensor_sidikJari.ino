#include <EEPROM.h>
int address = 0;
byte value;
int nilai =0;
uint8_t id; // cek id fingerprint
#include <SoftwareSerial.h>

#include <Adafruit_Fingerprint.h>
#include <Servo.h>
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

#if (defined(__AVR__) || defined(ESP8266)) && !defined(__AVR_ATmega2560__)
// For UNO and others without hardware serial, we must use software serial...
// pin #2 is IN from sensor (GREEN wire)
// pin #3 is OUT from arduino  (WHITE wire)
// Set up the serial port to use softwareserial..
SoftwareSerial mySerial(2,12);

#else
// On Leonardo/M0/etc, others with hardware serial, use hardware serial!
// #0 is green wire, #1 is white
#define mySerial Serial1

#endif

//Wifi
const char *ssid = "Xiaomi 13T";
const char *password = "12345678";

// MQTT Broker
const char *mqtt_broker = "broker.emqx.io";
const char *topicUtama = "PAGroup6/SmartDoor";
const char *topic2 = "PAGroup6/SmartDoor/controlling";
const char *mqtt_username = "";
const char *mqtt_password = "";
const int mqtt_port = 1883;

WiFiClient espClient;
PubSubClient client(espClient);

// String controllingReceive;
String sensor = "OFF";

Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial); 
const int buttonPin1 = D3;
const int buttonPin2 = D7;
const int servoPin = D5;  // Mengatur pin digital untuk servo (misalnya D5)
Servo servo;  // Membuat objek dari kelas Servo
int buzzer=4;
void setup()
{
pinMode(buttonPin1, INPUT_PULLUP);  // Mengatur pin tombol sebagai INPUT_PULLUP
pinMode(buttonPin2, INPUT_PULLUP);  // Mengatur pin tombol sebagai INPUT_PULLUP
pinMode(buzzer,OUTPUT);
servo.attach(servoPin);  // Menghubungkan servo dengan pin yang ditentukan
  
  Serial.begin(9600);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to the WiFi network");

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
    // subscribe topic
  client.setCallback(callback);  //1
  client.subscribe(topicUtama);      //2



  while (!Serial);  // For Yun/Leo/Micro/Zero/...
  delay(100);
  Serial.println("\n\nAdafruit finger detect test");

  // set the data rate for the sensor serial port
  finger.begin(57600);
  delay(5);
  if (finger.verifyPassword()) {
    Serial.println("Found fingerprint sensor!");
  } else {
    Serial.println("Did not find fingerprint sensor :(");
    while (1) { delay(1); }
  }

  Serial.println(F("Reading sensor parameters"));
  finger.getParameters();
  Serial.print(F("Status: 0x")); Serial.println(finger.status_reg, HEX);
  Serial.print(F("Sys ID: 0x")); Serial.println(finger.system_id, HEX);
  Serial.print(F("Capacity: ")); Serial.println(finger.capacity);
  Serial.print(F("Security level: ")); Serial.println(finger.security_level);
  Serial.print(F("Device address: ")); Serial.println(finger.device_addr, HEX);
  Serial.print(F("Packet len: ")); Serial.println(finger.packet_len);
  Serial.print(F("Baud rate: ")); Serial.println(finger.baud_rate);

  finger.getTemplateCount();

  if (finger.templateCount == 0) {
    Serial.print("Sensor doesn't contain any fingerprint data. Please run the 'enroll' example.");
  }
  else {
    Serial.println("Waiting for valid finger...");
      Serial.print("Sensor contains "); Serial.print(finger.templateCount); Serial.println(" templates");
  }
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
  if (strcmp(topic, topicUtama) == 0) {
    Serial.println("masuk");
    if (data == "1") {
      sensor = "ON";
    } else {
      sensor = "OFF";
    }
  }
}

void loop()                     // run over and over again
{
servo.write(0);
if(sensor == "ON")
{
    tone(buzzer, 10); // Menghasilkan nada 
    delay(500); 
    sensor == "OFF";
    noTone(buzzer);
}else{
  int button1 = digitalRead(buttonPin1);
  int button2 = digitalRead(buttonPin2);
  getFingerprintID();
  delay(100);            //don't ned to run this at full speed.
  if(button1 == LOW) // tombol daftar sidik jari
  {
    value = EEPROM.read(1);
    id=value+1;
    getFingerprintEnroll();
    EEPROM.write(1,id);
    delay(1000);
  }
  if(button2 == LOW) // tombol hapus sidik jari
  {
    value = EEPROM.read(1);
    id=value;
    deleteFingerprint(id);
    EEPROM.write(1,id-1);
    delay(1000);
  }

}
client.loop();

}

uint8_t getFingerprintID() {
  uint8_t p = finger.getImage();
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image taken");
      break;
    case FINGERPRINT_NOFINGER:
      Serial.println("No finger detected");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      return p;
    case FINGERPRINT_IMAGEFAIL:
      Serial.println("Imaging error");
      return p;
    default:
      Serial.println("Unknown error");
      return p;
  }

  // OK success!

  p = finger.image2Tz();
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image converted");
      break;
    case FINGERPRINT_IMAGEMESS:
      Serial.println("Image too messy");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      return p;
    case FINGERPRINT_FEATUREFAIL:
      Serial.println("Could not find fingerprint features");
      return p;
    case FINGERPRINT_INVALIDIMAGE:
      Serial.println("Could not find fingerprint features");
      return p;
    default:
      Serial.println("Unknown error");
      return p;
  }

  // OK converted!
  p = finger.fingerSearch();
  if (p == FINGERPRINT_OK) { //jari terdaftar
    digitalWrite(buzzer,HIGH); //SUARA BEEP CEPAT
      delay(100);
      digitalWrite(buzzer,LOW);
      delay(100);
      digitalWrite(buzzer,HIGH);
      delay(100);
      digitalWrite(buzzer,LOW); 
      servo.write(180);
      // digitalWrite(relay,LOW); //PINTU TERBUKA
      delay(3000);             // SELAMA 3 DETIK
      servo.write(0);
      // digitalWrite(relay,HIGH); //KEMUDIAN PINTU MENUTUP KEMBALI

    Serial.println("Found a print match!");
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error");
    return p;
  } else if (p == FINGERPRINT_NOTFOUND) { //tidak terdaftar
    digitalWrite(buzzer,HIGH); //SUARA BEEP AGAK PANJANG
    delay(1000);
    digitalWrite(buzzer,LOW);
    
    Serial.println("Did not find a match");
    return p;
  } else {
    Serial.println("Unknown error");
    return p;
  }

  // found a match!
  Serial.print("Found ID #"); Serial.print(finger.fingerID);
  Serial.print(" with confidence of "); Serial.println(finger.confidence);

  return finger.fingerID;
}

// returns -1 if failed, otherwise returns ID #
int getFingerprintIDez() {
  uint8_t p = finger.getImage();
  if (p != FINGERPRINT_OK)  return -1;

  p = finger.image2Tz();
  if (p != FINGERPRINT_OK)  return -1;

  p = finger.fingerFastSearch();
  if (p != FINGERPRINT_OK)  return -1;

  // found a match!
  Serial.print("Found ID #"); Serial.print(finger.fingerID);
  Serial.print(" with confidence of "); Serial.println(finger.confidence);
  return finger.fingerID;
}


//enrol sidik jari
uint8_t getFingerprintEnroll() {
  int p = -1;
  digitalWrite(buzzer,HIGH); //SUARA BEEP CEPAT
  delay(100);
  digitalWrite(buzzer,LOW);
  delay(100);
  Serial.print("Waiting for valid finger to enroll as #"); Serial.println(id);
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image taken");
      break;
    case FINGERPRINT_NOFINGER:
      Serial.println(".");
      break;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      break;
    case FINGERPRINT_IMAGEFAIL:
      Serial.println("Imaging error");
      break;
    default:
      Serial.println("Unknown error");
      break;
    }
  }

  // OK success!

  p = finger.image2Tz(1);
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image converted");
      break;
    case FINGERPRINT_IMAGEMESS:
      Serial.println("Image too messy");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      return p;
    case FINGERPRINT_FEATUREFAIL:
      Serial.println("Could not find fingerprint features");
      return p;
    case FINGERPRINT_INVALIDIMAGE:
      Serial.println("Could not find fingerprint features");
      return p;
    default:
      Serial.println("Unknown error");
      return p;
  }

  Serial.println("Remove finger");
  delay(2000);
  p = 0;
  while (p != FINGERPRINT_NOFINGER) {
    p = finger.getImage();
  }
  Serial.print("ID "); Serial.println(id);
  Serial.print("EEprom "); Serial.println(value);
  p = -1;
  digitalWrite(buzzer,HIGH); //SUARA BEEP CEPAT
  delay(100);
  digitalWrite(buzzer,LOW);
  delay(100);
  
  Serial.println("Place same finger again");
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image taken");
      break;
    case FINGERPRINT_NOFINGER:
      Serial.print(".");
      break;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      break;
    case FINGERPRINT_IMAGEFAIL:
      Serial.println("Imaging error");
      break;
    default:
      Serial.println("Unknown error");
      break;
    }
  }

  // OK success!

  p = finger.image2Tz(2);
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image converted");
      break;
    case FINGERPRINT_IMAGEMESS:
      Serial.println("Image too messy");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      return p;
    case FINGERPRINT_FEATUREFAIL:
      Serial.println("Could not find fingerprint features");
      return p;
    case FINGERPRINT_INVALIDIMAGE:
      Serial.println("Could not find fingerprint features");
      return p;
    default:
      Serial.println("Unknown error");
      return p;
  }

  // OK converted!
  Serial.print("Creating model for #");  Serial.println(id);

  p = finger.createModel();
  if (p == FINGERPRINT_OK) {
    Serial.println("Prints matched!");
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error");
    return p;
  } else if (p == FINGERPRINT_ENROLLMISMATCH) {
    Serial.println("Fingerprints did not match");
    return p;
  } else {
    Serial.println("Unknown error");
    return p;
  }

  Serial.print("ID "); Serial.println(id);
  p = finger.storeModel(id);
  if (p == FINGERPRINT_OK) {
      digitalWrite(buzzer,HIGH); //SUARA BEEP CEPAT
      delay(100);
      digitalWrite(buzzer,LOW);
      delay(100);
      digitalWrite(buzzer,HIGH);
      delay(100);
      digitalWrite(buzzer,LOW); 
 
    Serial.println("Stored!");
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error");
    return p;
  } else if (p == FINGERPRINT_BADLOCATION) {
    Serial.println("Could not store in that location");
    return p;
  } else if (p == FINGERPRINT_FLASHERR) {
    Serial.println("Error writing to flash");
    return p;
  } else {
    Serial.println("Unknown error");
    return p;
  }

  return true;
}



//hapus sidik jari
uint8_t deleteFingerprint(uint8_t id) {
  uint8_t p = -1;

  p = finger.deleteModel(id);

  if (p == FINGERPRINT_OK) {
          Serial.println("Akses Diterima");
      digitalWrite(buzzer,HIGH); //SUARA BEEP CEPAT
      delay(100);
      digitalWrite(buzzer,LOW);
      delay(100);
      digitalWrite(buzzer,HIGH);
      delay(100);
      digitalWrite(buzzer,LOW); 
 
    Serial.println("Deleted!");
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error");
  } else if (p == FINGERPRINT_BADLOCATION) {
    Serial.println("Could not delete in that location");
  } else if (p == FINGERPRINT_FLASHERR) {
    Serial.println("Error writing to flash");
  } else {
    Serial.print("Unknown error: 0x"); Serial.println(p, HEX);
  }

  return p;
}
