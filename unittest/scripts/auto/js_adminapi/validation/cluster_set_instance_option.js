//@<OUT> WL#11465: Create single-primary cluster
Creating a Classic session to 'root@localhost:<<<__mysql_sandbox_port1>>>'

//@ WL#11465: ArgumentErrors of setInstanceOption
||Invalid number of arguments, expected 3 but got 0 (ArgumentError)
||Invalid URI: empty. (ArgumentError)
||Argument #1 is expected to be a map (ArgumentError)
||Invalid connection options, no options provided. (ArgumentError)
||Invalid number of arguments, expected 3 but got 1 (ArgumentError)
||Invalid number of arguments, expected 3 but got 2 (ArgumentError)
||Option 'foobar' not supported. (ArgumentError)
||Invalid connection options, no options provided. (ArgumentError)

//@ WL#11465: F2.2.1.2 - Remove instance 2 from the cluster
||

//@<ERR> WL#11465: Error when executing setInstanceOption for a target instance that does not belong to the cluster
Cluster.setInstanceOption: The instance '<<<localhost>>>:<<<__mysql_sandbox_port2>>>' does not belong to the ReplicaSet: 'default'. (RuntimeError)

//@ WL#11465: F2.2.1.2 - Add instance 2 back to the cluster
||

//@<ERR> WL#11465: Error when executing setInstanceOption when the target instance is not ONLINE
Cluster.setInstanceOption: The instance 'localhost:<<<__mysql_sandbox_port3>>>' is not ONLINE. (RuntimeError)

//@<ERR> WL#11465: Error when executing setInstanceOption on a cluster with no visible quorum {VER(>=8.0.14)}
Cluster.setInstanceOption: There is no quorum to perform the operation (RuntimeError)

//@<ERR> WL#11465: Error when executing setInstanceOption on a cluster with no visible quorum 5.7 {VER(>=5.7.24) && VER(<8.0.0)}
Cluster.setInstanceOption: There is no quorum to perform the operation (RuntimeError)

//@ WL#11465: Re-create the cluster
||

//@<ERR> WL#11465: setInstanceOption label with invalid value for label 1
Cluster.setInstanceOption: The label can only start with an alphanumeric or the '_' character. (ArgumentError)

//@<ERR> WL#11465: setInstanceOption label with invalid value for label 2
Cluster.setInstanceOption: An instance with label '<<<hostname>>>:<<<__mysql_sandbox_port1>>>' is already part of this InnoDB cluster (ArgumentError)

//@<OUT> WL#11465: setInstanceOption label
Setting the value of 'label' to 'newLabel' in the instance: 'localhost:<<<__mysql_sandbox_port2>>>' ...

Successfully set the value of 'label' to 'newLabel' in the 'default' ReplicaSet member: 'localhost:<<<__mysql_sandbox_port2>>>'.

//@<OUT> WL#11465: Verify label changed correctly
newLabel

//@<OUT> WL#11465: setInstanceOption memberWeight {VER(>=8.0.0)}
Setting the value of 'memberWeight' to '25' in the instance: 'localhost:<<<__mysql_sandbox_port2>>>' ...

Successfully set the value of 'memberWeight' to '25' in the 'default' ReplicaSet member: 'localhost:<<<__mysql_sandbox_port2>>>'.

//@<OUT> WL#11465: setInstanceOption memberWeight 5.7 {VER(>=5.7.24) && VER(<8.0.0)}
WARNING: The settings cannot be persisted remotely on instance 'localhost:<<<__mysql_sandbox_port2>>>' because MySQL version <<<__version>>> does not support the SET PERSIST command (MySQL version >= 8.0.11 required). Please execute the <Dba>.configureLocalInstance() command locally to persist these changes.
Setting the value of 'memberWeight' to '25' in the instance: 'localhost:<<<__mysql_sandbox_port2>>>' ...

Successfully set the value of 'memberWeight' to '25' in the 'default' ReplicaSet member: 'localhost:<<<__mysql_sandbox_port2>>>'.

