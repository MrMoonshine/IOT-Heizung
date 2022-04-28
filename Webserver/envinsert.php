<?php
if(isset($_GET["solar"]) && isset($_GET["environment"])){
    try {
        require("mariadb.php");
        $sql = "insert into diffsolarenv (solar,environment) values (:solar,:env);";
        $dbo->beginTransaction();
        $sth = $dbo->prepare($sql);
        $sth->execute(["solar" => $_GET["solar"], "env" => $_GET["environment"]]);
        $dbo->commit();
        echo <<<END
        <script>
            window.location.replace("http://alpakagott/heizung/env.php");
        </script>
        END;
    } catch (Exception $e) {
        echo "<h1>SQL Error!</h1>";
        echo 'Caught exception: ',  $e->getMessage(), "\n<br>";
    }
}else{
    echo <<<END
        <h1>Fail</h1>
    END;
}
?>
<a href="env.php">Back</a>