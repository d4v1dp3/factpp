<?php

if (!isset($_POST['n']) || !isset($_POST['d']))
    return header('HTTP/1.0 400 Syntax error.');

require_once 'config.php';

function login()
{
    global $ldaphost;
    global $baseDN;
    global $groupDN;

    if (!isset($_SERVER['PHP_AUTH_USER']) || !isset($_SERVER['PHP_AUTH_PW']))
        return "Unauthorized.";

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

    return "Authorization failed.";
}

// --------------------------------------------------------------------

if (isset($_GET['logout']))
{
    if (!isset($_SERVER['PHP_AUTH_USER']) || !isset($_SERVER['PHP_AUTH_PW']))
        return;

    return header('HTTP/1.0 401 Successfull logout!');
}

// --------------------------------------------------------------------

$rc = login();
if ($rc!="")
{
    header('WWW-Authenticate: Basic realm="FACT Schedule"');
    return header('HTTP/1.0 401 '.$rc);
}

// ====================================================================

// This is the day/night from which the data is to be deleted
// and to which the data is to be submitted
$day  = $_POST['n'];

// This is the time of the last diabled entry (or the time from which
// on the data should be deleted/submitted)
// Note that there is no sanity check yet, therefore the data and the
// time variable must be consistent
// FIXME: This should be 11:59:59 the prev day to allow for 12:00 being
// the first possible entry, but this makes things below more complicated
$time = isset($_POST['t']) ? $_POST['t'] : "12:00:00";

// The data to be submitted
$data = json_decode($_POST['d']);

// Get user
$user = $_SERVER['PHP_AUTH_USER'];

// FIXME: Make sure that the date is valid (in the future)?

// ----------------------------------------------------------------

// Calculate the date for the next day, to have the correct
//date after midnight as well
$date = new DateTime($day);
$date->add(new DateInterval('P1D'));  // PnYnMnDTnHnMnS
$nextDay = $date->format('Y-m-d');

// ----------------------------------------------------------------

// Calculate the lower limit from which on data should be deleted.
// This is either noon (if the date is in the future) or the provided
// time (different from 12:00:00) during the night
$cut = $day." ".$time;

$d = new DateTime($cut);

// If the time lays before noon, it belongs to the next day
if ($d->format("His")<120000)
{
    $d->add(new DateInterval('P1D'));  // PnYnMnDTnHnMnS
    $cut = $d->format("Y-m-d H:i:s");
}

// ================================================================

$db = mysql_connect($dbhost,$dbuser,$dbpass);
if (!$db)
    die(mysql_error());

if (!mysql_select_db($dbname, $db))
    die(mysql_error());

$query = "SELECT * FROM MeasurementType";

$sql = mysql_query($query);
if (!$sql)
    die(mysql_error());

$measurements = array();
while($row = mysql_fetch_assoc($sql))
    $measurements[$row['fMeasurementTypeKey']] = $row;

// ----------------------------------------------------------------

// Now create the queries with the correct dates (date and time)
// from the posted data and the times therein
$queries = array();

array_push($queries, "LOCK TABLES Schedule WRITE");
array_push($queries, "DELETE FROM Schedule WHERE fStart>'".$cut."' AND DATE(ADDTIME(fStart, '-12:00')) = '".$day."'");

// ----------------------------------------------------------------

$last = $cut;

if (count($data)!=1 || !empty($data[0][0])) // empty schedule
foreach ($data as $row)
{
    $t = $row[0]; // time

    // If there is a time set (first task in an observation),
    // remember the time, if not this is just a measurement
    // within an observation so duplicate the time
    if (!isset($t))
    {
        $t = $save;
        $id++;
    }
    else
    {
        $save = $t;
        $id = 0;
        $exclusive = 0;
    }

    // Check if the time is before noon. If it is before noon,
    // it belongs to the next day
    $d = date_parse($t);
    $t = $d['hour']<12 ? $nextDay." ".$t : $day." ".$t;

    if ($d==FALSE)
        die("Could not parse time '".$t."' with date_parse.");

    // Check all but the last task in a measurement whether
    // the are not unlimited
    if ($last==$t)
    {
        if ($measurements[$m]['fIsUnlimited']==true)
            die("Unlimited task '".$measurements[$m]['fMeasurementTypeName']."' detected before end of observation.\n[".$last."|".($id-1)."]");
    }

    if ($last>$t)
        die("Times not sequential.\n[".$last."|".$t."]");

    $last = $t;

    $m = $row[1]; // measurement
    $s = $row[2]; // source
    $v = $row[3]; // value

    // Check if measurement is exclusive
    if ($measurements[$m]['fIsExclusive']==true || $exclusive>0)
    {
        if ($id>0)
            die("Task '".$measurements[$m]['fMeasurementTypeName']."' must be scheduled exclusively.\n[".$t."|".$id."]");

        $exclusive = $m+1;
    }

    // Check if task need source or must not have a source
    if ($measurements[$m]['fNeedsSource']==true && $s==0)
        die("Task '".$measurements[$m]['fMeasurementTypeName']."' needs source.\n[".$t."|".$id."]");
    if ($measurements[$m]['fNeedsSource']!=true && $s>0)
        die("Task '".$measurements[$m]['fMeasurementTypeName']."' must not have source.\n[".$t."|".$id."]");

    // Compile query
    $query = "INSERT INTO Schedule SET";
    $query .= " fStart='".$t."'";
    $query .= ",fMeasurementID=".$id;
    $query .= ",fMeasurementTypeKey=".$m;
    $query .= ",fUser='".$user."'";
    if ($s>0)
        $query .= ",fSourceKey=".$s;

    // Check if this is a valid JSON object
    if (!json_decode('{'.$v.'}'))
    {
        switch (json_last_error())
        {
        case JSON_ERROR_NONE:             break;
        case JSON_ERROR_DEPTH:            $err = 'Maximum stack depth exceeded'; break;
        case JSON_ERROR_STATE_MISMATCH:   $err = 'Invalid or malformed JSON';    break;
        case JSON_ERROR_CTRL_CHAR:        $err = 'Unexpected control character'; break;
        case JSON_ERROR_SYNTAX:           $err = 'Syntax error';                 break;
        case JSON_ERROR_UTF8:             $err = 'Malformed UTF-8 characters';   break;
        default:                          $err = 'Unknown error';                break;
        }

        if (isset($err))
            die($err." at ".$t." [entry #".($id+1)."]:\n".$v);
    }

    // PHP >= 5.5.0
    // if (!json_decode('{'.$v.'}'))
    //    die("Invalid option at ".$t.": ".$v." [JSON - ".json_last_error_msg()."]");



    if ($v)
        $query .= ",fData='".$v."'";

    // add query to the list of queries
    array_push($queries, $query);
}

array_push($queries, "UNLOCK TABLES");

// ================================================================
//                       Database interaction
// ================================================================

foreach ($queries as $query)
    if (!mysql_query($query))
        die(mysql_error());

mysql_close($db);

?>
