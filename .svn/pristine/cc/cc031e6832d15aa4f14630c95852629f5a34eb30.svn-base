<?php


if (!isset($_POST['n']))
   return header('HTTP/1.0 400 Syntax error.');

// ================================================================
//                       Database interaction
// ================================================================

function query($query)
{
   $sql = mysql_query($query);
   if (!$sql)
   	die(mysql_error());

   $rows = array();
   while($row = mysql_fetch_assoc($sql))
      $rows[] = $row;

   print(json_encode($rows).'\n');
}

// ----------------------------------------------------------------

$cut = "";
if (isset($_POST['t']))
{
    $cut = $_POST['n'].' '.$_POST['t'];

    $d = new DateTime($cut);
    if ($d->format("His")<120000)
    {
        $d->add(new DateInterval('P1D'));  // PnYnMnDTnHnMnS
        $cut = $d->format("Y-m-d H:i:s");
    }

    $cut = " AND fStart>'".$cut."' ";
}

// ----------------------------------------------------------------

require_once 'config.php';

$db = mysql_connect($dbhost,$dbuser,$dbpass);
if (!$db)
   die(mysql_error());

if (!mysql_select_db($dbname, $db))
   die(mysql_error());

/*mixed date_sunrise ( int $timestamp [, int $format = SUNFUNCS_RET_STRING [, float $latitude = ini_get("date.default_latitude") [, float $longitude = ini_get("date.default_longitude") [, float $zenith = ini_get("date.sunrise_zenith") [, float $gmt_offset = 0 ]]]]] )*/

query("SELECT DISTINCT(DATE(ADDTIME(fStart, '-12:00'))) AS 'd' FROM Schedule");
query("SELECT fSourceKEY AS 'key', fSourceName AS 'val' FROM Source WHERE fSourceTypeKEY=1");
query("SELECT fMeasurementTypeKEY AS 'key', fMeasurementTypeName AS 'val' FROM MeasurementType");
query("SELECT * FROM Schedule WHERE DATE(ADDTIME(fStart, '-12:00:00')) = '".$_POST['n']."'".$cut."ORDER BY fStart ASC, fMeasurementID ASC");

//sleep(3);

//print($test1."|".$test2."|".$test3."|SELECT * FROM Schedule WHERE DATE(ADDTIME(fStart, '-12:00:00')) = '".$_POST['day']."'".$cut."ORDER BY fScheduleID ASC, fMeasurementID ASC\n");

?>
