//@ WL#12049: Initialization
||

//@ WL#12049: Unsupported server version {VER(<5.7.24)}
||Option 'exitStateAction' not supported on target server version: '<<<__version>>>'

//@ WL#12049: Create cluster errors using exitStateAction option {VER(>=5.7.24)}
||Invalid value for exitStateAction, string value cannot be empty.
||Invalid value for exitStateAction, string value cannot be empty.
||Error starting cluster: Invalid value for exitStateAction, can't be set to the value of ':'
||Error starting cluster: Invalid value for exitStateAction, can't be set to the value of 'AB'
||Error starting cluster: Invalid value for exitStateAction, can't be set to the value of '10'

//@ WL#12049: Create cluster specifying a valid value for exitStateAction (ABORT_SERVER) {VER(>=5.7.24)}
||

//@ WL#12049: Finalization
||

//@ WL#11032: Initialization
||

//@ WL#11032: Unsupported server version {VER(<5.7.20)}
||Option 'memberWeight' not available for target server version.

//@ WL#11032: Create cluster errors using memberWeight option {VER(>=5.7.20)}
||Option 'memberWeight' Integer expected, but value is String (TypeError)
||Option 'memberWeight' is expected to be of type Integer, but is Bool (TypeError)
||Option 'memberWeight' Integer expected, but value is String (TypeError)
||Dba.createCluster: Option 'memberWeight' is expected to be of type Integer, but is Float (TypeError)


//@ WL#11032: Create cluster specifying a valid value for memberWeight (integer) {VER(>=5.7.20)}
||

//@ WL#11032: Finalization
||

//@ WL#12067: Initialization
||

//@ WL#12067: TSF1_6 Unsupported server version {VER(<8.0.14)}
||Option 'consistency' not supported on target server version: '<<<__version>>>'

//@ WL#12067: Create cluster errors using consistency option {VER(>=8.0.14)}
||Invalid value for consistency, string value cannot be empty.
||Invalid value for consistency, string value cannot be empty.
||Error starting cluster: Invalid value for consistency, can't be set to the value of ':'
||Error starting cluster: Invalid value for consistency, can't be set to the value of 'AB'
||Error starting cluster: Invalid value for consistency, can't be set to the value of '10'
||Option 'consistency' is expected to be of type String, but is Integer (TypeError)
||Cannot use the failoverConsistency and consistency options simultaneously. The failoverConsistency option is deprecated, please use the consistency option instead. (ArgumentError)

//@ WL#12067: TSF1_1 Create cluster using a valid as value for consistency {VER(>=8.0.14)}
||

//@<OUT> Create cluster using a valid value for failoverConsistency {VER(>=8.0.14)}
WARNING: The failoverConsistency option is deprecated. Please use the consistency option instead.

//@ WL#12067: Finalization
||

//@ WL#12050: Initialization
||

//@ WL#12050: TSF1_5 Unsupported server version {VER(<8.0.13)}
||Option 'expelTimeout' not supported on target server version: '<<<__version>>>'

//@ WL#12050: Create cluster errors using expelTimeout option {VER(>=8.0.13)}
// TSF1_3, TSF1_4, TSF1_6
||Option 'expelTimeout' Integer expected, but value is String (TypeError)
||Option 'expelTimeout' Integer expected, but value is String (TypeError)
||Option 'expelTimeout' is expected to be of type Integer, but is Float (TypeError)
||Option 'expelTimeout' is expected to be of type Integer, but is Bool (TypeError)
||Invalid value for expelTimeout, integer value must be in the range: [0, 3600] (ArgumentError)
||Invalid value for expelTimeout, integer value must be in the range: [0, 3600] (ArgumentError)

//@ WL#12050: TSF1_1 Create cluster using a valid value for expelTimeout {VER(>=8.0.13)}
||

//@ WL#12050: Finalization
||
