<?php
  $v = "v2";
  $d = date('i');

  $etag = "ETag: \"" . $v . "." . $d . "\"";

  if(isset($_SERVER['HTTP_IF_NONE_MATCH']) && eregi($_SERVER['HTTP_IF_NONE_MATCH'], $etag)){
    header("HTTP/1.1 304 Not Modified"); 
    header($etag);
    exit(0);
  } else {
    header($etag);
    echo "<html>";
    echo "<head><title>SIEGE: Etity Tag Test</title></head>";
    echo "<body>";
    echo "<h2>Siege Entity Tags</h2>";
    echo "This page sets a entity tag every minute. The header looks like this:<br>";
    echo "<code>";
    echo "$etag";
    echo "</code><br><br>";
    echo "To test ETags response, you should enable them in your apache webserver. To do this <br>";
    echo "on apache, you should add the following directive at the server, virtual host, directory <br>";
    echo "or .htaccess level:<br><br>";
    echo "<code>";
    echo "FileETag INode MTime Size";
    echo "</code>";
    echo "</body>";
    echo "</html>";
  }
?>
