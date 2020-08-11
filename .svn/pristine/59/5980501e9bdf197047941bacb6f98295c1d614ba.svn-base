<?PHP

require_once("config.php");

function log_sql_error($query, $error)
{
    if (!file_exists("log/"))
        mkdir("log/", 0777, true);

    $file = fopen("log/mysql.log", "a");
    fwrite($file, date("Y-m-d H:i:s")."\n".$query."\n".$error."\n\n");
    fclose($file);

    return header('HTTP/1.0 500 '.$error);
}

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

if (isset($_GET['logout']))
{
    Header( "HTTP/1.0 401 Logout successfull!");
    exit();
}

if (!isset($_GET['y']) || !isset($_GET['m']))
    return;

$y = $_GET['y'];
$m = $_GET['m'];

if (!mysql_connect($dbhost, $dbuser, $dbpass))
    return log_sql_error("connect: ".$dbhost."[".$dbuser."]", mysql_error());

if (!mysql_select_db($dbname))
    return log_sql_error("select_db: ".$dbname, mysql_error());

if (isset($_GET['comment']))
{
    $query = "SELECT d, c FROM Comments WHERE y=".$y." AND m=".$m;
    if (isset($_GET['d']))
        $query .= " AND d=".$_GET['d'];

    $result = mysql_query($query);
    if (!$result)
        return log_sql_error($query, mysql_error());

    if (isset($_GET['d']))
    {
        $row = mysql_fetch_array($result, MYSQL_NUM);
        print($row[1]);
        return;
    }

    while ($row = mysql_fetch_array($result, MYSQL_NUM))
    {
        printf("%04d%02d%s", strlen($row[1]), $row[0], $row[1]);
    }

    return;
}

if (isset($_GET['d']))
{
    if (!isset($_SERVER['PHP_AUTH_USER']) || !isset($_SERVER['PHP_AUTH_PW']))
    {
        header('WWW-Authenticate: Basic realm="Shift schedule"');
        header('HTTP/1.0 401 Unauthorized');
        return;
    }

    $rc = login();
    if ($rc!="")
    {
        header('HTTP/1.0 401 '.$rc);
        return;
    }

    $d = $_GET['d'];

    if (isset($_GET['c']))
    {
        $c = $_GET['c'];

        $query = "DELETE FROM Comments WHERE y=".$y." AND m=".$m." AND d=".$d;
        if (!mysql_query($query))
            return log_sql_error($query, mysql_error());

        if (strlen($c)<=0)
            return;

        $query = "INSERT Comments SET y=".$y.", m=".$m.", d=".$d.", c='".$c."'";
        if (!mysql_query($query))
            return log_sql_error($query, mysql_error());

        return;
    }

    $u = isset($_GET['u']) ? $_GET['u'] : $_SERVER['PHP_AUTH_USER'];

    $query = "DELETE FROM Data WHERE y=".$y." AND m=".$m." AND d=".$d." AND u='".$u."'";
    if (!mysql_query($query))
        return log_sql_error($query, mysql_error());

    if (mysql_affected_rows()==0)
    {
        $x = $_GET['x'];

        $query = "INSERT Data SET y=".$y.", m=".$m.", d=".$d.", x=".$x.", u='".$u."'";
        if (!mysql_query($query))
            return log_sql_error($query, mysql_error());
    }
}

$query = "SELECT d, u, x FROM Data WHERE y=".$y." AND m=".$m;
if (isset($_GET['d']))
    $query .= " AND d=".$_GET['d'];

$result = mysql_query($query);
if (!$result)
    if (!mysql_query($query))
        return log_sql_error($query, mysql_error());

while ($row = mysql_fetch_array($result, MYSQL_NUM))
    print($row[0]."\t".$row[1]."\t".$row[2]."\n");
?>
