/*
 * Copyright (c) 2017, 2019, Oracle and/or its affiliates. All rights reserved.
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

#include "modules/util/mod_util.h"
#include "modules/util/upgrade_check.h"
#include "mysqlshdk/libs/db/mysql/session.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_path.h"
#include "mysqlshdk/libs/utils/utils_string.h"
#include "unittest/test_utils.h"

using Version = mysqlshdk::utils::Version;

namespace mysqlsh {

class MySQL_upgrade_check_test : public Shell_core_test_wrapper {
 public:
  MySQL_upgrade_check_test()
      : opts{_target_server_version.get_base(), MYSH_VERSION, ""} {}

 protected:
  static void SetUpTestCase() {
    auto session = mysqlshdk::db::mysql::Session::create();
  }

  virtual void SetUp() {
    Shell_core_test_wrapper::SetUp();
    if (_target_server_version >= Version(5, 7, 0) ||
        _target_server_version < Version(8, 0, 0)) {
      session = mysqlshdk::db::mysql::Session::create();
      auto connection_options = shcore::get_connection_options(_mysql_uri);
      session->connect(connection_options);
    }
  }

  virtual void TearDown() {
    if (_target_server_version >= Version(5, 7, 0) ||
        _target_server_version < Version(8, 0, 0)) {
      if (!db.empty()) {
        session->execute("drop database if exists " + db);
        db.clear();
      }
      session->close();
    }
    Shell_core_test_wrapper::TearDown();
  }

  void PrepareTestDatabase(std::string name) {
    ASSERT_NO_THROW(session->execute("drop database if exists " + name));
    ASSERT_NO_THROW(
        session->execute("create database " + name + " CHARACTER SET utf8mb3"));
    db = name;
    ASSERT_NO_THROW(session->execute("use " + db));
  }

  Upgrade_check_options opts;
  std::shared_ptr<mysqlshdk::db::ISession> session;
  std::string db;
};

TEST_F(MySQL_upgrade_check_test, checklist_generation) {
  Version current(MYSH_VERSION);
  Version prev(current.get_major(), current.get_minor(),
               current.get_patch() - 1);
  EXPECT_THROW_LIKE(Upgrade_check::create_checklist("5.7", "5.7"),
                    std::invalid_argument, "This tool supports checking");
  EXPECT_THROW_LIKE(Upgrade_check::create_checklist("5.6.11", "8.0"),
                    std::invalid_argument, "at least at version 5.7");
  EXPECT_THROW_LIKE(Upgrade_check::create_checklist("5.7.19", "8.1.0"),
                    std::invalid_argument, "This tool supports checking");
  EXPECT_THROW_LIKE(
      Upgrade_check::create_checklist(current.get_base(), MYSH_VERSION),
      std::invalid_argument, "must upgrade MySQL Shell");
  EXPECT_THROW_LIKE(Upgrade_check::create_checklist("8.0.12", "8.0.12"),
                    std::invalid_argument, "Target version must be greater");
  std::vector<std::unique_ptr<Upgrade_check>> checks;
  EXPECT_NO_THROW(checks = Upgrade_check::create_checklist(
                      "5.7.19", current.get_base().c_str()));
  EXPECT_NO_THROW(checks = Upgrade_check::create_checklist("5.7.17", "8.0"));
  EXPECT_NO_THROW(checks = Upgrade_check::create_checklist("5.7", "8.0.12"));

  checks.clear();
  EXPECT_NO_THROW(
      checks = Upgrade_check::create_checklist(prev.get_base(), MYSH_VERSION));
  // Check for table command is there for every valid version as a last check
  EXPECT_FALSE(checks.empty());
  EXPECT_EQ(0, strcmp("checkTableOutput", checks.back()->get_name()));
}

TEST_F(MySQL_upgrade_check_test, old_temporal) {
  if (_target_server_version < Version(5, 7, 0) ||
      _target_server_version >= Version(8, 0, 0))
    SKIP_TEST("This test requires running against MySQL server version 5.7");
  std::unique_ptr<Sql_upgrade_check> check =
      Sql_upgrade_check::get_old_temporal_check();
  EXPECT_NE(nullptr, check->get_doc_link());
  std::vector<Upgrade_issue> issues;
  ASSERT_NO_THROW(issues = check->run(session, opts));
  EXPECT_TRUE(issues.empty());
  // No way to create test data in 5.7
}

TEST_F(MySQL_upgrade_check_test, reserved_keywords) {
  if (_target_server_version < Version(5, 7, 0) ||
      _target_server_version >= Version(8, 0, 0))
    SKIP_TEST("This test requires running against MySQL server version 5.7");
  std::unique_ptr<Sql_upgrade_check> check =
      Sql_upgrade_check::get_reserved_keywords_check();
  EXPECT_NE(nullptr, check->get_doc_link());
  std::vector<Upgrade_issue> issues;
  ASSERT_NO_THROW(issues = check->run(session, opts));
  for (auto &warning : issues) puts(to_string(warning).c_str());
  EXPECT_TRUE(issues.empty());

  PrepareTestDatabase("grouping");
  ASSERT_NO_THROW(
      session->execute("create table System(JSON_TABLE integer, cube int);"));
  ASSERT_NO_THROW(
      session->execute("create trigger first_value AFTER INSERT on System FOR "
                       "EACH ROW delete from Clone where JSON_TABLE<0;"));
  ASSERT_NO_THROW(
      session->execute("create view NTile as select * from System;"));
  ASSERT_NO_THROW(
      session->execute("CREATE FUNCTION rows (s CHAR(20)) RETURNS CHAR(50) "
                       "DETERMINISTIC RETURN CONCAT('Hello, ',s,'!');"));
  ASSERT_NO_THROW(session->execute(
      "CREATE EVENT LEAD ON SCHEDULE AT CURRENT_TIMESTAMP + INTERVAL 1 "
      "HOUR DO UPDATE System SET JSON_TABLE = JSON_TABLE + 1;"));

  ASSERT_NO_THROW(issues = check->run(session, opts));
  ASSERT_EQ(10, issues.size());
  EXPECT_EQ("grouping", issues[0].schema);
  EXPECT_EQ(Upgrade_issue::WARNING, issues[0].level);
  EXPECT_STRCASEEQ("system", issues[1].table.c_str());
  EXPECT_EQ("JSON_TABLE", issues[2].column);
  EXPECT_EQ("cube", issues[3].column);
  // Views columns are also displayed
  EXPECT_EQ("JSON_TABLE", issues[4].column);
  EXPECT_EQ("cube", issues[5].column);
  EXPECT_EQ("first_value", issues[6].table);
  EXPECT_STRCASEEQ("NTile", issues[7].table.c_str());
  EXPECT_EQ("rows", issues[8].table);
  EXPECT_EQ("LEAD", issues[9].table);
}

TEST_F(MySQL_upgrade_check_test, utf8mb3) {
  if (_target_server_version < Version(5, 7, 0) ||
      _target_server_version >= Version(8, 0, 0))
    SKIP_TEST("This test requires running against MySQL server version 5.7");
  PrepareTestDatabase("aaaaaaaaaaaaaaaa_utf8mb3");
  std::unique_ptr<Sql_upgrade_check> check =
      Sql_upgrade_check::get_utf8mb3_check();
  EXPECT_NE(nullptr, check->get_doc_link());
  std::vector<Upgrade_issue> issues;

  session->execute(
      "create table utf83 (s3 varchar(64) charset 'utf8mb3', s4 varchar(64) "
      "charset 'utf8mb4');");

  ASSERT_NO_THROW(issues = check->run(session, opts));
  ASSERT_GE(issues.size(), 2);
  EXPECT_EQ("aaaaaaaaaaaaaaaa_utf8mb3", issues[0].schema);
  EXPECT_EQ("s3", issues[1].column);
  EXPECT_EQ(Upgrade_issue::WARNING, issues[0].level);
}

TEST_F(MySQL_upgrade_check_test, mysql_schema) {
  if (_target_server_version < Version(5, 7, 0) ||
      _target_server_version >= Version(8, 0, 0))
    SKIP_TEST("This test requires running against MySQL server version 5.7");
  std::unique_ptr<Sql_upgrade_check> check =
      Sql_upgrade_check::get_mysql_schema_check();
  std::vector<Upgrade_issue> issues;
  ASSERT_NO_THROW(issues = check->run(session, opts));
  EXPECT_NE(nullptr, check->get_doc_link());
  EXPECT_TRUE(issues.empty());

  ASSERT_NO_THROW(session->execute("use mysql;"));
  EXPECT_NO_THROW(session->execute("create table Role_edges (i integer);"));
  EXPECT_NO_THROW(session->execute("create table triggers (i integer);"));
  EXPECT_NO_THROW(issues = check->run(session, opts));
  EXPECT_EQ(2, issues.size());
#ifdef _MSC_VER
  EXPECT_EQ("role_edges", issues[0].table);
#else
  EXPECT_EQ("Role_edges", issues[0].table);
#endif
  EXPECT_EQ("triggers", issues[1].table);
  EXPECT_EQ(Upgrade_issue::ERROR, issues[0].level);
  EXPECT_NO_THROW(session->execute("drop table triggers;"));
  EXPECT_NO_THROW(session->execute("drop table Role_edges;"));
}

TEST_F(MySQL_upgrade_check_test, innodb_rowformat) {
  if (_target_server_version < Version(5, 7, 0) ||
      _target_server_version >= Version(8, 0, 0))
    SKIP_TEST("This test requires running against MySQL server version 5.7");
  PrepareTestDatabase("test_innodb_rowformat");
  std::unique_ptr<Sql_upgrade_check> check =
      Sql_upgrade_check::get_innodb_rowformat_check();
  std::vector<Upgrade_issue> issues;
  ASSERT_NO_THROW(issues = check->run(session, opts));
  EXPECT_TRUE(issues.empty());
  for (auto &warning : issues) puts(to_string(warning).c_str());

  ASSERT_NO_THROW(session->execute(
      "create table compact (i integer) row_format=compact engine=innodb;"));
  EXPECT_NO_THROW(issues = check->run(session, opts));
  EXPECT_TRUE(issues.size() == 1 && issues[0].table == "compact");
  EXPECT_EQ(Upgrade_issue::WARNING, issues[0].level);
}

TEST_F(MySQL_upgrade_check_test, zerofill) {
  if (_target_server_version < Version(5, 7, 0) ||
      _target_server_version >= Version(8, 0, 0))
    SKIP_TEST("This test requires running against MySQL server version 5.7");
  PrepareTestDatabase("aaa_test_zerofill_nondefaultwidth");
  std::unique_ptr<Sql_upgrade_check> check =
      Sql_upgrade_check::get_zerofill_check();
  std::vector<Upgrade_issue> issues;
  ASSERT_NO_THROW(issues = check->run(session, opts));
  // some tables in mysql schema use () syntax
  size_t old_count = issues.size();
  issues.clear();

  ASSERT_NO_THROW(session->execute(
      "create table zero_fill (zf INT zerofill, ti TINYINT(3), tu tinyint(2) "
      "unsigned, si smallint(3), su smallint(3) unsigned, mi mediumint(5), mu "
      "mediumint(5) unsigned, ii INT(4), iu INT(4) unsigned, bi bigint(10), bu "
      "bigint(12) unsigned);"));

  ASSERT_NO_THROW(issues = check->run(session, opts));
  //  for (auto& warning : issues)
  //    puts(to_string(warning).c_str());
  ASSERT_EQ(11 + old_count, issues.size());
  EXPECT_EQ(Upgrade_issue::NOTICE, issues[0].level);
  EXPECT_EQ("zf", issues[0].column);
  EXPECT_EQ("ti", issues[1].column);
  EXPECT_EQ("tu", issues[2].column);
  EXPECT_EQ("si", issues[3].column);
  EXPECT_EQ("su", issues[4].column);
  EXPECT_EQ("mi", issues[5].column);
  EXPECT_EQ("mu", issues[6].column);
  EXPECT_EQ("ii", issues[7].column);
  EXPECT_EQ("iu", issues[8].column);
  EXPECT_EQ("bi", issues[9].column);
  EXPECT_EQ("bu", issues[10].column);
}

TEST_F(MySQL_upgrade_check_test, foreign_key_length) {
  if (_target_server_version < Version(5, 7, 0) ||
      _target_server_version >= Version(8, 0, 0))
    SKIP_TEST("This test requires running against MySQL server version 5.7");
  std::unique_ptr<Sql_upgrade_check> check =
      Sql_upgrade_check::get_foreign_key_length_check();
  EXPECT_NE(nullptr, check->get_doc_link());
  std::vector<Upgrade_issue> issues;
  ASSERT_NO_THROW(issues = check->run(session, opts));
  EXPECT_TRUE(issues.empty());
  // No way to prepare test data in 5.7
}

TEST_F(MySQL_upgrade_check_test, maxdb_sqlmode) {
  if (_target_server_version < Version(5, 7, 0) ||
      _target_server_version >= Version(8, 0, 0))
    SKIP_TEST("This test requires running against MySQL server version 5.7");
  PrepareTestDatabase("aaa_test_maxdb_sql_mode");
  std::unique_ptr<Sql_upgrade_check> check =
      Sql_upgrade_check::get_maxdb_sql_mode_flags_check();
  EXPECT_NE(nullptr, check->get_doc_link());
  std::vector<Upgrade_issue> issues;
  ASSERT_NO_THROW(issues = check->run(session, opts));
  ASSERT_TRUE(issues.empty());

  ASSERT_NO_THROW(
      session->execute("create table Clone(COMPONENT integer, cube int);"));

  std::size_t issues_count = issues.size();
  ASSERT_NO_THROW(session->execute("SET SESSION sql_mode = 'MAXDB';"));
  ASSERT_NO_THROW(session->execute(
      "CREATE FUNCTION TEST_MAXDB (s CHAR(20)) RETURNS CHAR(50) "
      "DETERMINISTIC RETURN CONCAT('Hello, ',s,'!');"));
  ASSERT_NO_THROW(issues = check->run(session, opts));
  ASSERT_GT(issues.size(), issues_count);
  issues_count = issues.size();
  issues.clear();
  ASSERT_NO_THROW(
      session->execute("create trigger TR_MAXDB AFTER INSERT on Clone FOR "
                       "EACH ROW delete from Clone where COMPONENT<0;"));
  ASSERT_NO_THROW(issues = check->run(session, opts));
  ASSERT_GT(issues.size(), issues_count);
  issues_count = issues.size();
  issues.clear();
  ASSERT_NO_THROW(
      session->execute("CREATE EVENT EV_MAXDB ON SCHEDULE AT CURRENT_TIMESTAMP "
                       "+ INTERVAL 1 HOUR "
                       "DO UPDATE Clone SET COMPONENT = COMPONENT + 1;"));
  ASSERT_NO_THROW(issues = check->run(session, opts));
  ASSERT_GT(issues.size(), issues_count);
}

TEST_F(MySQL_upgrade_check_test, obsolete_sqlmodes) {
  if (_target_server_version < Version(5, 7, 0) ||
      _target_server_version >= Version(8, 0, 0))
    SKIP_TEST("This test requires running against MySQL server version 5.7");
  PrepareTestDatabase("aaa_test_obsolete_sql_modes");
  std::unique_ptr<Sql_upgrade_check> check =
      Sql_upgrade_check::get_obsolete_sql_mode_flags_check();
  EXPECT_NE(nullptr, check->get_doc_link());
  std::vector<Upgrade_issue> issues;
  ASSERT_NO_THROW(issues = check->run(session, opts));
  ASSERT_TRUE(issues.empty());

  std::vector<std::string> modes = {"DB2",
                                    "MSSQL",
                                    "MYSQL323",
                                    "MYSQL40",
                                    "NO_FIELD_OPTIONS",
                                    "NO_KEY_OPTIONS",
                                    "NO_TABLE_OPTIONS",
                                    "ORACLE",
                                    "POSTGRESQL"};

  ASSERT_NO_THROW(
      session->execute("create table Clone(COMPONENT integer, cube int);"));

  for (const std::string &mode : modes) {
    std::size_t issues_count = issues.size();
    issues.clear();
    ASSERT_NO_THROW(session->execute(
        shcore::str_format("SET SESSION sql_mode = '%s';", mode.c_str())));
    ASSERT_NO_THROW(session->execute(shcore::str_format(
        "CREATE FUNCTION TEST_%s (s CHAR(20)) RETURNS CHAR(50) "
        "DETERMINISTIC RETURN CONCAT('Hello, ',s,'!');",
        mode.c_str())));
    ASSERT_NO_THROW(issues = check->run(session, opts));
    ASSERT_GT(issues.size(), issues_count);
    issues_count = issues.size();
    issues.clear();
    ASSERT_NO_THROW(session->execute(
        shcore::str_format("create trigger TR_%s AFTER INSERT on Clone FOR "
                           "EACH ROW delete from Clone where COMPONENT<0;",
                           mode.c_str())));
    ASSERT_NO_THROW(issues = check->run(session, opts));
    ASSERT_GT(issues.size(), issues_count);
    issues_count = issues.size();
    issues.clear();
    ASSERT_NO_THROW(session->execute(
        shcore::str_format("CREATE EVENT EV_%s ON SCHEDULE AT "
                           "CURRENT_TIMESTAMP + INTERVAL 1 HOUR "
                           "DO UPDATE Clone SET COMPONENT = COMPONENT + 1;",
                           mode.c_str())));
    ASSERT_NO_THROW(issues = check->run(session, opts));
    ASSERT_GT(issues.size(), issues_count);
  }
}

TEST_F(MySQL_upgrade_check_test, enum_set_element_length) {
  if (_target_server_version < Version(5, 7, 0) ||
      _target_server_version >= Version(8, 0, 0))
    SKIP_TEST("This test requires running against MySQL server version 5.7");
  PrepareTestDatabase("aaa_test_enum_set_element_length");
  std::unique_ptr<Sql_upgrade_check> check =
      Sql_upgrade_check::get_enum_set_element_length_check();
  EXPECT_EQ(
      0,
      strcmp(
          "https://dev.mysql.com/doc/refman/8.0/en/string-type-overview.html",
          check->get_doc_link()));
  std::vector<Upgrade_issue> issues;
  ASSERT_NO_THROW(issues = check->run(session, opts));
  std::size_t original = issues.size();

  ASSERT_NO_THROW(session->execute(
      "CREATE TABLE large_enum (e enum('aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
      "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb"
      "cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc"
      "dddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddd"
      "eeeeee'));"));

  ASSERT_NO_THROW(session->execute(
      "CREATE TABLE not_large_enum (e enum('aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
      "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb"
      "ccccccccccccccccccccccccccccccccc','cccccccccccccccccccccccccccccccccccc"
      "ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff"
      "dddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddd"
      "eeeeee', \"zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz\"));"));

  ASSERT_NO_THROW(session->execute(
      "CREATE TABLE large_set (s set('a', 'zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz"
      "yyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyy"
      "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
      "wwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwww"
      "vvvvvvvvvv', 'b', 'c'));"));

  ASSERT_NO_THROW(session->execute(
      "CREATE TABLE not_so_large (s set('a', 'zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz"
      "yyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyy"
      "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
      "vvvvvvvvvv', 'aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
      "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb', 'b', 'c'));"));

  EXPECT_NO_THROW(issues = check->run(session, opts));
  EXPECT_EQ(original + 2, issues.size());
  EXPECT_EQ(issues[0].table, "large_enum");
  EXPECT_EQ(issues[1].table, "large_set");
}

TEST_F(MySQL_upgrade_check_test, partitioned_tables_in_shared_tablespaces) {
  if (_target_server_version < Version(5, 7, 0) ||
      _target_server_version >= Version(8, 0, 13))
    SKIP_TEST("This test requires running against MySQL server version 5.7");
  PrepareTestDatabase("aaa_test_partitioned_in_shared");
  std::unique_ptr<Sql_upgrade_check> check =
      Sql_upgrade_check::get_partitioned_tables_in_shared_tablespaces_check(
          _target_server_version);
  std::vector<Upgrade_issue> issues;
  ASSERT_NO_THROW(issues = check->run(session, opts));
  EXPECT_EQ(0, strcmp("https://dev.mysql.com/doc/refman/8.0/en/"
                      "mysql-nutshell.html#mysql-nutshell-removals",
                      check->get_doc_link()));
  ASSERT_TRUE(issues.empty());

  EXPECT_NO_THROW(session->execute(
      "CREATE TABLESPACE tpists ADD DATAFILE 'tpists.ibd' ENGINE=INNODB;"));
  EXPECT_NO_THROW(session->execute(
      "create table part(i integer) TABLESPACE tpists partition "
      "by range(i) (partition p0 values less than (1000), "
      "partition p1 values less than MAXVALUE);"));
  EXPECT_NO_THROW(issues = check->run(session, opts));
  EXPECT_EQ(2, issues.size());
  EXPECT_NO_THROW(session->execute("drop table part"));
  EXPECT_NO_THROW(session->execute("drop tablespace tpists"));
}

TEST_F(MySQL_upgrade_check_test, removed_functions) {
  if (_target_server_version < Version(5, 7, 0) ||
      _target_server_version >= Version(8, 0, 0))
    SKIP_TEST("This test requires running against MySQL server version 5.7");
  PrepareTestDatabase("aaa_test_removed_functions");
  std::unique_ptr<Sql_upgrade_check> check =
      Sql_upgrade_check::get_removed_functions_check();
  EXPECT_NE(nullptr, check->get_doc_link());
  std::vector<Upgrade_issue> issues;
  ASSERT_NO_THROW(issues = check->run(session, opts));
  ASSERT_TRUE(issues.empty());

  ASSERT_NO_THROW(session->execute(
      "create table geotab1 (col1 int ,col2 geometry,col3 geometry, col4 int "
      "generated always as (contains(col2,col3)));"));

  ASSERT_NO_THROW(
      session->execute("create view touch_view as select *, "
                       "TOUCHES(`col2`,`col3`) from geotab1;"));

  ASSERT_NO_THROW(session->execute(
      "create trigger contr AFTER INSERT on geotab1 FOR EACH ROW delete from \n"
      "-- This is a test NUMGEOMETRIES ()\n"
      "# This is a test GLENGTH()\n"
      "geotab1 where TOUCHES(`col2`,`col3`);"));
  ASSERT_NO_THROW(session->execute(
      "create procedure contains_proc(p1 geometry,p2 geometry) begin select "
      "col1, 'Y()' from tab1 where col2=@p1 and col3=@p2 and contains(p1,p2) "
      "and TOUCHES(p1, p2);\n"
      "-- This is a test NUMGEOMETRIES ()\n"
      "# This is a test GLENGTH()\n"
      "/* just a comment X() */end;"));
  ASSERT_NO_THROW(session->execute(
      "create function test_astext() returns TEXT deterministic return "
      "AsText('MULTIPOINT(1 1, 2 2, 3 3)');"));
  ASSERT_NO_THROW(
      session->execute("create function test_enc() returns text deterministic "
                       "return encrypt('123');"));

  ASSERT_NO_THROW(
      session->execute("create event e_contains ON SCHEDULE AT "
                       "CURRENT_TIMESTAMP + INTERVAL 1 HOUR "
                       "DO select contains(col2,col3) from geotab1;"));
  // Unable to test generated columns as at least in 5.7.19 they are
  // automatically converted to supported functions
  ASSERT_NO_THROW(issues = check->run(session, opts));
  EXPECT_EQ(6, issues.size());
  EXPECT_NE(std::string::npos, issues[0].description.find("TOUCHES"));
  EXPECT_NE(std::string::npos, issues[0].description.find("ST_TOUCHES"));
  EXPECT_NE(std::string::npos, issues[0].description.find("VIEW"));
  EXPECT_NE(std::string::npos, issues[1].description.find("CONTAINS"));
  EXPECT_NE(std::string::npos,
            issues[1].description.find("consider using MBRCONTAINS"));
  EXPECT_NE(std::string::npos, issues[1].description.find("TOUCHES"));
  EXPECT_NE(std::string::npos, issues[1].description.find("PROCEDURE"));
  EXPECT_NE(std::string::npos,
            issues[0].description.find("ST_TOUCHES instead"));
  EXPECT_NE(std::string::npos, issues[2].description.find("ASTEXT"));
  EXPECT_NE(std::string::npos, issues[2].description.find("ST_ASTEXT"));
  EXPECT_NE(std::string::npos, issues[2].description.find("FUNCTION"));
  EXPECT_NE(std::string::npos, issues[3].description.find("ENCRYPT"));
  EXPECT_NE(std::string::npos, issues[3].description.find("SHA2"));
  EXPECT_NE(std::string::npos, issues[3].description.find("FUNCTION"));
  EXPECT_NE(std::string::npos, issues[4].description.find("TOUCHES"));
  EXPECT_NE(std::string::npos, issues[4].description.find("ST_TOUCHES"));
  EXPECT_NE(std::string::npos, issues[5].description.find("CONTAINS"));
  EXPECT_NE(std::string::npos, issues[5].description.find("MBRCONTAINS"));
  EXPECT_NE(std::string::npos, issues[5].description.find("EVENT"));
}

