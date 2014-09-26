<?php
$realm = "siege_basic_auth";
$user  = "siege";
$pass  = "haha";

if($_SERVER['PHP_AUTH_USER'] == $user && $_SERVER['PHP_AUTH_PW'] == $pass){
?>
<HTML>
  <head><title>SIEGE: Successful login</title></head>
  <body>
    <h2>Logged in as "siege"</h2>
    Congratulations. You are able to penetrate our defenses by entering the <br>username and password combination that we provided you on the login page.
  </body>
</HTML>
<?php
} else {
  Header("WWW-Authenticate: Basic realm=\"$realm\"");
  Header('Status: 401 Unauthorized');
  echo "<h2>Authorization required!</h2>";
  echo "You can log into this page with the following credentials:<br>";
  echo "<code>Username: $user<br>";
  echo "Password: $pass<br>";
  echo "Realm (optional): $realm</code>";
  exit;
}
?> 
