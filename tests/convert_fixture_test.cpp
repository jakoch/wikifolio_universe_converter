// SPDX-FileCopyrightText: 2021-2026 Jens A. Koch
// SPDX-License-Identifier: MIT
// This file is part of https://github.com/jakoch/wikifolio_universe_converter.

#include <catch2/catch_session.hpp>
#include <catch2/catch_test_macros.hpp>

#include <array>
#include <cstdio>
#include <cstdlib> // std::getenv
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <vector>

#include <sqlite3.h>

namespace
{

    // Size of the pipe buffer used to drain the child process output.
    constexpr std::size_t PIPE_BUFFER_SIZE = 256;

    constexpr char const * XLSX_NAME = "Investment_Universe.de.xlsx";
    constexpr char const * CSV_NAME  = "Investment_Universe.csv";
    constexpr char const * DB_NAME   = "Investment_Universe.sqlite";

    // The paths are passed via the environment by CTest, see CMakeLists.txt.
    // They are set once in main() and only read afterwards.
    std::filesystem::path& wiuc_exe()
    {
        static std::filesystem::path path;
        return path;
    }

    std::filesystem::path& fixture_file()
    {
        static std::filesystem::path path;
        return path;
    }

    std::filesystem::path& fixture_dir()
    {
        static std::filesystem::path path;
        return path;
    }

    // Creates a unique scratch directory for one test case, so that cases cannot
    // see each other's output. Removed again on destruction.
    class ScratchDir
    {
    public:
        explicit ScratchDir(std::string const & label)
        {
            // NOLINTNEXTLINE(misc-const-correctness)
            static int counter = 0;
            path = std::filesystem::temp_directory_path() / ("wiuc_test_" + label + "_" + std::to_string(++counter));

            // These take a non-const std::error_code&, so "ignored" cannot be const.
            // NOLINTNEXTLINE(misc-const-correctness)
            std::error_code ignored;
            std::filesystem::remove_all(path, ignored);
            std::filesystem::create_directories(path, ignored);
        }

        ~ScratchDir()
        {
            // This takes a non-const std::error_code&, so "ignored" cannot be const.
            // NOLINTNEXTLINE(misc-const-correctness)
            std::error_code ignored;
            std::filesystem::remove_all(path, ignored);
        }

        ScratchDir(ScratchDir const &)            = delete;
        ScratchDir& operator=(ScratchDir const &) = delete;
        ScratchDir(ScratchDir&&)                  = delete;
        ScratchDir& operator=(ScratchDir&&)       = delete;

        [[nodiscard]] std::filesystem::path const & dir() const noexcept
        { return path; }

        // Places the fixture in the scratch directory under the name wiuc expects,
        // so that wiuc skips the download and converts the local file instead.
        static void seed(std::filesystem::path const & scratch, std::filesystem::path const & fixture)
        { std::filesystem::copy_file(fixture, scratch / XLSX_NAME); }

        [[nodiscard]] static bool has(std::filesystem::path const & scratch, std::string const & name)
        { return std::filesystem::exists(scratch / name); }

    private:
        std::filesystem::path path;
    };

    // Runs "wiuc -c -o <dir>" and returns the process exit code.
    //
    // Running wiuc as a real subprocess is deliberate. It is a separate process, so
    // the coverage counters it records are written to its own .profraw file and stay
    // separate from the counters of this test binary.
    int run_wiuc(std::filesystem::path const & wiuc, std::filesystem::path const & dir)
    {
        // See the note in error_paths_test.cpp: std::string concatenation, not pointer arithmetic.
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        std::string const command = "\"" + wiuc.string() + "\" -c -o \"" + dir.string() + "\"";

        std::array<char, PIPE_BUFFER_SIZE> buffer{};
        FILE* const pipe = popen(command.c_str(), "r"); // NOLINT
        if (pipe == nullptr) {
            return -1;
        }

        // Drain the pipe, otherwise wiuc can block once the buffer is full.
        while (std::fgets(buffer.data(), static_cast<int>(buffer.size()), pipe) != nullptr) { }

        return pclose(pipe);
    }

    std::vector<std::string> read_lines(std::filesystem::path const & file)
    {
        std::vector<std::string> lines;
        std::ifstream input(file);
        std::string line{};

        while (std::getline(input, line)) {
            lines.push_back(line);
        }

        return lines;
    }

    // Counts the rows of the single table, or returns -1 on error.
    long long count_rows(std::filesystem::path const & database)
    {
        sqlite3* handle = nullptr;
        if (sqlite3_open(database.string().c_str(), &handle) != SQLITE_OK) {
            sqlite3_close(handle);
            return -1;
        }

        long long count = -1;

        sqlite3_stmt* statement = nullptr;
        if (sqlite3_prepare_v2(handle, "SELECT COUNT(*) FROM Anlageuniversum;", -1, &statement, nullptr) == SQLITE_OK) {
            if (sqlite3_step(statement) == SQLITE_ROW) {
                count = sqlite3_column_int64(statement, 0);
            }
            sqlite3_finalize(statement);
        }

        sqlite3_close(handle);
        return count;
    }