TEST_F(MySQL_upgrade_check_test, groupby_asc_desc_syntax) {
  if (_target_server_version < Version(5, 7, 0) ||
      _target_server_version >= Version(8, 0, 13))
    SKIP_TEST(
        "This test requires running against MySQL server version 5.7-8.0.12");
  PrepareTestDatabase("aaa_test_group_by_asc");
  std::unique_ptr<Sql_upgrade_check> check =
      Sql_upgrade_check::get_groupby_asc_syntax_check();
  EXPECT_EQ(0, strcmp("https://dev.mysql.com/doc/relnotes/mysql/8.0/en/"
                      "news-8-0-13.html#mysqld-8-0-13-sql-syntax",
                      check->get_doc_link()));
  std::vector<Upgrade_issue> issues;
  ASSERT_NO_THROW(issues = check->run(session, opts));
  ASSERT_TRUE(issues.empty());

  ASSERT_NO_THROW(
      session->execute("create table movies (title varchar(100), genre "
                       "varchar(100), year_produced Year);"));
  ASSERT_NO_THROW(
      session->execute("create table genre_summary (genre varchar(100), count "
                       "int, time timestamp);"));
  ASSERT_NO_THROW(session->execute(
      "create view genre_ob as select genre, count(*), year_produced from "
      "movies group by genre, year_produced order by year_produced desc;"));

  ASSERT_NO_THROW(session->execute(
      "create view genre_desc as select genre, count(*), year_produced from "
      "movies group\n/*comment*/by genre\ndesc;"));

  ASSERT_NO_THROW(session->execute(
      "create trigger genre_summary_asc AFTER INSERT on movies for each row "
      "INSERT INTO genre_summary (genre, count, time) select genre, count(*), "
      "now() from movies group/* psikus */by genre\nasc;"));
  ASSERT_NO_THROW(session->execute(
      "create trigger genre_summary_desc AFTER INSERT on movies for each row "
      "INSERT INTO genre_summary (genre, count, time) select genre, count(*), "
      "now() from movies group\nby genre# tralala\ndesc;"));
  ASSERT_NO_THROW(session->execute(
      "create trigger genre_summary_ob AFTER INSERT on movies for each row "
      "INSERT INTO genre_summary (genre, count, time) select genre, count(*), "
      "now() from movies group by genre order by genre asc;"));

  ASSERT_NO_THROW(
      session->execute("create procedure list_genres_asc() select genre, "
                       "count(*), 'group by desc' from movies group by genre\n"
                       "-- This is a test order ()\n"
                       "# This is a test order\n"
                       "/* just a comment order */asc;"));
  ASSERT_NO_THROW(
      session->execute("create procedure list_genres_desc() select genre, "
                       "\"group by asc\", "
                       "count(*) from movies group# psikus\nby genre\tdesc;"));
  ASSERT_NO_THROW(session->execute(
      "create procedure list_genres_ob() select genre, count(*) from movies "
      "group by genre order/* group */by genre desc;"));

  ASSERT_NO_THROW(session->execute(
      "create event mov_sec ON SCHEDULE AT CURRENT_TIMESTAMP + INTERVAL 1 HOUR "
      "DO select * from movies group by genre desc;"));

  ASSERT_NO_THROW(issues = check->run(session, opts));
  ASSERT_EQ(6, issues.size());
  EXPECT_EQ("genre_desc", issues[0].table);
  EXPECT_TRUE(shcore::str_beginswith(issues[0].description, "VIEW"));
  EXPECT_EQ("list_genres_asc", issues[1].table);
  EXPECT_TRUE(shcore::str_beginswith(issues[1].description, "PROCEDURE"));
  EXPECT_EQ("list_genres_desc", issues[2].table);
  EXPECT_TRUE(shcore::str_beginswith(issues[2].description, "PROCEDURE"));
  EXPECT_EQ("genre_summary_asc", issues[3].table);
  EXPECT_TRUE(shcore::str_beginswith(issues[3].description, "TRIGGER"));
  EXPECT_EQ("genre_summary_desc", issues[4].table);
  EXPECT_TRUE(shcore::str_beginswith(issues[4].description, "TRIGGER"));
  EXPECT_EQ("mov_sec", issues[5].table);
  EXPECT_TRUE(shcore::str_beginswith(issues[5].description, "EVENT"));
}

