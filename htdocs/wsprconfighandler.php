<?php

    include "kmtools.php";
    
    if($_SERVER['REQUEST_METHOD'] == "POST")
    {
        $password = $_POST['password'];
        if($password == $my_password)
        {
            if(isset($_POST['save']))
            {
                saveConfig();
            }
            if(isset($_POST['backup']))
            {
                backup();
            }
            if(isset($_POST['upload']))
            {
                Upload();
            }
        }
        else
        {
            header( "refresh:5;url=wspr_setup.html" );
            $infotext = "unauthorized access, wrong password";
            echo "<p style='font-size:30px; color:red;'>".$infotext."</p>";
        }

    }
    else
    {
        header( "refresh:1;url=wspr_setup.html" );
    }

    
    function backup()
    {
        $file = "phpdir/wsprconfig.js"; 
        header("Content-Description: File Transfer"); 
        header("Content-Type: application/octet-stream"); 
        header("Content-Disposition: attachment; filename=".basename($file)); 
        readfile ($file);
        exit();
    }
    
    function Upload()
    {
        header( "refresh:3;url=wspr_setup.html" );

        $target_dir = "phpdir/";
        $target_file = $target_dir.basename($_FILES["fileToUpload"]["name"]);
        $infotext = " ";
        if($target_file == $target_dir."wsprconfig.js")
        {
            if (move_uploaded_file($_FILES["fileToUpload"]["tmp_name"], $target_file)) {
                $infotext = "OK: The file ". basename( $_FILES["fileToUpload"]["name"]). " has been uploaded.";
            } else {
                $infotext = "ERROR: there was an error uploading your file.";
            }
        }
        else
            $infotext = "ERROR: the file ".basename($_FILES["fileToUpload"]["name"])." is not a WebWSPR configuration file";

        echo "<p style='font-size:30px; color:red;'>".$infotext."</p>";
    }
    
    
    function saveConfig()
    {
        // reload the page after 2 seconds
        $myrnd = mt_rand();
        header( "refresh:2;url=wspr_setup.html?reload=".$myrnd);
        
        // read the user entries
        $call = checkCall($_POST['call']);
        $qthloc = checkQthloc($_POST['qthloc']);
        $txmode = $_POST['txmode'];
        $txpower = $_POST['txpower'];
        $txoffset = checkNumber($_POST['txoffset'],1390,1610);
        $autoWF = $_POST['autoWF'];
        $levelWF = checkNumber($_POST['levelWF'],-10000,10000);
        $gainWF = checkNumber($_POST['gainWF'],-10000,10000);
        $midHF = checkNumber($_POST['midHF'],1,20);
        $call_ur1 = checkCallempty($_POST['call_ur1']);
        $call_ur2 = checkCallempty($_POST['call_ur2']);
        $call_ur3 = checkCallempty($_POST['call_ur3']);
        $call_ur4 = checkCallempty($_POST['call_ur4']);
        $call_ur5 = checkCallempty($_POST['call_ur5']);
        $call_ur_sel = $_POST['call_ur_sel'];
        $secondsPerLine = $_POST['secondsPerLine'];
        $FrequencyLeft = checkNumber($_POST['FrequencyLeft'],0,12000);
        $FrequencyRight = checkNumber($_POST['FrequencyRight'],0,12000);
        if($FrequencyRight < ($FrequencyLeft+50)) $FrequencyRight = $FrequencyLeft + 50;
        $civ_adr = $_POST['civ_adr'];
        $webtitle = $_POST['webtitle'];
        $dds_txpwr = checkNumber($_POST['dds_txpwr'],0,1000);
        $dds_cal = checkNumber($_POST['dds_cal'],-10000,10000);
        $dds_if = checkNumber($_POST['dds_if'],0,50000);
        $hamlib_trx = $_POST['hamlib_trx'];
        $sndcard = $_POST['sndcard'];
        
        // read configfile and split into a stringarray line by line
        $cfg = file_get_contents("phpdir/wsprconfig.js");
        $cfgarr = explode("\n",$cfg);
        
        // search for the Config Elements and replace the parameters
        $len = count($cfgarr);
        
        for($i=0; $i<$len; $i++)
        {
            setElement($cfgarr[$i],"call:",strtoupper($call));
            setElement($cfgarr[$i],"qthloc:",strtoupper($qthloc));
            setElement($cfgarr[$i],"txmode:",$txmode);
            setElement($cfgarr[$i],"txpower:",$txpower);
            setElement($cfgarr[$i],"txoffset:",$txoffset);
            setElement($cfgarr[$i],"autoWF:",$autoWF);
            setElement($cfgarr[$i],"levelWF:",$levelWF);
            setElement($cfgarr[$i],"gainWF:",$gainWF);
            setElement($cfgarr[$i],"midHF:",$midHF);
            setElement($cfgarr[$i],"call_ur1:",strtoupper($call_ur1));
            setElement($cfgarr[$i],"call_ur2:",strtoupper($call_ur2));
            setElement($cfgarr[$i],"call_ur3:",strtoupper($call_ur3));
            setElement($cfgarr[$i],"call_ur4:",strtoupper($call_ur4));
            setElement($cfgarr[$i],"call_ur5:",strtoupper($call_ur5));
            setElement($cfgarr[$i],"call_ur_sel:",$call_ur_sel);
            setElement($cfgarr[$i],"secondsPerLine:",$secondsPerLine);
            setElement($cfgarr[$i],"FrequencyLeft:",$FrequencyLeft);
            setElement($cfgarr[$i],"FrequencyRight:",$FrequencyRight);
            setElement($cfgarr[$i],"civ_adr:",$civ_adr);
            setElement($cfgarr[$i],"webtitle:",$webtitle);
            setElement($cfgarr[$i],"dds_txpwr:",$dds_txpwr);
            setElement($cfgarr[$i],"dds_cal:",$dds_cal);
            setElement($cfgarr[$i],"dds_if:",$dds_if);
            setElement($cfgarr[$i],"hamlib_trx:",$hamlib_trx);
            setElement($cfgarr[$i],"sndcard:",$sndcard);
            
            $cfgarr[$i] = $cfgarr[$i]."\n";
        }
        
        // copy into new array and ignore empty lines
        $newarr = array();
        for($i=0; $i<$len; $i++)
        {
            if(strlen($cfgarr[$i]) > 1)
                $newarr[] = $cfgarr[$i];
        }
        
        file_put_contents("phpdir/wsprconfig.js",$newarr);
        
        // tell the WSPR Server to update the config immediately
        // do this by creating an empty file for signaling
        $dummy = 1234;
        file_put_contents("phpdir/updateconfig.cmd",$dummy);
        file_put_contents("phpdir/updatelists.cmd",$dummy);

        $infotext = "OK: Data received, updating config ...";
        echo "<p style='font-size:30px; color:red;'>".$infotext."</p>";
    }
    
?>