    // Reads one ISIN's Bezeichnung straight out of the database.
    std::string read_bezeichnung(std::filesystem::path const & database, std::string const & isin)
    {
        sqlite3* handle = nullptr;
        if (sqlite3_open(database.string().c_str(), &handle) != SQLITE_OK) {
            sqlite3_close(handle);
            return {};
        }

        std::string result{};
        sqlite3_stmt* statement = nullptr;

        if (sqlite3_prepare_v2(
                handle, "SELECT Bezeichnung FROM Anlageuniversum WHERE ISIN = ?;", -1, &statement, nullptr) ==
            SQLITE_OK) {
            sqlite3_bind_text(statement, 1, isin.c_str(), -1, SQLITE_TRANSIENT);
            if (sqlite3_step(statement) == SQLITE_ROW) {
                // sqlite3_column_text() returns "unsigned char*"; the C API offers no
                // conversion to a typed pointer, so the cast is unavoidable here.
                // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
                auto const * const text = reinterpret_cast<char const *>(sqlite3_column_text(statement, 0));
                if (text != nullptr) {
                    result = text;
                }
            }
            sqlite3_finalize(statement);
        }

        sqlite3_close(handle);
        return result;
    }

} // namespace

TEST_CASE("wiuc converts the XLSX fixture to CSV and SQLite", "[e2e]")
{
    ScratchDir const scratch("happy");

    REQUIRE(std::filesystem::exists(fixture_file()));
    ScratchDir::seed(scratch.dir(), fixture_file());

    INFO("running: " << wiuc_exe().string() << " -c -o " << scratch.dir().string());

    REQUIRE(run_wiuc(wiuc_exe(), scratch.dir()) == 0);

    SECTION("the CSV is written and every field is quoted")
    {
        REQUIRE(ScratchDir::has(scratch.dir(), CSV_NAME));

        auto const lines = read_lines(scratch.dir() / CSV_NAME);
        REQUIRE_FALSE(lines.empty());

        // The header is renamed from "Anlageuniversum (Gruppe) 1" to "Anlagegruppe1".
        CHECK(lines.front().find("Anlagegruppe1") != std::string::npos);
        CHECK(lines.front().find("Anlageuniversum1") != std::string::npos);
        CHECK(lines.front().find("(Gruppe)") == std::string::npos);

        // Every field of every record is enclosed in double quotes (RFC 4180).
        for (auto const & line : lines) {
            INFO("line: " << line);
            CHECK(line.front() == '"');
            CHECK(line.back() == '"');
        }
    }

    SECTION("the SQLite database holds one row per CSV record")
    {
        REQUIRE(ScratchDir::has(scratch.dir(), DB_NAME));

        auto const lines = read_lines(scratch.dir() / CSV_NAME);
        REQUIRE_FALSE(lines.empty());

        // One header line, so one record fewer than there are lines.
        long const expected = static_cast<long>(lines.size()) - 1;
        CHECK(count_rows(scratch.dir() / DB_NAME) == expected);
    }

    SECTION("awkward characters survive the round trip into SQLite")
    {
        REQUIRE(ScratchDir::has(scratch.dir(), DB_NAME));

        // A comma, which is why the fields are quoted.
        CHECK(read_bezeichnung(scratch.dir() / DB_NAME, "DE0007100000") == "Preis EUR -,01");

        // An embedded double quote, doubled in the CSV and unescaped by the parser.
        CHECK(read_bezeichnung(scratch.dir() / DB_NAME, "US0378331005") == "Sagt \"Hallo\"");

        // An apostrophe, which must not terminate the SQL string literal.
        CHECK(read_bezeichnung(scratch.dir() / DB_NAME, "LU0000000000") == "O'Brien Holdings");

        // The plain row, to prove the ordinary case is intact too.
        CHECK(read_bezeichnung(scratch.dir() / DB_NAME, "DE0007236101") == "Siemens AG");
    }
}

TEST_CASE("wiuc keeps a value which contains a newline", "[e2e]")
{
    // RFC 4180 allows a line break inside a quoted field, so such a record spans
    // several lines of the CSV. A reader which splits the input on line boundaries
    // used to reject the record, which lost the value.
    ScratchDir const scratch("newline");

    REQUIRE(std::filesystem::exists(fixture_dir() / "newline_in_value.xlsx"));
    ScratchDir::seed(scratch.dir(), fixture_dir() / "newline_in_value.xlsx");

    REQUIRE(run_wiuc(wiuc_exe(), scratch.dir()) == 0);

    CHECK(count_rows(scratch.dir() / DB_NAME) == 1);
    CHECK(read_bezeichnung(scratch.dir() / DB_NAME, "DE0007236101") == "Multi\nline Name");
}

int main()
{
    // CTest passes the two paths as positional arguments, but Catch2's Session
    // parses those as test-name filters, so the arguments are taken from the
    // environment instead and main() forwards an argument-free list to Catch2.
    if (char const * const wiuc = std::getenv("WIUC_EXE")) {
        wiuc_exe() = wiuc;
    }
    if (char const * const fixture = std::getenv("WIUC_FIXTURE")) {
        fixture_file() = fixture;
    }
    if (char const * const dir = std::getenv("WIUC_FIXTURE_DIR")) {
        fixture_dir() = dir;
    }

    std::array<char const *, 1> const catch_argv{"convert_fixture_test"};
    return Catch::Session().run(static_cast<int>(catch_argv.size()), catch_argv.data());
}
