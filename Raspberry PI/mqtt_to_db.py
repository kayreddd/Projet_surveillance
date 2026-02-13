import paho.mqtt.client as mqtt
import sqlite3
import os
import subprocess
import threading
import time
from datetime import datetime

MQTT_BROKER = "localhost" 
MQTT_TOPIC = "arduino/capteurs/#"
DB_PATH = "/home/ralys/ma_base.db"
PHOTO_DIR = "/home/ralys/photos"

derniers_temps = {"motion": 0, "temperature": 0, "humidity": 0, "lux": 0, "distance_live": 0}

DELAI_MOTION = 10 
DELAI_LIVE = 2  

camera_lock = threading.Lock()

def save_to_db(nom, valeur):
    try:
        conn = sqlite3.connect(DB_PATH)
        cursor = conn.cursor()
        cursor.execute("INSERT INTO capteurs (nom, valeur, date) VALUES (?, ?, ?)", 
                       (nom, valeur, datetime.now().strftime("%Y-%m-%d %H:%M:%S")))
        conn.commit()
        conn.close()
    except Exception as e:
        print(f"[ERREUR DB] {e}")

def take_photo():
    if camera_lock.locked(): return
    with camera_lock:
        filename = datetime.now().strftime("%Y-%m-%d_%H-%M-%S.jpg")
        filepath = os.path.join(PHOTO_DIR, filename)
        try:
            subprocess.run(["rpicam-still", "-o", filepath, "--immediate", "--nopreview", "--timeout", "500"], 
                           check=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        except: pass

def on_message(client, userdata, msg):
    global derniers_temps
    nom = msg.topic.split('/')[-1]
    valeur_str = msg.payload.decode()
    maintenant = time.time()

    try:
        if nom == "distance":
            valeur = float(valeur_str)
            if valeur < 100:
                if maintenant - derniers_temps["motion"] > DELAI_MOTION:
                    # On enregistre le mouvement
                    save_to_db("motion", f"{valeur} cm")
                    threading.Thread(target=take_photo).start()
                    derniers_temps["motion"] = maintenant
                    print(f"[EVENT] Nouveau mouvement enregistré : {valeur}cm")

            if maintenant - derniers_temps["distance_live"] > DELAI_LIVE:
                save_to_db("distance", valeur_str)
                derniers_temps["distance_live"] = maintenant

        elif nom in ["temperature", "humidity", "lux"]:
            # On enregistre les autres capteurs seulement toutes les 2s pour le direct
            if maintenant - derniers_temps.get(nom, 0) > DELAI_LIVE:
                save_to_db(nom, valeur_str)
                derniers_temps[nom] = maintenant

    except ValueError:
        pass

client = mqtt.Client()
client.on_message = on_message
client.connect(MQTT_BROKER, 1883, 60)
client.subscribe(MQTT_TOPIC)
client.loop_forever()
