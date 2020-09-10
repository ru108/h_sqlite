# h_sqlite
Simple sqlite3 header only C++ wrapper

## Dependencies
* fmtlib formatting library for C++ https://github.com/fmtlib/fmt
* sqlite https://www.sqlite.org/
* C++17 standard

## Quick start
```git clone http://github.com/ru108/h_sqlite```

Add ```./include``` directory to your project

Add fmt lib and sqlite files to your project

## Usage

```c++
#include <h_sqlite/h_sqlite.h>

auto db = make_sqlite3_handle(db_name);
if (!db)
  throw std::runtime_error{ "Can't create database: " + db_name };

h_sqlite3_exec(db.get(), "CREATE TABLE IF NOT EXISTS student( \
  student_id         INTEGER NOT NULL PRIMARY KEY, \
  student_first_name TEXT, \
  student_last_name  TEXT, \
  country_id         INTEGER, \
  course_id          INTEGER);");
```
```c++
h_handbook_create(db.get(), "country");
h_handbook_create(db.get(), "course");

const auto country_id = h_handbook_get_id_or_insert(db.get(), "country", "United States");
const auto course_id = h_handbook_get_id_or_insert(db.get(), "course", "Data Science");

h_sqlite3_prepare_bind_step(db.get(),
  fmt::format("INSERT INTO {0}({0}_first_name, {0}_last_name, country_id, course_id) VALUES(?, ?, ?, ?);", "student"),
  "Jon", "Snow", country_id, course_id);
```
```c++
auto_sqlite3_stmt stmt;

h_sqlite3_prepare_v2(db.get(), stmt,
  "SELECT {0}_first_name, {0}_last_name, {1}_name, {2}_name FROM {0} \
   JOIN {1} ON {0}.{1}_id={1}.{1}_id \
   JOIN {2} ON {0}.{2}_id={2}.{2}_id;", "student", "country", "course");
```
```c++
std::cout << fmt::format("{0:<{4}} {1:<{4}} {2:<{4}} {3:<{4}}\n", "student_first_name", "student_last_name", "country_name", "course_name", COLUMN_WIDTH);
std::cout << fmt::format("{0:-<{1}} {0:-<{1}} {0:-<{1}} {0:-<{1}}\n", "", COLUMN_WIDTH);
while (SQLITE_ROW == sqlite3_step(stmt.stmt)) {
  for (int i = 0; i < 4; ++i)
    std::cout << fmt::format("{0:<{1}} ", reinterpret_cast<const char*>(sqlite3_column_text(stmt.stmt, i)), COLUMN_WIDTH);
  std::cout << '\n';
}
```

Output:

```
student_first_name   student_last_name    country_name         course_name
-------------------- -------------------- -------------------- --------------------
Jon                  Snow                 United States        Data Science
```

## Example
```
cd .\example
mkdir build && cd build && cmake ..
cmake --build . --target sample_h_sqlite
```

## License
The project is available under the [MIT](https://opensource.org/licenses/MIT) license.
