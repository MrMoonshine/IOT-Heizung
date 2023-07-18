<!DOCTYPE html>
<html lang="de">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Heizung</title>
    <link href="https://cdn.jsdelivr.net/npm/bootstrap@5.2.0-beta1/dist/css/bootstrap.min.css" rel="stylesheet" integrity="sha384-0evHe/X+R7YkIZDRvuzKMRqM+OrBnVFBL6DOitfPri4tjfHxaWutUpFmBp4vmVor" crossorigin="anonymous">
</head>

<body>
    <nav class="navbar navbar-expand-lg navbar-dark bg-primary d-flex d-print-none">
        <a class="navbar-brand" href="#">&#127777; Heizungssteuerung</a>

        <button class="navbar-toggler" type="button" data-bs-toggle="collapse" data-bs-target="#navbarToggleExternalContent" aria-controls="navbarToggleExternalContent" aria-expanded="false" aria-label="Toggle navigation">
            <span class="navbar-toggler-icon"></span>
        </button>

        <div class="collapse navbar-collapse" id="navbarToggleExternalContent">
            <div class="navbar-nav">
                <a class="nav-item nav-link" href="monitoring.html">Monitoring</a>
                <a class="nav-item nav-link active" href="#">Steuerung</a>
                <a class="nav-item nav-link" href="/">Home</a>
            </div>
        </div>
    </nav>
        <div class="container">
            <h3>Pumpensteuerung</h3>
            <div class="table-responsive">
                <table class="table" id="pumptable">
                    <thead>
                        <tr>
                            <th scope="col">Pumpe</th>
                            <th scope="col">Status</th>
                            <th scope="col">Schalter</th>
                        </tr>
                    </thead>
                    <tbody>
                            <?php
                                // Log all errors
                                ini_set('display_errors', 1);
                                ini_set('display_startup_errors', 1);
                                error_reporting(E_ALL);

                                require "/home/david/.confidential/databaseAccess.php";
                                /*$pumppyload="";
                                $header = [
                                    'Authorization: Basic '.base64_encode($username.":".$password),
                                    'Connection: close'
                                ];

                                if(isset($_POST["pump"]) && isset($_POST["state"])){
                                    // Add pump in header
                                    $header[] = $_POST["pump"].": ".$_POST["state"];
                                }else{
                                    // it somehow crshes without it lol
                                    $header[] = "mode: readonly";
                                }

                                $opts = array('http' =>
                                    array(
                                        'method'  => 'POST',
                                        'header'  => $header,
                                        'timeout' => 1.2
                                    )
                                );
                                $context = stream_context_create($opts);

                                $result = file_get_contents('http://heizung/api/pumps', false, $context);*/
                                $ch = curl_init();
                                // set url
                                curl_setopt($ch, CURLOPT_URL, "http://heizung/api/pumps?".http_build_query($_POST));
                                //echo("http://heizung/api/pumps?".http_build_query($_POST));
                                //return the transfer as a string
                                curl_setopt($ch, CURLOPT_RETURNTRANSFER, 1);
                                // Authentication
                                curl_setopt($ch, CURLOPT_USERPWD, $username . ":" . $password);
                                // Ingore certificate values
                                curl_setopt($ch, CURLOPT_SSL_VERIFYPEER, false);
                                curl_setopt($ch, CURLOPT_SSL_VERIFYHOST, false);
                                $result = curl_exec($ch);
                                if(curl_error($ch)){
                                    echo curl_error($ch);
                                }else{
                                    $statuscode = curl_getinfo($ch, CURLINFO_HTTP_CODE);
                                    if($statuscode != 200){
                                        echo("<b>Error ".$statuscode.":</b> <code>".$result."</code>");
                                    }
                                }

                                curl_close($ch);
                                $jdata = json_decode($result);
                                //var_dump($result);
                                if(property_exists($jdata, "pumps")){
                                    foreach ($jdata->pumps as $a) {
                                        $name = $a->name;
                                        echo '<tr>';
                                        echo('<th scope="row">'.$name.'</th>');
                                        echo('<td>');
                                        if($a->state == 0){
                                            echo('<span class="badge rounded-pill text-bg-success">Ein</span>');
                                        }else{
                                            echo('<span class="badge rounded-pill text-bg-danger">Aus</span>');
                                        }
                                        echo('</td>');
                                        
                                        echo <<<END
                                        <td class="d-flex">
                                        <form method="POST">
                                            <input type="text" name="pump" value="$name" readonly hidden/>
                                            <input type="number" name="state" value="0" readonly hidden/>
                                            <input type="submit" class="btn btn-success" value="Ein"/>
                                        </form>
                                        <form method="POST">
                                            <input type="text" name="pump" value="$name" readonly hidden/>
                                            <input type="number" name="state" value="1" readonly hidden/>
                                            <input type="submit" class="btn btn-danger" value="Aus"/>
                                        </form>
                                        </td>
                                        END;
                                        echo '</tr>';
                                    }
                                }
                            ?>
                    </tbody>
                </table>
            </div>
        </div>
    <div class="container">
        <h3>Uptime</h3>
        <p>
            Wenn der ESP32 rebooted, dann ist das hier in der Liste. Wenn man den Grund weiß, dann kann man den Check referenzieren. Dadurch weiß man dann, dass der Reboot bekannt ist.
        </p>
        <div class="table-responsive">
        <table class="table">
            <thead>
                <tr>
                    <th scope="col">Status</th>
                    <th scope="col">Time</th>
                    <th scope="col">Reason</th>
                    <th scope="col">User</th>
                    <th scope="col">Comment</th>
                    <th scope="col"></th>
                </tr>
            </thead>
            <?php
                require("mariadb.php");
                $sql = "select reload, reason, comment, user, reftime, ref from check_uptime order by reload desc LIMIT 7;";
                $dbo->beginTransaction();
                foreach ($dbo->query($sql) as $row) {
                    //print "<br>My State is: ".$row['stateID'];
                    print("<tr>");
                    if($row["ref"]){
                        print('<td><span class="badge rounded-pill text-bg-success">OK</span></td>');
                    }else{
                        print("<form method=\"GET\" action=\"checks.php\">");
                        print('<td><span class="badge rounded-pill text-bg-warning">WARNING</span></td>');
                    }
                    print("<td><input type=\"text\" name=\"time\" value=\"".$row["reload"]."\" readonly/></td>");
                    print("<input type=\"text\" name=\"check\" value=\"uptime\" readonly hidden/>");
                    print("<td><b>".$row["reason"]."</b></td>");
                    print("<td><img class=\"ldap_picture\" alt=\"".$row["user"]."\" /></td>");
                    print("<td><input type=\"text\" name=\"comment\" value=\"".$row["comment"]."\" ");
                    if($row["ref"]){
                        print("disabled ");
                    }
                    print("/></td>");
                    print("<td><button class=\"btn btn-success\"> &#x2714;</button></td>");
                    if($row["ref"]){
                        print("</tr>");
                    }else{
                        print("</tr>");
                        print("</form>");
                    }
                }
                unset($row);
            ?>
            </table>
            </div>
        </div>
    <article class="container">
        <h3>Temperaturdatenbank Ausräumen</h3>
        <p>Alle daten die älter als eine woche sind löschen:</p>
        <a href="dbsta.php?clean=">
            <button class="btn btn-primary">Ausräumen</button>
        </a>
    </article>
    <!-- Bootstrap include -->
    <script src="https://cdn.jsdelivr.net/npm/bootstrap@5.2.0-beta1/dist/js/bootstrap.bundle.min.js" integrity="sha384-pprn3073KE6tl6bjs2QrFaJGz5/SUsLqktiwsUTF55Jfv3qYSDhgCecCxMW52nD2" crossorigin="anonymous"></script>
</body>
</html>