TEST_F(MySQL_upgrade_check_test, removed_sys_log_vars) {
  if (_target_server_version < Version(5, 7, 0) ||
      _target_server_version >= Version(8, 0, 13))
    SKIP_TEST(
        "This test requires running against MySQL server version 5.7-8.0.13");

  std::unique_ptr<Upgrade_check> check =
      Sql_upgrade_check::get_removed_sys_log_vars_check(_target_server_version);
  EXPECT_EQ(0, strcmp("https://dev.mysql.com/doc/relnotes/mysql/8.0/en/"
                      "news-8-0-13.html#mysqld-8-0-13-logging",
                      check->get_doc_link()));
  std::vector<Upgrade_issue> issues;

  if (_target_server_version < Version(8, 0, 0)) {
    EXPECT_THROW_LIKE(
        check->run(session, opts), Upgrade_check::CheckConfigurationError,
        "To run this check requires full path to MySQL server configuration "
        "file to be specified at 'configPath' key of options dictionary");
  } else {
    ASSERT_NO_THROW(issues = check->run(session, opts));
    EXPECT_TRUE(issues.empty());
  }
}

extern "C" const char *g_test_home;

TEST_F(MySQL_upgrade_check_test, configuration_check) {
  Config_check defined("test",
                       {{"basedir", "homedir"},
                        {"option_to_drop_with_no_value", nullptr},
                        {"not_existing_var", nullptr},
                        {"again_not_there", "personalized msg"}},
                       Config_check::Mode::FLAG_DEFINED, Upgrade_issue::NOTICE,
                       "problem");
  std::vector<Upgrade_issue> issues;
  ASSERT_THROW(issues = defined.run(session, opts),
               Upgrade_check::CheckConfigurationError);

  opts.config_path.assign(
      shcore::path::join_path(g_test_home, "data", "config", "my.cnf"));
  ASSERT_NO_THROW(issues = defined.run(session, opts));

  ASSERT_EQ(2, issues.size());
  EXPECT_EQ("option_to_drop_with_no_value", issues[0].schema);
  EXPECT_EQ(Upgrade_issue::NOTICE, issues[0].level);
  EXPECT_EQ("problem", issues[0].description);
  EXPECT_EQ("basedir", issues[1].schema);
  EXPECT_NE(std::string::npos, issues[1].description.find("homedir"));

  Config_check undefined("test",
                         {{"basedir", "homedir"},
                          {"option_to_drop_with_no_value", nullptr},
                          {"not_existing_var", nullptr},
                          {"again_not_there", "personalized msg"}},
                         Config_check::Mode::FLAG_UNDEFINED,
                         Upgrade_issue::WARNING, "undefined");
  ASSERT_NO_THROW(issues = undefined.run(session, opts));

  ASSERT_EQ(2, issues.size());
  EXPECT_EQ("again_not_there", issues[0].schema);
  EXPECT_NE(std::string::npos, issues[0].description.find("personalized msg"));
  EXPECT_EQ("not_existing_var", issues[1].schema);
  EXPECT_EQ(Upgrade_issue::WARNING, issues[1].level);
  EXPECT_EQ("undefined", issues[1].description);

  opts.config_path.clear();
}

