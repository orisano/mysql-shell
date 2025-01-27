#@<OUT> dba
NAME
      dba - Global variable for InnoDB cluster management.

DESCRIPTION
      The global variable dba is used to access the AdminAPI functionality and
      perform DBA operations. It is used for managing MySQL InnoDB clusters.

PROPERTIES
      verbose
            Enables verbose mode on the dba operations.

FUNCTIONS
      check_instance_configuration(instance[, options])
            Validates an instance for MySQL InnoDB Cluster usage.

      configure_instance([instance][, options])
            Validates and configures an instance for MySQL InnoDB Cluster
            usage.

      configure_local_instance(instance[, options])
            Validates and configures a local instance for MySQL InnoDB Cluster
            usage.

      create_cluster(name[, options])
            Creates a MySQL InnoDB cluster.

      delete_sandbox_instance(port[, options])
            Deletes an existing MySQL Server instance on localhost.

      deploy_sandbox_instance(port[, options])
            Creates a new MySQL Server instance on localhost.

      drop_metadata_schema(options)
            Drops the Metadata Schema.

      get_cluster([name][, options])
            Retrieves a cluster from the Metadata Store.

      help([member])
            Provides help about this object and it's members

      kill_sandbox_instance(port[, options])
            Kills a running MySQL Server instance on localhost.

      reboot_cluster_from_complete_outage([clusterName][, options])
            Brings a cluster back ONLINE when all members are OFFLINE.

      start_sandbox_instance(port[, options])
            Starts an existing MySQL Server instance on localhost.

      stop_sandbox_instance(port[, options])
            Stops a running MySQL Server instance on localhost.

      For more help on a specific function use: dba.help('<functionName>')

      e.g. dba.help('deploy_sandbox_instance')

#@<OUT> dba.check_instance_configuration
NAME
      check_instance_configuration - Validates an instance for MySQL InnoDB
                                     Cluster usage.

SYNTAX
      dba.check_instance_configuration(instance[, options])

WHERE
      instance: An instance definition.
      options: Data for the operation.

RETURNS
       A descriptive text of the operation result.

DESCRIPTION
      This function reviews the instance configuration to identify if it is
      valid for usage with group replication. Use this to check for possible
      configuration issues on MySQL instances before creating a cluster with
      them or adding them to an existing cluster.

      The instance definition is the connection data for the instance.

      For additional information on connection data use \? connection.

      Only TCP/IP connections are allowed for this function.

      The options dictionary may contain the following options:

      - mycnfPath: Optional path to the MySQL configuration file for the
        instance. Alias for verifyMyCnf
      - verifyMyCnf: Optional path to the MySQL configuration file for the
        instance. If this option is given, the configuration file will be
        verified for the expected option values, in addition to the global
        MySQL system variables.
      - password: The password to get connected to the instance.
      - interactive: boolean value used to disable the wizards in the command
        execution, i.e. prompts are not provided to the user and confirmation
        prompts are not shown.

      The connection password may be contained on the instance definition,
      however, it can be overwritten if it is specified on the options.

      The returned descriptive text of the operation result indicates whether
      the instance is valid for InnoDB Cluster usage or not. If not, a table
      containing the following information is presented:

      - Variable: the invalid configuration variable.
      - Current Value: the current value for the invalid configuration
        variable.
      - Required Value: the required value for the configuration variable.
      - Note: the action to be taken.

      The note can be one of the following:

      - Update the config file and update or restart the server variable.
      - Update the config file and restart the server.
      - Update the config file.
      - Update the server variable.
      - Restart the server.

EXCEPTIONS
      ArgumentError in the following scenarios:

      - If the instance parameter is empty.
      - If the instance definition is invalid.
      - If the instance definition is a connection dictionary but empty.

      RuntimeError in the following scenarios:

      - If the instance accounts are invalid.
      - If the instance is offline.
      - If the instance is already part of a Replication Group.
      - If the instance is already part of an InnoDB Cluster.
      - If the given the instance cannot be used for Group Replication.

