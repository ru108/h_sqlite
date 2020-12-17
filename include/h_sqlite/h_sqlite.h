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

auto make_sqlite3_handle(const std::string& db_name) {
  sqlite3* p;

  auto rc = sqlite3_open(db_name.c_str(), &p);
  std::unique_ptr<sqlite3, sqlite3_deleter> h{ p };
  if (rc) h.reset();

  return h;
}

using rowid_t = sqlite3_int64;
using handbook_t = std::tuple<rowid_t, std::string>;
using rowid_column_t = std::tuple<rowid_t>;
using string_column_t = std::tuple<std::string>;

template<typename> inline constexpr bool always_false_v = false;

void h_sqlite3_exec(sqlite3* db, const std::string& sql) {
  auto_sqlite3_err err;

  if (sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &err.err))
    throw std::runtime_error{ "SQL error: "s + err.err };
}

template <typename... Args>
void h_sqlite3_prepare_v2(sqlite3* db, auto_sqlite3_stmt& stmt, std::string_view sql, Args&&... args) {
  if (sqlite3_prepare_v2(db, fmt::format(sql, std::forward<Args>(args)...).c_str(), -1, &stmt.stmt, 0))
    throw std::runtime_error{ "Failed to prepare statement: "s + sqlite3_errmsg(db) };
}

template <typename First, typename... Args>
void h_sqlite3_bind(sqlite3* db, auto_sqlite3_stmt& stmt, const int index, First&& first, Args&&... args) {
  auto res = SQLITE_ERROR;

  using T = std::decay_t<First>;

  if constexpr (std::is_same_v<T, rowid_t>)
    res = sqlite3_bind_int64(stmt.stmt, index + 1, first);
  else if constexpr (std::is_same_v<T, int>)
    res = sqlite3_bind_int(stmt.stmt, index + 1, first);
  else if constexpr (std::is_same_v<T, std::string_view>)
    res = sqlite3_bind_text(stmt.stmt, index + 1, first.data(), static_cast<int>(first.size()), nullptr);
  else if constexpr (std::is_same_v<T, std::string>)
    res = sqlite3_bind_text(stmt.stmt, index + 1, first.c_str(), -1, nullptr);

  if (res)
    throw std::runtime_error{ "Failed to bind statement: "s + sqlite3_errmsg(db) };

  if constexpr (sizeof...(Args) > 0)
    h_sqlite3_bind(db, stmt, index + 1, std::forward<Args>(args)...);
}

template <typename... Args>
void h_sqlite3_prepare_bind_step(sqlite3* db, std::string_view sql, Args&&... args) {
  auto_sqlite3_stmt stmt;

  h_sqlite3_prepare_v2(db, stmt, sql);
  h_sqlite3_bind(db, stmt, 0, std::forward<Args>(args)...);

  if (SQLITE_ERROR == sqlite3_step(stmt.stmt))
    throw std::runtime_error{ "SQl error: "s + sqlite3_errmsg(db) };
}

namespace detail {
  template <typename F, typename Tuple, std::size_t ...Idx>
  auto h_sqlite3_row_impl(F&& f, Tuple&& t, std::index_sequence<Idx...>) {
    return std::apply([&](auto&& ...items) {
      return std::apply([&](auto&& ...idx) {
        return std::make_tuple(f(std::forward<decltype(items)>(items), std::forward<decltype(idx)>(idx))...);
        }, std::make_tuple(Idx...));
      }, t);
  }
}

/// <summary>
/// Get row results from sqlite3
/// </summary>
/// <typeparam name="F"></typeparam>
/// <typeparam name="...Ts"></typeparam>
/// <param name="f">Function that apply on every column</param>
/// <param name="t">Tuple template of results</param>
/// <returns>Tuple of results</returns>
template <typename F, typename ...Ts>
auto h_sqlite3_row(F&& f, std::tuple<Ts...>&& t) {
  return detail::h_sqlite3_row_impl(
    std::forward<F>(f),
    std::forward<decltype(t)>(t),
    std::index_sequence_for<Ts...>{}
  );
}

