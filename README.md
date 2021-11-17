# IOT-Heizung
[![Platform: ESP-IDF](https://img.shields.io/badge/ESP--IDF-v3.0%2B-purple.svg)](https://docs.espressif.com/projects/esp-idf/en/stable/get-started/)
![good example](https://img.shields.io/badge/stability-bleedingEdge-red.svg)
[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](https://www.gnu.org/licenses/gpl-3.0)

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
Heizkreisregelung

---
## Solar
Die Solarpumpe wird nach folgenden Kriterien eingeschaltet:
* <i>&thetasym;_solar > &thetasym;_buffer + 10°C</i>
* 
Die Solarpumpe wird nach folgenden Kriterien ausgeschaltet:
* <i>&thetasym;_vorlauf < &thetasym;_rucklauf</i>
* oder <i>&thetasym;_solar < &thetasym;_buffer</i>

Das wird alle 2 Minuten abgefragt

**Solarsteuerung if-condition**
```c
//Es ist Invertiert weil die Relays der Pumpen invertiert sind
if(gpio_get_level(solarpumpe.gpio)){
    //Solarpump is not running at this point
    if(temps[TEMP_SOLAR] > temps[TEMP_BUFFER] + SOLAR_TO_BUFF_OFFSET){
        ESP_LOGI(SOLARTAG,"Solarpumpe einschalten.");
        gpio_set_level(solarpumpe.gpio,PUMP_ON);
        //Wie viele Berechnungen muss die Solarsteuerung aussetzen?
        blocker = 3;
    }
}else{
    //Solarpump is running
    if(
        temps[TEMP_VORLAUF] < temps[TEMP_RUCKLAUF] ||   //Vorlauf ist kälter als Rücklauf
        temps[TEMP_SOLAR] < temps[TEMP_BUFFER]          //oder Solarpannel ist kälter als Buffer
    ){  
        ESP_LOGI(SOLARTAG,"Solarpumpe ausschalten.");
        gpio_set_level(solarpumpe.gpio,PUMP_OFF);
        //Wie viele Berechnungen muss die Solarsteuerung aussetzen?
        blocker = 5;
    }
}
```

Es soll im Frontend trotzdem noch möglich sein die Solarpumpe
manuell einzuschalten, um im Sommer das Solarpannel zu kühlen.

# Frontend
Nicht jeder soll die Heizung Steuern dürfen. Accounts die auf die Heizung zugreifen dürfen, werden mit einem LDAP server authentifiziert.
Der Apache Webserver authentifiziert die Nutzer mit einem OpenLDAP server, und bearbeitet mit PHP die Datenbanken.
Das Frontend ist ein maßgeschneidertes Monitoring Tool, speziell für diesen Anwendungsfall.
