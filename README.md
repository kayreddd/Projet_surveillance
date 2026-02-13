# Projet_surveillance

# Résumé du projet

Votre équipe d’ingénieurs IoT conçoit une solution complète de surveillance pour une salle serveur.

**Objectifs du client :**
- Suivi en temps réel via un tableau de bord : température, luminosité, détection de mouvements, prise de photo en cas de mouvement illégitime…
- Possibilité de déclencher une alarme à distance depuis le tableau de bord.
- Respect de contraintes matérielles et logicielles (architecture légère, adaptée à l’embarqué).

**Résumé technique :**
Des capteurs connectés (ESP32, DHT11, LDR, PIR, caméra) communiquent via MQTT avec un serveur central (ex. Raspberry Pi) qui héberge le broker MQTT et le tableau de bord web. Toutes les données et alertes sont centralisées et pilotables à distance.


## Ce qui a été réalisé

- **Module Arduino** :
	- Acquisition de température/humidité (DHT11), luminosité (LDR), détection de mouvement (ultrason, PIR), gestion d’un buzzer.
	- Envoi des mesures et réception de commandes via MQTT (PubSubClient).
	- Réception d’ordres distants pour activer/désactiver l’alarme (buzzer).

- **Module ESP32** :
	- Acquisition de température (DHT11) et publication périodique sur MQTT.
	- Connexion Wi-Fi, reconnexion automatique au broker MQTT.
	- Code prêt à intégrer d’autres capteurs (PIR, LDR, caméra ESP32-CAM).

- **Dashboard Web** :
	- Interface HTML/CSS moderne pour visualiser les données en temps réel.
	- Prévu pour afficher température, luminosité, mouvements, images et permettre le déclenchement d’actions à distance.

L’architecture est modulaire : chaque sous-dossier correspond à une brique matérielle ou logicielle du système. La communication entre modules se fait via MQTT, ce qui permet une supervision centralisée et des commandes à distance.