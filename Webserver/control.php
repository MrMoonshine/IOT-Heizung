<!DOCTYPE html>
<html lang="de">

<head>
    <link rel="stylesheet" type="text/css" href="styles/universal-rounded.css">
    <link rel="stylesheet" type="text/css" href="styles/input-rounded.css">
    <link rel="stylesheet" type="text/css" href="styles/checks.css">
    <link rel="stylesheet" type="text/css" href="/styles/graph.css">
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Heizung</title>
</head>

<body>
    <nav class="navbar navbar-expand-lg navbar-light bg-light">
        <h1 class="navbar-brand" href="#">Monitoring Heizung</h1>
        <label for="navhide">Ausklappen</label>
        <input type="checkbox" id="navhide" hidden>
        <div class="navbar-collapse">
            <ul>
                <li>
                    <a href="monitoring.html">Monitoring</a>
                </li>
                <li>
                    <a href="/"><img src="http://alpakagott/assets/icon.gif" alt=""> Eingangshalle</a>
                </li>
            </ul>
        </div>
    </nav>
    <div class="dashboard">
        <div class="widget">
            <h3>Pumpensteuerung</h3>

            <!--<script src="pumps.js"></script>-->
        </div>
        <div class="widget">
            <h3>Reloads</h3>
            <table>
                <thead>
                    <tr>
                        <th>Status</th>
                        <th>Time</th>
                        <th>Reason</th>
                        <th>User</th>
                        <th>Comment</th>
                        <th></th>
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
                        print("<td><div class=\"badge badge-ok\">OK</div></td>");
                    }else{
                        print("<td><div class=\"badge badge-warning\">WARNING</div></td>");
                    }
                    print("<td><code>".$row["reload"]."</code></td>");
                    print("<td><b>".$row["reason"]."</b></td>");
                    print("<td>".$row["user"]."</td>");
                    print("<td><input type=\"text\" value=\"".$row["comment"]."\" /></td>");
                    print("<td><button class=\"button-reference\"> &#x2714; </button></td>");
                    print("</tr>");
                }
                unset($row);
            ?>
            </table>
        </div>
        <article class="widget">
            <h3>Temperaturdatenbank Ausräumen</h3>
            <p>Alle daten die älter als eine woche sind löschen:</p>
            <a href="dbsta.php?clean=">
                <button class="actor">Ausräumen</button>
            </a>
        </article>
    </div>
</body>

</html>