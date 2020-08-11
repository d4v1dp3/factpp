<?php

require_once("../smartfact/config.php");

function login()
{
    global $ldaphost;
    global $baseDN;
    global $groupDN;

    $username = $_SERVER['PHP_AUTH_USER'];
    $password = $_SERVER['PHP_AUTH_PW'];

    $con = @ldap_connect($ldaphost);
    if (!$con)
        return "ldap_connect failed to ".$ldaphost;

    //------------------ Look for user common name
    $attributes = array('cn', 'mail');
    $dn         = 'ou=People,'.$baseDN;
    $filter     = '(uid='.$username.')';

    $sr = @ldap_search($con, $dn, $filter, $attributes);
    if (!$sr)
        return "ldap_search failed for dn=".$dn.": ".ldap_error($con);

    $srData = @ldap_get_entries($con, $sr);
    if ($srData["count"]==0)
        return "No results returned by ldap_get_entries for dn=".$dn.".";

    $email         =$srData[0]['mail'][0];
    $userCommonName=$srData[0]['cn'][0];
    $userDN        =$srData[0]['dn'];

    //------------------ Authenticate user
    if (!@ldap_bind($con, $userDN, $password))
        return "ldap_bind failed: ".ldap_error($con);

    //------------------ Check if the user is in FACT ldap group
    $attributes= array("member");
    $filter= '(objectClass=*)';

    // Get all members of the group.
    $sr = @ldap_read($con, $groupDN, $filter, $attributes);
    if (!$sr)
        return "ldap_read failed for dn=".$groupDN.": ".ldap_error($con);

    // retrieve the corresponding data
    $srData = @ldap_get_entries($con, $sr);
    if ($srData["count"]==0)
        return "No results returned by ldap_get_entries for dn=".$dn.".";

    @ldap_unbind($con);

    $found = false;
    foreach ($srData[0]['member'] as $member)
        if (strpos($member, "cn=".$userCommonName.",")===0)
            return "";

    return "Sorry, your credentials don't match!";
}

/*
function ascii2entities($string)
{
    for ($i=128; $i<256; $i++)
    {
        $entity  = htmlentities(chr($i), ENT_QUOTES, 'cp1252');
        $temp    = substr($entity, 0, 1);
        $temp   .= substr($entity, -1, 1);
        $string  = str_replace(chr($i), $temp!='&;'?'':$entity, $string);
    }
    return $string;
}
*/