#@<OUT> dba.configure_instance
NAME
      configure_instance - Validates and configures an instance for MySQL
                           InnoDB Cluster usage.

SYNTAX
      dba.configure_instance([instance][, options])

WHERE
      instance: An instance definition.
      options: Additional options for the operation.

RETURNS
       A descriptive text of the operation result.

DESCRIPTION
      This function auto-configures the instance for InnoDB Cluster usage.If
      the target instance already belongs to an InnoDB Cluster it errors out.

      The instance definition is the connection data for the instance.

      For additional information on connection data use \? connection.

      Only TCP/IP connections are allowed for this function.

      The options dictionary may contain the following options:

      - mycnfPath: The path to the MySQL configuration file of the instance.
      - outputMycnfPath: Alternative output path to write the MySQL
        configuration file of the instance.
      - password: The password to be used on the connection.
      - clusterAdmin: The name of the InnoDB cluster administrator user to be
        created. The supported format is the standard MySQL account name
        format.
      - clusterAdminPassword: The password for the InnoDB cluster administrator
        account.
      - clearReadOnly: boolean value used to confirm that super_read_only must
        be disabled.
      - interactive: boolean value used to disable the wizards in the command
        execution, i.e. prompts are not provided to the user and confirmation
        prompts are not shown.
      - restart: boolean value used to indicate that a remote restart of the
        target instance should be performed to finalize the operation.

      If the outputMycnfPath option is used, only that file is updated and
      mycnfPath is treated as read-only.

      The connection password may be contained on the instance definition,
      however, it can be overwritten if it is specified on the options.

      This function reviews the instance configuration to identify if it is
      valid for usage in group replication and cluster. An exception is thrown
      if not.

      If the instance was not valid for InnoDB Cluster and interaction is
      enabled, before configuring the instance a prompt to confirm the changes
      is presented and a table with the following information:

      - Variable: the invalid configuration variable.
      - Current Value: the current value for the invalid configuration
        variable.
      - Required Value: the required value for the configuration variable.
      - Required Value: the required value for the configuration variable.

EXCEPTIONS
      ArgumentError in the following scenarios:

      - If 'interactive' is disabled and the instance parameter is empty.
      - If the instance definition is invalid.
      - If the instance definition is a connection dictionary but empty.
      - If the instance definition is a connection dictionary but any option is
        invalid.
      - If 'interactive' mode is disabled and the instance definition is
        missing the password.
      - If 'interactive' mode is enabled and the provided password is empty.

      RuntimeError in the following scenarios:

      - If the configuration file path is required but not provided or wrong.
      - If the instance accounts are invalid.
      - If the instance is offline.
      - If the instance is already part of a Replication Group.
      - If the instance is already part of an InnoDB Cluster.
      - If the given instance cannot be used for Group Replication.

#@<OUT> dba.configure_local_instance
NAME
      configure_local_instance - Validates and configures a local instance for
                                 MySQL InnoDB Cluster usage.

SYNTAX
      dba.configure_local_instance(instance[, options])

WHERE
      instance: An instance definition.
      options: Additional options for the operation.

RETURNS
       Nothing

DESCRIPTION
      This function reviews the instance configuration to identify if it is
      valid for usage in group replication and cluster. An exception is thrown
      if not.

      The instance definition is the connection data for the instance.

      For additional information on connection data use \? connection.

      Only TCP/IP connections are allowed for this function.

      The options dictionary may contain the following options:

      - mycnfPath: The path to the MySQL configuration file of the instance.
      - outputMycnfPath: Alternative output path to write the MySQL
        configuration file of the instance.
      - password: The password to be used on the connection.
      - clusterAdmin: The name of the InnoDB cluster administrator user to be
        created. The supported format is the standard MySQL account name
        format.
      - clusterAdminPassword: The password for the InnoDB cluster administrator
        account.
      - clearReadOnly: boolean value used to confirm that super_read_only must
        be disabled.
      - interactive: boolean value used to disable the wizards in the command
        execution, i.e. prompts are not provided to the user and confirmation
        prompts are not shown.

      If the outputMycnfPath option is used, only that file is updated and
      mycnfPath is treated as read-only.

      The connection password may be contained on the instance definition,
      however, it can be overwritten if it is specified on the options.

      The returned descriptive text of the operation result indicates whether
      the instance was successfully configured for InnoDB Cluster usage or if
      it was already valid for InnoDB Cluster usage.

      If the instance was not valid for InnoDB Cluster and interaction is
      enabled, before configuring the instance a prompt to confirm the changes
      is presented and a table with the following information:

      - Variable: the invalid configuration variable.
      - Current Value: the current value for the invalid configuration
        variable.
      - Required Value: the required value for the configuration variable.

