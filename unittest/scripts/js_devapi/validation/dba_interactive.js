//@ Initialization
||

//@ Session: validating members
|Session Members: 14|
|createCluster: OK|
|deleteSandboxInstance: OK|
|deploySandboxInstance: OK|
|dropMetadataSchema: OK|
|getCluster: OK|
|help: OK|
|killSandboxInstance: OK|
|startSandboxInstance: OK|
|checkInstanceConfiguration: OK|
|stopSandboxInstance: OK|
|configureInstance: OK|
|configureLocalInstance: OK|
|verbose: OK|
|rebootClusterFromCompleteOutage: OK|

//@# Dba: createCluster errors
||Dba.createCluster: Invalid number of arguments, expected 1 to 2 but got 0
||Dba.createCluster: Invalid number of arguments, expected 1 to 2 but got 4
||Dba.createCluster: Argument #1 is expected to be a string
||Dba.createCluster: The Cluster name cannot be empty
||Dba.createCluster: Invalid options: another, invalid
||Invalid value for memberSslMode option. Supported values: AUTO,DISABLED,REQUIRED.
||Invalid value for memberSslMode option. Supported values: AUTO,DISABLED,REQUIRED.
||Cannot use memberSslMode option if adoptFromGR is set to true.
||Cannot use memberSslMode option if adoptFromGR is set to true.
||Cannot use memberSslMode option if adoptFromGR is set to true.
||Cannot use multiPrimary option if adoptFromGR is set to true. Using adoptFromGR mode will adopt the primary mode in use by the Cluster.
||Cannot use multiPrimary option if adoptFromGR is set to true. Using adoptFromGR mode will adopt the primary mode in use by the Cluster.
||Cannot use multiMaster option if adoptFromGR is set to true. Using adoptFromGR mode will adopt the primary mode in use by the Cluster.
||Cannot use multiMaster option if adoptFromGR is set to true. Using adoptFromGR mode will adopt the primary mode in use by the Cluster.
||Cannot use the multiMaster and multiPrimary options simultaneously. The multiMaster option is deprecated, please use the multiPrimary option instead.
||Cannot use the multiMaster and multiPrimary options simultaneously. The multiMaster option is deprecated, please use the multiPrimary option instead.
||Invalid value for ipWhitelist: string value cannot be empty.
||Dba.createCluster: The Cluster name can only start with an alphabetic or the '_' character.

//@ Dba: createCluster with ANSI_QUOTES success
|Current sql_mode is: ANSI_QUOTES|
|Cluster successfully created. Use Cluster.addInstance() to add MySQL instances.|
|<Cluster:devCluster>|

//@ Dba: dissolve cluster created with ansi_quotes and restore original sql_mode
|The cluster was successfully dissolved.|
|Original SQL_MODE has been restored: true|

//@ Dba: create cluster using a non existing user that authenticates as another user (BUG#26979375)
|<Cluster:devCluster>|

//@ Dba: dissolve cluster created using a non existing user that authenticates as another user (BUG#26979375)
||

//@<OUT> Dba: createCluster with interaction {VER(>=8.0.11)}
A new InnoDB cluster will be created on instance 'root@localhost:<<<__mysql_sandbox_port1>>>'.

Validating instance at localhost:<<<__mysql_sandbox_port1>>>...
Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as <<<hostname>>>

Instance configuration is suitable.
Creating InnoDB cluster 'devCluster' on 'root@localhost:<<<__mysql_sandbox_port1>>>'...

Adding Seed Instance...
Cluster successfully created. Use Cluster.addInstance() to add MySQL instances.
At least 3 instances are needed for the cluster to be able to withstand up to
one server failure.

//@<OUT> Dba: createCluster with interaction {VER(<8.0.11)}
A new InnoDB cluster will be created on instance 'root@localhost:<<<__mysql_sandbox_port1>>>'.

Validating instance at localhost:<<<__mysql_sandbox_port1>>>...
Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as <<<hostname>>>

Instance configuration is suitable.
Creating InnoDB cluster 'devCluster' on 'root@localhost:<<<__mysql_sandbox_port1>>>'...

