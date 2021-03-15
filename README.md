# IOT-Heizung
IOT Steuerung für Solarpanels, Wärmepumpe und Heizung in ESP-IDF.
Alle Temperaturen werden mit DS18b20 sensoren Aufgenommen,
ausser die Temperatur der Solarpannels. Dort ließt der ESP32
einen analogen Wert von einem NTC ab der mit dem Polynom aus dem
Matlab Skript ausgelesen werden kann.

Die Heizung ist eine Hub and Spoke Architektur. Der Hub ist ein RaspberryPi Webserver für alle IOT geräte.

---
## Temperatursensoren
Alle sensoren haben einen Platz und eine Farbe.
Auf dem Hub werden die Temperaturen mit der Farbe gespeichert.
| Platzierung     | Farbe / Typ     |
| :-----------:   | :-------------: |
| Rücklauf Solar  | Rot             |
| Vorlauf Solar   | Blau            |
| unbekannt       | Grün            |
| unbekannt       | Gelb            |
| Buffer          | Weiss           |
| Vorlauf Heizung | Braun           |
| Solarpannel     | Polynom NTC     |
| Raumtemperatur  | (TO92) On Board |

---
## Pumpen
Alle Pumpen und Mischer werden mit Relays ein und aus geschalten.

<span style="color:red">**Alle GPIOs sind Invertiert**</span>.

| Name        | Datenbank Name | GPIO |
| :-------:   | :------------: | :--: |
||             **Relay Board 1**     || 
| Heizpumpe   | `HEATPUMP`     |   15 |
| Solarpumpe  | `SOLARPUMP`    |    2 |
| Ungenutzt   | `REDUNDANT`    |    4 |
| Bufferpumpe | `BUFFER`       |   16 |
||             **Relay Board 2**     || 
|*Mischer Auf*| `MIXER_OPEN`   |   17 |
|*Mischer Zu* | `MIXER_CLOSE`  |    5 |
|Zwischenpumpe| `GROUNDPUMP0`  |   18 |
| Wärmwpumpe  | `GROUNDPUMP1`  |   19 |

Der 4-Weg-Mischer hat 2 Relays. Die werden im Programm nicht
wie Pumpen behandelt.
Der ESP32 nutzt 1 Byte im NVS speicher um alle zuständer der Pumpen zu speichern. Die Bitreihenfolde ist in der Reihenfolge der 8 Relays mit ausnahme der Mischerrelays.
Bei einem Neustart wird dieses Byte ausgelesen und die Pumpen
werden entsprechend ein oder ausgeschalten.
Falls es Brownouts gibt, werden die Zustände der Pumpen sofort wiederhergestellt.

***
## 4-Wege-Mischer
*W.I.P*

---
## Solar
Die Solarpumpe wird nach Folgenden kriterien eingeschaltet:
* k1
* k2
* k3

Das wird alle 2 Minuten abgefragt

**Solarsteuerung if-condition**
```c
if(a > b){
    enable solar...
}else{
    disable solar...
}
```

Es soll im Frontend trotzdem noch möglich sein die Solarpumpe
manuell einzuschalten, um im Sommer das Solarpannel zu kühlen. 