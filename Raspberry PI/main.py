from fastapi import FastAPI, HTTPException
from fastapi.middleware.cors import CORSMiddleware
from fastapi.staticfiles import StaticFiles
from fastapi.responses import FileResponse
import sqlite3
import os
import paho.mqtt.publish as publish

app = FastAPI()

# Autoriser le Dashboard
app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_methods=["*"],
    allow_headers=["*"],
)

app.mount("/static", StaticFiles(directory="/home/ralys/photos"), name="static")

@app.get("/")
async def read_index():
    return FileResponse('index.html') 

DB_PATH = "/home/ralys/ma_base.db"

def get_db_connection():
    conn = sqlite3.connect(DB_PATH)
    conn.row_factory = sqlite3.Row
    return conn

#Capteurs
@app.get("/capteurs/complet")
def get_all_latest():
    try:
        conn = get_db_connection()
        query = """
            SELECT nom, valeur, date FROM capteurs 
            WHERE id IN (SELECT MAX(id) FROM capteurs GROUP BY nom)
        """
        rows = conn.execute(query).fetchall()
        conn.close()
        return [dict(row) for row in rows]
    except Exception as e:
        raise HTTPException(status_code=500, detail=str(e))

@app.get("/capteurs/{nom_capteur}")
def get_derniere_valeur(nom_capteur: str):
    conn = get_db_connection()
    row = conn.execute(
        "SELECT nom, valeur, date FROM capteurs WHERE nom = ? ORDER BY date DESC LIMIT 1", 
        (nom_capteur,)
    ).fetchone()
    conn.close()
    
    if row:
        return dict(row)
    raise HTTPException(status_code=404, detail=f"Capteur '{nom_capteur}' non trouvé")

@app.get("/photos")
def list_photos():
    try:
        files = [f for f in os.listdir("/home/ralys/photos") if f.endswith(".jpg")]
        files.sort(reverse=True)
        return {"photos": files}
    except Exception as e:
        return {"photos": [], "error": str(e)}

@app.post("/action/buzzer/{etat}")
def control_buzzer(etat: int):
    if etat not in [0, 1]:
        raise HTTPException(status_code=400, detail="L'état doit être 0 ou 1")
    try:
        publish.single("arduino/commande/buzzer", str(etat), hostname="localhost")
        return {"status": "success", "buzzer": etat}
    except Exception as e:
        raise HTTPException(status_code=500, detail=f"Erreur MQTT : {str(e)}")

@app.get("/historique/{nom_capteur}")
def get_historique(nom_capteur: str, limit: int = 10):
    conn = get_db_connection()
    rows = conn.execute(
        "SELECT * FROM capteurs WHERE nom = ? ORDER BY date DESC LIMIT ?", 
        (nom_capteur, limit)
    ).fetchall()
    conn.close()
    return [dict(row) for row in rows]