function ansi_decode($matches)
{
    static $colors =
        array(
              'black',
              'maroon',
              'green',
              'olive',
              'navy',
              'purple',
              'teal',
              'silver',
              'gray',
              'red',
              'lime',
              'yellow',
              'blue',
              'fuchsia',
              'aqua',
              'white'
             );

    // Default styles.
    static $styles =
        array(
              'background'   => null,  // Default is defined by the stylesheet.
              'blink'        => false,
              'bold'         => false,
              'color'        => null,  // Default is defined by the stylesheet.
              //'inverse'      => false, // Cannot be expressed in terms of CSS!
              'italic'       => false, // Not supported by DarkOwl's ANSI.
              'line-through' => false, // Not supported by DarkOwl's ANSI.
              'underline'    => false,
             );

    static $css = '';

    // Copy the previous styles.
    $newstyles = $styles;
    // Extract the codes from the escape sequences.
    preg_match_all('/\d+/', $matches[0], $matches);

    // Walk through the codes.
    foreach ($matches[0] as $code)
    {
        switch ($code)
        {
        case '0':
            // Reset all styles.
            $newstyles['background']   = null;
            $newstyles['blink']        = false;
            $newstyles['bold']         = false;
            $newstyles['color']        = null;
            //              $newstyles['inverse']      = false;
            $newstyles['italic']       = false;
            $newstyles['line-through'] = false;
            $newstyles['underline']    = false;
            break;

        case '1':
            // Set the bold style.
            $newstyles['bold'] = true;
            break;

        case '3':
            // Set the italic style.
            $newstyles['italic'] = true;
            break;

        case '4':
        case '21': // Actually double underline, but CSS doesn't support that yet.
            // Set the underline style.
            $newstyles['underline'] = true;
            break;

        case '5':
        case '6': // Actually rapid blinking, but CSS doesn't support that.
            // Set the blink style.
            $newstyles['blink'] = true;
            break;

//          case '7':
//              // Set the inverse style.
//              $newstyles['inverse'] = true;
//              break;

        case '9':
            // Set the line-through style.
            $newstyles['line-through'] = true;
            break;

        case '2': // Previously incorrectly interpreted by Pueblo/UE as cancel bold, now still supported for backward compatibility.
        case '22':
            // Reset the bold style.
            $newstyles['bold'] = false;
            break;

        case '23':
            // Reset the italic style.
            $newstyles['italic'] = false;
            break;

        case '24':
            // Reset the underline style.
            $newstyles['underline'] = false;
            break;

        case '25':
            // Reset the blink style.
            $newstyles['blink'] = false;
            break;

//          case '27':
//              // Reset the inverse style.
//              $newstyles['inverse'] = false;
//              break;

        case '29':
            // Reset the line-through style.
            $newstyles['line-through'] = false;
            break;

        case '30': case '31': case '32': case '33': case '34': case '35': case '36': case '37':
            // Set the foreground color.
            $newstyles['color'] = $code - 30;
            break;

        case '39':
            // Reset the foreground color.
            $newstyles['color'] = null;
            break;

        case '40': case '41': case '42': case '43': case '44': case '45': case '46': case '47':
            // Set the background color.
            $newstyles['background'] = $code - 40;
            break;

        case '49':
            // Reset the background color.
            $newstyles['background'] = null;
            break;

        default:
            // Unsupported code; simply ignore.
            break;
        }
    }

    // Styles are effectively unchanged; return nothing.
    if ($newstyles === $styles)
        return '';

    // Copy the new styles.
    $styles = $newstyles;
    // If there's a previous CSS in effect, close the <span>.
    $html = $css ? '</span>' : '';
    // Generate CSS.
    $css = '';

    // background-color property.
    if (!is_null($styles['background']))
        $css .= ($css ? ';' : '') . "background-color:{$colors[$styles['background']]}";

    // text-decoration property.
    if ($styles['blink'] || $styles['line-through'] || $styles['underline'])
    {
        $css .= ($css ? ';' : '') . 'text-decoration:';

        if ($styles['blink'])
            $css .= 'blink';

        if ($styles['line-through'])
            $css .= 'line-through';

        if ($styles['underline'])
            $css .= 'underline';
    }

    // font-weight property.
    if ($styles['bold'] && is_null($styles['color']))
        $css .= ($css ? ';' : '') . 'font-weight:bold';

    // color property.
    if (!is_null($styles['color']))
        $css .= ($css ? ';' : '') . "color:{$colors[$styles['color'] | $styles['bold'] << 3]}";

    // font-style property.
    if ($styles['italic'])
        $css .= ($css ? ';' : '') . 'font-style:italic';

    // Generate and return the HTML.
    if ($css)
        $html .= "<span style=\"$css\">";

    return $html;
}

function ansi2html($str)
{
    // Replace database strings
    $str = preg_replace("/\ (([[:word:].-]+)(:[^ ]+)?(@))?([[:word:].-]+)(:([[:digit:]]+))?(\/([[:word:].-]+))/", " $2$4$5$8", $str);

    // Replace special characters to their corresponding HTML entities
    //$str = ascii2entities($str);
    $str = htmlentities($str, ENT_NOQUOTES);

    // Replace ANSI codes.
    $str = preg_replace_callback('/(?:\e\[\d+(?:;\d+)*m)+/', 'ansi_decode', "$str\033[0m");

    // Strip ASCII bell.
    // $str = str_replace("\007", '', $str);

    // Replace \n
    // $str = str_replace("\n", "<br/>\n", $str);

    // Return the parsed string.
    return $str;
}

if (!isset($_SERVER['PHP_AUTH_USER']) || !isset($_SERVER['PHP_AUTH_PW']))
{
    header('WWW-Authenticate: Basic realm="SmartFACT++"');
    header('HTTP/1.1 401 Unauthorized');
    return;
}

$rc = login();
if ($rc!="")
    return header('HTTP/1.1 401 '.$rc);

$refresh = isset($_GET['refresh']) ? $_GET['refresh'] : -1;
if ($refresh>0 && $refresh<60)
   $refresh = 60;

unset($_GET['refresh']);

