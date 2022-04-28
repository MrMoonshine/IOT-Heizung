<?php
    // This file defines $username and $password
    require "/home/david/.confidential/databaseAccess.php";
    //echo "User: ".$username."\n";
    //echo "passwd: ".$password."\n";
    $dsn = 'mysql:host=127.0.0.1;dbname=heizung';
      
    try {
            $dbo = new PDO($dsn, $username, $password);
    }
    catch (PDOException $e) {
            echo 'Connection failed: ' . $e->getMessage();
    }
?>