TEST_F(MySQL_upgrade_check_test, removed_sys_vars) {
  if (_target_server_version < Version(5, 7, 0) ||
      _target_server_version >= Version(8, 0, 13))
    SKIP_TEST(
        "This test requires running against MySQL server version 5.7-8.0.13");

  std::unique_ptr<Upgrade_check> check =
      Sql_upgrade_check::get_removed_sys_vars_check(_target_server_version,
                                                    Version(MYSH_VERSION));
  EXPECT_EQ(0, strcmp("https://dev.mysql.com/doc/refman/8.0/en/"
                      "added-deprecated-removed.html#optvars-removed",
                      check->get_doc_link()));
  std::vector<Upgrade_issue> issues;

  if (_target_server_version < Version(8, 0, 0)) {
    EXPECT_THROW_LIKE(
        check->run(session, opts), Upgrade_check::CheckConfigurationError,
        "To run this check requires full path to MySQL server configuration "
        "file to be specified at 'configPath' key of options dictionary");
    opts.config_path.assign(
        shcore::path::join_path(g_test_home, "data", "config", "my.cnf"));
    EXPECT_NO_THROW(issues = check->run(session, opts));
    EXPECT_TRUE(issues.empty());
    opts.config_path.clear();
  } else {
    ASSERT_NO_THROW(issues = check->run(session, opts));
    EXPECT_TRUE(issues.empty());
  }
}

