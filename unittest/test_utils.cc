/* Copyright (c) 2015, 2019, Oracle and/or its affiliates. All rights reserved.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License, version 2.0,
   as published by the Free Software Foundation.

   This program is also distributed with certain software (including
   but not limited to OpenSSL) that is licensed under separate terms, as
   designated in a particular file or component or in included license
   documentation.  The authors of MySQL hereby grant you an additional
   permission to link the program and your derivative works with the
   separately licensed software that they have included with MySQL.
   This program is distributed in the hope that it will be useful,  but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
   the GNU General Public License, version 2.0, for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation, Inc.,
   51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA */

#include "test_utils.h"
#include <memory>
#include <random>
#include <string>
#include "db/replay/setup.h"
#include "db/uri_encoder.h"
#include "mysqlshdk/libs/textui/textui.h"
#include "mysqlshdk/libs/utils/utils_path.h"
#include "shellcore/base_session.h"
#include "shellcore/shell_resultset_dumper.h"
#include "utils/utils_file.h"
#include "utils/utils_general.h"
#include "utils/utils_string.h"

using namespace shcore;

std::vector<std::string> Shell_test_output_handler::log;
ngcommon::Logger *Shell_test_output_handler::_logger;

extern mysqlshdk::db::replay::Mode g_test_recording_mode;
extern bool g_profile_test_scripts;

Shell_test_output_handler::Shell_test_output_handler()
    : m_internal(false), m_answers_to_stdout(false) {
  deleg.user_data = this;
  deleg.print = &Shell_test_output_handler::deleg_print;
  deleg.print_error = &Shell_test_output_handler::deleg_print_error;
  deleg.print_diag = &Shell_test_output_handler::deleg_print_diag;
  deleg.prompt = &Shell_test_output_handler::deleg_prompt;
  deleg.password = &Shell_test_output_handler::deleg_password;

  full_output.clear();
  debug = false;

  // Initialize the logger and attach the hook for error verification
  // Assumes logfile already initialized
  ngcommon::Logger::setup_instance(
      ngcommon::Logger::singleton()->logfile_name().c_str(),
      getenv("TEST_DEBUG") != nullptr);
  _logger = ngcommon::Logger::singleton();
  _logger->attach_log_hook(log_hook);
}

Shell_test_output_handler::~Shell_test_output_handler() {
  _logger->detach_log_hook(log_hook);
}

void Shell_test_output_handler::log_hook(
    const ngcommon::Logger::Log_entry &entry) {
  ngcommon::Logger::LOG_LEVEL current_level = _logger->get_log_level();

  // If the level of the log is different than
  // the one set, we don't want to store the message
  if (current_level == entry.level) {
    std::string message_s(entry.message);
    log.push_back(message_s);
  }
}

void Shell_test_output_handler::deleg_print(void *user_data, const char *text) {
  Shell_test_output_handler *target = (Shell_test_output_handler *)(user_data);

  if (!target->m_internal) {
    target->full_output << text << std::endl;

    if (target->debug || g_test_trace_scripts ||
        shcore::str_beginswith(text, "**"))
      std::cout << text << std::flush;
  }

  std::lock_guard<std::mutex> lock(target->stdout_mutex);
  target->std_out.append(text);
}

void Shell_test_output_handler::deleg_print_error(void *user_data,
                                                  const char *text) {
  Shell_test_output_handler *target = (Shell_test_output_handler *)(user_data);

  target->full_output << makered(text) << std::endl;

  if (target->debug || g_test_trace_scripts)
    std::cout << makered(text) << std::endl;

  target->std_out.append(text);
}

void Shell_test_output_handler::deleg_print_diag(void *user_data,
                                                 const char *text) {
  Shell_test_output_handler *target = (Shell_test_output_handler *)(user_data);

  target->full_output << makered(text) << std::endl;

  if (target->debug || g_test_trace_scripts)
    std::cerr << makered(text) << std::endl;

  target->std_err.append(text);
}

