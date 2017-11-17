//@ mysql module: exports
|Exported Items: 3|
|getClassicSession: OK|
|getSession: OK|
|help: OK|

//@<OUT> help
Use this module to create a session using the traditional MySQL Protocol, for
example for MySQL Servers where the X Protocol is not available.

Note that the API interface on this module is very limited, even you can load
schemas, tables and views as objects there are no operations available on them.

The purpose of this module is to allow SQL Execution on MySQL Servers where the
X Protocol is not enabled.

To use the properties and functions available on this module you first need to
import it.

When running the shell in interactive mode, this module is automatically
imported.The following functions are currently supported.

 - getClassicSession Opens a classic MySQL protocol session to a MySQL server.
 - getSession        Opens a classic MySQL protocol session to a MySQL server.
 - help              Provides help about this class and it's members

//@<OUT> Help on getClassicSession
Opens a classic MySQL protocol session to a MySQL server.

SYNTAX

  <mysql>.getClassicSession(connectionData[, password])

WHERE

  connectionData: The connection data for the session
  password: Password for the session

RETURNS

 A ClassicSession

DESCRIPTION

A ClassicSession object uses the traditional MySQL Protocol to allow executing
operations on the connected MySQL Server.

The connection data may be specified in the following formats:

 - A URI string
 - A dictionary with the connection options

A basic URI string has the following format:

[scheme://][user[:password]@]<host[:port]|socket>[/schema][?option=value&option=value...]

The following options are valid for use either in a URI or in a dictionary:

 - ssl-mode: the SSL mode to be used in the connection.
 - ssl-ca: the path to the X509 certificate authority in PEM format.
 - ssl-capath: the path to the directory that contains the X509 certificates
   authorities in PEM format.
 - ssl-cert: The path to the X509 certificate in PEM format.
 - ssl-key: The path to the X509 key in PEM format.
 - ssl-crl: The path to file that contains certificate revocation lists.
 - ssl-crlpath: The path of directory that contains certificate revocation list
   files.
 - ssl-ciphers: List of permitted ciphers to use for connection encryption.
 - tls-version: List of protocols permitted for secure connections
 - auth-method: Authentication method

When these options are defined in a URI, their values must be URL encoded.

The following options are also valid when a dictionary is used:

 - scheme: the protocol to be used on the connection.
 - user: the MySQL user name to be used on the connection.
 - dbUser: alias for user.
 - password: the password to be used on the connection.
 - dbPassword: same as password.
 - host: the hostname or IP address to be used on a TCP connection.
 - port: the port to be used in a TCP connection.
 - socket: the socket file name to be used on a connection through unix
   sockets.
 - schema: the schema to be selected once the connection is done.

The connection options are case insensitive and can only be defined once.

If an option is defined more than once, an error will be generated.

For additional information on connection data use \? connection.

//@# getClassicSession errors
||Invalid number of arguments in mysql.getClassicSession, expected 1 to 2 but got 0
||Invalid number of arguments in mysql.getClassicSession, expected 1 to 2 but got 3
||mysql.getClassicSession: Invalid connection options, expected either a URI or a Dictionary.
||mysql.getClassicSession: Argument #2 is expected to be a string

//@<OUT> Help on getSession
Opens a classic MySQL protocol session to a MySQL server.

SYNTAX

  <mysql>.getSession(connectionData[, password])

WHERE

  connectionData: The connection data for the session
  password: Password for the session

RETURNS

 A ClassicSession

DESCRIPTION

A ClassicSession object uses the traditional MySQL Protocol to allow executing
operations on the connected MySQL Server.

The connection data may be specified in the following formats:

 - A URI string
 - A dictionary with the connection options

A basic URI string has the following format:

[scheme://][user[:password]@]<host[:port]|socket>[/schema][?option=value&option=value...]

The following options are valid for use either in a URI or in a dictionary:

 - ssl-mode: the SSL mode to be used in the connection.
 - ssl-ca: the path to the X509 certificate authority in PEM format.
 - ssl-capath: the path to the directory that contains the X509 certificates
   authorities in PEM format.
 - ssl-cert: The path to the X509 certificate in PEM format.
 - ssl-key: The path to the X509 key in PEM format.
 - ssl-crl: The path to file that contains certificate revocation lists.
 - ssl-crlpath: The path of directory that contains certificate revocation list
   files.
 - ssl-ciphers: List of permitted ciphers to use for connection encryption.
 - tls-version: List of protocols permitted for secure connections
 - auth-method: Authentication method

When these options are defined in a URI, their values must be URL encoded.

The following options are also valid when a dictionary is used:

 - scheme: the protocol to be used on the connection.
 - user: the MySQL user name to be used on the connection.
 - dbUser: alias for user.
 - password: the password to be used on the connection.
 - dbPassword: same as password.
 - host: the hostname or IP address to be used on a TCP connection.
 - port: the port to be used in a TCP connection.
 - socket: the socket file name to be used on a connection through unix
   sockets.
 - schema: the schema to be selected once the connection is done.

The connection options are case insensitive and can only be defined once.

If an option is defined more than once, an error will be generated.

For additional information on connection data use \? connection.


//@# getSession errors
||Invalid number of arguments in mysql.getSession, expected 1 to 2 but got 0
||Invalid number of arguments in mysql.getSession, expected 1 to 2 but got 3
||mysql.getSession: Invalid connection options, expected either a URI or a Dictionary.
||mysql.getSession: Argument #2 is expected to be a string
