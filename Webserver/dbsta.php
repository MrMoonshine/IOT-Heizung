<?php
require("mariadb.php");
if(isset($_GET['pump'])){
    //is fet is defined then set the pump
    if(isset($_GET['set'])){
        $usql = "UPDATE states SET stateID = :stid WHERE pumpID = :puid";
        $dbo->beginTransaction();
        $stmt = $dbo->prepare($usql);
        $stmt->execute(['puid' => $_GET['pump'],'stid' => $_GET['set']]);
        $dbo->commit();  
    }

    $sql = "SELECT stateID FROM states WHERE pumpID =" . $_GET['pump'] . " LIMIT 1"; //Select state from the pump in the get payload
    $dbo->beginTransaction();
    foreach ($dbo->query($sql) as $row) {
        //print "<br>My State is: ".$row['stateID'];
        $state = $row['stateID'];
    }
    unset($row);

    if(isset($state)){
        if($state == 0){echo "OFF";}
        else if($state == 1){echo "ON";}
        //Only for Solarpump 2 is automatic
        else if($state == 2 && $_GET['pump'] == 2){echo "AUTOMATIC";}
        else{echo "FAILURE";}
    }

}else if(isset($_GET["clean"])){
    $cleansql = "delete from temperatures where DATEDIFF(CURRENT_TIMESTAMP(), ts) > 7";
    $dbo->beginTransaction();
    $stmt = $dbo->prepare($cleansql);
    $stmt->execute();
    $dbo->commit();
    echo <<<END
        <script>
            window.location.replace("http://alpakagott/heizung/monitoring.html");
        </script>
    END; 
}else{
    echo "Insufficient payload!!!";
}
?>