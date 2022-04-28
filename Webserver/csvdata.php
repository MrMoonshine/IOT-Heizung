<?php
    //http://alpakagott/alpakaheizung/temperatures/graphs/plot.php?debug=0&red=0&blue=0&green=0&yellow=0&white=0&brown=0&room=0&solar=0&days=7
    $colors = array("red","blue","green","yellow","white","brown","room","solar");
    $sqlcols = "ts,red,blue,green,yellow,white,brown,room,solar";
    $RealNames = "ts,Vorlauf,Rucklauf,Grun,Gelb,Buffer,Heizung,Raumtempertur,Solartemperatur";
    $SOLAR_ON = 0;
    $SOLAR_OFF = 1;

    include "mariadb.php";

    function addIfExists($str){
      $outstr = "";
      //if(isset($_GET[$str])){
        $outstr .= ",";
        $outstr .= $str;
      //}
      return $outstr;
    }

    function createRowString(){
      $outstr = "";
      $outstr .= "ts";
      foreach($GLOBALS['colors'] as $col){
        $outstr .= addIfExists($col);
      }
      unset($col);

      return $outstr;
    }
    //Fetch CSV-Data from the database
    function getCSV($days){
      $cols = createRowString();
      $sql = 'SELECT '.$cols.' FROM   temperatures 
      WHERE  ts > Date_add(Sysdate(), INTERVAL -'.$days.' DAY) 
      ORDER  BY ts DESC 
      ; ';
      //echo $sql;

      $csv = $cols."\n";
      //$csv = "";
      //$csv = $RealNames."\n";
      $qrry = $GLOBALS['dbo']->query($sql);
      //var_dump($qrry);
      foreach($qrry as $row){
        $csv .= date('Y/m/d H:i', strtotime($row['ts'])); 
        foreach($GLOBALS['colors'] as $col){
          if(isset($row[$col])){
            if($col == "solar"){
              $val_t = $row[$col];

              switch($val_t){
                case "-1":
                  $val_t = "22";
                break;
                case "-2":
                  $val_t = "69";
                break;
                default:
                  ;
              }

              $csv .= ",";
              $csv .= $val_t;
            }else{
              $csv .= ",";
            $csv .= $row[$col];
            }
          }else{
            $csv .= ",NaN";
          }
        }
        unset($col);
        $csv .= "\n";
      }
      unset($row);

      return $csv;
    }

    function getSolarToggle($days){
      $sql = "SELECT id,ts
      FROM   actlog
      WHERE  ts > Date_add(Sysdate(), INTERVAL -".$days." day)  
      AND (id = ".$GLOBALS['SOLAR_ON']." OR id = ".$GLOBALS['SOLAR_OFF'].")
      ORDER  BY ts DESC 
      ;";

      $csv = "ts,action\n";

      $qrry = $GLOBALS['dbo']->query($sql);
      foreach($qrry as $row){
        $csv .= date('Y/m/d H:i:s', strtotime($row['ts'])); 
        $csv .= ",";
        //0 -> 1
        //1 -> 0
        $csv .= ($row['id'] + 1)%2;
        $csv .= "\n";
      }
      unset($row);
      return $csv;
    }

    function providejs($csv,$solardata){
      $csv_f = str_replace("\n","\\n",$csv);
      //$csv_f = "oida";
      $solar_f = str_replace("\n","\\n",$solardata);
      echo <<<EOB
        <script>
          var solartoggle = "$solar_f";
          var csv = "$csv_f";
        </script>
      EOB;
    }

    //echo nl2br(getCSV(10));
    if(!isset($_GET['days'])){
      die("<h1>Insufficient data! Please provide a amount of days!</h1>");
    }
    
    if(isset($_GET['days'])){
      $days = $_GET['days'];
      //providejs(getCSV($days),getSolarToggle($days));
      //$csv = str_replace("\n","\\n",getCSV($days));
      echo getCSV($days);
    }

    if(isset($_GET['debug'])){
      echo '<div class="subcont">';
      echo '<h2>Debug</h2>';
      var_dump($_GET);
      echo '</div>';
    }
?>