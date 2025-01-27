/*
 * Copyright (c) 2018, 2019, Oracle and/or its affiliates. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.0,
 * as published by the Free Software Foundation.
 *
 * This program is also distributed with certain software (including
 * but not limited to OpenSSL) that is licensed under separate terms, as
 * designated in a particular file or component or in included license
 * documentation.  The authors of MySQL hereby grant you an additional
 * permission to link the program and your derivative works with the
 * separately licensed software that they have included with MySQL.
 * This program is distributed in the hope that it will be useful,  but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 * the GNU General Public License, version 2.0, for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include <map>

#include "modules/adminapi/common/provision.h"
#include "modules/adminapi/common/sql.h"
#include "mysqlshdk/include/shellcore/console.h"
#include "mysqlshdk/libs/config/config_file_handler.h"
#include "mysqlshdk/libs/config/config_server_handler.h"
#include "mysqlshdk/libs/mysql/group_replication.h"
#include "mysqlshdk/libs/mysql/replication.h"
#include "mysqlshdk/libs/utils/nullable.h"
#include "mysqlshdk/libs/utils/utils_general.h"

namespace mysqlsh {
namespace dba {

void leave_replicaset(const mysqlshdk::mysql::Instance &instance) {
  std::string instance_address = instance.get_connection_options().as_uri(
      mysqlshdk::db::uri::formats::only_transport());

  auto console = mysqlsh::current_console();

  // Check if the instance is actively member of the cluster before trying to
  // stop it (otherwise stop might fail).
  mysqlshdk::gr::Member_state state = mysqlshdk::gr::get_member_state(instance);
  if (state != mysqlshdk::gr::Member_state::OFFLINE &&
      state != mysqlshdk::gr::Member_state::MISSING) {
    // Stop Group Replication (metadata already removed)
    console->print_info("Instance '" + instance_address +
                        "' is attempting to leave the cluster...");
    mysqlshdk::gr::stop_group_replication(instance);
    // Get final state and log info.
    state = mysqlshdk::gr::get_member_state(instance);
    log_debug("Instance state after stopping Group Replication: %s",
              mysqlshdk::gr::to_string(state).c_str());
  } else {
    console->print_note("The instance '" + instance_address + "' is " +
                        mysqlshdk::gr::to_string(state) +
                        ", Group Replication stop skipped.");
  }

  // Disable and persist GR start on boot and reset values for
  // group_replication_bootstrap_group, group_replication_force_members,
  // group_replication_group_seeds and group_replication_local_address
  // NOTE: Only for server supporting SET PERSIST, version must be >= 8.0.11
  // due to BUG#26495619.
  log_debug(
      "Disabling needed group replication variables after stopping Group "
      "Replication, using SET PERSIST (if supported)");
  if (instance.get_version() >= mysqlshdk::utils::Version(8, 0, 11)) {
    const char *k_gr_remove_instance_vars_default[]{
        "group_replication_bootstrap_group", "group_replication_force_members",
        "group_replication_group_seeds", "group_replication_local_address"};
    instance.set_sysvar("group_replication_start_on_boot", false,
                        mysqlshdk::mysql::Var_qualifier::PERSIST);

    for (auto gr_var : k_gr_remove_instance_vars_default) {
      instance.set_sysvar_default(gr_var,
                                  mysqlshdk::mysql::Var_qualifier::PERSIST);
    }

    bool persist_load = *instance.get_sysvar_bool(
        "persisted_globals_load", mysqlshdk::mysql::Var_qualifier::GLOBAL);
    if (!persist_load) {
      std::string warn_msg =
          "On instance '" + instance_address +
          "' the persisted cluster configuration will not be loaded upon "
          "reboot since 'persisted-globals-load' is set to 'OFF'. Please set "
          "'persisted-globals-load' to 'ON' on the configuration file or set "
          "the 'group_replication_start_on_boot' variable to 'OFF' in the "
          "server configuration file, otherwise it might rejoin the cluster "
          "upon restart.";
      console->print_warning(warn_msg);
    }
  } else {
    std::string warn_msg =
        "On instance '" + instance_address +
        "' configuration cannot be persisted since MySQL version " +
        instance.get_version().get_base() +
        " does not support the SET PERSIST command (MySQL version >= 8.0.11 "
        "required). Please set the 'group_replication_start_on_boot' variable "
        "to 'OFF' in the server configuration file, otherwise it might rejoin "
        "the cluster upon restart.";
    console->print_warning(warn_msg);
  }
}

std::vector<mysqlshdk::gr::Invalid_config> check_instance_config(
    const mysqlshdk::mysql::IInstance &instance,
    const mysqlshdk::config::Config &config) {
  auto invalid_cfgs_vec = std::vector<mysqlshdk::gr::Invalid_config>();

  // validate server_id
  mysqlshdk::gr::check_server_id_compatibility(instance, config,
                                               &invalid_cfgs_vec);
  // validate log_bin
  mysqlshdk::gr::check_log_bin_compatibility(instance, config,
                                             &invalid_cfgs_vec);
  // validate rest of server variables required for gr
  mysqlshdk::gr::check_server_variables_compatibility(config,
                                                      &invalid_cfgs_vec);

  // NOTE: The order in the invalid_cfgs_vec is important since this vector
  // will be used for the configure_instance operation to set the correct
  // values for the variables and there are dependencies between some variables,
  // i.e, some variables need to be set before others.

  // Check if the config server handler supports the set persist syntax.
  auto srv_cfg_handler =
      dynamic_cast<mysqlshdk::config::Config_server_handler *>(
          config.get_handler(mysqlshdk::config::k_dft_cfg_server_handler));
  bool cannot_persist = (srv_cfg_handler->get_default_var_qualifier() !=
                         mysqlshdk::mysql::Var_qualifier::PERSIST);

  // For each of the configs, if the variable is a read-only variable (i.e,
  // requires a restart of the server), the user didn't provide a configuration
  // file and the instance doesn't support the set persist of the variable, then
  // we need to write that change to the configuration file as well.

  if (cannot_persist &&
      !config.has_handler(mysqlshdk::config::k_dft_cfg_file_handler)) {
    for (auto &invalid_cfg : invalid_cfgs_vec) {
      // log_bin variable is a special case and needs to be handled differently
      // since it cannot be persisted it always requires a configuration file.
      if (invalid_cfg.var_name != "log_bin" && invalid_cfg.restart)
        invalid_cfg.types.set(mysqlshdk::gr::Config_type::CONFIG);
    }
  }
  return invalid_cfgs_vec;
}

bool configure_instance(
    mysqlshdk::config::Config *config,
    const std::vector<mysqlshdk::gr::Invalid_config> &invalid_configs) {
  // An non-null Config with an server configuration handler is expected.
  // NOTE: a option file handler might not be needed/available.
  assert(config);
  assert(config->has_handler(mysqlshdk::config::k_dft_cfg_server_handler));

  // List of read_only variables and that cannot be persisted to be handled in
  // a custom way.
  std::vector<std::string> read_only_cfgs{"enforce_gtid_consistency",
                                          "log_slave_updates",
                                          "gtid_mode",
                                          "master_info_repository",
                                          "relay_log_info_repository",
                                          "transaction_write_set_extraction",
                                          "server_id"};
  std::vector<std::string> only_opt_file_cfgs{"log_bin"};

  // Workaround for server BUG#27629719, requiring some GR required variables
  // to be set in a certain order, namely: enforce_gtid_consistency before
  // gtid_mode. The is expected to be correct from the input parameter
  // invalid_configs and maintained. However, a delay is required to avoid them
  // from having the same timestamp in mysqld-auto.cnf when persisted.
  std::vector<std::string> persist_delay_cfgs{"enforce_gtid_consistency"};

  // Get the config server handler reference (to persist read_only variables).
  auto *srv_cfg_handler =
      dynamic_cast<mysqlshdk::config::Config_server_handler *>(
          config->get_handler(mysqlshdk::config::k_dft_cfg_server_handler));

  // Check if set persist is supported.
  log_debug("Server variable will be changed using SET PERSIST/PERSIST_ONLY.");
  bool use_set_persist = (srv_cfg_handler->get_default_var_qualifier() ==
                          mysqlshdk::mysql::Var_qualifier::PERSIST);

  // Lambda functions to set server variables using PERSIST_ONLY.
  auto set_persist_only = [&srv_cfg_handler](mysqlshdk::gr::Invalid_config &ic,
                                             uint32_t d) {
    if (ic.val_type == shcore::Value_type::Integer) {
      srv_cfg_handler->set(ic.var_name,
                           mysqlshdk::utils::nullable<int64_t>(
                               shcore::lexical_cast<int64_t>(ic.required_val)),
                           mysqlshdk::mysql::Var_qualifier::PERSIST_ONLY, d);
    } else {
      srv_cfg_handler->set(
          ic.var_name, mysqlshdk::utils::nullable<std::string>(ic.required_val),
          mysqlshdk::mysql::Var_qualifier::PERSIST_ONLY, d);
    }
  };

  // Set required values for incompatible configurations.
  bool need_restart = false;
  for (auto invalid_cfg : invalid_configs) {
    // Check if any of the invalid configurations requires a restart.
    if (invalid_cfg.restart) {
      need_restart = true;
    }

    // Generate server_id if one of the variables to configure.
    if (invalid_cfg.var_name == "server_id") {
      invalid_cfg.required_val =
          std::to_string(mysqlshdk::mysql::generate_server_id());
    }

    // Determine if the variable can only be changed on the option file.
    bool only_opt_file =
        std::find(only_opt_file_cfgs.begin(), only_opt_file_cfgs.end(),
                  invalid_cfg.var_name) != only_opt_file_cfgs.end();

    // Determine if the variable is read-only (to use SET PERSIST_ONLY or not
    // change it on the server).
    bool read_only_var =
        std::find(read_only_cfgs.begin(), read_only_cfgs.end(),
                  invalid_cfg.var_name) != read_only_cfgs.end();
    bool persist_only_var = use_set_persist && read_only_var;

    // Determine if the variable requires a delay for SET PERSIST.
    // Workaround related with server BUG#27629719: wait 1 ms after each
    // SET PERSIST to ensure a different timestamp is produced.
    uint32_t delay =
        (use_set_persist &&
         std::find(persist_delay_cfgs.begin(), persist_delay_cfgs.end(),
                   invalid_cfg.var_name) != persist_delay_cfgs.end())
            ? 1
            : 0;

    // Invalid configuration on the server.
    // NOTE: Skip it if it can only be changed on the option file.
    if (invalid_cfg.types.is_set(mysqlshdk::gr::Config_type::SERVER) &&
        !only_opt_file) {
      log_debug(
          "Setting '%s' to '%s' on server (no change actually applied yet).",
          invalid_cfg.var_name.c_str(), invalid_cfg.required_val.c_str());

      if (persist_only_var) {
        // Use SET PERSIST_ONLY for read-only variables if supported.
        // NOTE: The only variable that requires a delay is a PERSIST_ONLY one.
        set_persist_only(invalid_cfg, delay);
      } else if (!read_only_var) {
        // Otherwise set variable using server supported configuration, but only
        // if it is not a read_only variable.
        // NOTE: Convert value to set to the proper type (i.e., int) if needed.
        if (invalid_cfg.val_type == shcore::Value_type::Integer) {
          config->set(
              invalid_cfg.var_name,
              mysqlshdk::utils::nullable<int64_t>(
                  shcore::lexical_cast<int64_t>(invalid_cfg.required_val)),
              mysqlshdk::config::k_dft_cfg_server_handler);
        } else {
          config->set(invalid_cfg.var_name, invalid_cfg.required_val,
                      mysqlshdk::config::k_dft_cfg_server_handler);
        }
      }
    }
    // Invalid configuration on the option file.
    // NOTE: Skip it if option file is not available.
    if (invalid_cfg.types.is_set(mysqlshdk::gr::Config_type::CONFIG) &&
        config->has_handler(mysqlshdk::config::k_dft_cfg_file_handler)) {
      // Check if the option needs to be removed from the option file.
      // NOTE: Only applies to skip-log-bin and disable-log-bin options which
      //       do not have a corresponding server variable.
      if (invalid_cfg.required_val == mysqlshdk::gr::k_value_not_set) {
        log_debug(
            "Removing '%s' from the option file (no change actually applied "
            "yet).",
            invalid_cfg.var_name.c_str());

        // Get the config file handler to remove the option from the file.
        auto file_cfg_handler =
            dynamic_cast<mysqlshdk::config::Config_file_handler *>(
                config->get_handler(mysqlshdk::config::k_dft_cfg_file_handler));
        file_cfg_handler->remove(invalid_cfg.var_name);
      } else {
        log_debug(
            "Setting '%s' to '%s' on option file (no change actually applied "
            "yet).",
            invalid_cfg.var_name.c_str(), invalid_cfg.required_val.c_str());
        mysqlshdk::utils::nullable<std::string> req_val{
            invalid_cfg.required_val};
        if (invalid_cfg.required_val == mysqlshdk::gr::k_no_value) {
          // convert special string no_value to an empty nullable.
          req_val = mysqlshdk::utils::nullable<std::string>();
        }
        // Set the variable on the option file.
        config->set(invalid_cfg.var_name, req_val,
                    mysqlshdk::config::k_dft_cfg_file_handler);
      }
    }
  }
  // Apply configuration changes.
  log_debug("Applying changes for all variables previously set.");
  config->apply();

  return need_restart;
}

void persist_gr_configurations(const mysqlshdk::mysql::IInstance &instance,
                               mysqlshdk::config::Config *config) {
  // An non-null Config with an option file configuration handler is expected.
  assert(config);
  assert(config->has_handler(mysqlshdk::config::k_dft_cfg_file_handler));

  // Get group seeds information from metadata.
  // NOTE: Need to use the reported host to get the correct information from
  //       the MetaData.
  std::string reported_host = mysqlshdk::mysql::get_report_host(instance);
  Connection_options cnx_opts = instance.get_connection_options();
  cnx_opts.clear_host();  // Clear first to avoid error for being already set.
  cnx_opts.set_host(reported_host);
  std::vector<std::string> seeds = get_peer_seeds(
      instance.get_session(),
      cnx_opts.as_uri(mysqlshdk::db::uri::formats::only_transport()));

  // Get all GR configurations.
  log_debug("Get all group replication configurations.");
  std::map<std::string, mysqlshdk::utils::nullable<std::string>> gr_cfgs =
      mysqlshdk::gr::get_all_configurations(instance);

  // Set all GR configurations.
  log_debug("Set all group replication configurations to be applied.");
  for (const auto &gr_cfg : gr_cfgs) {
    config->set(gr_cfg.first, gr_cfg.second,
                mysqlshdk::config::k_dft_cfg_file_handler);
  }

  // Update the group_replication_group_seeds.
  if (!seeds.empty()) {
    std::string peer_seeds = shcore::str_join(seeds, ",");
    config->set("group_replication_group_seeds",
                mysqlshdk::utils::nullable<std::string>(peer_seeds));
  }

  // Apply all changes.
  log_debug("Apply group replication configurations (write to file).");
  config->apply();
}

}  // namespace dba
}  // namespace mysqlsh