Adding Seed Instance...
WARNING: On instance 'localhost:<<<__mysql_sandbox_port1>>>' membership change cannot be persisted since MySQL version <<<__version>>> does not support the SET PERSIST command (MySQL version >= 8.0.11 required). Please use the <Dba>.configureLocalInstance() command locally to persist the changes.
Cluster successfully created. Use Cluster.addInstance() to add MySQL instances.
At least 3 instances are needed for the cluster to be able to withstand up to
one server failure.

//@ Dba: checkInstanceConfiguration error
|Please provide the password for 'root@localhost:<<<__mysql_sandbox_port1>>>':|Dba.checkInstanceConfiguration: This function is not available through a session to an instance already in an InnoDB cluster (RuntimeError)
//@<OUT> Dba: checkInstanceConfiguration ok 1
Please provide the password for 'root@localhost:<<<__mysql_sandbox_port2>>>': Validating local MySQL instance listening at port <<<__mysql_sandbox_port2>>> for use in an InnoDB cluster...
Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as <<<hostname>>>

Checking whether existing tables comply with Group Replication requirements...
No incompatible tables detected

Checking instance configuration...
Instance configuration is compatible with InnoDB cluster

The instance 'localhost:<<<__mysql_sandbox_port2>>>' is valid for InnoDB cluster usage.

//@<OUT> Dba: checkInstanceConfiguration ok 2
Validating local MySQL instance listening at port <<<__mysql_sandbox_port2>>> for use in an InnoDB cluster...
Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as <<<hostname>>>

Checking whether existing tables comply with Group Replication requirements...
No incompatible tables detected

Checking instance configuration...
Instance configuration is compatible with InnoDB cluster

The instance 'localhost:<<<__mysql_sandbox_port2>>>' is valid for InnoDB cluster usage.


//@<OUT> Dba: checkInstanceConfiguration report with errors {VER(>=8.0.3)}
Please provide the password for 'root@localhost:<<<__mysql_sandbox_port2>>>': Validating local MySQL instance listening at port <<<__mysql_sandbox_port2>>> for use in an InnoDB cluster...
Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as <<<hostname>>>

Checking whether existing tables comply with Group Replication requirements...
No incompatible tables detected

Checking instance configuration...
Configuration file mybad.cnf will also be checked.

Some configuration options need to be fixed:
+----------------------------------+---------------+----------------+------------------------+
| Variable                         | Current Value | Required Value | Note                   |
+----------------------------------+---------------+----------------+------------------------+
| binlog_checksum                  | <not set>     | NONE           | Update the config file |
| binlog_format                    | <not set>     | ROW            | Update the config file |
| enforce_gtid_consistency         | <not set>     | ON             | Update the config file |
| gtid_mode                        | OFF           | ON             | Update the config file |
| log_slave_updates                | <not set>     | ON             | Update the config file |
| master_info_repository           | <not set>     | TABLE          | Update the config file |
| relay_log_info_repository        | <not set>     | TABLE          | Update the config file |
| report_port                      | <not set>     | <<<__mysql_sandbox_port2>>>           | Update the config file |
| server_id                        | <not set>     | <unique ID>    | Update the config file |
| transaction_write_set_extraction | <not set>     | XXHASH64       | Update the config file |
+----------------------------------+---------------+----------------+------------------------+

Please use the dba.configureInstance() command to repair these issues.

//@<OUT> Dba: checkInstanceConfiguration report with errors {VER(<8.0.3)}
Please provide the password for 'root@localhost:<<<__mysql_sandbox_port2>>>': Validating local MySQL instance listening at port <<<__mysql_sandbox_port2>>> for use in an InnoDB cluster...
Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as <<<hostname>>>

Checking whether existing tables comply with Group Replication requirements...
No incompatible tables detected

Checking instance configuration...
Configuration file mybad.cnf will also be checked.

