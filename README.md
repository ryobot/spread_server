spread_server
=============
liquide spreadding simulation

Set proxy in httpd.conf (apache):
----
# proxy for spread server port:12345
ProxyPass /foo http://localhost:12345/
ProxyPassReverse /foo http://localhost:12345/
----
