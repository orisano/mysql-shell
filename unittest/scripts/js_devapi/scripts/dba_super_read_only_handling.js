// Assumptions: smart deployment rountines available
//@ Initialization
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname});
testutil.snapshotSandboxConf(__mysql_sandbox_port1);
testutil.deploySandbox(__mysql_sandbox_port2, "root", {report_host: hostname});
testutil.snapshotSandboxConf(__mysql_sandbox_port2);
testutil.deploySandbox(__mysql_sandbox_port3, "root", {report_host: hostname});
testutil.snapshotSandboxConf(__mysql_sandbox_port3);

var mysql = require('mysql');

function setupInstance(connection, super_read_only) {
    var tmpSession = mysql.getClassicSession(connection);

    tmpSession.runSql('SET GLOBAL super_read_only = 0');
    if (super_read_only) {
        tmpSession.runSql('set global super_read_only=ON');
    }

    tmpSession.close();
}

function ensureSuperReadOnly(connection) {
    var tmpSession = mysql.getClassicSession(connection);
    tmpSession.runSql('set global super_read_only=ON');
    tmpSession.close();
}

var connection1 = {scheme: 'mysql', host: localhost, port: __mysql_sandbox_port1, user: 'root', password: 'root'};
var connection2 = {scheme: 'mysql', host: localhost, port: __mysql_sandbox_port2, user: 'root', password: 'root'};
var connection3 = {scheme: 'mysql', host: localhost, port: __mysql_sandbox_port3, user: 'root', password: 'root'};

setupInstance(connection1, true);
setupInstance(connection2, true);
setupInstance(connection3, false);

//@<OUT> Configures the instance, answers 'yes' on the read only prompt
//NOTE: super-read-only is only required to be disable if we create the clusterAdmin user
dba.configureInstance(connection1, {clusterAdmin: "testUser", clusterAdminPassword: "testUserPwd"});

//@<OUT> Configures the instance, read only set, no prompt
dba.configureInstance(connection2, {clusterAdmin: "testUser", clusterAdminPassword: "testUserPwd", clearReadOnly: true});

//@<OUT> Configures the instance, no prompt
dba.configureInstance(connection3, {clusterAdmin: "testUser", clusterAdminPassword: "testUserPwd"});


//@<> Connect
shell.connect(connection1);

//@<OUT> Creates Cluster succeeds, answers 'yes' on read only prompt
var cluster = dba.createCluster('sample');

var local_address = session.runSql("SELECT @@group_replication_local_address").fetchOne()[0];
if (hostname+":"+(__mysql_sandbox_port1*10+1) != local_address)
    testutil.fail("group_replication_local_address has unexpected value " + local_address);

//@<OUT> Adds a read only instance
cluster.addInstance(connection2);
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");

// Wait for the second added instance to fetch all the replication data
testutil.waitMemberTransactions(__mysql_sandbox_port2);

//@<OUT> Adds other instance
cluster.addInstance(connection3);
testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");

// Wait for the third added instance to fetch all the replication data
testutil.waitMemberTransactions(__mysql_sandbox_port3);

// Rejoin instance
testutil.stopSandbox(__mysql_sandbox_port3);
testutil.waitMemberState(__mysql_sandbox_port3, "(MISSING)");
testutil.startSandbox(__mysql_sandbox_port3);
ensureSuperReadOnly(connection3);
//@<OUT> Rejoins an instance
cluster.rejoinInstance(connection3);

cluster.disconnect();

// killSandboxInstance does not wait until the process is actually killed
// before returning, so the function does not fit this use-case.
// OTOH stopSandboxInstance waits until the MySQL classic port is not listening
// anymore, but the x-protocol port may take a bit longer. As so, we must use
// testutil.startSandbox() to make sure the instance is restarted.

//@ Stop sandbox 2
testutil.stopSandbox(__mysql_sandbox_port2);
testutil.waitMemberState(__mysql_sandbox_port2, "(MISSING)");

//@ Stop sandbox 3
testutil.stopSandbox(__mysql_sandbox_port3);
testutil.waitMemberState(__mysql_sandbox_port3, "(MISSING)");
session.close();

//@ Stop sandbox 1
testutil.stopSandbox(__mysql_sandbox_port1);

//@ Start sandbox 1
testutil.startSandbox(__mysql_sandbox_port1);
testutil.waitForDelayedGRStart(__mysql_sandbox_port1, 'root', 0);

//@ Start sandbox 2
testutil.startSandbox(__mysql_sandbox_port2);
testutil.waitForDelayedGRStart(__mysql_sandbox_port2, 'root', 0);

//@ Start sandbox 3
testutil.startSandbox(__mysql_sandbox_port3);
testutil.waitForDelayedGRStart(__mysql_sandbox_port3, 'root', 0);

//@<OUT> Reboot the cluster
shell.connect(connection1);
session.runSql('set global super_read_only=ON');
var cluster = dba.rebootClusterFromCompleteOutage("sample");
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");
testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");

//@ Cleanup
session.close();
cluster.disconnect();

testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
testutil.destroySandbox(__mysql_sandbox_port3);
