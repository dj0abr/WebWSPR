function setRadiobuttons(mode)
{
    var myrand = Math.random();
    
    document.getElementById('gotorxspots').onclick = function(){window.location.href = "/wspr_spots.html?cb=" + myrand;}
    document.getElementById('gototxspots').onclick = function(){window.location.href = "/wspr_spotstx.html?cb=" + myrand;}
    document.getElementById('gototxstat').onclick = function(){window.location.href = "/wspr_spots_tx.html?cb=" + myrand;}
    document.getElementById('gotorxstat').onclick = function(){window.location.href = "/wspr_spots_rx.html?cb=" + myrand;}
    document.getElementById('gotostatistics').onclick = function(){window.location.href = "/wspr_spots_stat.html?cb=" + myrand;}
    document.getElementById('goto2way').onclick = function(){window.location.href = "/wspr_2way.html?cb=" + myrand;}
    
    document.getElementById('gotorxspots').checked = (mode == 0);
    document.getElementById('gotorxstat').checked = (mode == 1);
    document.getElementById('gotostatistics').checked = (mode == 2);
    document.getElementById('gototxspots').checked = (mode == 3);
    document.getElementById('gototxstat').checked = (mode == 4);
    document.getElementById('goto2way').checked = (mode == 5);
}

function printHeader_simple()
{
    var myrand = Math.random();
    
    document.getElementById('headline').innerHTML =
`
    <div class="header_mytitle">
    <div class="header_mytitle_inner_leds" id="onoff_leds"><img id="alive_icon" class="alive_icon"><img id="snd_alive_icon" class="alive_icon"><img id="dec_alive_icon" class="alive_icon"></div>
    <div class="header_mytitle_inner_title" id="header_mytitle_inner_title"></div>
    <div class="header_mytitle_inner_but" id="onoffbuttons"></div>
    </div>
    <div class="header_info" id="header_info">
        <div class="header_time" id="header_time"></div>
        <div class="header_qrg" id="header_qrg"></div>
        <div class="header_nextqrg" id="header_nextqrg"></div>
        <div class="header_time_right_tx" id="header_nexttx"></div>
        <div class="header_time_left_tx" id="header_nexttxqrg"></div>
        <div class="header_call" id="header_call"></div>
    </div>
    
    <div class="header_info" id="header_info">
        <div class="header_interval" id="header_interval">
            <div class="myBar" id="myBar"></div>
        </div>
        <div class="header_intervalTX" id="header_intervalTX">
            <div class="RXlevelBar" id="RXlevelBar"></div>
        </div>
        <div class="header_result1" id="header_result1">
        </div>
        <div class="header_space" id="header_space">
            <img src="Arrow-icon.png" alt="<->">
        </div>
        <div class="header_result2" id="header_result2">
        </div>
    </div>

<div class="listcontainer">
    <div class="linkbuts">
        <a href="index.html?cb=` + myrand + `" class="diagselbutton">WebWSPR</a>
        <a href="wspr_spots.html?cb=` + myrand + `" class="diagselbutton">Spots</a>
        <a href="wspr_ranking.html?cb=` + myrand + `" class="diagselbutton">Ranking</a>
        <a href="wspr_map.html?cb=` + myrand + `" class="diagselbutton">Map</a>
        <a href="wspr_txsettings.html?cb=` + myrand + `" class="diagselbutton">RX/TX QRG</a>
        <a href="wspr_setup.html?cb=` + myrand + `" class="diagselbutton">SETUP</a>
    </div>
    <div>
        <div class="listbuts_top">
        </div>
        <div class="listbuts_but">
        </div>
    </div>
</div>    
`;
}