shcore::Prompt_result Shell_test_output_handler::deleg_prompt(
    void *user_data, const char *prompt, std::string *ret) {
  Shell_test_output_handler *target = (Shell_test_output_handler *)(user_data);
  std::string answer;
  std::string expected_prompt;

  target->full_output << prompt;
  {
    std::lock_guard<std::mutex> lock(target->stdout_mutex);
    target->std_out.append(prompt);
  }

  shcore::Prompt_result ret_val = shcore::Prompt_result::Cancel;
  if (!target->prompts.empty()) {
    std::tie(expected_prompt, answer) = target->prompts.front();
    target->prompts.pop_front();

    if (expected_prompt == "*" ||
        shcore::str_beginswith(prompt, expected_prompt)) {
      target->debug_print(makegreen(
          shcore::str_format("\n--> prompt %s %s", prompt, answer.c_str())));
      target->full_output << answer << std::endl;
    } else {
      ADD_FAILURE() << "Mismatched prompts. Expected: '" << expected_prompt
                    << "'\n"
                    << "actual: '" << prompt << "'";
      target->debug_print(
          makered(shcore::str_format("\n--> mismatched prompt '%s'", prompt)));
    }
    if (answer != "<<<CANCEL>>>") ret_val = shcore::Prompt_result::Ok;
  } else {
    ADD_FAILURE() << "Unexpected prompt for '" << prompt << "'";
    target->debug_print(
        makered(shcore::str_format("\n--> unexpected prompt '%s'", prompt)));
  }

  if (target->m_answers_to_stdout) target->std_out.append(answer).append("\n");

  *ret = answer;
  return ret_val;
}

shcore::Prompt_result Shell_test_output_handler::deleg_password(
    void *user_data, const char *prompt, std::string *ret) {
  Shell_test_output_handler *target = (Shell_test_output_handler *)(user_data);
  std::string answer;
  std::string expected_prompt;

  target->full_output << prompt;
  {
    std::lock_guard<std::mutex> lock(target->stdout_mutex);
    target->std_out.append(prompt);
  }

  shcore::Prompt_result ret_val = shcore::Prompt_result::Cancel;
  if (!target->passwords.empty()) {
    std::tie(expected_prompt, answer) = target->passwords.front();
    target->passwords.pop_front();

    if (expected_prompt == "*" ||
        shcore::str_beginswith(prompt, expected_prompt)) {
      target->debug_print(makegreen(
          shcore::str_format("\n--> password %s %s", prompt, answer.c_str())));
      target->full_output << answer << std::endl;
    } else {
      ADD_FAILURE() << "Mismatched pwd prompts. Expected: '" << expected_prompt
                    << "'\n"
                    << "actual: '" << prompt << "'";
      target->debug_print(makered(
          shcore::str_format("\n--> mismatched pwd prompt '%s'", prompt)));
    }

    if (answer != "<<<CANCEL>>>") ret_val = shcore::Prompt_result::Ok;
  } else {
    ADD_FAILURE() << "Unexpected password prompt for '" << prompt << "'";
    target->debug_print(makered(
        shcore::str_format("\n--> unexpected pwd prompt '%s'", prompt)));
  }

  if (target->m_answers_to_stdout) {
    std::string password(answer.size(), '*');
    target->std_out.append(password).append("\n");
  }

  *ret = answer;
  return ret_val;
}

void Shell_test_output_handler::validate_stdout_content(
    const std::string &content, bool expected) {
  bool found = std_out.find(content) != std::string::npos;

  if (found != expected) {
    std::string error = expected ? "Missing" : "Unexpected";
    error += " Output: " + shcore::str_replace(content, "\n", "\n\t");
    ADD_FAILURE() << error << "\n"
                  << "STDOUT Actual: " +
                         shcore::str_replace(std_out, "\n", "\n\t")
                  << "\n"
                  << "STDERR Actual: " +
                         shcore::str_replace(std_err, "\n", "\n\t");
  }
}