EXCEPTIONS
      ArgumentError in the following scenarios:

      - If the instance parameter is empty.
      - If the instance definition is invalid.
      - If the instance definition is a connection dictionary but empty.
      - If the instance definition is a connection dictionary but any option is
        invalid.
      - If the instance definition is missing the password.
      - If the provided password is empty.
      - If the configuration file path is required but not provided or wrong.

      RuntimeError in the following scenarios:

      - If the instance accounts are invalid.
      - If the instance is offline.
      - If the instance is already part of a Replication Group.
      - If the given instance cannot be used for Group Replication.

#@<OUT> dba.create_cluster
NAME
      create_cluster - Creates a MySQL InnoDB cluster.

SYNTAX
      dba.create_cluster(name[, options])

WHERE
      name: The name of the cluster object to be created.
      options: Dictionary with options that modify the behavior of this
               function.

RETURNS
       The created cluster object.

DESCRIPTION
      Creates a MySQL InnoDB cluster taking as seed instance the active global
      session.

      The options dictionary can contain the following values:

      - multiMaster: boolean value used to define an InnoDB cluster with
        multiple writable instances.
      - multiPrimary: boolean value used to define an InnoDB cluster with
        multiple writable instances.
      - force: boolean, confirms that the multiPrimary option must be applied.
      - interactive: boolean value used to disable the wizards in the command
        execution, i.e. prompts are not provided to the user and confirmation
        prompts are not shown.
      - adoptFromGR: boolean value used to create the InnoDB cluster based on
        existing replication group.
      - memberSslMode: SSL mode used to configure the members of the cluster.
      - ipWhitelist: The list of hosts allowed to connect to the instance for
        group replication.
      - clearReadOnly: boolean value used to confirm that super_read_only must
        be disabled.
      - groupName: string value with the Group Replication group name UUID to
        be used instead of the automatically generated one.
      - localAddress: string value with the Group Replication local address to
        be used instead of the automatically generated one.
      - groupSeeds: string value with a comma-separated list of the Group
        Replication peer addresses to be used instead of the automatically
        generated one.
      - exitStateAction: string value indicating the group replication exit
        state action.
      - memberWeight: integer value with a percentage weight for automatic
        primary election on failover.
      - failoverConsistency: string value indicating the consistency guarantees
        that the cluster provides.
      - expelTimeout: integer value to define the time period in seconds that
        cluster members should wait for a non-responding member before evicting
        it from the cluster.
      - autoRejoinTries: integer value to define the number of times an
        instance will attempt to rejoin the cluster after being expelled.

      ATTENTION: The multiMaster option will be removed in a future release.
                 Please use the multiPrimary option instead.

      ATTENTION: The failoverConsistency option will be removed in a future
                 release. Please use the consistency option instead.

      An InnoDB cluster may be setup in two ways:

      - Single Primary: One member of the cluster allows write operations while
        the rest are in read only mode.
      - Multi Primary: All the members in the cluster support both read and
        write operations.

      By default this function create a Single Primary cluster, use the
      multiPrimary option set to true if a Multi Primary cluster is required.

      The memberSslMode option supports the following values:

      - REQUIRED: if used, SSL (encryption) will be enabled for the instances
        to communicate with other members of the cluster
      - DISABLED: if used, SSL (encryption) will be disabled
      - AUTO: if used, SSL (encryption) will be enabled if supported by the
        instance, otherwise disabled

      If memberSslMode is not specified AUTO will be used by default.

      The exitStateAction option supports the following values:

      - ABORT_SERVER: if used, the instance shuts itself down if it leaves the
        cluster unintentionally.
      - READ_ONLY: if used, the instance switches itself to super-read-only
        mode if it leaves the cluster unintentionally.

      If exitStateAction is not specified READ_ONLY will be used by default.

      The consistency option supports the following values:

      - BEFORE_ON_PRIMARY_FAILOVER: if used, new queries (read or write) to the
        new primary will be put on hold until after the backlog from the old
        primary is applied.
      - EVENTUAL: if used, read queries to the new primary are allowed even if
        the backlog isn't applied.

      If consistency is not specified, EVENTUAL will be used by default.

      The ipWhitelist format is a comma separated list of IP addresses or
      subnet CIDR notation, for example: 192.168.1.0/24,10.0.0.1. By default
      the value is set to AUTOMATIC, allowing addresses from the instance
      private network to be automatically set for the whitelist.

      The groupName, localAddress, and groupSeeds are advanced options and
      their usage is discouraged since incorrect values can lead to Group
      Replication errors.

      The value for groupName is used to set the Group Replication system
      variable 'group_replication_group_name'.

      The value for localAddress is used to set the Group Replication system
      variable 'group_replication_local_address'. The localAddress option
      accepts values in the format: 'host:port' or 'host:' or ':port'. If the
      specified value does not include a colon (:) and it is numeric, then it
      is assumed to be the port, otherwise it is considered to be the host.
      When the host is not specified, the default value is the value of the
      system variable 'report_host' if defined (i.e., not 'NULL'), otherwise it
      is the hostname value. When the port is not specified, the default value
      is the port of the current active connection (session) * 10 + 1. In case
      the automatically determined default port value is invalid (> 65535) then
      a random value in the range [10000, 65535] is used.

      The value for groupSeeds is used to set the Group Replication system
      variable 'group_replication_group_seeds'. The groupSeeds option accepts a
      comma-separated list of addresses in the format:
      'host1:port1,...,hostN:portN'.

      The value for exitStateAction is used to configure how Group Replication
      behaves when a server instance leaves the group unintentionally, for
      example after encountering an applier error. When set to ABORT_SERVER,
      the instance shuts itself down, and when set to READ_ONLY the server
      switches itself to super-read-only mode. The exitStateAction option
      accepts case-insensitive string values, being the accepted values:
      ABORT_SERVER (or 1) and READ_ONLY (or 0).

      The default value is READ_ONLY.

      The value for memberWeight is used to set the Group Replication system
      variable 'group_replication_member_weight'. The memberWeight option
      accepts integer values. Group Replication limits the value range from 0
      to 100, automatically adjusting it if a lower/bigger value is provided.

      Group Replication uses a default value of 50 if no value is provided.

      The value for consistency is used to set the Group Replication system
      variable 'group_replication_consistency' and configure the transaction
      consistency guarantee which a cluster provides.

      When set to to BEFORE_ON_PRIMARY_FAILOVER, whenever a primary failover
      happens in single-primary mode (default), new queries (read or write) to
      the newly elected primary that is applying backlog from the old primary,
      will be hold before execution until the backlog is applied. When set to
      EVENTUAL, read queries to the new primary are allowed even if the backlog
      isn't applied but writes will fail (if the backlog isn't applied) due to
      super-read-only mode being enabled. The client may return old values.

      When set to BEFORE, each transaction (RW or RO) waits until all preceding
      transactions are complete before starting its execution. This ensures
      that each transaction is executed on the most up-to-date snapshot of the
      data, regardless of which member it is executed on. The latency of the
      transaction is affected but the overhead of synchronization on RW
      transactions is reduced since synchronization is used only on RO
      transactions.

      When set to AFTER, each RW transaction waits until its changes have been
      applied on all of the other members. This ensures that once this
      transaction completes, all following transactions read a database state
      that includes its changes, regardless of which member they are executed
      on. This mode shall only be used on a group that is used for
      predominantly RO operations to  to ensure that subsequent reads fetch the
      latest data which includes the latest writes. The overhead of
      synchronization on every RO transaction is reduced since synchronization
      is used only on RW transactions.

      When set to BEFORE_AND_AFTER, each RW transaction waits for all preceding
      transactions to complete before being applied and until its changes have
      been applied on other members. A RO transaction waits for all preceding
      transactions to complete before execution takes place. This ensures the
      guarantees given by BEFORE and by AFTER. The overhead of synchronization
      is higher.

      The consistency option accepts case-insensitive string values, being the
      accepted values: EVENTUAL (or 0), BEFORE_ON_PRIMARY_FAILOVER (or 1),
      BEFORE (or 2), AFTER (or 3), and BEFORE_AND_AFTER (or 4).

      The default value is EVENTUAL.

      The value for expelTimeout is used to set the Group Replication system
      variable 'group_replication_member_expel_timeout' and configure how long
      Group Replication will wait before expelling from the group any members
      suspected of having failed. On slow networks, or when there are expected
      machine slowdowns, increase the value of this option. The expelTimeout
      option accepts positive integer values in the range [0, 3600].

      The default value is 0.

      The value for autoRejoinTries is used to set the Group Replication system
      variable 'group_replication_autorejoin_tries' and configure how many
      times an instance will try to rejoin a Group Replication group after
      being expelled. In scenarios where network glitches happen but recover
      quickly, setting this option prevents users from having to manually add
      the expelled node back to the group. The autoRejoinTries option accepts
      positive integer values in the range [0, 2016].

      The default value is 0.

