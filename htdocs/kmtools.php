<?php
    
    $my_password = "abcd";

    function setElement(&$str,$search, $newelem)
    {
        // empty elements or commas are not allowed since it would destroy the wsprconfig.js format
        if(strlen($newelem) < 1) $newelem = "0";
        if(strstr($newelem,",") != FALSE) $newelem = "0";
    
        if(strstr($str,$search) != FALSE)
        {
            $pos = strpos($str,"'");
            if($pos === false) $pos = strpos($str,"\"");
            if($pos !== FALSE) 
            {
                $startstring = substr($str,0,$pos);
                $str = $startstring."'".$newelem."',";
            }
            else
            {
                // number between : and ,
                $pos = strpos($str,":");
                $startstring = substr($str,0,$pos+1);
                $str = $startstring." ".$newelem.",";
            }
        }
    }
    
    function getNextInterval()
    {
        $sec_epoch = time();
        // make to minutes
        $min_epoch = $sec_epoch / 60;
        // make to minutes of the current day
        $min_epoch = $min_epoch % (24*60);
        // convert to interval 
        $intv = $min_epoch / 2;
        // return next interval number
        return $intv + 1;
    }
    
    function isLetter($c)
    {
        if($c >= 'A' && $c <= 'Z') return 1;
        return 0;
    }
    
    function isNumber($c)
    {
        if($c >= '0' && $c <= '9') return 1;
        return 0;
    }
    
    function checkCall($call)
    {
        if(strlen($call) < 3 || strlen($call) > 13) return "NOCALL";
        
        $call = trim($call);
        $call = strtoupper($call);
        $len = strlen($call);
        if(strcmp($qthloc,"NOCALL"))
        {
            $ls = 0;
            $num = 0;
            for($i=0; $i<$len; $i++)
            {
                if(isLetter($call[$i])) $ls++;
                if(isNumber($call[$i])) $num++;
            }
            if($ls != 0 && $num != 0) return $call;
        }
        return "NOCALL";
    }
    
    function checkCallempty($call)
    {
        $call = checkCall($call);
        if($call == "NOCALL") return "";
        return $call;
    }
    
    function checkQthloc($qthloc)
    {
        $qthloc = trim($qthloc);
        $qthloc = strtoupper($qthloc);
        if(strcmp($qthloc,"AA00AA") && strlen($qthloc) == 6)
        {
            if( isLetter($qthloc[0]) &&
                isLetter($qthloc[1]) &&
                isNumber($qthloc[2]) &&
                isNumber($qthloc[3]) &&
                isLetter($qthloc[4]) &&
                isLetter($qthloc[5]))
                
                return $qthloc;
        }
        return "AA00AA";
    }
    
    function checkNumber($numstr, $min, $max)
    {
        if(strlen($numstr)<1) return "0";
        $iv = intval($numstr);
        if($iv == 0) return "0";
        
        if($min != -1)
        {
            if($iv < $min) return "0";
        }
        
        if($max != -1)
        {
            if($iv > $max) return "0";
        }
        return $numstr;
    }
?>