// big header including radio buttons for the spot lists
function printHeader()
{
    var myrand = Math.random();
    
    document.getElementById('headline').innerHTML =
`
    <div class="header_mytitle">
    <div class="header_mytitle_inner_leds" id="onoff_leds"><img id="alive_icon" class="alive_icon"><img id="snd_alive_icon" class="alive_icon"><img id="dec_alive_icon" class="alive_icon"></div>
    <div class="header_mytitle_inner_title" id="header_mytitle_inner_title"></div>
    <div class="header_mytitle_inner_but" id="onoffbuttons"></div>
    </div>
    <div class="header_info" id="header_info">
        <div class="header_time" id="header_time"></div>
        <div class="header_qrg" id="header_qrg"></div>
        <div class="header_nextqrg" id="header_nextqrg"></div>
        <div class="header_time_right_tx" id="header_nexttx"></div>
        <div class="header_time_left_tx" id="header_nexttxqrg"></div>
        <div class="header_call" id="header_call"></div>
    </div>
    
    <div class="header_info" id="header_info">
        <div class="header_interval" id="header_interval">
            <div class="myBar" id="myBar"></div>
        </div>
        <div class="header_intervalTX" id="header_intervalTX">
            <div class="RXlevelBar" id="RXlevelBar"></div>
        </div>
        <div class="header_result1" id="header_result1">
        </div>
        <div class="header_space" id="header_space">
            <img src="Arrow-icon.png" alt="<->">
        </div>
        <div class="header_result2" id="header_result2">
        </div>
    </div>

    <div class="listcontainer">
        <div class="linkbuts">
            <a href="index.html?cb=` + myrand + `" class="diagselbutton">WebWSPR</a>
            <a href="wspr_spots.html?cb=` + myrand + `" class="diagselbutton">Spots</a>
            <a href="wspr_ranking.html?cb=` + myrand + `" class="diagselbutton">Ranking</a>
            <a href="wspr_map.html?cb=` + myrand + `" class="diagselbutton">Map</a>
            <a href="wspr_txsettings.html?cb=` + myrand + `" class="diagselbutton">RX/TX QRG</a>
            <a href="wspr_setup.html?cb=` + myrand + `" class="diagselbutton">SETUP</a>
        </div>
            <div class="listbuts">
                <div class="listbuts_top">
                    <input type="radio" class="myradio" id="gotorxspots">RX-Spots
                    <input type="radio" class="myradio" id="gotorxstat">RX-Statistics
                    <input type="radio" class="myradio" id="gotostatistics">Statistics
                </div>
                <div class="listbuts_but">
                    <input type="radio" class="myradio" id="gototxspots">TX-Spots
                    <input type="radio" class="myradio" id="gototxstat">TX-Statistics
                    <input type="radio" class="myradio" id="goto2way">RX/TX 2-way
                </div>
            </div>
        </div>    
    </div>
`;
}

function printUntertitel_map()
{
    document.getElementById('untertitel').innerHTML =
`
<form action="/bandsel.php" method="post" enctype="multipart/form-data">
    <div class="untertitel_mapsel" id="untertitel_bandsel"></div>
    <div class="untertitel_mapsel" id="untertitel_timesel"></div>
    <div class="untertitel_mapsel" id="untertitel_dirsel"></div>
</form>
`;

    createBandselListbox('bandselID','untertitel_bandsel');
    createTimeselListbox('timeselID','untertitel_bandsel');
    setBandTimeListBoxDefaults();
    createDirselListbox('dirselID','untertitel_bandsel');
}

function printUntertitel_stat()
{
    document.getElementById('untertitel').innerHTML =
`
<form action="/bandsel.php" method="post" enctype="multipart/form-data">
    <div class="untertitel_bandsel" id="untertitel_bandsel"></div>
    <div class="untertitel_bandsel" id="untertitel_timesel"></div>
</form>

<div class="untertitel1" id="untertitel1"></div>
<div class="untertitel2" id="untertitel2"></div>
`;

document.getElementById('untertitel1').style.width = ut1wid;
document.getElementById('untertitel2').style.width = ut2wid;

    createBandselListbox('bandselID','untertitel_bandsel');
    setBandTimeListBoxDefaults();
}

