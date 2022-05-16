<?php
    if(!isset($_GET["comment"])){
        echo "No check specified!  <a href=\"control.php\">Go Back</a>";
        exit();
    }else if(!isset($_GET["comment"])){
        echo "Comment not set!  <a href=\"control.php\">Go Back</a>";
        exit();
    }

    $comment = $_GET["comment"];
    if(strlen($comment) < 1){
        echo "Comment not set!  <a href=\"control.php\">Go Back</a>";
        exit();
    }
    //echo 'Current script owner: ' . get_current_user();
    $user = get_current_user();
    if($_GET["check"] == "uptime"){
        $time = $_GET["time"];
        $sql = <<<END
        update check_uptime set 
            user="$user",
            comment="$comment",
            ref=true
        where
            reload = "$time"
        ;
        END;

        echo $sql;

        require("mariadb.php");
        $dbo->beginTransaction();
        $sth = $dbo->prepare($sql);
        $sth->execute();
        $dbo->commit();
    }
    echo "<br><a href=\"control.php\">Go Back</a>";
    echo <<<END
        <script>
            window.location.replace("http://alpakagott/heizung/control.php");
        </script>
    END; 
?>