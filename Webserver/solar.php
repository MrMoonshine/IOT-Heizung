<!DOCTYPE html>
<html lang="de">

<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Heizung</title>
    <link href="https://cdn.jsdelivr.net/npm/bootstrap@5.2.0-beta1/dist/css/bootstrap.min.css" rel="stylesheet"
        integrity="sha384-0evHe/X+R7YkIZDRvuzKMRqM+OrBnVFBL6DOitfPri4tjfHxaWutUpFmBp4vmVor" crossorigin="anonymous">

    <!--Katex-->
    <link rel="stylesheet" href="https://cdn.jsdelivr.net/npm/katex@0.16.8/dist/katex.css"
        integrity="sha384-pRsDYiLLocCzWnUN/YEr9TBTTaZOUi5x8adKfqi6Qt44lDaFkoP++x1j2ohSMtdf" crossorigin="anonymous">
    <script defer src="https://cdn.jsdelivr.net/npm/katex@0.16.8/dist/katex.js"
        integrity="sha384-tMzugJpfLv7v0f+KXzNMqNCC6sVzLMM3sCnZDgzy0lcO/0h3sAkEBg/URFcV0JpE"
        crossorigin="anonymous"></script>
</head>

<body>
    <nav class="navbar navbar-expand-lg navbar-dark bg-primary d-flex d-print-none">
        <a class="navbar-brand" href="#">&#127777; Heizungssteuerung</a>

        <button class="navbar-toggler" type="button" data-bs-toggle="collapse"
            data-bs-target="#navbarToggleExternalContent" aria-controls="navbarToggleExternalContent"
            aria-expanded="false" aria-label="Toggle navigation">
            <span class="navbar-toggler-icon"></span>
        </button>

        <div class="collapse navbar-collapse" id="navbarToggleExternalContent">
            <div class="navbar-nav">
                <a class="nav-item nav-link" href="monitoring.html">Monitoring</a>
                <a class="nav-item nav-link" href="control.php">Steuerung</a>
                <a class="nav-item nav-link active" href="solar.php">Solar</a>
                <a class="nav-item nav-link" href="/">Home</a>
            </div>
        </div>
    </nav>
    <div class="container">

        <?php
        function curl_error_verbose($ch, $result = "")
        {
            if (curl_error($ch)) {
                echo curl_error($ch);
            } else {
                $statuscode = curl_getinfo($ch, CURLINFO_HTTP_CODE);
                if ($statuscode != 200) {
                    echo ("<b>Error " . $statuscode . ":</b> <code>" . $result . "</code>");
                }
            }
        }
        // Log all errors
        ini_set('display_errors', 1);
        ini_set('display_startup_errors', 1);
        error_reporting(E_ALL);

        require "/home/david/.confidential/databaseAccess.php";
        $ch = curl_init();
        //return the transfer as a string
        curl_setopt($ch, CURLOPT_RETURNTRANSFER, 1);
        // Authentication
        curl_setopt($ch, CURLOPT_USERPWD, $username . ":" . $password);
        // Ingore certificate values
        curl_setopt($ch, CURLOPT_SSL_VERIFYPEER, false);
        curl_setopt($ch, CURLOPT_SSL_VERIFYHOST, false);
        /*
            API Calls
        */
        $path = "";
        if (isset($_POST["path"])) {
            if (strlen($_POST["path"]) > 0) {
                $path = $_POST["path"];
                $url = "http://heizung" . $path . "?" . http_build_query($_POST);
                echo "Request: <code>".$url."</code>";
                curl_setopt($ch, CURLOPT_URL, $url);
                $result = curl_exec($ch);
                curl_error_verbose($ch, $result);
            } else {
                echo ("Path will be ignored. Please check \$_POST vars:");
                var_dump($_POST);
            }
        }
        ?>
        <div>
            <h3>Solarmode</h3>
            <?php
            //var_dump($_POST);
            curl_setopt($ch, CURLOPT_URL, "http://heizung/api/solar");
            $result = curl_exec($ch);
            curl_error_verbose($ch, $result);
            $solarinfo = json_decode($result);
            $automatic = $solarinfo->modus == "automatisch";
            //var_dump($solarinfo);
            ?>
            <p>
                Betriebsmodus: <span class="badge rounded-pill text-bg-info">
                    <?php
                    echo ($solarinfo->modus);
                    ?>
                </span>
            </p>
            <form method="POST">
                <input class="d-none" name="path" value="/api/solar">
                <input class="d-none" name="manuell" value="<?php
                echo ($automatic ? "1" : "0");
                ?>" />
                <input class="btn btn-warning" type="submit" value="<?php
                echo ("Ändern zu " . ($automatic ? "manuell" : "automatisch"));
                ?>">
                </from>
                <h4>Solar ADC-Info</h4>
                <p>Das misst der ESP32</p>
                <ul>
                    <li><b>Temperatur:</b>
                        <?php echo ($solarinfo->adc_T_C); ?> °C
                    </li>
                    <li><b>Wiederstand:</b>
                        <?php echo ($solarinfo->adc_R_Ohm); ?> &Omega;
                    </li>
                    <li><b>ADC-Spannung:</b>
                        <?php echo ($solarinfo->adc_U_mV / 1000); ?> V
                    </li>
                </ul>
            </form>
        </div>
        <div>
            <h3>B-Formel</h3>
            <?php
            curl_setopt($ch, CURLOPT_URL, "http://heizung/api/ntc");
            $result = curl_exec($ch);
            curl_error_verbose($ch, $result);

            $params = json_decode($result);
            ?>
            <form method="POST">
                <input class="d-none" name="path" value="/api/ntc">
                <p><i>Kommazahlen mit punkt machen. Nicht mit komma! ESP nutzt <code>atof()</code></i></p>
                <div class="input-group mb-3">
                    <div class="input-group-text">B</div>
                    <input name="B" value="<?php echo($params->B);?>" type="number" class="form-control" step="0.01" lang="en" autocomplete="off">
                    <div class="input-group-text">_</div>
                </div>
                <div class="input-group mb-3">
                    <div class="input-group-text">T<sub>R</sub></div>
                    <input name="Tr" value="<?php echo($params->Tr);?>" type="number" class="form-control" step="0.01" lang="en" autocomplete="off">
                    <div class="input-group-text">°K</div>
                </div>
                <div class="input-group mb-3">
                    <div class="input-group-text">R<sub>R</sub></div>
                    <input name="Rr" value="<?php echo($params->Rr);?>" type="number" class="form-control" step="0.01" lang="en" autocomplete="off">
                    <div class="input-group-text">&Omega;</div>
                </div>
                <input class="btn btn-primary" type="submit" value="B-Formel Ändern"/>
            </form>
            <h3>Formeln</h3>
            <div id="b-formel"></div>
            <div id="temp-formel"></div>
            <ul>
                <li><b>T<sub>R</sub></b> ... Eine bestimmte temperatur</li>
                <li><b>R<sub>R</sub></b> ... Wiederstand bei T<sub>R</sub></li>
                <li><b>T<sub>1, 2</sub></b> ... Messwerte für B</li>
            </ul>
        </div>
    </div>
    <!-- Bootstrap include -->
    <script src="https://cdn.jsdelivr.net/npm/bootstrap@5.2.0-beta1/dist/js/bootstrap.bundle.min.js"
        integrity="sha384-pprn3073KE6tl6bjs2QrFaJGz5/SUsLqktiwsUTF55Jfv3qYSDhgCecCxMW52nD2"
        crossorigin="anonymous"></script>
    <script>
        function drawBFormula() {
            let b_formel = document.getElementById("b-formel");
            console.log(katex);
            katex.render("B = {T_1 * T_2 \\over T_2 - T_1} * ln{R_1 \\over R_2}", b_formel, {
                throwOnError: false
            });

            let temp_formel = document.getElementById("temp-formel");
            console.log(katex);
            katex.render("T_{solar} = {1 \\over {1 \\over T_R} + ln {R_{solar} \\over R_R}}", temp_formel, {
                throwOnError: false
            });
        };
        setTimeout(drawBFormula, 1000);
    </script>
</body>

</html>