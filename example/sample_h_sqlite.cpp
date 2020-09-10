#include <iostream>
#include <map>
#include <string>
#include <h_sqlite/h_sqlite.h>

int main() {
  try {

    const int COLUMN_WIDTH = 20;

    // simple in memory sqlite database
    const std::string db_name = ":memory:";
    auto db = make_sqlite3_handle(db_name);
    if (!db)
      throw std::runtime_error{ "Can't create database: " + db_name };

    std::map<std::string, std::string> handbook_tables = {
      { "Handbook of countries", "country" },
      { "Handbook of courses", "course" }
    };

    // students data
    std::vector<std::tuple<std::string, std::string, std::string, std::string>> students = {
      {"Jon",      "Snow",      "The North",       "Data Science"},
      {"Tyrion",   "Lannister", "The Westerlands", "Deep Learning"},
      {"Daenerys", "Targaryen", "The Crownlands",  "Machine Learning"}
    };

    // create main table
    h_sqlite3_exec(db.get(), "CREATE TABLE IF NOT EXISTS student( \
      student_id         INTEGER NOT NULL PRIMARY KEY, \
      student_first_name TEXT, \
      student_last_name  TEXT, \
      country_id         INTEGER, \
      course_id          INTEGER);");

    // create handbook tables
    for (const auto& [_, table] : handbook_tables)
      h_handbook_create(db.get(), table);

    // insert some data in students
    for (auto&& student: students) {
      const auto& [first_name, last_name, country, course] = student;
      const auto country_id = h_handbook_get_id_or_insert(db.get(), "country", country);
      const auto course_id = h_handbook_get_id_or_insert(db.get(), "course", course);

      h_sqlite3_prepare_bind_step(db.get(),
        fmt::format("INSERT INTO {0}({0}_first_name, {0}_last_name, country_id, course_id) VALUES(?, ?, ?, ?);", "student"),
        first_name, last_name, country_id, course_id);

      // alternative variant

      //auto_sqlite3_stmt stmt;

      //h_sqlite3_prepare_v2(db.get(), stmt,
      //  "INSERT INTO {0}({0}_first_name, {0}_last_name, country_id, course_id) VALUES(?, ?, ?, ?);", "student");
      //h_sqlite3_bind(db.get(), stmt, 0, first_name, last_name, country_id, course_id);

      //if (SQLITE_ERROR == sqlite3_step(stmt.stmt))
      //  throw std::runtime_error{ "Failed to insert data: "s + sqlite3_errmsg(db.get()) };
    }

    // get data from students
    auto_sqlite3_stmt stmt;

    h_sqlite3_prepare_v2(db.get(), stmt,
      "SELECT {0}_first_name, {0}_last_name, {1}_name, {2}_name FROM {0} \
       JOIN {1} ON {0}.{1}_id={1}.{1}_id \
       JOIN {2} ON {0}.{2}_id={2}.{2}_id;", "student", "country", "course");

    std::cout << fmt::format("{0:<{4}} {1:<{4}} {2:<{4}} {3:<{4}}\n", "student_first_name", "student_last_name", "country_name", "course_name", COLUMN_WIDTH);
    std::cout << fmt::format("{0:-<{1}} {0:-<{1}} {0:-<{1}} {0:-<{1}}\n", "", COLUMN_WIDTH);
    while (SQLITE_ROW == sqlite3_step(stmt.stmt)) {
      for (int i = 0; i < 4; ++i)
        std::cout << fmt::format("{0:<{1}} ", reinterpret_cast<const char*>(sqlite3_column_text(stmt.stmt, i)), COLUMN_WIDTH);
      std::cout << '\n';
    }


    // get id of some name in handbook
    auto country_id = h_handbook_get_id(db.get(), "country", "The North");
    // get name by id
    auto country_name = h_handbook_get_name(db.get(), "country", country_id);
    std::cout << "\nCountry id: " << country_id << ", name: " << country_name << '\n';

    auto course_id = h_handbook_get_id(db.get(), "course", "Deep Learning");
    auto course_name = h_handbook_get_name(db.get(), "course", course_id);
    std::cout << "Course id: " << course_id << ", name: " << course_name << '\n';

    // if name not found by name in handbook
    if (auto some_id = h_handbook_get_id(db.get(), "course", "Python in depth"); !some_id)
      std::cout << "Can't find course by name: " << "Python in depth" << '\n';

    // if name not found by id in handbook
    if (auto some_name = h_handbook_get_name(db.get(), "country", 100); some_name.empty())
      std::cout << "Can't find country by id: " << 100 << '\n';

    // get list names from handbook by name ascending
    std::cout << "\nascending\n" << fmt::format("{0:<{1}}", "course_name", COLUMN_WIDTH) << '\n' << fmt::format("{0:-<{1}}", "", COLUMN_WIDTH) << '\n';
    for (auto&& course : h_handbook_get_names(db.get(), "course"))
      std::cout << fmt::format("{0:<{1}}", course, COLUMN_WIDTH) << '\n';

    // get list names from handbook by name descending
    std::cout << "\ndescending\n" << fmt::format("{0:<{1}}", "country_name", COLUMN_WIDTH) << '\n' << fmt::format("{0:-<{1}}", "", COLUMN_WIDTH) << '\n';
    for (auto&& country : h_handbook_get_names(db.get(), "country", "DESC"))
      std::cout << fmt::format("{0:<{1}}", country, COLUMN_WIDTH) << '\n';

    // get list ids from handbook
    std::cout << '\n' << fmt::format("{0:<{1}}", "country_id", COLUMN_WIDTH) << '\n' << fmt::format("{0:-<{1}}", "", COLUMN_WIDTH) << '\n';
    for (auto&& id : h_handbook_get_ids(db.get(), "country"))
      std::cout << fmt::format("{0:<{1}}", id, COLUMN_WIDTH) << '\n';

  }
  catch (const std::exception& ex) {
    std::cerr << ex.what() << '\n';
  }
}