Some configuration options need to be fixed:
+----------------------------------+---------------+----------------+------------------------+
| Variable                         | Current Value | Required Value | Note                   |
+----------------------------------+---------------+----------------+------------------------+
| binlog_checksum                  | <not set>     | NONE           | Update the config file |
| binlog_format                    | <not set>     | ROW            | Update the config file |
| enforce_gtid_consistency         | <not set>     | ON             | Update the config file |
| gtid_mode                        | OFF           | ON             | Update the config file |
| log_bin                          | <not set>     | <no value>     | Update the config file |
| log_slave_updates                | <not set>     | ON             | Update the config file |
| master_info_repository           | <not set>     | TABLE          | Update the config file |
| relay_log_info_repository        | <not set>     | TABLE          | Update the config file |
| report_port                      | <not set>     | <<<__mysql_sandbox_port2>>>           | Update the config file |
| server_id                        | <not set>     | <unique ID>    | Update the config file |
| transaction_write_set_extraction | <not set>     | XXHASH64       | Update the config file |
+----------------------------------+---------------+----------------+------------------------+

Please use the dba.configureInstance() command to repair these issues.

//@<OUT> Dba: configureLocalInstance error 3 {VER(<8.0.11)}
Please provide the password for 'root@localhost:<<<__mysql_sandbox_port1>>>': The instance 'localhost:<<<__mysql_sandbox_port1>>>' belongs to an InnoDB cluster.
Sandbox MySQL configuration file at: <<<__output_sandbox_dir>>><<<__mysql_sandbox_port1>>><<<__path_splitter>>>my.cnf
Persisting the cluster settings...
WARNING: The 'group_replication_group_seeds' is not defined on instance 'localhost:<<<__mysql_sandbox_port1>>>'. This option is mandatory to allow the server to automatically rejoin the cluster after reboot. Please manually update its value on the option file.
The instance 'localhost:<<<__mysql_sandbox_port1>>>' was configured for use in an InnoDB cluster.

The instance cluster settings were successfully persisted.

//@ Dba: configureLocalInstance error 3 bad call {VER(>=8.0.11)}
|The instance 'localhost:<<<__mysql_sandbox_port1>>>' belongs to an InnoDB cluster.|
|Calling this function on a cluster member is only required for MySQL versions 8.0.4 or earlier.|

//@ Dba: Create user without all necessary privileges
|Number of accounts: 1|

//@# Dba: configureLocalInstance not enough privileges 1 {VER(>=8.0.0)}
|ERROR: Unable to check privileges for user 'missingprivileges'@'<<<localhost>>>'. User requires SELECT privilege on mysql.* to obtain information about all roles.|
||Dba.configureLocalInstance: Unable to get roles information. (RuntimeError)

//@# Dba: configureLocalInstance not enough privileges 1 {VER(<8.0.0)}
|ERROR: The account 'missingprivileges'@'localhost' is missing privileges required to manage an InnoDB cluster:|
|Missing global privileges: FILE, GRANT OPTION, PROCESS, RELOAD, REPLICATION CLIENT, REPLICATION SLAVE, SHUTDOWN.|
|Missing privileges on schema 'mysql': DELETE, INSERT, SELECT, UPDATE.|
|Missing privileges on schema 'mysql_innodb_cluster_metadata': ALTER, ALTER ROUTINE, CREATE, CREATE ROUTINE, CREATE TEMPORARY TABLES, CREATE VIEW, DELETE, DROP, EVENT, EXECUTE, INDEX, INSERT, LOCK TABLES, REFERENCES, SELECT, SHOW VIEW, TRIGGER, UPDATE.|
|Missing privileges on schema 'sys': SELECT.|
|For more information, see the online documentation.|
||Dba.configureLocalInstance: The account 'missingprivileges'@'localhost' is missing privileges required to manage an InnoDB cluster. (RuntimeError)

//@# Dba: configureLocalInstance not enough privileges 2 {VER(>=8.0.0)}
|ERROR: Unable to check privileges for user 'missingprivileges'@'<<<localhost>>>'. User requires SELECT privilege on mysql.* to obtain information about all roles.|
||Dba.configureLocalInstance: Unable to get roles information. (RuntimeError)