EXCEPTIONS
      MetadataError in the following scenarios:

      - If the Metadata is inaccessible.
      - If the Metadata update operation failed.

      ArgumentError in the following scenarios:

      - If the Cluster name is empty.
      - If the Cluster name is not valid.
      - If the options contain an invalid attribute.
      - If adoptFromGR is true and the memberSslMode option is used.
      - If the value for the memberSslMode option is not one of the allowed.
      - If adoptFromGR is true and the multiPrimary option is used.
      - If the value for the ipWhitelist, groupName, localAddress, groupSeeds,
        exitStateAction or consistency options is empty.
      - If the value for the expelTimeout is not in the range: [0, 3600]

      RuntimeError in the following scenarios:

      - If the value for the groupName, localAddress, groupSeeds,
        exitStateAction, memberWeight, consistency, expelTimeout or
        autoRejoinTries options is not valid for Group Replication.
      - If the current connection cannot be used for Group Replication.

#@<OUT> dba.delete_sandbox_instance
NAME
      delete_sandbox_instance - Deletes an existing MySQL Server instance on
                                localhost.

SYNTAX
      dba.delete_sandbox_instance(port[, options])

WHERE
      port: The port of the instance to be deleted.
      options: Dictionary with options that modify the way this function is
               executed.

RETURNS
       Nothing.

