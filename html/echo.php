<?php
  echo $_SERVER['REQUEST_METHOD']."/siege/echo.php HTTP/1.1<br />";
  foreach (getallheaders() as $name => $value) {
    echo "$name: $value<br />";
  }

  echo "<br /><br />";
  ob_end_flush();
  flush();
  foreach (apache_response_headers() as $name => $value) {
    echo "$name: $value<br />";
  }

?>
