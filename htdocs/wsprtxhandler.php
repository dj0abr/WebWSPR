<?php

    include "kmtools.php";
    include "txintv.php";
    
    if($_SERVER['REQUEST_METHOD'] == "POST")
    {
        if($_POST['password'] == $my_password)
        {
            if(isset($_POST['save']))
            {
                saveValues();
            }
        }
        else
        {
            header( "refresh:5;url=wspr_txsettings.html" );
            $infotext = "unauthorized access, wrong password";
            echo "<p style='font-size:30px; color:red;'>".$infotext."</p>";
        }

    }
    else
    {
        header( "refresh:1;url=wspr_txsettings.html");
    }
    
    // variales and arrays for the GUI controls
    $rxarr = array();   // rx band selection
    $txarr = array();   // tx band selection
    $interval = 0;      // main interval
    $tonoffarr = array();// timer on/off
    $tstartarr = array();// timer start time in full minutes
    $tendarr = array(); // timer end time in full minutes
    $tintvarr = array();// timer interval
    $tbandarr = array();// timer band
    $bandmaparr = array();  // the Map
    $txrxmaparr = array();  // the Map
    
    function saveValues()
    {
        $myrnd = mt_rand();
        header( "refresh:2;url=wspr_txsettings.html?reload=".$myrnd);
        
        // read rx and tx settings
        for($i=0; $i<=16; $i++)
        {
            $number = str_pad($i,2,'0',STR_PAD_LEFT);
            $elem = "cbrx".$number;
            $rxarr[] = (isset($_POST[$elem])?1:0);

            $elemtx = "cbtx".$number;
            $txarr[] = (isset($_POST[$elemtx])?1:0);
        }
        
        // read main interval
        $interval = $_POST['actintv'];
        
        // read timer values
        for($i=0; $i<6; $i++)
        {
            // on off checkboxes
            $tnum = str_pad($i,2,'0',STR_PAD_LEFT);
            $elem = "cbtm".$tnum;
            $tonoffarr[] = (isset($_POST[$elem])?1:0);
            
            // start time
            $elem = "timstart".$i;
            $stime = $_POST[$elem];
            // stime format: 00h:00m
            $h = substr($stime,0,2);
            $m = substr($stime,4,2);
            $tstartarr[] = $h*60+$m;
            
            // end time
            $elem = "timend".$i;
            $stime = $_POST[$elem];
            // stime format: 00h:00m
            $h = substr($stime,0,2);
            $m = substr($stime,4,2);
            $tendarr[] = $h*60+$m;
            
            // interval
            $elem = "timint".$i;
            $tintvarr[] = $_POST[$elem];
            
            // band
            $elem = "timband".$i;
            $band = $_POST[$elem];
            $tbandarr[] = filter_var($band, FILTER_SANITIZE_NUMBER_INT);
        }
        
        // txnext selection
        $txnextradio = $_POST['txnextradio'];
        //echo "{".$txnextradio."}".strlen($txnextradio)."<br>";
        
        // write all data into a text string, separate everything with \n
        // rx settings
        $text = "RX\n";
        for($i=0; $i<=16; $i++) $text = $text.$rxarr[$i]."\n";
        // tx settings
        $text = $text."TX\n";
        for($i=0; $i<=16; $i++) $text = $text.$txarr[$i]."\n";
        // main interval
        $text = $text."main interval\n";
        $text = $text.$interval."\n";
        // timer onoff
        $text = $text."timer on/off\n";
        for($i=0; $i<6; $i++) $text = $text.$tonoffarr[$i]."\n";
        // timer start minute
        $text = $text."timer start minute\n";
        for($i=0; $i<6; $i++) $text = $text.$tstartarr[$i]."\n";
        // timer stop minute
        $text = $text."timer stop minute\n";
        for($i=0; $i<6; $i++) $text = $text.$tendarr[$i]."\n";
        // timer interval
        $text = $text."timer interval\n";
        for($i=0; $i<6; $i++) $text = $text.$tintvarr[$i]."\n";
        // timer band
        $text = $text."timer band\n";
        for($i=0; $i<6; $i++) $text = $text.$tbandarr[$i]."\n";

        // set $txnextradio to 99 (unselected)
        $txnextmem = $txnextradio;
        $text = $text."99\n";

        // calculate the default map
        $text = calcMapStepping($text,$rxarr,$txarr,$interval,$txnextmem,$tonoffarr,$tstartarr,$tendarr,$tintvarr,$tbandarr);
        
        // write all data into a file
        file_put_contents("phpdir/txsettings.txt",$text);
        
        
        // tell the WSPR Server to update the frequencies immediately
        // do this by creating an empty file for signaling
        $dummy = 1234;
        file_put_contents("phpdir/updateqrgs.cmd",$dummy);

        // Debuginfo
        $infotext = "OK: Data received, updating configuration<br><br>";
        /*$infotext = $infotext."RX:".json_encode($rxarr)."<br>TX:".json_encode($txarr)."<br>";
        $infotext = $infotext."Main Interval:".$interval."<br>";
        $infotext = $infotext."Timer on/off:".json_encode($tonoffarr)."<br>";
        $infotext = $infotext."Timer start:".json_encode($tstartarr)."<br>";
        $infotext = $infotext."Timer stop:".json_encode($tendarr)."<br>";
        $infotext = $infotext."Timer intervall:".json_encode($tintvarr)."<br>";
        $infotext = $infotext."Timer band:".json_encode($tbandarr)."<br>";*/$deftxband = 0;
        
        echo "<p style='font-size:30px; color:red;'>".$infotext."</p>";
        
    }
    
?>
