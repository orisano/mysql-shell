/*
 * Copyright (c) 2018, Oracle and/or its affiliates. All rights reserved.
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

#include "modules/mod_shell_reports.h"

#include <algorithm>
#include <limits>
#include <locale>
#include <set>

#include "modules/mod_utils.h"
#include "mysqlshdk/include/shellcore/shell_resultset_dumper.h"
#include "mysqlshdk/include/shellcore/utils_help.h"
#include "mysqlshdk/libs/textui/textui.h"
#include "mysqlshdk/libs/utils/options.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_string.h"

namespace mysqlsh {

namespace {

REGISTER_HELP_OBJECT(reports, shell);
REGISTER_HELP(REPORTS_BRIEF,
              "Gives access to built-in and user-defined reports.");

REGISTER_HELP(REPORTS_DETAIL,
              "The 'reports' object provides access to built-in reports.");
REGISTER_HELP(REPORTS_DETAIL1,
              "All user-defined reports registered using the "
              "shell.<<<registerReport>>>() method are also available here.");
REGISTER_HELP(REPORTS_DETAIL2,
              "The reports are provided as methods of this object, with names "
              "corresponding to the names of the available reports.");
REGISTER_HELP(REPORTS_DETAIL3,
              "All methods have the same signature: <b>Dict report(Session "
              "session, List argv, Dict options)</b>, where:");
REGISTER_HELP(
    REPORTS_DETAIL4,
    "@li session - Session object used by the report to obtain the data.");
REGISTER_HELP(REPORTS_DETAIL5,
              "@li argv (optional) - Array of strings representing additional "
              "arguments.");
REGISTER_HELP(REPORTS_DETAIL6,
              "@li options (optional) - Dictionary with values for various "
              "report-specific options.");
REGISTER_HELP(REPORTS_DETAIL7,
              "Each report returns a dictionary with the following keys:");
REGISTER_HELP(REPORTS_DETAIL8,
              "@li report (required) - List of JSON objects containing the "
              "report. The number and types of items in this list depend on "
              "type of the report.");
REGISTER_HELP(REPORTS_DETAIL9,
              "For more information on a report use: "
              "<b>shell.reports.help('report_name')</b>.");

static constexpr auto k_report_key = "report";
static constexpr auto k_vertical_key = "vertical";
static constexpr auto k_wildcard_character = "*";
static constexpr auto k_report_type_list = "list";
static constexpr auto k_report_type_report = "report";
static constexpr auto k_report_type_print = "print";
static constexpr size_t k_help_width = 80;
static constexpr size_t k_help_left_padding = 0;
static constexpr uint32_t k_asterisk = std::numeric_limits<uint32_t>::max();

Report::Type to_report_type(const std::string &type) {
  if (k_report_type_list == type) {
    return Report::Type::LIST;
  } else if (k_report_type_report == type) {
    return Report::Type::REPORT;
  } else if (k_report_type_print == type) {
    return Report::Type::PRINT;
  } else {
    throw shcore::Exception::argument_error(
        "Report type must be one of: " +
        shcore::str_join(
            std::vector<std::string>{k_report_type_list, k_report_type_report,
                                     k_report_type_print},
            ", ") +
        ".");
  }
}

std::string to_string(Report::Type type) {
  switch (type) {
    case Report::Type::LIST:
      return k_report_type_list;

    case Report::Type::REPORT:
      return k_report_type_report;

    case Report::Type::PRINT:
      return k_report_type_print;
  }

  throw std::logic_error("Unknown value of Report::Type");
}

std::string to_string(const Report::Argc &argc) {
  std::string result;

  if (argc.first == 0 && argc.second == k_asterisk) {
    result += "any number of";
  } else {
    result += std::to_string(argc.first);

    if (argc.first != argc.second) {
      result += "-";

      if (argc.second == k_asterisk) {
        result += k_wildcard_character;
      } else {
        result += std::to_string(argc.second);
      }
    }
  }

  result += " argument";

  if (argc.first != argc.second || argc.first != 1) {
    result += "s";
  }

  return result;
}

void validate_option_type(shcore::Value_type type) {
  switch (type) {
    case shcore::Value_type::String:
    case shcore::Value_type::Bool:
    case shcore::Value_type::Integer:
    case shcore::Value_type::Float:
      // valid type
      return;

    default:
      throw shcore::Exception::argument_error(
          "Option type must be one of: 'string', 'bool', 'integer', 'float'.");
  }
}

uint32_t to_uint32(const std::string &v) {
  try {
    return shcore::lexical_cast<uint32_t>(v);
  } catch (const std::invalid_argument &) {
    throw shcore::Exception::argument_error("Cannot convert '" + v +
                                            "' to an unsigned integer.");
  }
}

Report::Argc get_report_argc(const std::string &argc_s) {
  if (argc_s.empty()) {
    return {0, 0};
  }

  const auto argc = shcore::str_split(argc_s, "-");
  Report::Argc result;

  switch (argc.size()) {
    case 1:
      if (argc[0] == k_wildcard_character) {
        result = std::make_pair(0, k_asterisk);
      } else {
        const auto limit = to_uint32(argc[0]);
        result = std::make_pair(limit, limit);
      }
      break;

    case 2: {
      const auto lower = to_uint32(argc[0]);
      const auto upper =
          argc[1] == k_wildcard_character ? k_asterisk : to_uint32(argc[1]);
      result = std::make_pair(lower, upper);
    } break;

    default:
      throw shcore::Exception::argument_error(
          "The value associated with the key named 'argc' has wrong format.");
  }

  return result;
}

class Native_report_function : public shcore::Cpp_function {
 public:
  Native_report_function(const std::string &name,
                         const Report::Native_report &report)
      : shcore::Cpp_function(
            &m_metadata, [report](const shcore::Argument_list &args) {
              const auto &session = args.object_at<ShellBaseSession>(0);

              if (!session) {
                throw shcore::Exception::argument_error(
                    "Argument #1 is expected to be one of 'ClassicSession, "
                    "Session'.");
              }

              if (!session->is_open()) {
                throw shcore::Exception::argument_error(
                    "Executing the report requires an open session.");
              }

              // it's not an array if it's null
              const auto argv =
                  args.size() < 2 || args[1].type != shcore::Value_type::Array
                      ? shcore::make_array()
                      : args.array_at(1);
              // it's not a map if it's null
              const auto options =
                  args.size() < 3 || args[2].type != shcore::Value_type::Map
                      ? shcore::make_dict()
                      : args.map_at(2);

              return shcore::Value(report(session, argv, options));
            }) {
    for (std::size_t i = 0; i < shcore::array_size(m_metadata.name); ++i) {
      m_metadata.name[i] = name;
    }

    m_metadata.param_types = {{"session", shcore::Value_type::Object},
                              {"?argv", shcore::Value_type::Array},
                              {"?options", shcore::Value_type::Map}};
    m_metadata.signature = gen_signature(m_metadata.param_types);
    m_metadata.return_type = shcore::Value_type::Map;
    m_metadata.var_args = false;
  }

  ~Native_report_function() override = default;

 private:
  shcore::Cpp_function::Metadata m_metadata;
};

// minimal implementation of a cursor over an array in order to reuse
// Resultset_dumper
class Array_as_result : public mysqlshdk::db::IResult {
 public:
  explicit Array_as_result(const shcore::Array_t &array) {
    if (array->size() < 1) {
      throw shcore::Exception::runtime_error(
          "List report should contain at least one row.");
    }

    for (const auto &row : *array) {
      if (row.type != shcore::Value_type::Array) {
        throw shcore::Exception::runtime_error(
            "List report should return a list of lists.");
      }

      m_data.emplace_back();

      for (const auto &value : *row.as_array()) {
        bool is_null = value.type == shcore::Value_type::Undefined ||
                       value.type == shcore::Value_type::Null;
        m_data.back().emplace_back(is_null ? "NULL" : value.descr());
      }
    }

    for (const auto &column : m_data[0]) {
      m_metadata.emplace_back(
          "unknown",  // catalog will not be used by dumper
          "unknown",  // schema will not be used by dumper
          "unknown",  // table name will not be used by dumper
          "unknown",  // table label will not be used by dumper
          column,     // column name
          column,     // column label
          1,  // length will not be used by dumper if zero-fill is set to false
          1,  // fractional digits will not be used by dumper
          mysqlshdk::db::Type::String,  // type
          1,       // collation ID will not be used by dumper
          false,   // unsigned
          false,   // zero-fill
          false);  // binary will not be used by dumper
    }
  }

  const mysqlshdk::db::IRow *fetch_one() override {
    if (!has_resultset()) {
      return nullptr;
    }

    if (m_current_row < m_data.size()) {
      m_row = shcore::make_unique<Vector_as_row>(m_data[m_current_row]);
      ++m_current_row;
      return m_row.get();
    } else {
      return nullptr;
    }
  }

  bool next_resultset() override {
    m_has_result = false;
    return has_resultset();
  }

  bool has_resultset() override {
    return static_cast<const Array_as_result *>(this)->has_resultset();
  }

  bool has_resultset() const { return m_has_result; }

  const std::vector<mysqlshdk::db::Column> &get_metadata() const override {
    if (!has_resultset()) {
      throw std::logic_error("No result, unable to fetch metadata");
    }

    return m_metadata;
  }

  void buffer() override {
    // data is always buffered
  }

  void rewind() override {
    if (!has_resultset()) {
      throw std::logic_error("No result, unable to rewind");
    }

    m_current_row = 1;
  }

  std::unique_ptr<mysqlshdk::db::Warning> fetch_one_warning() override {
    throw std::logic_error("Not implemented.");
  }

  int64_t get_auto_increment_value() const override {
    throw std::logic_error("Not implemented.");
  }

  uint64_t get_affected_row_count() const override {
    throw std::logic_error("Not implemented.");
  }

  uint64_t get_fetched_row_count() const override {
    throw std::logic_error("Not implemented.");
  }

  uint64_t get_warning_count() const override {
    throw std::logic_error("Not implemented.");
  }

  std::string get_info() const override {
    throw std::logic_error("Not implemented.");
  }

  const std::vector<std::string> &get_gtids() const override {
    throw std::logic_error("Not implemented.");
  }

  std::shared_ptr<mysqlshdk::db::Field_names> field_names() const override {
    throw std::logic_error("Not implemented.");
  }

 private:
  class Vector_as_row : public mysqlshdk::db::IRow {
   public:
    explicit Vector_as_row(const std::vector<std::string> &row) : m_row(row) {}

    mysqlshdk::db::Type get_type(uint32_t idx) const override {
      return mysqlshdk::db::Type::String;
    }

    bool is_null(uint32_t idx) const override { return idx >= m_row.size(); }

    std::string get_as_string(uint32_t idx) const override {
      return get_string(idx);
    }

    std::string get_string(uint32_t idx) const override {
      if (idx < m_row.size()) {
        return m_row[idx];
      } else {
        return "NULL";
      }
    }

    int64_t get_int(uint32_t) const override {
      throw std::logic_error("Not implemented.");
    }

    uint64_t get_uint(uint32_t) const override {
      throw std::logic_error("Not implemented.");
    }

    float get_float(uint32_t idx) const override {
      throw std::logic_error("Not implemented.");
    }

    double get_double(uint32_t idx) const override {
      throw std::logic_error("Not implemented.");
    }

    uint32_t num_fields() const override {
      throw std::logic_error("Not implemented.");
    }

    std::pair<const char *, size_t> get_string_data(uint32_t) const override {
      throw std::logic_error("Not implemented.");
    }

    uint64_t get_bit(uint32_t) const override {
      throw std::logic_error("Not implemented.");
    }

   private:
    const std::vector<std::string> &m_row;
  };

  bool m_has_result = true;
  size_t m_current_row = 1;
  std::vector<mysqlshdk::db::Column> m_metadata;
  std::vector<std::vector<std::string>> m_data;
  std::unique_ptr<Vector_as_row> m_row;
};

std::string list_formatter(const shcore::Array_t &a,
                           const shcore::Dictionary_t &options) {
  bool is_vertical = false;
  shcore::Option_unpacker{options}.required(k_vertical_key, &is_vertical).end();

  Array_as_result result{a};
  Resultset_writer writer{&result};

  const auto output =
      is_vertical ? writer.write_vertical() : writer.write_table();

  // output is empty if report returned just names of columns
  return output.empty() ? "Report returned no data." : output;
}

std::string report_formatter(const shcore::Array_t &a,
                             const shcore::Dictionary_t &) {
  if (a->size() != 1) {
    throw shcore::Exception::runtime_error(
        "Report of type 'report' should contain exactly one element.");
  }

  return a->at(0).yaml();
}

std::string print_formatter(const shcore::Array_t &,
                            const shcore::Dictionary_t &) {
  // print formatter suppresses all output
  return "";
}

shcore::Dictionary_t query_report(
    const std::shared_ptr<ShellBaseSession> &session,
    const shcore::Array_t &argv, const shcore::Dictionary_t &) {
  std::string query;

  for (const auto &a : *argv) {
    query += a.as_string() + " ";
  }

  query += ";";

  // note: we're expecting a single resultset
  const auto result = session->get_core_session()->query(query);
  auto report = shcore::make_array();

  // write headers
  {
    auto headers = shcore::make_array();

    for (const auto &column : result->get_metadata()) {
      headers->emplace_back(column.get_column_label());
    }

    report->emplace_back(std::move(headers));
  }

  // write data
  while (const auto row = result->fetch_one()) {
    auto json_row = shcore::make_array();
    auto values = get_row_values(*row);

    json_row->reserve(values.size());
    json_row->insert(json_row->end(), std::make_move_iterator(values.begin()),
                     std::make_move_iterator(values.end()));

    report->emplace_back(std::move(json_row));
  }

  const auto json_result = shcore::make_dict();

  json_result->emplace(k_report_key, std::move(report));

  return json_result;
}

Parameter_definition::Options upcast(const Report::Options &in) {
  return Parameter_definition::Options(in.begin(), in.end());
}

Report::Options downcast(const Parameter_definition::Options &in) {
  Report::Options out;

  for (const auto &p : in) {
    out.emplace_back(std::dynamic_pointer_cast<Report::Option>(p));
  }

  return out;
}

struct Argv_validator : public shcore::Parameter_validator {
  explicit Argv_validator(const Report::Argc &argc) : m_argc(argc) {}

  void validate(const shcore::Parameter &param, const shcore::Value &data,
                const shcore::Parameter_context &context) const override {
    Parameter_validator::validate(param, data, context);

    const auto argv = data.as_array();
    const auto argc = argv ? argv->size() : 0;

    if (argc < m_argc.first || argc > m_argc.second) {
      throw shcore::Exception::argument_error(
          shcore::str_format("%s 'argv' is expecting %s.",
                             context.str().c_str(), to_string(m_argc).c_str()));
    }
  }

 private:
  Report::Argc m_argc;
};

std::string normalize_report_name(const std::string &name) {
  return shcore::str_lower(shcore::str_replace(name, "-", "_"));
}

}  // namespace

class Shell_reports::Report_options : public shcore::Options {
 public:
  explicit Report_options(std::unique_ptr<Report> r)
      : m_report_name(std::move(r->m_name)),
        m_options(std::move(r->m_options)),
        m_argc(std::move(r->m_argc)),
        m_formatter(std::move(r->m_formatter)) {
    // each report has an option to display help
    add_startup_options()(&m_show_help, false, shcore::opts::cmdline("--help"),
                          "Display this help and exit.");

    // list reports can be displayed vertically
    if (Report::Type::LIST == r->type()) {
      add_startup_options()(&m_vertical, false,
                            shcore::opts::cmdline("--vertical", "-E"),
                            "Display records vertically.");
    }

    prepare_options();

    // add options expected by the report
    for (const auto &o : m_options) {
      const auto &p = o->parameter;
      std::vector<std::string> names;
      std::string long_name = "--" + p->name;

      // non-Boolean reports require a value
      if (shcore::Value_type::Bool != p->type()) {
        long_name += "=" + shcore::str_lower(shcore::type_name(p->type()));
      }

      names.emplace_back(long_name);

      // options can optionally have short (one letter) form
      if (!o->short_name.empty()) {
        names.emplace_back("-" + o->short_name);
      }

      // register the option
      add_startup_options()(
          std::move(names), o->brief.c_str(),
          [&o, &p, this](const std::string &, const char *new_value) {
            if (shcore::Value_type::String == p->type()) {
              p->validator<shcore::String_validator>()->validate(
                  *p, shcore::Value(new_value),
                  {m_report_name + ": option '--" + p->name + "'"});
            }

            // convert to the expected type
            shcore::Value value;

            switch (p->type()) {
              case shcore::Value_type::String:
                value = shcore::Value(new_value);
                break;

              case shcore::Value_type::Bool:
                value = shcore::Value(true);
                break;

              case shcore::Value_type::Integer:
                try {
                  value =
                      shcore::Value(shcore::lexical_cast<int64_t>(new_value));
                } catch (const std::invalid_argument &) {
                  throw prepare_exception("cannot convert '" +
                                          std::string(new_value) +
                                          "' to a signed integer");
                }
                break;

              case shcore::Value_type::Float:
                try {
                  value =
                      shcore::Value(shcore::lexical_cast<double>(new_value));
                } catch (const std::invalid_argument &) {
                  throw prepare_exception("cannot convert '" +
                                          std::string(new_value) +
                                          "' to a floating-point number");
                }
                break;

              default:
                throw std::logic_error("Unexpected type: " +
                                       shcore::type_name(p->type()));
            }

            // store the value
            m_parsed_options->emplace(p->name, value);

            // if option is required, erase it from the list of missing ones
            if (o->is_required()) {
              m_missing_options.erase(
                  std::remove(m_missing_options.begin(),
                              m_missing_options.end(), p->name),
                  m_missing_options.end());
            }
          });
    }

    initialize_help(r->brief(), r->details());
  }

  const std::string &name() const { return m_report_name; }

  bool show_help() const { return m_show_help; }

  bool vertical() const { return m_vertical; }

  void parse_args(const std::vector<std::string> &args) {
    // reset previous values and prepare for parsing
    reset();

    // prepare arguments
    std::vector<const char *> raw_args;
    raw_args.emplace_back(m_report_name.c_str());

    for (const auto &a : args) {
      raw_args.emplace_back(a.c_str());
    }

    // NULL must be the last argument in the vector
    raw_args.emplace_back(nullptr);

    // validate and parse provided arguments
    // NULL should not be included in the size of arguments
    handle_cmdline_options(
        raw_args.size() - 1, &raw_args[0], false, [this](Cmdline_iterator *it) {
          // handle extra arguments
          if (it->valid() && (it->peek()[0] != '-' || m_arguments_started)) {
            // first option which does not begin with '-' marks the beginning of
            // arguments
            m_arguments_started = true;
            m_arguments.emplace_back(it->get());
            return true;
          } else {
            return false;
          }
        });

    // if help was not requested, perform additional validations
    if (!show_help()) {
      // check if all required options are here
      if (!m_missing_options.empty()) {
        std::transform(m_missing_options.begin(), m_missing_options.end(),
                       m_missing_options.begin(),
                       [](const std::string &o) { return "--" + o; });
        throw prepare_exception("missing required option(s): " +
                                shcore::str_join(m_missing_options, ", ") +
                                ".");
      }

      // check if there's the right amount of additional arguments
      const auto argc = m_arguments.size();

      if (argc < m_argc.first || argc > m_argc.second) {
        throw prepare_exception("expecting " + to_string(m_argc) + ".");
      }

      // optionally add additional arguments
      for (const auto &a : m_arguments) {
        m_parsed_arguments->emplace_back(a);
      }
    }
  }

  shcore::Array_t get_parsed_arguments() const {
    if (show_help()) {
      throw std::logic_error(
          "User requested help, arguments are not available.");
    }

    return m_parsed_arguments;
  }

  shcore::Dictionary_t get_parsed_options() const {
    if (show_help()) {
      throw std::logic_error("User requested help, options are not available.");
    }

    return m_parsed_options;
  }

  std::string help() const { return m_help; }

  bool requires_argv() const { return m_argc.second > 0 || requires_options(); }

  bool requires_options() const { return !m_options.empty(); }

  const Report::Formatter &formatter() const { return m_formatter; }

 private:
  void reset() {
    // reset to default values
    m_show_help = false;
    m_vertical = false;
    m_arguments_started = false;

    m_missing_options.clear();
    m_arguments.clear();
    m_parsed_options = shcore::make_dict();
    m_parsed_arguments = shcore::make_array();

    // mark all required options as missing
    for (const auto &o : m_options) {
      if (o->is_required()) {
        m_missing_options.emplace_back(o->parameter->name);
      }
    }
  }

  void prepare_options() {
    for (auto &o : m_options) {
      o->brief = (o->is_required() ? "(required) " : "") + o->brief;

      if (shcore::Value_type::String == o->parameter->type()) {
        const auto &allowed =
            o->parameter->validator<shcore::String_validator>()->allowed();

        if (!allowed.empty()) {
          o->brief +=
              " Allowed values: " + shcore::str_join(allowed, ", ") + ".";
        }
      }
    }
  }

  void initialize_help(const std::string &brief,
                       const std::vector<std::string> &details) {
    if (m_help.empty()) {
      std::vector<std::string> contents;

      contents.emplace_back(m_report_name + " - " + brief);
      contents.emplace_back("");

      for (const auto &d : details) {
        contents.emplace_back(d);
        contents.emplace_back("");
      }

      const auto argc = m_argc;
      const auto has_arguments = argc.second > 0;

      contents.emplace_back("Usage:");

      for (const auto &command : {"show", "watch"}) {
        std::string line = "       \\" + std::string(command) + " " +
                           m_report_name + " [OPTIONS]";

        if (has_arguments) {
          line += " [ARGUMENTS]";
        }

        contents.emplace_back(std::move(line));
      }

      contents.emplace_back("");
      contents.emplace_back("Options:");

      for (const auto &line : get_cmdline_help(30, 48)) {
        contents.emplace_back("  " + line);
      }

      contents.emplace_back("");

      for (auto &o : m_options) {
        for (const auto &d : o->details) {
          contents.emplace_back("  " + d);
          contents.emplace_back("");
        }
      }

      if (has_arguments) {
        contents.emplace_back("Arguments:");
        contents.emplace_back("  This report accepts " + to_string(argc) + ".");
        contents.emplace_back("");
      }

      m_help = mysqlshdk::textui::format_markup_text(
          contents, k_help_width, k_help_left_padding, false);
    }
  }

  shcore::Exception prepare_exception(const std::string &e) const {
    return shcore::Exception::argument_error(m_report_name + ": " + e);
  }

  const std::string m_report_name;
  const Report::Options m_options;
  const Report::Argc m_argc;
  const Report::Formatter m_formatter;
  std::string m_help;
  bool m_show_help;
  bool m_vertical;
  bool m_arguments_started;
  std::vector<std::string> m_missing_options;
  std::vector<std::string> m_arguments;
  shcore::Dictionary_t m_parsed_options;
  shcore::Array_t m_parsed_arguments;
};

Report::Option::Option(const std::string &n, shcore::Value_type type,
                       bool required)
    : Parameter_definition(n, type,
                           required ? shcore::Param_flag::Mandatory
                                    : shcore::Param_flag::Optional) {}

Report::Report(const std::string &name, Type type,
               const shcore::Function_base_ref &function)
    : m_name(name), m_type(type), m_function(function) {
  switch (type) {
    case Type::LIST:
      m_formatter = list_formatter;
      break;

    case Type::REPORT:
      m_formatter = report_formatter;
      break;

    case Type::PRINT:
      m_formatter = print_formatter;
      break;
  }
}

Report::Report(const std::string &name, Type type,
               const Native_report &function)
    : Report(name, type,
             std::make_shared<Native_report_function>(name, function)) {}

const std::string &Report::name() const { return m_name; }

Report::Type Report::type() const { return m_type; }

const shcore::Function_base_ref &Report::function() const { return m_function; }

const std::string &Report::brief() const { return m_brief; }

void Report::set_brief(const std::string &brief) { m_brief = brief; }

const std::vector<std::string> &Report::details() const { return m_details; }

void Report::set_details(const std::vector<std::string> &details) {
  m_details = details;
}

void Report::set_options(const Options &options) {
  std::set<std::string> long_names = {"help", "interval", "nocls"};
  std::set<std::string> short_names = {"i"};

  if (type() == Report::Type::LIST) {
    long_names.emplace("vertical");
    short_names.emplace("E");
  }

  for (const auto &o : options) {
    validate_option(*o);

    if (!long_names.emplace(o->parameter->name).second) {
      throw shcore::Exception::argument_error(
          "Report already has an option named: '" + o->parameter->name + "'.");
    }

    if (!o->short_name.empty() && !short_names.emplace(o->short_name).second) {
      throw shcore::Exception::argument_error(
          "Report already has an option with short name: '" + o->short_name +
          "'.");
    }
  }

  m_options = options;
}

const Report::Argc &Report::argc() const { return m_argc; }

void Report::set_argc(const Argc &argc) {
  if (argc.first > argc.second) {
    throw shcore::Exception::argument_error(
        "The lower limit of 'argc' cannot be greater than upper limit.");
  }

  m_argc = argc;
}

const Report::Formatter &Report::formatter() const { return m_formatter; }

void Report::set_formatter(const Report::Formatter &formatter) {
  m_formatter = formatter;
}

bool Report::requires_options() const {
  return m_options.end() !=
         std::find_if(m_options.begin(), m_options.end(),
                      [](const std::shared_ptr<Report::Option> &o) {
                        return o->parameter->flag ==
                               shcore::Param_flag::Mandatory;
                      });
}

bool Report::has_options() const { return !m_options.empty(); }

void Report::validate_option(const Report::Option &option) {
  if (!option.short_name.empty()) {
    if (option.short_name.length() > 1) {
      throw shcore::Exception::argument_error(
          "Short name of an option must be exactly one character long.");
    }

    if (!std::isalnum(option.short_name[0], std::locale{})) {
      throw shcore::Exception::argument_error(
          "Short name of an option must be an alphanumeric character.");
    }
  }

  validate_option_type(option.parameter->type());

  if (shcore::Value_type::Bool == option.parameter->type() &&
      option.is_required()) {
    throw shcore::Exception::argument_error(
        "Option of type 'bool' cannot be required.");
  }
}

Shell_reports::Shell_reports(const std::string &name,
                             const std::string &qualified_name)
    : Extensible_object(name, qualified_name) {
  enable_help();

  // register query report
  auto query =
      shcore::make_unique<Report>("query", Report::Type::LIST, query_report);
  query->set_brief("Executes the SQL statement given as arguments.");
  query->set_argc({1, k_asterisk});

  register_report(std::move(query));
}

// needs to be defined here due to Shell_reports::Report_options being
// incomplete type
Shell_reports::~Shell_reports() = default;

std::shared_ptr<Parameter_definition> Shell_reports::start_parsing_parameter(
    const shcore::Dictionary_t &definition,
    shcore::Option_unpacker *unpacker) const {
  auto option = std::make_shared<Report::Option>("");

  unpacker->optional("shortcut", &option->short_name);

  // type is optional here, but base class requires it, insert default value if
  // not present
  if (definition->end() == definition->find("type")) {
    definition->emplace("type", "string");
  }

  return option;
}

void Shell_reports::register_report(const std::string &name,
                                    const std::string &type,
                                    const shcore::Function_base_ref &report,
                                    const shcore::Dictionary_t &description) {
  if (!report) {
    throw shcore::Exception::argument_error(
        "Argument #3 is expected to be a function");
  }

  auto new_report =
      shcore::make_unique<Report>(name, to_report_type(type), report);

  if (description) {
    std::string brief;
    std::vector<std::string> details;
    shcore::Array_t options;
    std::string argc;

    shcore::Option_unpacker{description}
        .optional("brief", &brief)
        .optional("details", &details)
        .optional("options", &options)
        .optional("argc", &argc)
        .end();

    new_report->set_brief(brief);
    new_report->set_details(details);
    new_report->set_options(
        downcast(parse_parameters(options, {"'options'"}, false)));
    new_report->set_argc(get_report_argc(argc));
  }

  register_report(std::move(new_report));
}

void Shell_reports::register_report(std::unique_ptr<Report> report) {
  auto normalized_name = normalize_report_name(report->name());

  {
    const auto it = m_reports.find(normalized_name);

    if (m_reports.end() != it) {
      const auto error = report->name() == it->second->name()
                             ? "Duplicate report: " + report->name()
                             : "Name '" + report->name() +
                                   "' conflicts with an existing report: " +
                                   it->second->name();
      throw shcore::Exception::argument_error(error);
    }
  }

  std::vector<std::string> details = {
      "This is a '" + to_string(report->type()) + "' type report."};
  details.insert(details.end(), report->details().begin(),
                 report->details().end());

  std::vector<std::shared_ptr<Parameter_definition>> parameters;

  {
    // first parameter - session (required)
    auto session = std::make_shared<Parameter_definition>(
        "session", shcore::Value_type::Object, shcore::Param_flag::Mandatory);
    session->brief = "A Session object to be used to execute the report.";
    session->parameter->validator<shcore::Object_validator>()->set_allowed(
        {"ClassicSession", "Session"});

    parameters.emplace_back(std::move(session));
  }

  // second parameter - argv - if report expects arguments or has any options
  if (report->argc().second > 0 || report->has_options()) {
    // argv is mandatory if report expects at least one argument or if options
    // are required
    auto argv = std::make_shared<Parameter_definition>(
        "argv", shcore::Value_type::Array,
        report->argc().first > 0 || report->requires_options()
            ? shcore::Param_flag::Mandatory
            : shcore::Param_flag::Optional);
    argv->brief =
        "Extra arguments. Report expects " + to_string(report->argc()) + ".";
    argv->parameter->set_validator(
        shcore::make_unique<Argv_validator>(report->argc()));

    parameters.emplace_back(std::move(argv));
  }

  // third parameter - options - only if report has any options
  if (report->has_options()) {
    // this parameter is mandatory if any of the options is required
    auto options = std::make_shared<Parameter_definition>(
        "options", shcore::Value_type::Map,
        report->requires_options() ? shcore::Param_flag::Mandatory
                                   : shcore::Param_flag::Optional);
    options->brief = "Options expected by the report.";
    options->set_options(upcast(report->m_options));

    parameters.emplace_back(std::move(options));
  }

  Function_definition fd{parameters, report->brief(), details};

  // method should have the same name in both JS and Python
  register_function(report->name() + "|" + report->name(), report->function(),
                    fd);

  m_reports.emplace(std::move(normalized_name),
                    shcore::make_unique<Report_options>(std::move(report)));
}

std::vector<std::string> Shell_reports::list_reports() const {
  std::vector<std::string> reports;

  std::transform(m_reports.begin(), m_reports.end(),
                 std::back_inserter(reports),
                 [](const decltype(m_reports)::value_type &pair) {
                   return pair.second->name();
                 });

  return reports;
}

std::string Shell_reports::call_report(
    const std::string &name, const std::shared_ptr<ShellBaseSession> &session,
    const std::vector<std::string> &args) {
  // session must be open
  if (!session || !session->is_open()) {
    throw shcore::Exception::argument_error(
        "Executing the report requires an existing, open session.");
  }

  // report must exist
  const auto report_iterator = m_reports.find(normalize_report_name(name));

  if (m_reports.end() == report_iterator) {
    throw shcore::Exception::argument_error("Unknown report: " + name);
  }

  // arguments must be valid
  const auto report_options = report_iterator->second.get();
  report_options->parse_args(args);

  if (report_options->show_help()) {
    // get help
    return report_options->help();
  } else {
    // prepare arguments
    shcore::Argument_list arguments;
    arguments.push_back(shcore::Value(session));

    if (report_options->requires_argv()) {
      arguments.push_back(
          shcore::Value(report_options->get_parsed_arguments()));
    }

    if (report_options->requires_options()) {
      // convert args into dictionary
      arguments.push_back(shcore::Value(report_options->get_parsed_options()));
    }

    // call the report
    const auto result = call(report_options->name(), arguments);

    if (shcore::Value_type::Map != result.type) {
      throw shcore::Exception::runtime_error(
          "Report should return a dictionary.");
    }

    shcore::Array_t report;
    shcore::Option_unpacker{result.as_map()}
        .required(k_report_key, &report)
        .end();

    if (!report) {
      throw shcore::Exception::runtime_error(
          "Option 'report' is expected to be of type Array, but is Null");
    }

    const auto display_options = shcore::make_dict();
    display_options->emplace(k_vertical_key, report_options->vertical());
    return report_options->formatter()(report, display_options);
  }
}

}  // namespace mysqlsh