DESCRIPTION
      This function will delete an existing MySQL Server instance on the local
      host. The following options affect the result:

      - sandboxDir: path where the instance is located.

      The sandboxDir must be the one where the MySQL instance was deployed. If
      not specified it will use:

        ~/mysql-sandboxes on Unix-like systems or
      %userprofile%\MySQL\mysql-sandboxes on Windows systems.

      If the instance is not located on the used path an error will occur.

EXCEPTIONS
      ArgumentError in the following scenarios:

      - If the options contain an invalid attribute.
      - If the port value is < 1024 or > 65535.

#@<OUT> dba.deploy_sandbox_instance
NAME
      deploy_sandbox_instance - Creates a new MySQL Server instance on
                                localhost.

SYNTAX
      dba.deploy_sandbox_instance(port[, options])

WHERE
      port: The port where the new instance will listen for connections.
      options: Dictionary with options affecting the new deployed instance.

RETURNS
       Nothing.

DESCRIPTION
      This function will deploy a new MySQL Server instance, the result may be
      affected by the provided options:

      - portx: port where the new instance will listen for X Protocol
        connections.
      - sandboxDir: path where the new instance will be deployed.
      - password: password for the MySQL root user on the new instance.
      - allowRootFrom: create remote root account, restricted to the given
        address pattern (eg %).
      - ignoreSslError: Ignore errors when adding SSL support for the new
        instance, by default: true.

      If the portx option is not specified, it will be automatically calculated
      as 10 times the value of the provided MySQL port.

      The password option specifies the MySQL root password on the new
      instance.

      The sandboxDir must be an existing folder where the new instance will be
      deployed. If not specified the new instance will be deployed at:

        ~/mysql-sandboxes on Unix-like systems or
      %userprofile%\MySQL\mysql-sandboxes on Windows systems.

      SSL support is added by default if not already available for the new
      instance, but if it fails to be added then the error is ignored. Set the
      ignoreSslError option to false to ensure the new instance is deployed
      with SSL support.