function printUntertitel()
{
    document.getElementById('untertitel').innerHTML =
`
<form action="/bandsel.php" method="post" enctype="multipart/form-data">
    <div class="untertitel_bandsel" id="untertitel_bandsel"></div>
    <div class="untertitel_bandsel" id="untertitel_timesel"></div>
</form>

<div class="untertitel1" id="untertitel1"></div>
<div class="untertitel2" id="untertitel2"></div>
`;

document.getElementById('untertitel1').style.width = ut1wid;
document.getElementById('untertitel2').style.width = ut2wid;

    createBandselListbox('bandselID','untertitel_bandsel');
    createTimeselListbox('timeselID','untertitel_bandsel');
    setBandTimeListBoxDefaults();
}


function footer()
{
    
        document.getElementById('header_mytitle_inner_title').innerHTML = "U02-WebWSPR V2.31";
        document.getElementById('footline').innerHTML =
`
    <a class="footnote" href="http://www.dj0abr.de">WebWSPR by DJ0ABR</a><br>
    <a class="footnote" href="http://www.physics.princeton.edu/pulsar/K1JT/">the WSPR system and WSPR decoder by K1JT</a><br>
    <a class="footnote" href="https://www.darc.de/der-club/distrikte/u/ortsverbaende/02/">U02 is a DARC club in district eastern bavaria "Bayrischer Wald"</a><br>
`;
}

// add status and on/off controls to the title
function add_onoff()
{
    createTextfield("txt_id_passwd","txt_passwd","onoffbuttons");
    createButton("but_id_on","smallbutton","onoffbuttons","START",callback_on);
    createButton("but_id_off","smallbutton","onoffbuttons","STOP",callback_off);
}

function createTextfield(txt_id, txt_class, parent_id)
{
    var txtfield = document.createElement("INPUT");
    txtfield.setAttribute("type", "text");
    txtfield.setAttribute("value", "");
    txtfield.setAttribute("name", "password");
    txtfield.id = txt_id;
    txtfield.className = txt_class;
    document.getElementById(parent_id).appendChild(txtfield);
}

function createButton(but_id, but_class, parent_id, label, callbackfunction)
{
    var but = document.createElement("BUTTON");
    var txt = document.createTextNode(label);
    but.appendChild(txt);
    but.id = but_id;
    but.className = but_class;
    but.onclick = function() {callbackfunction()};
    document.getElementById(parent_id).appendChild(but);
}

function callback_on()
{
    var pw = document.getElementById("txt_id_passwd").value;
    
    $.ajax({
        type: "POST",
        url: "onoff.php",
        data: { name: "ON", password: pw }
        }).done(function( msg ) {
    });
}

function callback_off()
{
    var pw = document.getElementById("txt_id_passwd").value;
    
    $.ajax({
        type: "POST",
        url: "onoff.php",
        data: { name: "OFF", password: pw }
        }).done(function( msg ) {
    });
}


var selectBandList;
function createBandselListbox(bandselLbID, parentID)
{
    selectBandList = document.createElement("select");
    selectBandList.id = bandselLbID;
    selectBandList.name = bandselLbID;
    selectBandList.onchange = function(){bandSelChanged();};
    selectBandList.style.height="30px";
    document.getElementById(parentID).appendChild(selectBandList);

    //Create and append the options
    for (var i = 0; i < bandindex.length; i++) {
        var option = document.createElement("option");
        option.value = bandName(i);
        option.text = bandName(i);
        selectBandList.appendChild(option);
    }
}

function bandSelChanged()
{
    clearGUI();
    var band = document.getElementById('bandselID').value;
    document.getElementById('bandselID').value = " ";   // clear selection, will be set after the ajax/php response
    
    // send selected band via php to server
    var xmlhttp = new XMLHttpRequest();
    xmlhttp.onreadystatechange = function() {
        if (this.readyState == 4 && this.status == 200) {
            // server "echos" the band
            document.getElementById('bandselID').value = this.responseText.trim();
            updateGUI();
        }
    }
    xmlhttp.open("GET", "setband.php?band="+band, true);
    xmlhttp.send();
}