$prg = empty($_GET['log']) ? "dimserver" : $_GET['log'];
$dir = empty($_GET['dir']) ? "" : $_GET['dir'];

if (!strpos($prg, "/")===false || !strpos($dir, "/")===false)
{
    header('HTTP/1.1 403 Access forbidden.');
    print("HTTP/1.1 403 Access forbidden.\n");
    return;
}

if (empty($_GET['dir']))
{
    if ($prg=="schedule")
        $prg = "scripts/schedule.js";

    $filename = "/users/fact/operation/".$prg;
    if (is_link($filename))
        $filename = "/users/fact/operation/".dirname(readlink($filename))."/".$prg.".log";
}

if (empty($filename))
    $filename = "/users/fact/".$dir."/".$prg.".log";

$size = filesize($filename);

$file=array();
$fp = fopen($filename, "r");

if ($fp===false)
{
    header('HTTP/1.1 403 Access forbidden.');
    print("Access forbidden.\n");
    return;
}

fseek($fp, -min(10000000, $size), SEEK_END);
fgets($fp);
while(!feof($fp))
{
   $line = fgets($fp);
   array_push($file, $line);
}
fclose($fp);


$dir  = basename(dirname($filename));
$name = basename($filename);
?>

<!DOCTYPE HTML>
<html>
<head>
<?php
if ($refresh>0)
   print("<meta http-equiv='refresh' content='".$refresh."'>\n");
?>
<meta charset="UTF-8">
<title><?php print($dir." - ".$name);?></title>
<link rel="stylesheet" type="text/css" href="index.css" />
<script src="jquery-2.0.0.min.js" type="text/javascript"></script>
<script>
$(function(){
   $("#nav li:has(ul)").hover(function(){
      $(this).find("ul").slideDown(200);
   }, function(){
      $(this).find("ul").hide();
   });
});
</script>
</head>
<body onload="if (location.hash.length==0) location.hash = '#bottom';">
<a class="up" href="#top">go to top &uarr;</a>
<span id="nav">
   <ul>
      <li>
	 <a>Logs</a>
            <ul>
	       <li><a href="?log=biasctrl">biasctrl</a></li>
	       <li><a href="?log=agilentctrl">agilentctrl</a></li>
	       <li><a href="?log=chatserv">chatserv</a></li>
	       <li><a href="?log=datalogger">datalogger</a></li>
	       <li><a href="?log=dimserver"><b>dimserver</b></a></li>
	       <li><a href="?log=dimctrl">dimctrl</a></li>
	       <li><a href="?log=drivectrl">drivectrl</a></li>
	       <li><a href="?log=fadctrl">fadctrl</a></li>
	       <li><a href="?log=feedback">feedback</a></li>
	       <li><a href="?log=fscctrl">fscctrl</a></li>
	       <li><a href="?log=ftmctrl">ftmctrl</a></li>
	       <li><a href="?log=gcn">gcn</a></li>
	       <li><a href="?log=gpsctrl">gpsctrl</a></li>
	       <li><a href="?log=lidctrl">lidctrl</a></li>
	       <li><a href="?log=magiclidar">magiclidar</a></li>
	       <li><a href="?log=magicweather">magicweather</a></li>
	       <li><a href="?log=mcp">mcp</a></li>
	       <li><a href="?log=pwrctrl">pwrctrl</a></li>
	       <li><a href="?log=ratecontrol">ratecontrol</a></li>
	       <li><a href="?log=ratescan">ratescan</a></li>
	       <li><a href="?log=sqmctrl">sqmctrl</a></li>
	       <li><a href="?log=temperature">temperature</a></li>
	       <li><a href="?log=timecheck">timecheck</a></li>
	       <li><a href="?log=tngweather">tngweather</a></li>
	       <li><a href="?log=pfminictrl">pfminictrl</a></li>
            </ul>
      </li>
   </ul>
</span>
</span>
<a class="dn" href="#bottom">go to bottom &darr;</a>


<H2 id="top"><?php printf("%s - %s   (%dkB)", $dir, $name, $size/1000);?></H2>

<pre style="font-size:small;font-family:'Lucida Console',Monaco,monospace">
<?php
foreach ($file as $line)
    print(ansi2html(substr($line, 0, -1))."\n");
?>

</pre>
<div id="bottom"></div>
</body>
</html>
