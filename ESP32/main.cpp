#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>

// --- CONFIGURATION WI-FI ---
const char* ssid = "Kaaris";
const char* password = "password123";

// --- CONFIGURATION MQTT ---
const char* mqtt_server = "10.160.24.214"; 
const char* topic_temp = "arduino/capteurs/temperature";

// --- CONFIGURATION CAPTEUR ----
#define DHTPIN 4       // Broche DATA du DHT connectée au GPIO 4
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// --- OBJETS ---
WiFiClient espClient;
PubSubClient client(espClient);

unsigned long lastMsg = 0;
float derniereTemp = 0;

// Connexion au réseau Wi-Fi
void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connexion a ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connecte !");
  Serial.print("Adresse IP : ");
  Serial.println(WiFi.localIP());
}

// Reconnexion au Broker MQTT si la connexion est perdue
void reconnect() {
  while (!client.connected()) {
    Serial.print("Tentative de connexion MQTT...");
    // Création d'un ID client unique
    String clientId = "ESP32Client-";
    clientId += String(random(0xffff), HEX);
    
    if (client.connect(clientId.c_str())) {
      Serial.println("connecte");
    } else {
      Serial.print("echec, rc=");
      Serial.print(client.state());
      Serial.println(" nouvelle tentative dans 5 secondes");
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    delay(10); // Attend que le port série soit prêt
  }
  delay(1000); // Petit délai de sécurité supplémentaire
  
  Serial.println("--- DEMARRAGE DU SYSTEME ---");
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  dht.begin();
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  unsigned long now = millis();
  // Envoi toutes les 5 secondes
  if (now - lastMsg > 5000) {
    lastMsg = now;

    float t = dht.readTemperature();

    // Vérification si la lecture a réussi
    if (isnan(t)) {
      Serial.println("Erreur de lecture du capteur DHT !");
      return;
    }

    // On n'envoie la donnée que si elle a varié de 0.2°C pour éviter le bruit
    if (abs(t - derniereTemp) >= 0.2) {
      Serial.print("Nouvelle temperature : ");
      Serial.println(t);
      
      // Conversion du float en String pour l'envoi MQTT
      client.publish(topic_temp, String(t, 2).c_str());
      derniereTemp = t;
    }
  }
}