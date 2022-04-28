<!DOCTYPE html>
<html lang="de">

<head>
    <link rel="stylesheet" type="text/css" href="styles/universal-rounded.css">
    <link rel="stylesheet" type="text/css" href="styles/input-rounded.css">
    <style>
        :root {
            --color-main: #E15D44;
        }

        .graph {
            background: transparent;
            position: relative;
            left: 1px;
            right: 1px;
            top: 1px;
            bottom: 1px;
            min-width: 95%;
            min-height: 32rem;
            max-height: 100%;
        }
    </style>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Heizung</title>
</head>

<body>
    <nav class="navbar navbar-expand-lg navbar-light bg-light">
        <h1 class="navbar-brand" href="#">Aussentemperaturvergleichsdatenbank</h1>
        <label for="navhide">Ausklappen</label>
        <input type="checkbox" id="navhide" hidden>
        <div class="navbar-collapse">
            <ul>
                <li>
                    <a href="monitoring.html"><img src="http://alpakagott/assets/icon.gif" alt=""> Monitoring</a>
                </li>
                <li>
                    <a href="http://alpakagott/"><img src="http://alpakagott/assets/icon.gif" alt=""> Eingangshalle</a>
                </li>
            </ul>
        </div>
    </nav>
    <div class="dashboard">
        <article class="widget">
            <h3>Temperaturdifferenzdatenbank</h3>
            <?php
                /*
                    Value Table is:
                    Create table diffsolarenv(
                        solar float not null,
                        environment float not null unique
                    );
                */
                require("mariadb.php");
                echo("<table><tr><th>Solar [°C]</th><th>Aussen [°C]</th></tr>");
                $csv = "solar, environment\n";
                $sql = "select solar, environment from diffsolarenv order by environment"; //Select state from the pump in the get payload
                $dbo->beginTransaction();
                foreach ($dbo->query($sql) as $row) {
                    echo("<tr><td>".$row["solar"]."</td><td>".$row["environment"]."</td></tr>");
                    $csv .= $row["solar"].", ".$row["environment"]."\n";
                }
                unset($row);
                unset($sql);
                $dbo->commit();
                echo("</table>");

                echo <<< TEXTFIELD
                <label for="csv">CSV:</label><br>
                <textarea id="csv" name="csv" rows="4" cols="50">
                $csv
                </textarea>
                TEXTFIELD;
            ?>
        </article>
        <div class="widget">
            <h3>Aktuelle Werte</h3>
            <?php
                $solartemp = -273; 
                $sql = "select solar from temperatures order by ts desc limit 1;"; //Select state from the pump in the get payload
                $dbo->beginTransaction();
                foreach ($dbo->query($sql) as $row) {
                    $solartemp = $row["solar"];
                }
                unset($row);
                $dbo->commit();
                echo <<< LIST
                    <ul>
                        <li><b>Solar</b>: <number id="solartemp">$solartemp</number> °C</li>
                        <li><b>Aussen</b>: <number id="envtemp"></number> °C</li>
                    </ul>
                LIST;
                // Create API Key JS Variable
                $apikey = file_get_contents('../openweather.apikey');
                echo <<< APIKEY
                <script>
                    const apikey = "$apikey";
                </script>
                APIKEY;
            ?>
            <a id="addlink">
                <button class="form-button">
                    Aktuelle werte hinzufügen
                </button>
            </a>
            <script>
                const xhr = new XMLHttpRequest();
                const apicall = "https://api.openweathermap.org/data/2.5/weather?q=Graz&appid=" + apikey;
                xhr.open('GET', apicall, true);
                xhr.onload = function(){
                    var obj = JSON.parse(xhr.responseText);
                    var envtemp = obj.main.temp - 273.15;
                    envtemp = Math.round(100*envtemp)/100;
                    document.getElementById("envtemp").innerHTML = envtemp;
                    document.getElementById("addlink").href = "http://alpakagott/heizung/envinsert.php?solar=" + document.getElementById("solartemp").innerHTML + "&environment=" + envtemp;
                }
                xhr.send();
            </script>
            <h3>Eigenen Wert Einfügen</h3>
            <form action="envinsert.php" method="get">
                <label for="solar">Solar:</label>
                <input type="number" step="0.01" id="solar" name="solar" />
                <label for="environment">Environment:</label>
                <input type="number" step="0.01" id="environment" name="environment" />
                <input type="submit" />
            </form>
        </div>
    </div>
</body>

</html>