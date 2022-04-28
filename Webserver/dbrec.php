<?php
    require("mariadb.php");
    function isAnythingSet($arr){
        $out = 0;
        foreach($arr as $r){
            if($_GET[$r] != -127 && isset($_GET[$r])){
                $out += 1;
            }
        }
        unset($r);
        return $out;
    }

    function arr2csv($arr){
        $out = "";
        for($x = 0; $x < count($arr); $x++){
            $out .= $arr[$x];
        if($x != count($arr) - 1){
            $out .= ",";
        }
        }
        return $out;
    }

    function val2csv($arr){
        $out = "";
        for($x = 0; $x < count($arr); $x++){
            if($_GET[$arr[$x]] != -127 && isset($_GET[$arr[$x]])){
                $out .= $_GET[$arr[$x]];
            }else{
                $out .= "NULL";
            }

            if($x != count($arr) - 1){
                $out .= ",";
            }
        }
        return $out;
    }

    function pl2insertable($pl){
        if($_GET[$pl] != -127 && isset($_GET[$pl])){
            $out = $_GET[$pl];
        }else{
            $out = null;
        }
        return $out;
    }

    $temps = array("red","blue","green","yellow","white","brown","room","solar");

    if(isAnythingSet($temps)){
        //$sql = "INSERT INTO temperatures(". arr2csv($temps) . ") VALUES (:red,:blue,:green,:yellow,:white,:brown,:room,:solar)";
        $sql = "INSERT INTO temperatures(". arr2csv($temps) . ") VALUES (" . val2csv($temps) .");";
        echo $sql;
        $dbo->beginTransaction();
        $sth = $dbo->prepare($sql);
        $sth->execute();
        $dbo->commit();
    }else if(isset($_GET['reset'])){
        $sql1 = "INSERT INTO crashlog (logid) VALUES (:logid);";
        echo $sql1 . "<br>";
        $dbo->beginTransaction();
        $sth1 = $dbo->prepare($sql1);
        $sth1->execute(['logid' => $_GET['reset']]);
        $dbo->commit();
    }else{
        echo "Insufficient payload!!!";
    }
?>