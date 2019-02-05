<?php

    include "kmtools.php";

    if(!empty($_REQUEST["band"]))
    {
        // get the q parameter from URL
        $band = $_REQUEST["band"];
        
        // delete signaling file, will be re-created when the database lists are updates
        // this is used by the map display
        unlink("phpdir/ready.sig");

        // save band in wsconfig.js file
        // read configfile and split into a stringarray line by line
        $cfg = file_get_contents("phpdir/wsprconfig.js");
        $cfgarr = explode("\n",$cfg);
        
        // search for the Config Elements and replace the parameters
        $len = count($cfgarr);
        
        for($i=0; $i<$len; $i++)
        {
            setElement($cfgarr[$i],"listband:",$band);
            
            if(($cfgarr[$i])[0] != '\n')
                $cfgarr[$i] = $cfgarr[$i]."\n";
        }
        
        file_put_contents("phpdir/wsprconfig.js",$cfgarr);
        
        // tell the WSPR Server to update the lists immediately
        // do this by creating an empty file for signaling
        $dummy = 1234;
        file_put_contents("phpdir/updatelists.cmd",$dummy);
        
        echo $band;
    }
    
    if(!empty($_REQUEST["time"]))
    {
        // get the q parameter from URL
        $tim = $_REQUEST["time"];

        // delete signaling file, will be re-created when the database lists are updates
        // this is used by the map display
        unlink("phpdir/ready.sig");

        // save band in wsconfig.js file
        // read configfile and split into a stringarray line by line
        $cfg = file_get_contents("phpdir/wsprconfig.js");
        $cfgarr = explode("\n",$cfg);
        
        // search for the Config Elements and replace the parameters
        $len = count($cfgarr);
        
        for($i=0; $i<$len; $i++)
        {
            setElement($cfgarr[$i],"listtime:",$tim);
            
            if(($cfgarr[$i])[0] != '\n')
                $cfgarr[$i] = $cfgarr[$i]."\n";
        }
        
        file_put_contents("phpdir/wsprconfig.js",$cfgarr);
        
        // tell the WSPR Server to update the lists immediately
        // do this by creating an empty file for signaling
        $dummy = 1234;
        file_put_contents("phpdir/updatelists.cmd",$dummy);
        
        echo $tim;
    }
?> 