/// <summary>
/// Return sqlite3 column result by type
/// </summary>
/// <param name="stmt"></param>
/// <returns></returns>
auto h_sqlite3_column(auto_sqlite3_stmt& stmt) {
  return [&](auto&& column, auto&& index) {
    using T = std::decay_t<decltype(column)>;

    if constexpr (std::is_same_v<T, rowid_t>)
      return sqlite3_column_int64(stmt.stmt, index);
    else if constexpr (std::is_same_v<T, int>)
      return sqlite3_column_int(stmt.stmt, index);
    else if constexpr (std::is_same_v<T, float>)
      return static_cast<float>(sqlite3_column_double(stmt.stmt, index));
    else if constexpr (std::is_same_v<T, std::string>)
      return reinterpret_cast<const char*>(sqlite3_column_text(stmt.stmt, index));
    else
      static_assert(always_false_v<T>, "Unknown column type!");
  };
};

template <typename T, typename... Args>
auto h_rows(sqlite3* db, T&& type, std::string_view sql, Args&&... args) -> std::vector<T> {
  auto_sqlite3_stmt stmt;
  h_sqlite3_prepare_v2(db, stmt, sql);
  if constexpr (sizeof...(Args) > 0)
    h_sqlite3_bind(db, stmt, 0, std::forward<Args>(args)...);

  std::vector<T> rows;
  while (SQLITE_ROW == sqlite3_step(stmt.stmt))
    rows.emplace_back(h_sqlite3_row(h_sqlite3_column(stmt), std::forward<T>(type)));

  return rows;
}

template <typename T, typename... Args>
auto h_row(sqlite3* db, T&& type, std::string_view sql, Args&&... args) -> std::decay_t<decltype(type)> {

  auto rows = h_rows(db, std::forward<T>(type), sql, std::forward<Args>(args)...);
  return !rows.empty() ? rows.front() : type;
}

template <typename T, typename... Args>
auto h_column(sqlite3* db, T&& tuple, std::string_view sql, Args&&... args) -> std::decay_t<decltype(std::get<0>(tuple))> {

  auto rows = h_rows(db, std::forward<T>(tuple), sql, std::forward<Args>(args)...);
  return std::get<0>(!rows.empty() ? rows.front() : tuple);
}



void h_handbook_create(sqlite3* db, std::string_view handbook) {
  h_sqlite3_exec(db, fmt::format("CREATE TABLE IF NOT EXISTS {0}(id INTEGER NOT NULL PRIMARY KEY, name TEXT NOT NULL DEFAULT '');", handbook));
}

auto h_handbook_get_id(sqlite3* db, std::string_view handbook, std::string_view handbook_name) -> rowid_t {
  return h_column(db, rowid_column_t{}, fmt::format("SELECT id FROM {} WHERE name=? LIMIT 1;", handbook), handbook_name);
}

auto h_handbook_get_name(sqlite3* db, std::string_view handbook, rowid_t rowid) -> std::string {
  return h_column(db, string_column_t{}, fmt::format( "SELECT name FROM {} WHERE id=? LIMIT 1;", handbook), rowid);
}

auto h_handbook_get_id_or_insert(sqlite3* db, std::string_view handbook, std::string_view handbook_name) -> rowid_t {
  if (auto rowid = h_handbook_get_id(db, handbook, handbook_name); rowid > 0)
    return rowid;

  h_sqlite3_prepare_bind_step(db, fmt::format("INSERT INTO {0}(name) VALUES(?);", handbook), handbook_name);

  return sqlite3_last_insert_rowid(db);
}

auto h_handbook_get_list(sqlite3* db, std::string_view handbook, const char* const order = "ASC") -> std::vector<handbook_t> {
  return h_rows(db, handbook_t{}, fmt::format("SELECT id, name FROM {0} ORDER BY name {1};", handbook, order));
}

#endif // _H_SQLITE_