TEST_F(MySQL_upgrade_check_test, sys_vars_new_defaults) {
  if (_target_server_version < Version(5, 7, 0) ||
      _target_server_version >= Version(8, 0, 0))
    SKIP_TEST("This test requires running against MySQL server version 5.7");

  std::unique_ptr<Upgrade_check> check =
      Sql_upgrade_check::get_sys_vars_new_defaults_check();
  EXPECT_EQ(0, strcmp("https://mysqlserverteam.com/new-defaults-in-mysql-8-0/",
                      check->get_doc_link()));
  std::vector<Upgrade_issue> issues;

  EXPECT_THROW_LIKE(
      check->run(session, opts), Upgrade_check::CheckConfigurationError,
      "To run this check requires full path to MySQL server configuration "
      "file to be specified at 'configPath' key of options dictionary");
  opts.config_path.assign(
      shcore::path::join_path(g_test_home, "data", "config", "my.cnf"));
  EXPECT_NO_THROW(issues = check->run(session, opts));
  EXPECT_EQ(26, issues.size());
  EXPECT_EQ("back_log", issues[0].schema);
  EXPECT_EQ("transaction_write_set_extraction", issues.back().schema);
  opts.config_path.clear();
}

TEST_F(MySQL_upgrade_check_test, schema_inconsitencies) {
  if (_target_server_version < Version(5, 7, 0) ||
      _target_server_version >= Version(8, 0, 0))
    SKIP_TEST("This test requires running against MySQL server version 5.7");

  // Preparing data for this check requires manipulating datadir by hand, we
  // only check here that queries run fine
  auto check = Sql_upgrade_check::get_schema_inconsistency_check();

  // Make sure special characters like hyphen are handled well
  PrepareTestDatabase("schema_inconsitencies_test");
  EXPECT_NO_THROW(session->execute("create table `!@#$%&*-_.:?` (i integer);"));

  // Make sure partitioned tables do not get positively flagged by accident
  EXPECT_NO_THROW(session->execute(
      "create table t(a datetime(5) not null) engine=innodb default "
      "charset=latin1 row_format=dynamic partition by range columns(a) "
      "(partition p0 values less than ('2019-01-23 16:59:53'), partition p1 "
      "values less than ('2019-02-22 10:17:03'), partition p2 values less than "
      "(maxvalue));"));

  std::vector<Upgrade_issue> issues;
  ASSERT_NO_THROW(issues = check->run(session, opts));
  ASSERT_TRUE(issues.empty());
}

