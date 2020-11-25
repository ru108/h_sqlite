#pragma once
#ifndef _H_SQLITE_
#define _H_SQLITE_

#include <memory>

using namespace std::string_literals;

#include <sqlite3.h>
#include <fmt/format.h>

template <typename Func>
struct scope_exit {
  explicit scope_exit(Func f) : _f{ f } {}
  ~scope_exit() { _f(); }
private:
  Func _f;
};

struct auto_sqlite3_stmt {
  ~auto_sqlite3_stmt() { sqlite3_finalize(stmt); }
  sqlite3_stmt* stmt{ nullptr };
};

struct auto_sqlite3_err {
  ~auto_sqlite3_err() { sqlite3_free(err); }
  char* err{ nullptr };
};

struct sqlite3_deleter {
  void operator () (sqlite3* db) const { sqlite3_close(db); }
};

struct sqlite3_stmt_deleter {
  void operator () (sqlite3_stmt* stmt) const { sqlite3_finalize(stmt); }
};

using sqlite3_handle = std::unique_ptr<sqlite3, sqlite3_deleter>;
using handbook_t = std::tuple<sqlite3_int64, std::string>;

auto make_sqlite3_handle(const std::string& db_name) {
  sqlite3* p;

  auto rc = sqlite3_open(db_name.c_str(), &p);
  std::unique_ptr<sqlite3, sqlite3_deleter> h{ p };
  if (rc) h.reset();

  return h;
}

template <typename... Args>
void h_sqlite3_prepare_v2(sqlite3* db, auto_sqlite3_stmt& stmt, const std::string_view& sql, Args&&... args) {
  if (sqlite3_prepare_v2(db, fmt::format(sql, std::forward<Args>(args)...).c_str(), -1, &stmt.stmt, 0))
    throw std::runtime_error{ "Failed to prepare statement: "s + sqlite3_errmsg(db) };
}

template <typename First, typename... Args>
void h_sqlite3_bind(sqlite3* db, auto_sqlite3_stmt& stmt, int index, First&& first, Args&&... args) {

  auto res = SQLITE_ERROR;

  if constexpr (std::is_same_v<typename std::decay_t<First>, std::string_view>)
    res = sqlite3_bind_text(stmt.stmt, index + 1, first.data(), static_cast<int>(first.size()), nullptr);
  else if constexpr (std::is_same_v<typename std::decay_t<First>, std::string>)
    res = sqlite3_bind_text(stmt.stmt, index + 1, first.c_str(), -1, nullptr);
  else if constexpr (std::is_same_v<typename std::decay_t<First>, sqlite3_int64>)
    res = sqlite3_bind_int64(stmt.stmt, index + 1, first);
  else if constexpr (std::is_same_v<typename std::decay_t<First>, int>)
    res = sqlite3_bind_int(stmt.stmt, index + 1, first);

  if (res)
    throw std::runtime_error{ "Failed to bind statement: "s + sqlite3_errmsg(db) };

  if constexpr (sizeof...(Args) > 0)
    h_sqlite3_bind(db, stmt, index + 1, args...);
}

void h_sqlite3_exec(sqlite3* db, const std::string& sql) {
  auto_sqlite3_err err;

  if (sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &err.err))
    throw std::runtime_error{ "SQL error: "s + err.err };
}

template <typename First, typename... Args>
void h_sqlite3_prepare_bind_step(sqlite3* db, const std::string& sql, First&& first, Args&&... args) {
  auto_sqlite3_stmt stmt;

  h_sqlite3_prepare_v2(db, stmt, sql);
  h_sqlite3_bind(db, stmt, 0, first, args...);

  if (SQLITE_ERROR == sqlite3_step(stmt.stmt))
    throw std::runtime_error{ "SQl error: "s + sqlite3_errmsg(db) };
}

void h_handbook_create(sqlite3* db, const std::string_view& handbook) {
  h_sqlite3_exec(db, fmt::format("CREATE TABLE IF NOT EXISTS {0}({0}_id INTEGER NOT NULL PRIMARY KEY, {0}_name TEXT NOT NULL DEFAULT '');", handbook));
}


sqlite3_int64 h_handbook_get_id(sqlite3* db, const std::string_view& handbook, const std::string_view& handbook_name) {
  auto_sqlite3_stmt stmt;

  h_sqlite3_prepare_v2(db, stmt, "SELECT {0}_id FROM {0} WHERE {0}_name=? LIMIT 1;", handbook);
  h_sqlite3_bind(db, stmt, 0, handbook_name);

  if (SQLITE_ROW == sqlite3_step(stmt.stmt))
    return sqlite3_column_int64(stmt.stmt, 0);

  return 0ll;
}

sqlite3_int64 h_handbook_get_id_or_insert(sqlite3* db, const std::string_view& handbook, const std::string_view& handbook_name) {
  if (auto rowid = h_handbook_get_id(db, handbook, handbook_name); rowid)
    return rowid;

  h_sqlite3_prepare_bind_step(db, fmt::format("INSERT INTO {0}({0}_name) VALUES(?);", handbook), handbook_name);

  return sqlite3_last_insert_rowid(db);
}

std::string h_handbook_get_name(sqlite3* db, const std::string_view& handbook, sqlite3_int64 rowid) {
  auto_sqlite3_stmt stmt;

  h_sqlite3_prepare_v2(db, stmt, "SELECT {0}_name FROM {0} WHERE {0}_id=? LIMIT 1;", handbook);
  h_sqlite3_bind(db, stmt, 0, rowid);

  if (SQLITE_ROW == sqlite3_step(stmt.stmt))
    return reinterpret_cast<const char*>(sqlite3_column_text(stmt.stmt, 0));

  return "";
}

auto h_handbook_get_list(sqlite3* db, const std::string_view& handbook, const char* const order = "ASC") -> std::vector<handbook_t> {
  auto_sqlite3_stmt stmt;
  std::vector<handbook_t> names;

  h_sqlite3_prepare_v2(db, stmt, "SELECT {0}_id, {0}_name FROM {0} ORDER BY {0}_name {1};", handbook, order);

  while (SQLITE_ROW == sqlite3_step(stmt.stmt))
    names.emplace_back(
      sqlite3_column_int64(stmt.stmt, 0),
      reinterpret_cast<const char*>(sqlite3_column_text(stmt.stmt, 1)));

  return names;
}

std::vector<sqlite3_int64> h_handbook_get_ids(sqlite3* db, const std::string_view& handbook) {
  auto_sqlite3_stmt stmt;
  std::vector<sqlite3_int64> ids;

  h_sqlite3_prepare_v2(db, stmt, "SELECT {0}_id FROM {0};", handbook);

  while (SQLITE_ROW == sqlite3_step(stmt.stmt))
    ids.push_back(sqlite3_column_int64(stmt.stmt, 0));

  return ids;
}

#endif // _H_SQLITE_
