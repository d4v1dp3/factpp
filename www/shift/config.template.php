<?PHP

$dbhost = "mysql.host.com";
$dbuser = "user";
$dbpass = "password";
$dbname = "calendar";

if (strpos($_SERVER['PHP_SELF'], 'ToO') !== false)
    $dbname= "ToOcalendar";

$ldaphost = "161.72.93.133:389";
$baseDN   = "dc=fact,dc=iac,dc=es";
$groupDN  = "cn=Operations,ou=Application Groups,".$baseDN;

?>