TEST_F(MySQL_upgrade_check_test, check_table_command) {
  if (_target_server_version < Version(5, 7, 0) ||
      _target_server_version >= Version(8, 0, 0))
    SKIP_TEST("This test requires running against MySQL server version 5.7");
  PrepareTestDatabase("mysql_check_table_test");
  Check_table_command check;
  std::vector<Upgrade_issue> issues;
  ASSERT_NO_THROW(issues = check.run(session, opts));
  EXPECT_TRUE(issues.empty());

  ASSERT_NO_THROW(
      session->execute("create table part(i integer) engine=myisam partition "
                       "by range(i) (partition p0 values less than (1000), "
                       "partition p1 values less than MAXVALUE);"));
  EXPECT_NO_THROW(issues = check.run(session, opts));
  EXPECT_TRUE(issues.size() == 1 && issues[0].table == "part");
}

TEST_F(MySQL_upgrade_check_test, manual_checks) {
  auto manual = Upgrade_check::create_checklist("5.7", "8.0.11");
  manual.erase(std::remove_if(manual.begin(), manual.end(),
                              [](const std::unique_ptr<Upgrade_check> &c) {
                                return c->is_runnable();
                              }),
               manual.end());
  ASSERT_EQ(1, manual.size());

  auto auth = dynamic_cast<Manual_check *>(manual[0].get());
  ASSERT_NE(nullptr, auth);
  ASSERT_EQ(0, strcmp(auth->get_name(), "defaultAuthenticationPlugin"));
  ASSERT_EQ(0, strcmp(auth->get_title(),
                      "New default authentication plugin considerations"));
  ASSERT_EQ(Upgrade_issue::WARNING, auth->get_level());
  ASSERT_NE(nullptr, strstr(auth->get_doc_link(),
                            "https://dev.mysql.com/doc/refman/8.0/en/"
                            "upgrading-from-previous-series.html#upgrade-"
                            "caching-sha2-password-compatibility-issues"));
  ASSERT_NE(nullptr,
            strstr(auth->get_description(),
                   "Warning: The new default authentication plugin "
                   "'caching_sha2_password' offers more secure password "
                   "hashing than previously used 'mysql_native_password' (and "
                   "consequent improved client connection authentication)."));
}

