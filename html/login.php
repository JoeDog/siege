<?php
  main();

  function main(){
    $arr = array();
    $arr = $_GET + $_POST;
    foreach($arr as $key => $val){
      $arr[$key] = $vay;
      switch($key){
        case "username":
          $username = $val;
          break;
        case "password":
          $password = $val;
          break;
      }
    }
    if(empty($username) && empty($password)){
      login();
      return;
    }
    if($username == "siege" && $password == "haha"){
      success();
      return;
    } else {
      header('HTTP/1.1 403 Forbidden', true, 403);
      print <<<END
<html>
<head><title>SIEGE: Access denied</title></head>
<body>
<h2>Access denied</h2>
Seriously. We provided you with login credentials. How did you mess that up?
</body>
</html>
END;
      exit; 
    }
  }
 
  function success(){
    print <<<END
<HTML>
  <head><title>SIEGE: Successful login</title></head>
  <body>
    <h2>Logged in as "siege"</h2>
    Congratulations. You are able to penetrate our defenses by entering the <br>username and password combination that we provided you on the login page.
  </body>
</HTML>  
END;
  }

  function login(){
    print <<<END
    <HTML>
  <head><title>SIEGE: Login Page</title></head>
  <body>
    <h2>Restricted area</h2>
    <form name='login' action='login.php' method='GET'>
    <table>
      <tr>
        <td colspan='2'>Welcome to the top-secret siege login page. To login, user the following credentials:<br><br>
                        <code>
                        username: siege<br>
                        password: haha
                        </code>
                        <br><br>
                        This page accepts both GET and POST requests. You may construct siege URLs in either manner:<br><br>
                        <code>
                        siege -c1 -r1 "http://my.server.com/login.php?username=siege&password=haha"<br>
                        </code>OR<br>
                        <code>
                        siege -c1 -r1 "http://my.server.com/login.php POST username=siege&password=haha"
                        </code>
                        <br><br> 
      </tr>
      <tr>
        <td>Username: </td><td><input type='text' name='username' value='' size='30'></td>
      </tr>
      <tr>
        <td>Password: </td><td><input type='password' name='password' value='' size='30'></td>
      </tr>
      <tr>
        <td></td><td><input type='submit' name='submit' value=' OK '></td>
      </tr>
    </table> 
    </form>
  </body>
</HTML> 
END;
  }

?>