var selectTimeList;
function createTimeselListbox(ID, parentID)
{
    var timearr = ["1h","2h","3h","4h","5h","6h","12h","1d","2d","4d","10d","20d","act mon"];
    selectTimeList = document.createElement("select");
    selectTimeList.id = ID;
    selectTimeList.name = ID;
    selectTimeList.onchange = function(){timeSelChanged();};
    selectTimeList.style.height="30px";
    document.getElementById(parentID).appendChild(selectTimeList);

    //Create and append the options
    for (var i = 0; i < timearr.length; i++) {
        var option = document.createElement("option");
        option.value = timearr[i];
        option.text = timearr[i];
        selectTimeList.appendChild(option);
    }
}

function timeSelChanged()
{
    clearGUI();
    var tim = document.getElementById('timeselID').value;
    document.getElementById('timeselID').value = " ";
    
    // send selected time via php to server
    var xmlhttp = new XMLHttpRequest();
    xmlhttp.onreadystatechange = function() {
        if (this.readyState == 4 && this.status == 200) {
            // server "echos" the time
            var str1 = this.responseText.trim();
            document.getElementById('timeselID').value = str1;
            
            config.listtime = str1; // update config, because it may be old from browser cache

            updateGUI();
        }
    }
    xmlhttp.open("GET", "setband.php?time="+tim, true);
    xmlhttp.send();
}

// we need to set the list box to the current band
// unfortunately this is (sometimes) old due to browser caching
// so we need to reload this information manually
var bl_xmlHttpObject = new XMLHttpRequest();
function setBandTimeListBoxDefaults()
{
    bl_xmlHttpObject.open('get',"/phpdir/wsprconfig.js?cb="+Math.random());
    bl_xmlHttpObject.onreadystatechange = bl_handler;
    bl_xmlHttpObject.send(null);
    return false;
}