TEST_F(MySQL_upgrade_check_test, corner_cases_of_upgrade_check) {
  if (_target_server_version < Version(5, 7, 0) ||
      _target_server_version >= Version(8, 0, 0))
    SKIP_TEST("This test requires running against MySQL server version 5.7");
  Util util(_interactive_shell->shell_context().get());
  shcore::Argument_list args;

  // valid mysql 5.7 superuser
  args.push_back(shcore::Value(_mysql_uri));
  try {
    util.check_for_server_upgrade(args);
  } catch (std::exception &e) {
    std::cerr << e.what() << std::endl;
    EXPECT_TRUE(false);
  }
  args.clear();

  // valid mysql 5.7 superuser X protocol
  args.push_back(shcore::Value(_uri));
  EXPECT_NO_THROW(util.check_for_server_upgrade(args));
  args.clear();

  // new user with all privileges sans grant option and '%' in host
  EXPECT_NO_THROW(session->execute(
      "create user if not exists 'percent'@'%' identified by 'percent';"));
  std::string percent_uri = _mysql_uri;
  percent_uri.replace(0, 4, "percent:percent");
  args.push_back(shcore::Value(percent_uri));
  // No privileges function should throw
  EXPECT_THROW(util.check_for_server_upgrade(args), shcore::Exception);

  // Still not enough privileges
  EXPECT_NO_THROW(session->execute("grant SUPER on *.* to 'percent'@'%';"));
  EXPECT_THROW(util.check_for_server_upgrade(args), shcore::Exception);

  // Privileges check out we should succeed
  EXPECT_NO_THROW(session->execute("grant ALL on *.* to 'percent'@'%';"));
  EXPECT_NO_THROW(util.check_for_server_upgrade(args));

  EXPECT_NO_THROW(session->execute("drop user 'percent'@'%';"));
}

