#include <Arduino.h>
#include <SPI.h>
#include <Ethernet.h>
#include <PubSubClient.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>

// --- CONFIGURATION ---
#define DHTPIN 2      
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

#define ULTRASON_PIN 3 
#define BUZZER_PIN 4      
#define LDR_PIN A1
#define SEUIL_MOUVEMENT 5 

byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
IPAddress ip(10, 160, 24, 150); 
IPAddress gateway(10, 160, 24, 1);
IPAddress subnet(255, 255, 254, 0);
IPAddress dns(8, 8, 8, 8);

const char* mqtt_server = "10.160.24.214"; 

EthernetClient ethClient;
PubSubClient client(ethClient);

unsigned long lastMeteoSend = 0;
long derniereDistance = 0;

// --- FONCTION DE RÉCEPTION DES ORDRES ---
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Commande reçue [");
  Serial.print(topic);
  Serial.print("] : ");
  
  char message = (char)payload[0];
  Serial.println(message);

  if (String(topic) == "arduino/commande/buzzer") {
    if (message == '1') {
      digitalWrite(BUZZER_PIN, HIGH);
    } else if (message == '0') {
      digitalWrite(BUZZER_PIN, LOW);
    }
  }
}

long readDistance() {
  pinMode(ULTRASON_PIN, OUTPUT);
  digitalWrite(ULTRASON_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(ULTRASON_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(ULTRASON_PIN, LOW);
  pinMode(ULTRASON_PIN, INPUT);
  long duration = pulseIn(ULTRASON_PIN, HIGH);
  long distance = duration * 0.034 / 2;
  if (distance > 400 || distance <= 0) return derniereDistance;
  return distance;
}

void reconnect() {
  while (!client.connected()) {
    if (client.connect("Arduino_Ralys_Client")) {
      Serial.println("MQTT Connecté !");
      client.subscribe("arduino/commande/buzzer");
    } else {
      delay(2000);
    }
  }
}

void setup() {
  Serial.begin(9600);
  dht.begin();
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);
  
  Ethernet.begin(mac, ip, dns, gateway, subnet);
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  delay(1500);
}

void loop() {
  if (!client.connected()) reconnect();
  client.loop();

  unsigned long now = millis();

  // 1. SURVEILLANCE DISTANCE
  long distanceActuelle = readDistance();
  if (abs(distanceActuelle - derniereDistance) >= SEUIL_MOUVEMENT) {
      char dStr[8];
      sprintf(dStr, "%ld", distanceActuelle);
      client.publish("arduino/capteurs/distance", dStr);
      derniereDistance = distanceActuelle;
  }

  // 2. ENVOI MÉTÉO ET LUMINOSITÉ (Périodique - toutes les 5s)
  if (now - lastMeteoSend >= 5000) {
    lastMeteoSend = now;
    
    // Lecture DHT
    float h = dht.readHumidity();
    float t = dht.readTemperature();
    
    // Lecture Luminosité (Conversion 0-1023 vers 0-100%)
    int luxRaw = analogRead(LDR_PIN);
    int luxPercent = map(luxRaw, 0, 1023, 0, 100);

    // Publication Température et Humidité
    if (!isnan(h) && !isnan(t)) {
      char tStr[8], hStr[8];
      dtostrf(t, 1, 2, tStr);
      dtostrf(h, 1, 2, hStr);
      client.publish("arduino/capteurs/temperature", tStr);
      client.publish("arduino/capteurs/humidity", hStr);
    }

    // Publication Luminosité
    char lStr[5];
    sprintf(lStr, "%d", luxPercent);
    client.publish("arduino/capteurs/lux", lStr);
    
    Serial.print(" Temp: "); Serial.print(t);
    Serial.print(" Hum: "); Serial.print(h);
    Serial.print(" Lux: "); Serial.println(luxPercent);
  }
}