function bl_handler()
{
    if (bl_xmlHttpObject.readyState == 4)
    {
        // Gesamte Datei in Zeilen aufgeteilt
        var lines = bl_xmlHttpObject.responseText.split('\n');
        for(i=0; i<lines.length; i++)
        {
            var s = lines[i];
            if(s.includes("listband"))
            {
                // replace all leading non-digits with nothin
                var number = s.replace( /^\D+/g, '');
                number = number.replace(/\',+$/g, '')
                selectBandList.value = number;
            }
            if(s.includes("listtime"))
            {
                var fi = s.indexOf("'");
                var str = s.substring(fi+1);
                var li = str.indexOf("'");
                var str1 = str.substring(0,li);
                selectTimeList.value = str1;
            }
        }
    }
}


function createDirselListbox(ID, parentID)
{
    var dirarr = ["my RX","my TX","ur RX", "ur TX"];
    var selectList = document.createElement("select");
    selectList.id = ID;
    selectList.name = ID;
    selectList.onchange = function(){dirSelChanged();};
    selectList.style.height="30px";
    document.getElementById(parentID).appendChild(selectList);

    //Create and append the options
    for (var i = 0; i < dirarr.length; i++) {
        var option = document.createElement("option");
        option.value = dirarr[i];
        option.text = dirarr[i];
        selectList.appendChild(option);
    }
    
    selectList.value = "my RX";
}

function dirSelChanged()
{
    var dir = document.getElementById('dirselID').value;
    
    if(dir == "my RX") qthfile = "QTH_MY_RX.txt"; 
    if(dir == "my TX") qthfile = "QTH_MY_TX.txt"; 
    if(dir == "ur RX") qthfile = "QTH_UR_RX.txt"; 
    if(dir == "ur TX") qthfile = "QTH_UR_TX.txt"; 
    updateGUI();
}

function updateGUI()
{
    var pagename = location.pathname.substring(1);
    if(pagename == "wspr_map.html")
        updateMap();
}

// redirect to config if call/qth has not been entered
// the wsprconfig.js can be old because of this bullsh... of browser caching
// so we need to read and evaluate the config file manually
var cfg_xmlHttpObject = new XMLHttpRequest();
function forceToConfigPage()
{
    cfg_xmlHttpObject.open('get',"/phpdir/wsprconfig.js?cb="+Math.random());
    cfg_xmlHttpObject.onreadystatechange = cfg_handler;
    cfg_xmlHttpObject.send(null);
    return false;
}

function cfg_handler()
{
    if (cfg_xmlHttpObject.readyState == 4)
    {
        // Gesamte Datei in einer Zeile
        var line = cfg_xmlHttpObject.responseText;
        if(line.includes("NOCALL") || line.includes("AA00AA"))
        {
            var pagename = location.pathname.substring(1);
            if(pagename != "wspr_setup.html")
            {
                // if we are not already at the setup page, redirect to the setup page
                window.location.href = "/wspr_setup.html?cb=" + Math.random();
            }
        }
    }
}

function clearGUI()
{
    var pagename = location.pathname.substring(1);
    if(pagename == "wspr_spots_stat.html")
        clearTable();
    if(pagename == "wspr_spots_rx.html")
        clearTable();
    if(pagename == "wspr_spots_tx.html")
        clearTable();
    if(pagename == "wspr_2way.html")
        clearTable();
}

// Show current DateTime and Time to next WSPR Interval as text and bar
function CurrentDate()
{
    document.title = config.webtitle;

    // go to setup if no call exists
    forceToConfigPage();

    // print current date/time
    var d = new Date(); 
    var datetime = "UTC: " + d.getUTCDate() + "."
                + ('0' + (d.getUTCMonth()+1)).slice(-2)  + "." 
                + ('0' + (d.getUTCFullYear())).slice(-2) + "  "  
                + ('0' + (d.getUTCHours())).slice(-2) + ":"  
                + ('0' + (d.getUTCMinutes())).slice(-2) + ":" 
                + ('0' + (d.getUTCSeconds())).slice(-2);
                
    document.getElementById("header_time").innerHTML = datetime;
    
    // this takes the time where wsprtk is running
    var korrsecond =  (actsecond+2)%120;    // correct delay for time transmission from wsprtk to here of abt 2s
    var restsec = 120 - korrsecond;
    var ttext = Math.trunc(restsec/60) + ":" + ('0' + restsec%60).slice(-2);

    document.getElementById("myBar").innerHTML = ttext;

    var elem = document.getElementById("myBar");   
    elem.style.width = (restsec * 100 / 120) + '%';
    
    document.getElementById("header_call").innerHTML = "Op.: " + config.call + " " + config.qthloc;
    
    // Update Time to TX every second
    UpdateTimeToTX();
}

var lastalivecnt = 0;
var alive = -1;
var notalivetime = 0;
var actsecond=0;

var snd_lastalivecnt = 0;
var snd_alive = -1;
var snd_notalivetime = 0;

var dec_lastalivestat = -1;

// request async read of ststus.txt by ajax
var status_xmlHttpObject = new XMLHttpRequest();
function UpdateTimeToTX()
{
    // Lade Datei mit den aktuellen Messwerten
    status_xmlHttpObject.open('get',"/phpdir/status.txt?cb="+Math.random());
    status_xmlHttpObject.onreadystatechange = status_handler;
    status_xmlHttpObject.send(null);
    return false;
}

// ajax read status.txt, handle it here
function status_handler()
{
    if (status_xmlHttpObject.readyState == 4)
    {
        // Gesamte Datei in Zeilen aufgeteilt
        var lines = status_xmlHttpObject.responseText.split('\n');
        handler_UpdateTimeToTX(lines);
    }
}

function handler_UpdateTimeToTX(sa)        
{
var txing = 0;
var mtw = 0;

    for(i=0; i<sa.length; i++)
    {
        if(i==0)
        {
            // RX Level in %
            var rxlevelpercent = Number(sa[i]);
            rxlevelpercent = 100 * (Math.log10(rxlevelpercent+10)) - 100;
            if(rxlevelpercent > 100) rxlevelpercent = 100;
            document.getElementById("RXlevelBar").innerHTML = sa[i] + "%"
            document.getElementById("RXlevelBar").style.width = rxlevelpercent + '%';
        }
        if(i==1)
        {
            // PTT status (0 or 1)
            if(Number(sa[i]) == 1)
            {
                document.getElementById("RXlevelBar").innerHTML = "TX";
                document.getElementById("RXlevelBar").style.width = '100%';
                document.getElementById("RXlevelBar").style.backgroundColor = "#FF0000";
                txing = 1;
            }
            else
            {
                document.getElementById("RXlevelBar").style.backgroundColor = "#a0a000";
            }
        }
        if(i==2)
        {
            if(config.txmode == 0)
            {
                document.getElementById("header_nexttx").innerHTML = "TX off (CAT-mode: off)";
            }
            else if(Number(sa[i]) == -1)
            {
                document.getElementById("header_nexttx").innerHTML = "TX off (no TX specified)";
            }
            else if(txing == 0)
            {
                // next TX minute (in minutes per day)
                var h = Math.floor(sa[i]/60);
                var m = Math.floor(sa[i] - h*60) & ~1;
                document.getElementById("header_nexttx").innerHTML = "next TX: " + h + ":" + ('0' + m).slice(-2);
            }
            else
            {
                document.getElementById("header_nexttx").innerHTML = "TX now";
            }
        }
        if(i==3)
        {
            // actual RXTX Frequency
            document.getElementById("header_qrg").innerHTML = "QRG: " + sa[i]/1000000 + "MHz";
        }
        if(i==4)
        {
            // next RXTX Frequency
            document.getElementById("header_nextqrg").innerHTML = "(next:" + (sa[i]/1000000).toFixed(4) + "MHz)";
        }
        if(i==5)
        {
            // next RXTX Frequency
            if(Number(sa[i]) == -1)
            {
                document.getElementById("header_nexttxqrg").innerHTML = "";
            }
            else
            {
                var nextf = sa[i]/1000000;
                if(nextf > 0.1)
                {
                    document.getElementById("header_nexttxqrg").innerHTML = "(" + nextf.toFixed(4) + "MHz)";
                }
            }
        }
        if(i==6)
        {
            // Alive counter of wsprtk
            // the program is running if this counter counts up
            var cnt = Number(sa[i]);
            actsecond = cnt & 0xffff;
            
            // initial setting after page load
            if(alive == -1)
            {
                document.getElementById("alive_icon").src = "alive.png";
                alive = 1;
                notalivetime = 0;
            }
            
            // get alive status
            if(cnt != lastalivecnt)
            {
                // system is alive
                notalivetime = 0;
                // draw icon if system was not alive before
                if(alive == 0)
                {
                    document.getElementById("alive_icon").src = "alive.png";
                    alive = 1;
                }
            }
            else
            {
                // system may be not alive, make additional checks
                if(++notalivetime >= 2)
                {
                    // system is not alive
                    // draw icon if it was alive before
                    if(alive == 1)
                    {
                        document.getElementById("alive_icon").src = "notalive.png";
                        alive = 0;
                    }
                }
            }
            
            lastalivecnt = cnt;
        }
        if(i==7)
        {
            // Alive counter of soundcard
            // if the soundcard is open this counter counts up
            var cnt = Number(sa[i]);
            // initial setting after page load
            if(snd_alive == -1)
            {
                document.getElementById("snd_alive_icon").src = "snd_alive.png";
                snd_alive = 1;
                snd_notalivetime = 0;
            }
            
            // get alive status
            if(cnt != snd_lastalivecnt)
            {
                // system is alive
                snd_notalivetime = 0;
                // draw icon if system was not alive before
                if(snd_alive == 0)
                {
                    document.getElementById("snd_alive_icon").src = "snd_alive.png";
                    snd_alive = 1;
                }
            }
            else
            {
                // system may be not alive, make additional checks
                if(++snd_notalivetime >= 2)
                {
                    // system is not alive
                    // draw icon if it was alive before
                    if(snd_alive == 1)
                    {
                        document.getElementById("snd_alive_icon").src = "snd_notalive.png";
                        snd_alive = 0;
                    }
                }
            }
            
            snd_lastalivecnt = cnt;
        }
        if(i==8)
        {
            // WSPR Decoder running, or not
            var stat = Number(sa[i]);
            if(dec_lastalivestat != stat)
            {
                if(dec_lastalivestat != 1 && stat == 1)
                {
                    // changed from not alive to alive
                    // show green icon
                    document.getElementById("dec_alive_icon").src = "decode.gif";
                    dec_alive = 1;
                }
                
                if(dec_lastalivestat != 0 && stat == 0)
                {
                    document.getElementById("dec_alive_icon").src = "decode_off.png";
                    dec_alive = 1;
                }
                dec_lastalivestat = stat;
            }
        }
        if(i==9)
        {
            mtw = Number(sa[i]);
        }
        if(i==10)
        {
            UpdateResult(mtw,Number(sa[i]));
        }
    }
}


function UpdateResult(nmy,nur)
{
        
    var myinfo = config.call + " ";
    var urinfo = " ";
    
    if(nmy < nur)
    {
        myinfo += "<a style='font-size:14px; color:red;'>" + nmy + "</a>";
        urinfo += "<a style='font-size:14px; color:green;'>" + nur + "</a>";
    }
    else if(nmy > nur)
    {
        myinfo += "<a style='font-size:14px; color:green;'>" + nmy + "</a>";
        urinfo += "<a style='font-size:14px; color:red;'>" + nur + "</a>";
    }
    else
    {
        myinfo += "2w:" + nmy;
        urinfo += "2w:" + nur;
    }
    
    myinfo += " ";
    if(config.call_ur_sel == 'c1') urinfo += " " + config.call_ur1;
    if(config.call_ur_sel == 'c2') urinfo += " " + config.call_ur2;
    if(config.call_ur_sel == 'c3') urinfo += " " + config.call_ur3;
    if(config.call_ur_sel == 'c4') urinfo += " " + config.call_ur4;
    if(config.call_ur_sel == 'c5') urinfo += " " + config.call_ur5;
    
    
    document.getElementById("header_result1").innerHTML = "2-way result (" + config.listtime + "): " + myinfo;
    document.getElementById("header_result2").innerHTML = urinfo;
}

function getSelUrcall()
{
    if(config.call_ur_sel == 'c1') return config.call_ur1;
    if(config.call_ur_sel == 'c2') return config.call_ur2;
    if(config.call_ur_sel == 'c3') return config.call_ur3;
    if(config.call_ur_sel == 'c4') return config.call_ur4;
    if(config.call_ur_sel == 'c5') return config.call_ur5;
    return " ";
}

// ===== loadFile() =====
// dieses Script liest ein Textfile auf dem Server und gibt den Inhalt zurück
// Usage: var mytext = loadFile("test.txt");    lese den Dateiinhalt
// document.getElementById("demo").innerHTML = mytext;  druckt ihn dort aus wo
// das Element <p id="demo"></p> steht
// synchrones lesen, besser is folgendes asynchrones
function loadFile(filePath) 
{
    var result = null;
    var xmlhttp = new XMLHttpRequest();
    xmlhttp.open("GET", filePath + "?cba=" + Math.random(), false);
    xmlhttp.send();
    if (xmlhttp.status==200) {
        result = xmlhttp.responseText;
    }
    return result;
}

/* Sample for an async file loader:

var xmlHttpObject = new XMLHttpRequest();
function loadFileAsync(filePath, handler)
{
    // Lade Datei mit den aktuellen Messwerten
    xmlHttpObject.open('get',filePath);
    xmlHttpObject.onreadystatechange = handler;
    xmlHttpObject.send(null);
    return false;
}

function handler()
{
    if (xmlHttpObject.readyState == 4)
    {
        // Gesamte Datei in Zeilen aufgeteilt
        var lines = xmlHttpObject.responseText.split('\n');
        doSomethingwiththeLinesArray(lines);
    }
}
*/

// convert QTHloc to long/lat
// some parts of this formular from: Iacopo Giangrandi, http://www.giangrandi.ch
// but evaluation of 4char QTHloc corrected
var str_chr_up = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
var str_num = "0123456789";
var lat,lon;

function qthlocToLonLat(qth)
{
    // check length
    if (qth.length != 4 && qth.length != 6)
        return 0;

    qth = qth.toUpperCase();

    // check format first 4 chars
    if ((qth.charAt(0) < 'A') || (qth.charAt(0) > 'S') || (qth.charAt(1) < 'A') || (qth.charAt(1) > 'S') ||
        (qth.charAt(2) < '0') || (qth.charAt(2) > '9') || (qth.charAt(3) < '0') || (qth.charAt(3) > '9'))
    return 0;

    // check format of 6 digit locator
    if ((qth.length == 6) && ((qth.charAt(4) < 'A') || (qth.charAt(4) > 'X') ||
        (qth.charAt(5) < 'A') || (qth.charAt(5) > 'X')))
    return 0;

    lat = str_chr_up.indexOf(qth.charAt(1)) * 10;               // 2nd digit: 10deg latitude slot.
    lon = str_chr_up.indexOf(qth.charAt(0)) * 20;               // 1st digit: 20deg longitude slot.
    lat += str_num.indexOf(qth.charAt(3)) * 1;                  // 4th digit: 1deg latitude slot.
    lon += str_num.indexOf(qth.charAt(2)) * 2;                  // 3rd digit: 2deg longitude slot.
    if (qth.length == 6)
    {
        lat += str_chr_up.indexOf(qth.charAt(5)) * 2.5 / 60;    // 6th digit: 2.5min latitude slot.
        lon += str_chr_up.indexOf(qth.charAt(4)) * 5 / 60;      // 5th digit: 5min longitude slot.
    }

    if (qth.length == 4)                                        // Get coordinates of the center of the square.
    {
        lat += 0.5 * 1;
        lon += 0.5 * 2;
    }
    else
    {
        lat += 0.5 * 2.5 / 60;
        lon += 0.5 * 5 / 60;
    }

    lat -= 90;                                                  // Locator lat/lon origin shift.
    lon -= 180;
    
    return 1;
}

function to2(numberstring)
{
    return ('0' + numberstring).slice(-2)
}

// calc WSPR offset, data=qrg in Hz
function calcOffset(qrg)
{
    var fqrg = qrg / 1000000;
    var idx = 0;
    
    if(fqrg<0.3) idx = 0;
    else if(fqrg<1) idx = 1;
    else if(fqrg<2) idx = 2;
    else if(fqrg<4) idx = 3;
    else if(fqrg<6) idx = 4;
    else if(fqrg<8) idx = 5;
    else if(fqrg<11) idx = 6;
    else if(fqrg<15) idx = 7;
    else if(fqrg<19) idx = 8;
    else if(fqrg<24) idx = 9;
    else if(fqrg<26) idx = 10;
    else if(fqrg<30) idx = 11;
    else if(fqrg<60) idx = 12;
    else if(fqrg<100) idx = 13;
    else if(fqrg<200) idx = 14;
    else if(fqrg<500) idx = 15;
    else idx = 16;
        
    return (qrg - qrglist[idx]*1000000).toFixed(0);
}

var qrglist = [
    0.1360,
    0.4742,
    1.8366,
    3.5926,
    5.3647,
    7.0386,
    10.1387,
    14.0956,
    18.1046,
    21.0946,
    24.9246,
    28.1246,
    50.2930,
    70.0910,
    144.4890,
    432.3000,
    1296.500
];

    // band index from 0 to 16
var bandindex = [
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
];

function bandName(idx)
{
    if(idx <= 14) return bandindex[idx]+"m";
    return bandindex[idx]+"cm";
}

