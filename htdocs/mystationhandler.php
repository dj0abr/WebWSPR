<?php

    include "kmtools.php";
    
    if($_SERVER['REQUEST_METHOD'] == "POST")
    {
        $password = $_POST['password'];
        if($password == $my_password)
        {
            saveStation();
        }
        else
        {
            header( "refresh:5;url=wspr_station.html" );
            $infotext = "unauthorized access, wrong password";
            echo "<p style='font-size:30px; color:red;'>".$infotext."</p>";
        }

    }
    else
    {
        header( "refresh:1;url=wspr_station.html" );
    }
    
    function saveStation()
    {
        // reload the page after 2 seconds
        $myrnd = mt_rand();
        header( "refresh:2;url=wspr_station.html?reload=".$myrnd);
        
        // read the user entries
        $rawtext = $_POST['rawtext'];
        
        file_put_contents("phpdir/mystation.html",$rawtext);

        $infotext = "OK: Data received, updating station info ...";
        echo "<p style='font-size:30px; color:red;'>".$infotext."</p>";
    }
    
?>
