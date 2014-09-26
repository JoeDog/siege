<?php
  $secs = 30;
  $x  = $_COOKIE['number'];
  $x .= "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX<br>" .
        "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX<br>" .
        "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX<br>" .
        "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX<br>" .
        "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX<br>"; 
  
  if(!isset($_COOKIE['nelson'])){ 
    setcookie("nelson", 'HAHA!', time()+$secs);  
    setcookie("number",  'X'); 
  } else {
    setcookie("number",  $x); 
  }
?> 
<html>
<head><title>SIEGE: Cookie Test</title></head>
<body>
<H2>Siege Cookie Test</H2>
In verbose mode, you see siege pull down consistently more data each time it downloads<br>
the page. This pattern will continue for <?php echo $secs ?> seconds until the nelson cookie expires. The<br>
number of bytes downloaded will drop to its original size and then stair-step back up again.<br><br>
<code>
Hardy: $ siege -c1 -r100 http://hardy/cookie-test.php<br>
** SIEGE 2.66<br>
** Preparing 1 concurrent users for battle.<br>
The server is now under siege...<br>
HTTP/1.1 200   0.00 secs:     652 bytes ==> /cookie-test.php<br>
HTTP/1.1 200   0.00 secs:     653 bytes ==> /cookie-test.php<br>
HTTP/1.1 200   0.01 secs:     933 bytes ==> /cookie-test.php<br>
HTTP/1.1 200   0.01 secs:    1213 bytes ==> /cookie-test.php<br>
HTTP/1.1 200   0.01 secs:    1493 bytes ==> /cookie-test.php<br>
HTTP/1.1 200   0.02 secs:    1773 bytes ==> /cookie-test.php<br>
HTTP/1.1 200   0.01 secs:    2053 bytes ==> /cookie-test.php<br>
HTTP/1.1 200   0.01 secs:    2333 bytes ==> /cookie-test.php<br>
HTTP/1.1 200   0.02 secs:    2613 bytes ==> /cookie-test.php<br>
[...]<br>
HTTP/1.1 200 0.00 secs: 652 bytes ==> /cookie-test.php
</code>
<br><br>
Eventually, bytes will top out at page size plus MAX_COOKIE_SIZE. You can test this<br>
page manually with the reload button in your web browser.<br><br>
<?php echo $x ?>
</body>
</html>