//@<OUT> WL#11465: memberWeight label changed correctly
25

//@<ERR> WL#11465: setInstanceOption exitStateAction with invalid value
Cluster.setInstanceOption: Variable 'group_replication_exit_state_action' can't be set to the value of 'ABORT' (RuntimeError)

//@<OUT> WL#11465: setInstanceOption exitStateAction {VER(>=8.0.0)}
Setting the value of 'exitStateAction' to 'ABORT_SERVER' in the instance: 'localhost:<<<__mysql_sandbox_port2>>>' ...

Successfully set the value of 'exitStateAction' to 'ABORT_SERVER' in the 'default' ReplicaSet member: 'localhost:<<<__mysql_sandbox_port2>>>'.

//@<OUT> WL#11465: setInstanceOption exitStateAction 5.7 {VER(>=5.7.24) && VER(<8.0.0)}
WARNING: The settings cannot be persisted remotely on instance 'localhost:<<<__mysql_sandbox_port2>>>' because MySQL version <<<__version>>> does not support the SET PERSIST command (MySQL version >= 8.0.11 required). Please execute the <Dba>.configureLocalInstance() command locally to persist these changes.
Setting the value of 'exitStateAction' to 'ABORT_SERVER' in the instance: 'localhost:<<<__mysql_sandbox_port2>>>' ...

Successfully set the value of 'exitStateAction' to 'ABORT_SERVER' in the 'default' ReplicaSet member: 'localhost:<<<__mysql_sandbox_port2>>>'.

//@<OUT> WL#11465: exitStateAction label changed correctly
ABORT_SERVER

//@<OUT> WL#12066: TSF6_1 setInstanceOption autoRejoinTries {VER(>=8.0.16)}
WARNING: The member will only proceed according to its exitStateAction if auto-rejoin fails (i.e. all retry attempts are exhausted).

Setting the value of 'autoRejoinTries' to '2016' in the instance: 'localhost:<<<__mysql_sandbox_port1>>>' ...

Successfully set the value of 'autoRejoinTries' to '2016' in the 'default' ReplicaSet member: 'localhost:<<<__mysql_sandbox_port1>>>'.
WARNING: The member will only proceed according to its exitStateAction if auto-rejoin fails (i.e. all retry attempts are exhausted).

Setting the value of 'autoRejoinTries' to '20' in the instance: 'localhost:<<<__mysql_sandbox_port2>>>' ...

Successfully set the value of 'autoRejoinTries' to '20' in the 'default' ReplicaSet member: 'localhost:<<<__mysql_sandbox_port2>>>'.
Setting the value of 'autoRejoinTries' to '0' in the instance: 'localhost:<<<__mysql_sandbox_port3>>>' ...

Successfully set the value of 'autoRejoinTries' to '0' in the 'default' ReplicaSet member: 'localhost:<<<__mysql_sandbox_port3>>>'.

//@ WL#12066: TSF3_4 setInstanceOption autoRejoinTries doesn't accept negative values {VER(>=8.0.16)}
||Variable 'group_replication_autorejoin_tries' can't be set to the value of '-1' (RuntimeError)

//@ WL#12066: TSF3_5 setInstanceOption autoRejoinTries doesn't accept values out of range {VER(>=8.0.16)}
||Variable 'group_replication_autorejoin_tries' can't be set to the value of '2017' (RuntimeError)

//@ WL#12066: TSF3_3 Verify autoRejoinTries changed correctly in instance 1 {VER(>=8.0.16)}
|2016|
|group_replication_autorejoin_tries = 2016|

//@ WL#12066: TSF3_3 Verify autoRejoinTries changed correctly in instance 2 {VER(>=8.0.16)}
|20|
|group_replication_autorejoin_tries = 20|

//@ WL#12066: TSF3_3 Verify autoRejoinTries changed correctly in instance 3 {VER(>=8.0.16)}
|0|
|group_replication_autorejoin_tries = 0|

//@ WL#11465: Finalization
||