void Shell_test_output_handler::validate_stderr_content(
    const std::string &content, bool expected) {
  if (content.empty()) {
    if (std_err.empty() != expected) {
      std::string error = std_err.empty() ? "Missing" : "Unexpected";
      error += " Error: " + shcore::str_replace(content, "\n", "\n\t");
      ADD_FAILURE() << error << "\n"
                    << "STDERR Actual: " +
                           shcore::str_replace(std_err, "\n", "\n\t")
                    << "\n"
                    << "STDOUT Actual: " +
                           shcore::str_replace(std_out, "\n", "\n\t");
    }
  } else {
    bool found = std_err.find(content) != std::string::npos;

    if (found != expected) {
      std::string error = expected ? "Missing" : "Unexpected";
      error += " Error: " + shcore::str_replace(content, "\n", "\n\t");
      ADD_FAILURE() << error << "\n"
                    << "STDERR Actual: " +
                           shcore::str_replace(std_err, "\n", "\n\t")
                    << "\n"
                    << "STDOUT Actual: " +
                           shcore::str_replace(std_out, "\n", "\n\t");
    }
  }
}

void Shell_test_output_handler::validate_log_content(
    const std::vector<std::string> &content, bool expected, bool clear) {
  for (auto &value : content) {
    bool found = false;

    if (std::find_if(log.begin(), log.end(), [&value](const std::string &str) {
          return str.find(value) != std::string::npos;
        }) != log.end()) {
      found = true;
    }

    if (found != expected) {
      std::string error = expected ? "Missing" : "Unexpected";
      error += " LOG: " + value;
      std::string s;
      for (const auto &piece : log) s += piece;

      ADD_FAILURE() << error << "\n"
                    << "LOG Actual: " + s;
    }
  }

  // Wipe the log here
  if (clear) wipe_log();
}

void Shell_test_output_handler::validate_log_content(const std::string &content,
                                                     bool expected,
                                                     bool clear) {
  bool found = false;

  if (std::find_if(log.begin(), log.end(), [&content](const std::string &str) {
        return str.find(content) != std::string::npos;
      }) != log.end()) {
    found = true;
  }

  if (found != expected) {
    std::string error = expected ? "Missing" : "Unexpected";
    error += " LOG: " + content;
    std::string s;
    for (const auto &piece : log) s += piece;

    ADD_FAILURE() << error << "\n"
                  << "LOG Actual: " + s;
  }

  // Wipe the log here
  if (clear) wipe_log();
}

void Shell_test_output_handler::debug_print(const std::string &line) {
  if (debug || g_test_trace_scripts) std::cout << line << std::endl;

  full_output << line.c_str() << std::endl;
}

void Shell_test_output_handler::debug_print_header(const std::string &line) {
  if (debug || g_test_trace_scripts) std::cerr << makebold(line) << std::endl;

  std::string splitter(line.length(), '-');

  full_output << splitter.c_str() << std::endl;
  full_output << line.c_str() << std::endl;
  full_output << splitter.c_str() << std::endl;
}

void Shell_test_output_handler::flush_debug_log() {
  full_output.flush();
  std::cerr << full_output.str();

  full_output.str(std::string());
  full_output.clear();
}

void Shell_core_test_wrapper::connect_classic() {
  execute("\\connect --mc " + _mysql_uri);
}

void Shell_core_test_wrapper::connect_x() { execute("\\connect --mx " + _uri); }

std::string Shell_core_test_wrapper::context_identifier() {
  std::string ret_val;

  auto test_info = info();

  if (test_info) {
    ret_val.append(test_info->test_case_name());
    ret_val.append(".");
    ret_val.append(test_info->name());
  }

  if (!_custom_context.empty()) {
    if (ret_val.empty())
      ret_val = _custom_context;
    else
      ret_val.append(": " + _custom_context);
  }

  return ret_val;
}

std::string Shell_core_test_wrapper::get_options_file_name(const char *name) {
  return shcore::path::join_path(shcore::get_user_config_path(), name);
}