//@# Dba: configureLocalInstance not enough privileges 2 {VER(<8.0.0)}
|ERROR: The account 'missingprivileges'@'localhost' is missing privileges required to manage an InnoDB cluster:|
|Missing global privileges: FILE, GRANT OPTION, PROCESS, RELOAD, REPLICATION CLIENT, REPLICATION SLAVE, SHUTDOWN.|
|Missing privileges on schema 'mysql': DELETE, INSERT, SELECT, UPDATE.|
|Missing privileges on schema 'mysql_innodb_cluster_metadata': ALTER, ALTER ROUTINE, CREATE, CREATE ROUTINE, CREATE TEMPORARY TABLES, CREATE VIEW, DELETE, DROP, EVENT, EXECUTE, INDEX, INSERT, LOCK TABLES, REFERENCES, SELECT, SHOW VIEW, TRIGGER, UPDATE.|
|Missing privileges on schema 'sys': SELECT.|
|For more information, see the online documentation.|
||Dba.configureLocalInstance: The account 'missingprivileges'@'localhost' is missing privileges required to manage an InnoDB cluster. (RuntimeError)

//@# Dba: configureLocalInstance not enough privileges 3 {VER(>=8.0.0)}
|ERROR: Unable to check privileges for user 'missingprivileges'@'<<<localhost>>>'. User requires SELECT privilege on mysql.* to obtain information about all roles.|
||Dba.configureLocalInstance: Unable to get roles information. (RuntimeError)

//@# Dba: configureLocalInstance not enough privileges 3 {VER(<8.0.0)}
|ERROR: The account 'missingprivileges'@'localhost' is missing privileges required to manage an InnoDB cluster:|
|Missing global privileges: FILE, GRANT OPTION, PROCESS, RELOAD, REPLICATION CLIENT, REPLICATION SLAVE, SHUTDOWN.|
|Missing privileges on schema 'mysql': DELETE, INSERT, SELECT, UPDATE.|
|Missing privileges on schema 'mysql_innodb_cluster_metadata': ALTER, ALTER ROUTINE, CREATE, CREATE ROUTINE, CREATE TEMPORARY TABLES, CREATE VIEW, DELETE, DROP, EVENT, EXECUTE, INDEX, INSERT, LOCK TABLES, REFERENCES, SELECT, SHOW VIEW, TRIGGER, UPDATE.|
|Missing privileges on schema 'sys': SELECT.|
|For more information, see the online documentation.|
||Dba.configureLocalInstance: The account 'missingprivileges'@'localhost' is missing privileges required to manage an InnoDB cluster. (RuntimeError)

//@ Dba: Show list of users to make sure the user missingprivileges@% was not created
|Number of accounts: 0|

//@ Dba: Delete created user and reconnect to previous sandbox
|Number of accounts: 0|

//@<OUT> Dba: configureLocalInstance updating config file {VER(>=8.0.3)}
Please provide the password for 'root@localhost:<<<__mysql_sandbox_port2>>>': Configuring local MySQL instance listening at port <<<__mysql_sandbox_port2>>> for use in an InnoDB cluster...
Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as <<<hostname>>>

Some configuration options need to be fixed:
+----------------------------------+---------------+----------------+------------------------+
| Variable                         | Current Value | Required Value | Note                   |
+----------------------------------+---------------+----------------+------------------------+
| binlog_checksum                  | <not set>     | NONE           | Update the config file |
| binlog_format                    | <not set>     | ROW            | Update the config file |
| enforce_gtid_consistency         | <not set>     | ON             | Update the config file |
| gtid_mode                        | OFF           | ON             | Update the config file |
| log_slave_updates                | <not set>     | ON             | Update the config file |
| master_info_repository           | <not set>     | TABLE          | Update the config file |
| relay_log_info_repository        | <not set>     | TABLE          | Update the config file |
| report_port                      | <not set>     | <<<__mysql_sandbox_port2>>>           | Update the config file |
| server_id                        | <not set>     | <unique ID>    | Update the config file |
| transaction_write_set_extraction | <not set>     | XXHASH64       | Update the config file |
+----------------------------------+---------------+----------------+------------------------+

Do you want to perform the required configuration changes? [y/n]: Configuring instance...
The instance 'localhost:<<<__mysql_sandbox_port2>>>' was configured for InnoDB cluster usage.


//@<OUT> Dba: configureLocalInstance updating config file {VER(<8.0.3)}
Please provide the password for 'root@localhost:<<<__mysql_sandbox_port2>>>': Configuring local MySQL instance listening at port <<<__mysql_sandbox_port2>>> for use in an InnoDB cluster...
Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as <<<hostname>>>

