/*
 * Copyright (c) 2015, 2019, Oracle and/or its affiliates. All rights reserved.
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

#ifndef _SHELLCORE_PYTHON_H_
#define _SHELLCORE_PYTHON_H_

#include <string>

#include "shellcore/shell_core.h"

namespace shcore {
class Python_context;

class Shell_python : public Shell_language {
 public:
  Shell_python(Shell_core *shcore);

  void set_global(const std::string &name, const Value &value) override;

  void set_result_processor(
      std::function<void(shcore::Value, bool)> result_processor);

  std::function<void(shcore::Value, bool)> result_processor() const {
    return _result_processor;
  }

  std::string preprocess_input_line(const std::string &s) override;
  void handle_input(std::string &code, Input_state &state) override;

  bool is_module(const std::string &file_name) override;
  void execute_module(const std::string &file_name) override;
  void load_plugin(const std::string &file_name) override;

  std::shared_ptr<Python_context> python_context() { return _py; }

  void clear_input() override;
  std::string get_continued_input_context() override;

 private:
  static int check_signals(void *);
  std::shared_ptr<Python_context> _py;
  std::function<void(shcore::Value, bool)> _result_processor;
  Input_state m_last_input_state;
  long _pending_interrupt_thread;
  bool _aborted = false;

  void abort(long tid) noexcept;
};
};  // namespace shcore

#endif
