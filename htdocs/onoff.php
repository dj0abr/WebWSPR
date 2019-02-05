<?php

    include "kmtools.php";

    if($_SERVER['REQUEST_METHOD'] == "POST")
    {
        $onoff = $_POST['name'];
        $password = $_POST['password'];
        
        if($password == $my_password)
        {
            // start or stop the WSPR server
            file_put_contents("phpdir/onoff.cmd",$onoff);
            
            $infotext = "WSPR program switched: ".$onoff;
        }
        else
        {
            $infotext = "unauthorized access, wrong password";
        }
        echo $infotext;
    }
    
?>