void Shell_core_test_wrapper::SetUp() {
  Shell_base_test::SetUp();

  m_start_time = static_cast<unsigned int>(time(NULL));

  output_handler.debug_print_header(context_identifier());

  debug = false;
  output_handler.debug = debug;

  // Initializes the interactive shell
  reset_shell();

  if (getenv("TEST_DEBUG")) {
    output_handler.set_log_level(ngcommon::Logger::LOG_DEBUG);
    enable_debug();
  }
}

void Shell_core_test_wrapper::TearDown() {
  if (testutil) {
    _interactive_shell->set_global_object("testutil", {});
    testutil.reset();
  }
  _interactive_shell.reset();

  tests::Shell_base_test::TearDown();
}

void Shell_core_test_wrapper::enable_testutil() {
  bool dummy_sandboxes =
      g_test_recording_mode == mysqlshdk::db::replay::Mode::Replay;

  testutil.reset(
      new tests::Testutils(_sandbox_dir, _recording_enabled && dummy_sandboxes,
                           _interactive_shell, get_path_to_mysqlsh()));
  testutil->set_test_callbacks(
      [this](const std::string &prompt, const std::string &text) {
        output_handler.prompts.push_back({prompt, text});
      },
      [this](const std::string &prompt, const std::string &pass) {
        output_handler.passwords.push_back({prompt, pass});
      },
      [this](bool one) -> std::string {
        if (one) {
          return shcore::str_partition_after_inpl(&output_handler.std_out,
                                                  "\n");
        } else {
          return output_handler.std_out;
        }
      },
      [this](bool one) -> std::string {
        if (one) {
          return shcore::str_partition_after_inpl(&output_handler.std_err,
                                                  "\n");
        } else {
          return output_handler.std_err;
        }
      });

  if (g_test_recording_mode != mysqlshdk::db::replay::Mode::Direct)
    testutil->set_sandbox_snapshot_dir(
        mysqlshdk::db::replay::current_recording_dir());

  _interactive_shell->set_global_object("testutil", testutil);
}

void Shell_core_test_wrapper::enable_replay() {
  // Assumes reset_mysql() was already called
  setup_recorder();
}

void Shell_core_test_wrapper::reset_replayable_shell(
    const char *sub_test_name) {
  setup_recorder(sub_test_name);  // must be called before set_defaults()
  reset_shell();
  execute_setup();

#ifdef _WIN32
  mysqlshdk::db::replay::set_replay_query_hook([](const std::string &sql) {
    return shcore::str_replace(sql, ".dll", ".so");
  });
#endif

  // Intercept queries and hack their results so that we can have
  // recorded local sessions that match the actual local environment
  mysqlshdk::db::replay::set_replay_row_hook(std::bind(
      &Shell_test_env::set_replay_row_hook, this, std::placeholders::_1,
      std::placeholders::_2, std::placeholders::_3));

  // Set up hook to replace (non-deterministic) queries.
  mysqlshdk::db::replay::set_replay_query_hook(std::bind(
      &Shell_test_env::query_replace_hook, this, std::placeholders::_1));
}

void Shell_core_test_wrapper::execute(int location, const std::string &code) {
  std::string _code(code);

  unsigned int elapsed = static_cast<unsigned int>(time(NULL)) - m_start_time;

  if (g_profile_test_scripts) {
    std::string executed_input =
        makeblue(shcore::str_format("[%2u:%02u] %4d> %s", elapsed / 60,
                                    elapsed % 60, location, _code.c_str()));
    output_handler.debug_print(executed_input);
  } else {
    std::string executed_input =
        makeblue(shcore::str_format("%4d> %s", location, _code.c_str()));
    output_handler.debug_print(executed_input);
  }

  _interactive_shell->process_line(_code);
}

void Shell_core_test_wrapper::execute(const std::string &code) {
  std::string _code(code);

  std::string executed_input = makeblue("----> " + _code);
  output_handler.debug_print(executed_input);

  _interactive_shell->process_line(_code);
}

void Shell_core_test_wrapper::execute_internal(const std::string &code) {
  std::string _code(code);

  output_handler.set_internal(true);
  _interactive_shell->process_line(_code);
  output_handler.set_internal(false);
}