Some configuration options need to be fixed:
+----------------------------------+---------------+----------------+------------------------+
| Variable                         | Current Value | Required Value | Note                   |
+----------------------------------+---------------+----------------+------------------------+
| binlog_checksum                  | <not set>     | NONE           | Update the config file |
| binlog_format                    | <not set>     | ROW            | Update the config file |
| enforce_gtid_consistency         | <not set>     | ON             | Update the config file |
| gtid_mode                        | OFF           | ON             | Update the config file |
| log_bin                          | <not set>     | <no value>     | Update the config file |
| log_slave_updates                | <not set>     | ON             | Update the config file |
| master_info_repository           | <not set>     | TABLE          | Update the config file |
| relay_log_info_repository        | <not set>     | TABLE          | Update the config file |
| report_port                      | <not set>     | <<<__mysql_sandbox_port2>>>           | Update the config file |
| server_id                        | <not set>     | <unique ID>    | Update the config file |
| transaction_write_set_extraction | <not set>     | XXHASH64       | Update the config file |
+----------------------------------+---------------+----------------+------------------------+

Do you want to perform the required configuration changes? [y/n]: Configuring instance...
The instance 'localhost:<<<__mysql_sandbox_port2>>>' was configured for InnoDB cluster usage.


//@ Dba: create an admin user with all needed privileges
|Number of 'mydba'@'localhost' accounts: 1|

//@<OUT> Dba: configureLocalInstance create different admin user
Please provide the password for 'mydba@localhost:<<<__mysql_sandbox_port2>>>': Configuring local MySQL instance listening at port <<<__mysql_sandbox_port2>>> for use in an InnoDB cluster...
Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as <<<hostname>>>

ERROR: User 'mydba' can only connect from 'localhost'. New account(s) with proper source address specification to allow remote connection from all instances must be created to manage the cluster.

1) Create remotely usable account for 'mydba' with same grants and password
2) Create a new admin account for InnoDB cluster with minimal required grants
3) Ignore and continue
4) Cancel

Please select an option [1]: Please provide an account name (e.g: icroot@%) to have it created with the necessary
privileges or leave empty and press Enter to cancel.
Account Name:
The instance 'localhost:<<<__mysql_sandbox_port2>>>' is valid for InnoDB cluster usage.

Cluster admin user 'dba_test'@'%' created.

//@<OUT> Dba: configureLocalInstance create existing valid admin user
Please provide the password for 'mydba@localhost:<<<__mysql_sandbox_port2>>>': Configuring local MySQL instance listening at port <<<__mysql_sandbox_port2>>> for use in an InnoDB cluster...
Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as <<<hostname>>>

ERROR: User 'mydba' can only connect from 'localhost'. New account(s) with proper source address specification to allow remote connection from all instances must be created to manage the cluster.

1) Create remotely usable account for 'mydba' with same grants and password
2) Create a new admin account for InnoDB cluster with minimal required grants
3) Ignore and continue
4) Cancel

Please select an option [1]: Please provide an account name (e.g: icroot@%) to have it created with the necessary
privileges or leave empty and press Enter to cancel.
Account Name: User 'dba_test'@'%' already exists and will not be created.

The instance 'localhost:<<<__mysql_sandbox_port2>>>' is valid for InnoDB cluster usage.

//@ Dba: remove needed privilege (REPLICATION SLAVE) from created admin user
||

//@ Dba: configureLocalInstance create existing invalid admin user
||Dba.configureLocalInstance: The account 'mydba'@'localhost' is missing privileges required to manage an InnoDB cluster. (RuntimeError)

//@ Dba: Delete previously create an admin user with all needed privileges
|Number of 'mydba'@'localhost' accounts: 0|

//@# Check if all missing privileges are reported for user with no privileges {VER(>=8.0.0)}
|ERROR: Unable to check privileges for user 'no_privileges'@'%'. User requires SELECT privilege on mysql.* to obtain information about all roles.|
||Dba.configureLocalInstance: Unable to get roles information. (RuntimeError)

