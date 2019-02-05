<?php

   
    /*
    all RX/TX interval information is stored in the file phpdir/txsettings.txt
    in this format (one information per line):
    
    RX bands as slected in the GUI from 2200m to 23cm:
        Line 2 ... 18 (0=off, 1=on)
    TX bands as slected in the GUI from 2200m to 23cm:
        Line 19 ... 36 (0=off, 1=on)
    main tx interval:
        Line 38 (minutes)
        
    Timer information (6 timers)
    on/off
        Line 40 ... 45 (0=off, 1=on)
    start minute
        Line 47 ... 52
    stop minute
        Line 54 ... 59
    interval
        Line 61 ... 66
    band
        Line 68 ... 73
    
    TX-Next band:
        Line 74
    this line 74 is the last line received from the GUI
    now the RXTX map is calculated from the above information
    and filled in the next 720 lines (one line per interval of the day)
    Format:
    RX or TX, space, band number
    if this interval comes from a timer, the bandnumber is multipied by 10000 (i.e. band6 = 60000)
    */

    
    function calcMapStepping($t,$rx,$tx,$mintv,$txnext,$tonoff,$tstart,$tend,$tintv,$tband)
    {
        $maparr_band = array();
        $maparr_txrx = array();
        $intv_cnt = 0;  // counts intervals from 0 to 720
        $tx_intvcnt = 1;            // counts the intervals from one ttx to the next
        
        if($txnext == 0 || strlen($txnext) == 0)
            $txnext = -1;
        
        // start with the next interval
        // get cuttent time
        $intv = getNextInterval();
        $maintxintv = $mintv / 2;    // main interval
        
        // loop over the next 24h which are 720 intervals
        for($intv_cnt=0; $intv_cnt<720; $intv_cnt++)
        {
            // is txnext set ? if yes, then $intv is a TX interval
            if($txnext != -1)
            {
                $maparr_txrx[$intv] = 1;
                $maparr_band[$intv] = $txnext-1; // $txnext is the bandnum+1 because 0 or len=0 is used by the radio buttons as "not selected"
                $tx_intvcnt = 0;
                $txnext = -1;
            }
            else
            {
                $tinfo = getTimerInfo($intv,$tx_intvcnt,$tonoff,$tstart,$tend,$tintv,$tband);
                if($tinfo != -1)
                {
                    // a timer specified this interval
                    // $tinfo ... band für this interval, if bit 7 is set then it is a TX interval otherwise RX
                    if($tinfo & 0x80)
                    {
                        // TX interval from a timer
                        $maparr_txrx[$intv] = 1;
                        $maparr_band[$intv] = ($tinfo & 0x7f) * 10000; // 10000...marks that this is a timer generated interval
                        $tx_intvcnt = 0;
                    }
                    else
                    {
                        // RX interval from a timer
                        $maparr_txrx[$intv] = 0;
                        $maparr_band[$intv] = $tinfo * 10000;
                    }
                }
                else
                {
                    // no timer is active handle default intervals
                    // is the coming interval TX or RX ?
                    if($tx_intvcnt >= $maintxintv && $maintxintv != 0)
                    {
                        // it is TX
                        $maparr_txrx[$intv] = 1;
                        $maparr_band[$intv] = _getNextTXband($tx);
                        
                        if($maparr_band[$intv] == -1)
                        {
                            // no TX band selected, use RX instead
                            $maparr_txrx[$intv] = 0;
                            $maparr_band[$intv] = _getNextRXband($rx);
                        }
                        else
                            $tx_intvcnt = 0;
                    }
                    else
                    {
                        // it is RX
                        $maparr_txrx[$intv] = 0;
                        $maparr_band[$intv] = _getNextRXband($rx);
                    }
                }
            }
            
            // goto next interval
            $intv += 1;
            $intv %= 720;
            // count from last tx
            $tx_intvcnt++;
        }
        
        // the TXRX map is now in the arrays, add it to $t
        for($i=0; $i<720; $i++)
        {
            if($maparr_txrx[$i] == 0) 
                $t = $t."RX ";
            else 
                $t = $t."TX ";
                
            $sband = $maparr_band[$i];
            /*if($sband >= 10000)
                $sband /= 10000;*/

            $t = $t.$sband."\n";
        }
        //echo $t;
        return $t;
    }
    
    function _getNextRXband($rarr)
    {
        static $defrxband = 0;
        
        for($b=0; $b<17; $b++)
        {
            $defrxband++;
            if($defrxband >= 17) $defrxband = 0;
            if($rarr[$defrxband] == 1)
            {
                // found the next active band
                return $defrxband;
            }
        }
        return 6;  // no band selected, return 40m, because one band MUST always be selected
    }
    
    function _getNextTXband($tarr)
    {
        static $deftxband = 0;
        
        for($b=0; $b<17; $b++)
        {
            $deftxband++;
            if($deftxband >= 17) $deftxband = 0;
            if($tarr[$deftxband] == 1)
            {
                // found the next active band
                return $deftxband;
            }
        }
        return -1;  // no band selected
    }
    
    // check the 6 timers
    // parameters:
    // $intv ... actual interval to check
    // $tonoff ... timer is on or off
    // $tstart ... timer start time
    // $tend ... timer end time
    // $tintv ... timer TX interval
    // $tband ... timer band
    // returns:
    // RX...band, TX...band|0x80, no timer action...-1
    function getTimerInfo($intv,$tx_intvcnt,$tonoff,$tstart,$tend,$tintv,$tband)
    {
        for($timer=0; $timer<6; $timer++)
        {
            // check if the timer is switched ON
            if($tonoff[$timer] == 1)
            {
                // get the start/end interval for this timer
                $startintv = $tstart[$timer] >> 1;
                $endintv = $tend[$timer] >> 1;
                // are we within start/end ?
                if($intv >= $startintv && $intv < $endintv)
                {
                    // this timer is valid, now check what we should do
                    
                    // is the coming interval TX or RX ?
                    if($tx_intvcnt >= ($tintv[$timer]>>1) && $tintv[$timer] != 0)
                    {
                        // it is a TX interval
                        return getBandIndex($tband[$timer]) | 0x80;
                    }
                    else
                    {
                        // it is a RX interval
                        return getBandIndex($tband[$timer]);
                    }
                }
            }
        }
        return -1;
    }
    
    function getBandIndex($bandname)
    {
        $bandlist = array(
                2200,
                630,
                160,
                80,
                60,
                40,
                30,
                20,
                17,
                15,
                12,
                10,
                6,
                4,
                2,
                70,
                23
                );
        
        for($i=0; $i<17; $i++)
        {
            if($bandlist[$i] == $bandname)
                return $i;
        }
        return 0;
    }

?>
