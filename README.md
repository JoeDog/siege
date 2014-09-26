
                       Siege README

WHAT IS IT?
-----------
Siege is an open source regression  test and  benchmark utility. 
It can stress test a single URL  with a user  defined  number of 
simulated  users, or  it can  read  many  URLs into  memory  and 
stress  them  simultaneously.  The  program  reports  the  total 
number  of  hits  recorded,  bytes  transferred,  response time, 
concurrency, and return status.  Siege supports HTTP/1.0 and 1.1 
protocols, the  GET and POST  directives,  cookies,  transaction 
logging, and basic authentication. Its features are configurable 
on a per user basis.

Most features  are configurable  with command line options which
also include  default values  to  minimize the complexity of the 
program's invocation.  Siege allows  you  to stress a web server 
with  n number of users  t number  of  times,  where n and t are 
defined  by the user.  It records the duration  time of the test
as  well as the duration of each single transaction.  It reports
the number  of  transactions,  elapsed time,  bytes transferred,
response time,  transaction rate,  concurrency and the number of
times the server responded OK, that is status code 200. 

Siege  was  designed  and  implemented  by Jeffrey Fulmer in his 
position as  Webmaster  for Armstrong World Industries.  It  was
modeled  in  part after Lincoln Stein's torture.pl and it's data
reporting  is  almost  identical.  But torture.pl does not allow 
one to stress many  URLs simultaneously;  out of that need siege
was born....

When a HTTP server is being hit by the program, it is said to be 
"under siege."


WHY DO I NEED IT?
-----------------
Siege was written for both web developers and web systems admin-
istrators.  It  allows  those individuals to test their programs 
and  their systems under duress.  As a web professional, you are 
responsible  for the intregrity of your product, yet you have no 
control  over who  accesses it.  Traffic spikes can occur at any 
moment.  How do you know if you're prepared?

Siege  will  allow you to place those programs  under duress, to 
allow  you  to  better  understand  the  load that they can with 
stand.  You'll sleep  better knowing your site can withstand the 
weight  of  400 simultaneous transactions if your site currently
peaks at 250.

A transaction  is  characterized  by the server opening a socket
for the client,  handling a request,  serving data over the wire 
and closing the socket upon completion.  It is important to note 
that  HUMAN  internet  users  take time to digest the data which
comes back to them.  Siege users do not.  In practice I've found
that 400  simultaneous  siege users  translates to at least five 
times that amount in real internet  sessions.  This is why siege
allows you to set a delay ( --delay=NUM ).  When set, each siege
user sleeps for a  random  number  of seconds between 1 and NUM.
Through your  server logs you should be  able to get the average
amount of time spent on a page.  It is  recommended that you use 
that number for your delay when simulating internet activity.


WHERE IS IT?
------------
The latest version of  siege can be obtained via  anonymous FTP:
ftp://sid.joedog.org/pub/siege/siege-latest.tar.gz

Siege is mirrored at nerf-herder:
http://nerf-herder.net/siege

Updates and announcements are distributed via freshmeat:
http://freshmeat.net/projects/siege


INSTALLATION
------------
Siege  was  built with  GNU  autoconf.  If you are familiar with
GNU software,  then you should be  comfortable  installing siege
Please consult the file INSTALL for more details.

PREREQUISITES
-------------
To  enable HTTPS  support, you  will  need openssl  installed on 
your system.  http://www.openssl.org.  For complete installation 
instructions, see section 2 of INSTALL.


DOCUMENTATION
-------------
Documentation is available in man pages  siege(1) layingsiege(1)
An html manual is included with this distribution:   manual.html

Complete documentation for siege can be found at  www.joedog.org


LICENSE
-------
Consult  the  file  COPYING  for  complete  license information.
 
Copyright (C) 2000-2009 by Jeffrey Fulmer <jeff@joedog.org>
 
Permission is  granted  to anyone to make or distribute verbatim
copies  of  this  document as received,  in any medium, provided 
that  the  copyright  notice  and  this  permission  notice  are 
preserved,  thus giving the recipient permission to redistribute 
in turn.
 
Permission  is  granted  to distribute modified versions of this
document,  or  of portions of it,  under  the above  conditions,
provided also that they carry prominent notices stating who last
changed them.
 
In addition, as a special exception, the  copyright holders give
permission to link the code of portions of this program with the
OpenSSL  library  under certain  conditions as described in each
individual  source  file,  and  distribute  linked  combinations
including the two.

You must  obey the  GNU General Public License  in all  respects
for all of the code  used  other  than OpenSSL.  If  you  modify
file(s)  with  this exception,  you may extend this exception to 
your version of the file(s), but you are not obligated to do so.  
If you do  not wish  to do so,  delete this exception  statement 
from your version. If you delete  this exception  statement from 
all source files in the program, then also delete it here.  