TEST_F(MySQL_upgrade_check_test, JSON_output_format) {
  if (_target_server_version < Version(5, 7, 0) ||
      _target_server_version >= Version(8, 0, 0))
    SKIP_TEST("This test requires running against MySQL server version 5.7");
  Util util(_interactive_shell->shell_context().get());
  shcore::Argument_list args;

  // valid mysql 5.7 superuser
  args.push_back(shcore::Value(_mysql_uri));
  shcore::Value::Map_type_ref opts(new shcore::Value::Map_type());
  opts->set("outputFormat", shcore::Value("JSON"));
  args.push_back(shcore::Value(opts));
  try {
    util.check_for_server_upgrade(args);
    rapidjson::Document d;
    d.Parse(output_handler.std_out.c_str());

    ASSERT_FALSE(d.HasParseError());
    ASSERT_TRUE(d.IsObject());
    ASSERT_TRUE(d.HasMember("serverAddress"));
    ASSERT_TRUE(d["serverAddress"].IsString());
    ASSERT_TRUE(d.HasMember("serverVersion"));
    ASSERT_TRUE(d["serverVersion"].IsString());
    ASSERT_TRUE(d.HasMember("targetVersion"));
    ASSERT_TRUE(d["targetVersion"].IsString());
    ASSERT_TRUE(d.HasMember("errorCount"));
    ASSERT_TRUE(d["errorCount"].IsInt());
    ASSERT_TRUE(d.HasMember("warningCount"));
    ASSERT_TRUE(d["warningCount"].IsInt());
    ASSERT_TRUE(d.HasMember("noticeCount"));
    ASSERT_TRUE(d["noticeCount"].IsInt());
    ASSERT_TRUE(d.HasMember("summary"));
    ASSERT_TRUE(d["summary"].IsString());
    ASSERT_TRUE(d.HasMember("checksPerformed"));
    ASSERT_TRUE(d["checksPerformed"].IsArray());
    auto checks = d["checksPerformed"].GetArray();
    ASSERT_GT(checks.Size(), 1);
    for (rapidjson::SizeType i = 0; i < checks.Size(); i++) {
      ASSERT_TRUE(checks[i].IsObject());
      ASSERT_TRUE(checks[i].HasMember("id"));
      ASSERT_TRUE(checks[i]["id"].IsString());
      ASSERT_TRUE(checks[i].HasMember("title"));
      ASSERT_TRUE(checks[i]["title"].IsString());
      ASSERT_TRUE(checks[i].HasMember("status"));
      ASSERT_TRUE(checks[i]["status"].IsString());
      if (checks[i].HasMember("documentationLink")) {
        ASSERT_TRUE(checks[i]["documentationLink"].IsString());
      }
      if (strcmp(checks[i]["status"].GetString(), "OK") == 0) {
        ASSERT_TRUE(checks[i].HasMember("detectedProblems"));
        ASSERT_TRUE(checks[i]["detectedProblems"].IsArray());
        auto issues = checks[i]["detectedProblems"].GetArray();
        for (rapidjson::SizeType j = 0; j < issues.Size(); j++) {
          ASSERT_TRUE(issues[j].IsObject());
          ASSERT_TRUE(issues[j].HasMember("level"));
          ASSERT_TRUE(issues[j]["level"].IsString());
          ASSERT_TRUE(issues[j].HasMember("dbObject"));
          ASSERT_TRUE(issues[j]["dbObject"].IsString());
        }
      } else {
        ASSERT_TRUE(checks[i].HasMember("description"));
        ASSERT_TRUE(checks[i]["description"].IsString());
      }
    }
    ASSERT_TRUE(d.HasMember("manualChecks"));
    ASSERT_TRUE(d["manualChecks"].IsArray());
    auto manual = d["manualChecks"].GetArray();
    ASSERT_GT(manual.Size(), 0);
    for (rapidjson::SizeType i = 0; i < manual.Size(); i++) {
      ASSERT_TRUE(manual[i].IsObject());
      ASSERT_TRUE(manual[i].HasMember("id"));
      ASSERT_TRUE(manual[i]["id"].IsString());
      ASSERT_TRUE(manual[i].HasMember("title"));
      ASSERT_TRUE(manual[i]["title"].IsString());
      ASSERT_TRUE(manual[i].HasMember("description"));
      ASSERT_TRUE(manual[i]["description"].IsString());
      if (manual[i].HasMember("documentationLink")) {
        ASSERT_TRUE(manual[i]["documentationLink"].IsString());
      }
    }
  } catch (std::exception &e) {
    std::cerr << e.what() << std::endl;
    EXPECT_TRUE(false);
  }
  args.clear();
}

TEST_F(MySQL_upgrade_check_test, server_version_not_supported) {
  Version shell_version(MYSH_VERSION);
  // session established with 8.0 server
  if (_target_server_version < Version(8, 0, 0) ||
      _target_server_version < shell_version)
    SKIP_TEST(
        "This test requires running against MySQL server version 8.0, equal "
        "or greater than the shell version");
  Util util(_interactive_shell->shell_context().get());
  shcore::Argument_list args;
  args.push_back(shcore::Value(_mysql_uri));
  EXPECT_THROW(util.check_for_server_upgrade(args), shcore::Exception);
}

TEST_F(MySQL_upgrade_check_test, password_prompted) {
  Util util(_interactive_shell->shell_context().get());
  shcore::Argument_list args;
  args.push_back(shcore::Value(_mysql_uri_nopasswd));

  output_handler.passwords.push_back(
      {"Please provide the password for "
       "'" +
           _mysql_uri_nopasswd + "': ",
       "WhAtEvEr"});
  EXPECT_THROW(util.check_for_server_upgrade(args), shcore::Exception);

  // Passwords are consumed if prompted, so verifying this indicates the
  // password was prompted as expected and consumed
  EXPECT_TRUE(output_handler.passwords.empty());
}

TEST_F(MySQL_upgrade_check_test, password_no_prompted) {
  Util util(_interactive_shell->shell_context().get());
  shcore::Argument_list args;
  args.push_back(shcore::Value(_mysql_uri));

  output_handler.passwords.push_back(
      {"If this was prompted it is an error", "WhAtEvEr"});

  try {
    util.check_for_server_upgrade(args);
  } catch (...) {
    // We don't really care for this test
  }

  // Passwords are consumed if prompted, so verifying this indicates the
  // password was NOT prompted as expected and so, NOT consumed
  EXPECT_FALSE(output_handler.passwords.empty());
  output_handler.passwords.clear();
}

TEST_F(MySQL_upgrade_check_test, password_no_promptable) {
  _options->wizards = false;
  reset_shell();
  Util util(_interactive_shell->shell_context().get());
  shcore::Argument_list args;
  args.push_back(shcore::Value(_mysql_uri_nopasswd));

  output_handler.passwords.push_back(
      {"If this was prompted it is an error", "WhAtEvEr"});

  try {
    util.check_for_server_upgrade(args);
  } catch (...) {
    // We don't really care for this test
  }

  // Passwords are consumed if prompted, so verifying this indicates the
  // password was NOT prompted as expected and so, NOT consumed
  EXPECT_FALSE(output_handler.passwords.empty());
  output_handler.passwords.clear();
}
}  // namespace mysqlsh
