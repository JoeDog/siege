<?php
$key = "exes";
$sec = 15; 
$str = "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX";

if (isset($_COOKIE[$key])) {
  $tmp = $_COOKIE[$key].$str;
  setcookie($key, $tmp);
} else {
  setcookie($key, $str, time()+$sec);
}

print "Cookie value is: ";
print $_COOKIE[$key];
print "<br />\n";

?>