void Shell_core_test_wrapper::execute_noerr(const std::string &code) {
  ASSERT_EQ("", output_handler.std_err);
  execute(code);
  ASSERT_EQ("", output_handler.std_err);
}

void Shell_core_test_wrapper::exec_and_out_equals(const std::string &code,
                                                  const std::string &out,
                                                  const std::string &err) {
  std::string expected_output(out);
  std::string expected_error(err);

  if (_interactive_shell->interactive_mode() ==
          shcore::Shell_core::Mode::Python &&
      out.length())
    expected_output += "\n";

  if (_interactive_shell->interactive_mode() ==
          shcore::Shell_core::Mode::Python &&
      err.length())
    expected_error += "\n";

  execute(code);

  output_handler.std_out = str_strip(output_handler.std_out, " ");
  output_handler.std_err = str_strip(output_handler.std_err, " ");

  if (expected_output != "*") {
    EXPECT_EQ(expected_output, output_handler.std_out);
  }

  if (expected_error != "*") {
    EXPECT_EQ(expected_error, output_handler.std_err);
  }

  output_handler.wipe_all();
}

void Shell_core_test_wrapper::exec_and_out_contains(const std::string &code,
                                                    const std::string &out,
                                                    const std::string &err) {
  execute(code);

  if (out.length()) {
    SCOPED_TRACE("STDOUT missing: " + out);
    SCOPED_TRACE("STDOUT actual: " + output_handler.std_out);
    EXPECT_NE(-1, int(output_handler.std_out.find(out)));
  }

  if (err.length()) {
    SCOPED_TRACE("STDERR missing: " + err);
    SCOPED_TRACE("STDERR actual: " + output_handler.std_err);
    EXPECT_NE(-1, int(output_handler.std_err.find(err)));
  }

  output_handler.wipe_all();
}

void Crud_test_wrapper::set_functions(const std::string &functions) {
  std::vector<std::string> str_spl = split_string_chars(functions, ", ", true);
  std::copy(str_spl.begin(), str_spl.end(),
            std::inserter(_functions, _functions.end()));
}

// Validates only the specified functions are available
// non listed functions are validated for unavailability
void Crud_test_wrapper::ensure_available_functions(
    const std::string &functions) {
  bool is_js = _interactive_shell->interactive_mode() ==
               shcore::Shell_core::Mode::JavaScript;
  std::vector<std::string> v = split_string_chars(functions, ", ", true);
  std::set<std::string> valid_functions(v.begin(), v.end());

  // Retrieves the active functions on the crud operation
  if (is_js)
    exec_and_out_equals("var real_functions = dir(crud)");
  else
    exec_and_out_equals("real_functions = crud.__members__");

  // Ensures the number of available functions is the expected
  std::stringstream ss;
  ss << valid_functions.size();

  {
    SCOPED_TRACE("Unexpected number of available functions.");
    if (is_js)
      exec_and_out_equals("print(real_functions.length)", ss.str());
    else
      exec_and_out_equals("print(len(real_functions))", ss.str());
  }

  std::set<std::string>::iterator index, end = _functions.end();
  for (index = _functions.begin(); index != end; index++) {
    // If the function is suppossed to be valid it needs to be available on the
    // crud dir
    if (valid_functions.find(*index) != valid_functions.end()) {
      SCOPED_TRACE("Function " + *index + " should be available and is not.");
      if (is_js)
        exec_and_out_equals(
            "print(real_functions.indexOf('" + *index + "') != -1)", "true");
      else
        exec_and_out_equals("index=real_functions.index('" + *index + "')");
    }

    // If not, should not be on the crud dir and calling it should be illegal
    else {
      SCOPED_TRACE("Function " + *index + " should NOT be available.");
      if (is_js)
        exec_and_out_equals(
            "print(real_functions.indexOf('" + *index + "') == -1)", "true");
      else
        exec_and_out_contains("print(real_functions.index('" + *index + "'))",
                              "", "is not in list");

      exec_and_out_contains("crud." + *index + "('');", "",
                            "Forbidden usage of " + *index);
    }
  }
}
