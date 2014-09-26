<?php
  $secs = 30;
  header("Cache-Control: s-maxage=$secs, max-age=$secs, must-revalidate, proxy-revalidate");
?>
<html>
<head><title>SIEGE: Cache control</title></head>
<body>
<h2>Siege Cache-Control</h2>
This page sets a max-age cache control for <?php echo $secs?> seconds. The header looks like this:<br>
<code>
Cache-Control: s-maxage=<?php echo $secs?>, max-age=<?php echo $secs?>, must-revalidate, proxy-revalidate
</code>
</body>
</html>