//@# Check if all missing privileges are reported for user with no privileges {VER(<8.0.0)}
|ERROR: The account 'no_privileges'@'%' is missing privileges required to manage an InnoDB cluster:|
|Missing global privileges: CREATE USER, FILE, GRANT OPTION, PROCESS, RELOAD, REPLICATION CLIENT, REPLICATION SLAVE, SHUTDOWN, SUPER.|
|Missing privileges on schema 'mysql': DELETE, INSERT, SELECT, UPDATE.|
|Missing privileges on schema 'mysql_innodb_cluster_metadata': ALTER, ALTER ROUTINE, CREATE, CREATE ROUTINE, CREATE TEMPORARY TABLES, CREATE VIEW, DELETE, DROP, EVENT, EXECUTE, INDEX, INSERT, LOCK TABLES, REFERENCES, SELECT, SHOW VIEW, TRIGGER, UPDATE.|
|Missing privileges on schema 'sys': SELECT.|
|Missing privileges on table 'performance_schema.replication_applier_configuration': SELECT.|
|Missing privileges on table 'performance_schema.replication_applier_status': SELECT.|
|Missing privileges on table 'performance_schema.replication_applier_status_by_coordinator': SELECT.|
|Missing privileges on table 'performance_schema.replication_applier_status_by_worker': SELECT.|
|Missing privileges on table 'performance_schema.replication_connection_configuration': SELECT.|
|Missing privileges on table 'performance_schema.replication_connection_status': SELECT.|
|Missing privileges on table 'performance_schema.replication_group_member_stats': SELECT.|
|Missing privileges on table 'performance_schema.replication_group_members': SELECT.|
|Missing privileges on table 'performance_schema.threads': SELECT.|
|For more information, see the online documentation.|
||Dba.configureLocalInstance: The account 'no_privileges'@'%' is missing privileges required to manage an InnoDB cluster. (RuntimeError)

//@ configureLocalInstance() should fail if user does not have global GRANT OPTION
|Configuring local MySQL instance listening at port <<<__mysql_sandbox_port2>>> for use in an InnoDB cluster...|
|ERROR: The account 'no_global_grant'@'%' is missing privileges required to manage an InnoDB cluster:|
|Missing global privileges: GRANT OPTION.|
|For more information, see the online documentation.|
||Dba.configureLocalInstance: The account 'no_global_grant'@'%' is missing privileges required to manage an InnoDB cluster. (RuntimeError)

//@ createCluster() should fail if user does not have global GRANT OPTION
|A new InnoDB cluster will be created on instance 'no_global_grant@localhost:<<<__mysql_sandbox_port2>>>'.|
|Validating instance at localhost:<<<__mysql_sandbox_port2>>>...|
|ERROR: The account 'no_global_grant'@'%' is missing privileges required to manage an InnoDB cluster:|
|Missing global privileges: GRANT OPTION.|
|For more information, see the online documentation.|
||Dba.createCluster: The account 'no_global_grant'@'%' is missing privileges required to manage an InnoDB cluster. (RuntimeError)

//@# Dba: getCluster errors
||Dba.getCluster: Invalid cluster name: Argument #1 is expected to be a string
||Dba.getCluster: Invalid number of arguments, expected 0 to 2 but got 3
||Invalid typecast: Map expected, but value is Integer
||Dba.getCluster: The cluster with the name '' does not exist.
||Dba.getCluster: The cluster with the name '#' does not exist.
||Dba.getCluster: The cluster with the name 'over40chars_12345678901234567890123456789' does not exist.

//@<OUT> Dba: getCluster with interaction
<Cluster:devCluster>

//@<OUT> Dba: getCluser validate object serialization output - tabbed
tabbed
<Cluster:devCluster>

//@<OUT> Dba: getCluser validate object serialization output - table
table
<Cluster:devCluster>

//@<OUT> Dba: getCluser validate object serialization output - vertical
vertical
<Cluster:devCluster>

//@<OUT> Dba: getCluser validate object serialization output - json
json
<Cluster:devCluster>

//@<OUT> Dba: getCluser validate object serialization output - json/raw
json/raw
<Cluster:devCluster>

//@<OUT> Dba: getCluster with interaction (default)
<Cluster:devCluster>

//@<OUT> Dba: getCluster with interaction (default null)
<Cluster:devCluster>

//@ Finalization
||