EXCEPTIONS
      ArgumentError in the following scenarios:

      - If the options contain an invalid attribute.
      - If the root password is missing on the options.
      - If the port value is < 1024 or > 65535.

      RuntimeError in the following scenarios:

      - If SSL support can be provided and ignoreSslError: false.

#@<OUT> dba.drop_metadata_schema
NAME
      drop_metadata_schema - Drops the Metadata Schema.

SYNTAX
      dba.drop_metadata_schema(options)

WHERE
      options: Dictionary containing an option to confirm the drop operation.

RETURNS
       Nothing.

DESCRIPTION
      The options dictionary may contain the following options:

      - force: boolean, confirms that the drop operation must be executed.
      - clearReadOnly: boolean value used to confirm that super_read_only must
        be disabled

EXCEPTIONS
      MetadataError in the following scenarios:

      - If the Metadata is inaccessible.

      RuntimeError in the following scenarios:

      - If the current connection cannot be used for Group Replication.

#@<OUT> dba.get_cluster
NAME
      get_cluster - Retrieves a cluster from the Metadata Store.

SYNTAX
      dba.get_cluster([name][, options])

WHERE
      name: Parameter to specify the name of the cluster to be returned.
      options: Dictionary with additional options.

RETURNS
       The cluster object identified by the given name or the default cluster.

DESCRIPTION
      If name is not specified or is null, the default cluster will be
      returned.

      If name is specified, and no cluster with the indicated name is found, an
      error will be raised.

      The options dictionary accepts the connectToPrimary option, which
      defaults to true and indicates the shell to automatically connect to the
      primary member of the cluster.

EXCEPTIONS
      MetadataError in the following scenarios:

      - If the Metadata is inaccessible.
      - If the Metadata update operation failed.

      ArgumentError in the following scenarios:

      - If the Cluster name is empty.
      - If the Cluster name is invalid.
      - If the Cluster does not exist.

      RuntimeError in the following scenarios:

      - If the current connection cannot be used for Group Replication.

#@<OUT> dba.help
NAME
      help - Provides help about this object and it's members

SYNTAX
      dba.help([member])

WHERE
      member: If specified, provides detailed information on the given member.

#@<OUT> dba.kill_sandbox_instance
NAME
      kill_sandbox_instance - Kills a running MySQL Server instance on
                              localhost.

SYNTAX
      dba.kill_sandbox_instance(port[, options])

WHERE
      port: The port of the instance to be killed.
      options: Dictionary with options affecting the result.

RETURNS
       Nothing.

DESCRIPTION
      This function will kill the process of a running MySQL Server instance on
      the local host. The following options affect the result:

      - sandboxDir: path where the instance is located.

      The sandboxDir must be the one where the MySQL instance was deployed. If
      not specified it will use:

        ~/mysql-sandboxes on Unix-like systems or
      %userprofile%\MySQL\mysql-sandboxes on Windows systems.

      If the instance is not located on the used path an error will occur.

