<?PHP

require_once("config.php");

function escape($msg)
{
    $msg = str_replace("\\", "\\\\", $msg);
    $msg = str_replace('\"', '\"',   $msg);
    return $msg;
}

function login()
{
    global $ldaphost;
    global $baseDN;
    global $groupDN;

    if (!isset($_SERVER['PHP_AUTH_USER']) || !isset($_SERVER['PHP_AUTH_PW']))
        return "Unauthorized";

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

function execute($cmd, $out)
{
    // Execute
    $str = exec($cmd, $out, $rc);

        // Logging (mainly for debugging)
    $d = date("Y/m");
    $path = "log/".$d;

    if (!file_exists($path))
        mkdir($path, 0777, true);

    $file = fopen($path."/exec.log", "a");

    fwrite($file, date("Y-m-d H:i:s.u").": ");
    fwrite($file, $cmd);
    fwrite($file, "\n");
    if ($rc>0)
        fwrite($file, print_r($out,true)."\n");
    fwrite($file, "\n");

    fclose($file);

    return $rc;
}

// --------------------------------------------------------------------

if (isset($_GET['load']))
{
    //require_once('log/Browscap.php');

    $d = date("Y/m");

    $path = "log/".$d;

    if (!file_exists("log/cache"))
        mkdir("log/cache", 0777, true);

    if (!file_exists($path))
        mkdir($path, 0777, true);

    $addr = isset($_SERVER['REMOTE_ADDR'])     ? $_SERVER['REMOTE_ADDR']     : "";
    $user = isset($_SERVER['PHP_AUTH_USER'])   ? $_SERVER['PHP_AUTH_USER']   : "";
    $dns  = gethostbyaddr($addr);

    //$bcap = new phpbrowscap\Browscap('log/cache');
    //$info = $bcap->getBrowser();

    $file = fopen($path."/smartfact.log", "a");
    fwrite($file,
           date("Y-m-d H:i:s\t").$addr.
           "\t".//$info->Platform.
           "\t".//$info->Browser.
           "\t".//$info->Version.
           "\t".//($info->isMobileDevice?"mobile":"").
           "\t".$user.
           "\t".$dns."\n");
    fclose($file);

    // http://ip-address-lookup-v4.com/ip/92.205.118.219

    print($user);

    return;
}

if (isset($_GET['sourcelist']))
{
    $server = mysql_connect($dbhost, $dbuser, $dbpass);
    if (!$server)
        die(mysql_error());

    if (!mysql_select_db($dbname, $server))
        die(mysql_error());

    $result = mysql_query("SELECT fSourceName AS name FROM source", $server);
    if (!$result)
        die(mysql_error());


//    var res = db.query("SELECT fSourceName, fRightAscension, fDeclination ",
//              "FROM source");

   // store the record of the "example" table into $row

    // Print out the contents of the entry

    while ($row=mysql_fetch_array($result, MYSQL_NUM))
        print("'".$row[0]."'\n");

    mysql_close($server);

    return;
}

if (isset($_GET['source']) && isset($_GET['time']))
{
    // $args = "filename":label --arg:"key1=value" --arg:"key2=value"
    $cmd = $path.'/makedata '.escapeshellarg($_GET['source']).' '.escapeshellarg($_GET['time']);

    // Execute
    passthru($cmd, $str);

    // Logging (mainly for debugging)
    $d = date("Y/m");
    $path = "log/".$d;
    if (!file_exists($path))
        mkdir($path, 0777, true);
    $file = fopen($path."/exec.log", "a");
    fwrite($file, $cmd."\n".$str."\n\n");
    fclose($file);

    print_r($str);

    return;
}

if (isset($_GET['logout']))
{
    if (!isset($_SERVER['PHP_AUTH_USER']) || !isset($_SERVER['PHP_AUTH_PW']))
        return;

    return header('HTTP/1.0 401 Successfull logout!');
}

// --------------------------------------------------------------------

if (!isset($_GET['start']) && !isset($_GET['stop']) && !isset($_GET['interrupt']))
    return header('HTTP/1.0 400 Command not supported');

// --------------------------------------------------------------------

$rc = login();
if ($rc!="")
{
    header('WWW-Authenticate: Basic realm="SmartFACT++"');
    header('HTTP/1.0 401 '.$rc);
    return;
}

// --------------------------------------------------------------------

$out = array();

if (isset($_GET['stop']))
{
    unset($_GET['stop']);

    $cmd = $path."/dimctrl --no-log --user '".$_SERVER['PHP_AUTH_USER']."' --stop 2>&1";

    $rc = execute($cmd, $out);
}

if (isset($_GET['start']))
{
    // Filename
    $script = '"scripts/'.$_GET['start'].'"';

    unset($_GET['start']);

    /*
     $args = "";
     foreach ($_GET as $key => $value)
        $args .= " --arg:".$key."=".$value;
     $str = exec($path."/dimctrl --exec ".$args, $out, $rc);
     */

    // Label
    if (isset($_GET['label']))
    {
        if ($_GET['label']>=0)
            $script .= ":".$_GET['label'];
        unset($_GET['label']);
    }

    $msg = "";
    if (isset($_GET['msg']))
    {
        $msg = $_GET['msg'];
        unset($_GET['msg']);
    }

    // Arguments
    if (!empty($script) && empty($msg))
    {
        //foreach ($_GET as $key => $value)
        //    $args .= ' --arg:"'.$key.'='.escape($value).'"';

        $args = "";
        foreach ($_GET as $key => $value)
            $args .= ' "'.$key.'"="'.$value.'"';

        // $args = "filename":label --arg:"key1=value" --arg:"key2=value"
        $cmd = $path.'/dimctrl --no-log --user "'.$_SERVER['PHP_AUTH_USER'].'"  --start '.escapeshellarg($script.$args). " 2>&1";

	$rc = execute($cmd, $out);
    }

    if (!empty($msg))
    {
        $msg = escape($msg);

        // $args = "filename":label --arg:"key1=value" --arg:"key2=value"
        $cmd = $path.'/dimctrl --no-log --user "'.$_SERVER['PHP_AUTH_USER'].'"  --msg '.escapeshellarg($msg)." 2>&1";

        $rc = execute($cmd, $out);
    }

    // -------------------------------------------
}

if (isset($_GET['interrupt']))
{
    $irq = $_GET['interrupt'];
    unset($_GET['interrupt']);

    $args = "";
    foreach ($_GET as $key => $value)
        $args .= ' "'.$key.'"="'.$value.'"';

    $cmd = $path.'/dimctrl --no-log --user "'.$_SERVER['PHP_AUTH_USER'].'"  --interrupt '.escapeshellarg($irq.$args)." 2>&1";

    $rc = execute($cmd, $out);
}

if ($rc>1)
    return header('HTTP/1.0 500 Execution failed [rc='.$rc."]");
if ($rc==1)
    return header('HTTP/1.0 500 Sending command failed.');

print($_SERVER['PHP_AUTH_USER']);

if (isset($_GET['debug']))
{
    print("\n");
    print_r($out);
}

?>