EXCEPTIONS
      ArgumentError in the following scenarios:

      - If the options contain an invalid attribute.
      - If the port value is < 1024 or > 65535.

#@<OUT> dba.reboot_cluster_from_complete_outage
NAME
      reboot_cluster_from_complete_outage - Brings a cluster back ONLINE when
                                            all members are OFFLINE.

SYNTAX
      dba.reboot_cluster_from_complete_outage([clusterName][, options])

WHERE
      clusterName: The name of the cluster to be rebooted.
      options: Dictionary with options that modify the behavior of this
               function.

RETURNS
       The rebooted cluster object.

DESCRIPTION
      The options dictionary can contain the next values:

      - password: The password used for the instances sessions required
        operations.
      - removeInstances: The list of instances to be removed from the cluster.
      - rejoinInstances: The list of instances to be rejoined on the cluster.
      - clearReadOnly: boolean value used to confirm that super_read_only must
        be disabled

      This function reboots a cluster from complete outage. It picks the
      instance the MySQL Shell is connected to as new seed instance and
      recovers the cluster. Optionally it also updates the cluster
      configuration based on user provided options.

      On success, the restored cluster object is returned by the function.

      The current session must be connected to a former instance of the
      cluster.

      If name is not specified, the default cluster will be returned.

EXCEPTIONS
      MetadataError in the following scenarios:

      - If the Metadata is inaccessible.

      ArgumentError in the following scenarios:

      - If the Cluster name is empty.
      - If the Cluster name is not valid.
      - If the options contain an invalid attribute.

      RuntimeError in the following scenarios:

      - If the Cluster does not exist on the Metadata.
      - If some instance of the Cluster belongs to a Replication Group.

#@<OUT> dba.start_sandbox_instance
NAME
      start_sandbox_instance - Starts an existing MySQL Server instance on
                               localhost.

SYNTAX
      dba.start_sandbox_instance(port[, options])

WHERE
      port: The port where the instance listens for MySQL connections.
      options: Dictionary with options affecting the result.

RETURNS
       Nothing.

DESCRIPTION
      This function will start an existing MySQL Server instance on the local
      host. The following options affect the result:

      - sandboxDir: path where the instance is located.

      The sandboxDir must be the one where the MySQL instance was deployed. If
      not specified it will use:

        ~/mysql-sandboxes on Unix-like systems or
      %userprofile%\MySQL\mysql-sandboxes on Windows systems.

      If the instance is not located on the used path an error will occur.

EXCEPTIONS
      ArgumentError in the following scenarios:

      - If the options contain an invalid attribute.
      - If the port value is < 1024 or > 65535.

#@<OUT> dba.stop_sandbox_instance
NAME
      stop_sandbox_instance - Stops a running MySQL Server instance on
                              localhost.

SYNTAX
      dba.stop_sandbox_instance(port[, options])

WHERE
      port: The port of the instance to be stopped.
      options: Dictionary with options affecting the result.

RETURNS
       Nothing.

DESCRIPTION
      This function will gracefully stop a running MySQL Server instance on the
      local host. The following options affect the result:

      - sandboxDir: path where the instance is located.
      - password: password for the MySQL root user on the instance.

      The sandboxDir must be the one where the MySQL instance was deployed. If
      not specified it will use:

        ~/mysql-sandboxes on Unix-like systems or
      %userprofile%\MySQL\mysql-sandboxes on Windows systems.

      If the instance is not located on the used path an error will occur.

EXCEPTIONS
      ArgumentError in the following scenarios:

      - If the options contain an invalid attribute.
      - If the root password is missing on the options.
      - If the port value is < 1024 or > 65535.

#@<OUT> dba.verbose
NAME
      verbose - Enables verbose mode on the dba operations.

SYNTAX
      dba.verbose

DESCRIPTION
      The assigned value can be either boolean or integer, the result depends
      on the assigned value:

      - 0: disables mysqlprovision verbosity
      - 1: enables mysqlprovision verbosity
      - >1: enables mysqlprovision debug verbosity
      - Boolean: equivalent to assign either 0 or 1

