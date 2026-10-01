// SPDX-FileCopyrightText: 2021-2026 Jens A. Koch
// SPDX-License-Identifier: MIT
// This file is part of https://github.com/jakoch/wikifolio_universe_converter.

#include "main.h"

#include <algorithm> // std::ranges::all_of
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <filesystem> // NOLINT(build/c++17): <filesystem> unapproved C++17 header. sure.
#include <fstream>
#include <iostream>
#include <memory>
#include <ostream>
#include <ranges> // std::ranges::all_of
#include <span>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <fmt/format.h>
using fmt::format;

#include <sqlite3.h>

#include "csv_format.h"
#include "download.h"
#include "paths.h"

#include "xlsxio_read.h"

#include "version.h"

XLSXReader::XLSXReader(char const * filename) : handle(xlsxioread_open(filename))
{
    // init
}

XLSXReader::~XLSXReader()
{ xlsxioread_close(handle); }

bool XLSXReader::is_open() const noexcept
{ return handle != nullptr; }

std::unique_ptr<XLSXSheet> XLSXReader::OpenSheet(char const * sheetname, unsigned int flags)
{
    if (!is_open()) {
        return nullptr;
    }

    if (auto* sheet = xlsxioread_sheet_open(handle, sheetname, flags)) {
        return std::unique_ptr<XLSXSheet>(new XLSXSheet(sheet));
    }
    return nullptr;
}

XLSXSheet::XLSXSheet(xlsxioreadersheet sheet) noexcept : sheethandle(sheet) { }

XLSXSheet::~XLSXSheet()
{ xlsxioread_sheet_close(sheethandle); }

bool XLSXSheet::GetNextRow()
{ return (xlsxioread_sheet_next_row(sheethandle) != 0); }

bool XLSXSheet::GetNextCellString(char*& value)
{
    if (xlsxioread_sheet_next_cell_string(sheethandle, &value) == 0) {
        value = nullptr;
        return false;
    }
    return true;
}

bool XLSXSheet::GetNextCellString(std::string& value)
{
    char* result = nullptr;
    if (xlsxioread_sheet_next_cell_string(sheethandle, &result) == 0) {
        value.clear();
        return false;
    }
    value.assign(result);
    free(result); // NOLINT
    return true;
}

bool XLSXSheet::GetNextCellInt(int64_t& value)
{
    if (xlsxioread_sheet_next_cell_int(sheethandle, &value) == 0) {
        value = 0;
        return false;
    }
    return true;
}

bool XLSXSheet::GetNextCellFloat(double& value)
{
    if (xlsxioread_sheet_next_cell_float(sheethandle, &value) == 0) {
        value = 0;
        return false;
    }
    return true;
}

bool XLSXSheet::GetNextCellDateTime(time_t& value)
{
    if (0 == xlsxioread_sheet_next_cell_datetime(sheethandle, &value)) {
        value = 0;
        return false;
    }
    return true;
}

namespace
{

    // Reports elapsed time since the first use of this class.
    // The start time is a single static baseline, so every stop() label reports a
    // cumulative point in the run rather than the duration of that one phase.
    // There is deliberately no instance state: construct nothing, just call stop().
    class Timer
    {
    public:
        using Time = std::conditional_t<
            std::chrono::high_resolution_clock::is_steady,
            std::chrono::high_resolution_clock,
            std::chrono::steady_clock>;

    private:
        inline static Time::time_point const start_time = Time::now();

    public:
        // There is no instance state, so instances are neither created, copied nor
        // moved. Declaring them explicitly satisfies the Rule of Five.
        Timer() = default;

        Timer(Timer const &)            = delete;
        Timer& operator=(Timer const &) = delete;
        Timer(Timer&&)                  = delete;
        Timer& operator=(Timer&&)       = delete;
        ~Timer()                        = default;

        static void stop(char const * time_point_name)
        {
            auto const stop_time       = Time::now();
            auto const time_diff       = stop_time - start_time;
            auto const ms_duration     = std::chrono::duration_cast<std::chrono::milliseconds>(time_diff).count();
            char constexpr const * fmt = "[{}] Time Elapsed: {}.{} sec\n";
            int const msec             = 1000;
            std::cout << fmt::format(fmt, time_point_name, (ms_duration / msec), (ms_duration % msec));
        }
    };

    bool xlsx_to_csv(std::string const & xlsx_filename, std::string const & csv_filename)
    {
        XLSXReader file(xlsx_filename.c_str());

        if (!file.is_open()) {
            std::cerr << format(
                "Error: xlsxio was unable to open \"{}\".\n"
                "The file is either corrupt, incomplete or not a valid XLSX file.\n",
                xlsx_filename);
            return false;
        }

        std::unique_ptr<XLSXSheet> sheet = file.OpenSheet(nullptr, XLSXIOREAD_SKIP_EMPTY_ROWS);

        if (sheet == nullptr) {
            std::cout << "xlsxio was unable to find the first sheet.";
            return false;
        }

        std::ofstream csvfile;
        csvfile.open(csv_filename);

        std::string csv_row;
        std::string value;
        while (sheet->GetNextRow()) {
            csv_row.clear();
            bool first_cell = true;
            while (sheet->GetNextCellString(value)) {
                // printf("%s\t", value.c_str());
                if (!first_cell) {
                    csv_row.append(","); // cells are separated, the last one is not followed by a separator
                }
                first_cell = false;
                csv_row.append(quote_csv_field(value));
            }
            csv_row.append("\n");
            // printf("\n");
            csvfile << csv_row;
        }
        csvfile.close();

        return true;
    };

    bool rename_header_columns(std::string const & csv_filename, std::string const & csv_tmp_filename)
    {
        std::fstream input_file(csv_filename.c_str(), std::ios::in);
        std::ofstream output_file(csv_tmp_filename.c_str());

        if (!input_file.is_open() || !output_file.is_open()) {
            std::cerr << format("Error: Could not open \"{}\" or \"{}\".\n", csv_filename, csv_tmp_filename);
            return false;
        }

        std::string line;
        bool replaced = false;

        while (std::getline(input_file, line)) {
            // change column names on first line of file only once
            if (!replaced) {
                line     = replace(line, "Anlageuniversum (Gruppe) 1", "Anlagegruppe1");
                line     = replace(line, "Anlageuniversum 1", "Anlageuniversum1");
                line     = replace(line, "Anlageuniversum (Gruppe) 2", "Anlagegruppe2");
                line     = replace(line, "Anlageuniversum 2", "Anlageuniversum2");
                line     = replace(line, "Anlageuniversum (Gruppe) 3", "Anlagegruppe3");
                line     = replace(line, "Anlageuniversum 3", "Anlageuniversum3");
                line     = replace(line, "Anlageuniversum (Gruppe) 4", "Anlagegruppe4");
                line     = replace(line, "Anlageuniversum 4", "Anlageuniversum4");
                replaced = true;
            }

            output_file << line << '\n';
        }
        output_file.close();
        input_file.close();

        // delete old "Investment_Universe.csv"
        if (std::remove(csv_filename.c_str()) != 0) {
            std::cerr << format("Could not delete file: {}\n", csv_filename);
            return false;
        }

        // rename "Investment_Universe.tmp.csv" -> "Investment_Universe.csv"
        if (std::rename(csv_tmp_filename.c_str(), csv_filename.c_str()) != 0) {
            std::cerr << format("Could not rename: {} -> {}\n", csv_tmp_filename, csv_filename);
            return false;
        }

        return true;
    }

    // Owns an open SQLite connection and closes it on destruction.
    // This keeps the database handle closed on every early return path.
    class SQLiteConnection
    {
    private:
        sqlite3* db_handle = nullptr;

    public:
        explicit SQLiteConnection(std::string const & filename)
        {
            int const res = sqlite3_open(filename.c_str(), &db_handle);

            if (res != SQLITE_OK) {
                std::cerr << format("[SQLite][Error][{}]\nDB connection error: {}\n", res, sqlite3_errmsg(db_handle));
                if (db_handle != nullptr) {
                    sqlite3_close(db_handle);
                    db_handle = nullptr;
                }
            }
        }

        ~SQLiteConnection()
        {
            if (db_handle != nullptr) {
                sqlite3_close(db_handle);
            }
        }

        SQLiteConnection(SQLiteConnection const &)            = delete;
        SQLiteConnection& operator=(SQLiteConnection const &) = delete;
        SQLiteConnection(SQLiteConnection&&)                  = delete;
        SQLiteConnection& operator=(SQLiteConnection&&)       = delete;

        [[nodiscard]] sqlite3* get() const noexcept
        { return db_handle; }

        explicit operator bool() const noexcept
        { return db_handle != nullptr; }
    };

    bool create_table(sqlite3* _db)
    {
        static char const * sql_table_schema =
            "CREATE TABLE Anlageuniversum ("
            "ISIN TEXT,"
            "WKN TEXT,"
            "SecurityType TEXT,"
            "Bezeichnung TEXT,"
            "Emittent TEXT,"
            "Anlagegruppe1 TEXT,"
            "Anlageuniversum1 TEXT,"
            "Anlagegruppe2 TEXT,"
            "Anlageuniversum2 TEXT,"
            "Anlagegruppe3 TEXT,"
            "Anlageuniversum3 TEXT,"
            "Anlagegruppe4 TEXT,"
            "Anlageuniversum4 TEXT)";

        int const res = sqlite3_exec(_db, sql_table_schema, nullptr, nullptr, nullptr);

        if (res != SQLITE_OK) {
            std::cerr << format("[SQLite][Error][{}]\nFailed to create table: {}\n", res, sqlite3_errmsg(_db));
            return false;
        }

        return true;
    }

    bool csv_to_sqlite(std::string const & csv_filename, std::string const & sqlite_filename)
    {
        int res = 0;

        char* zErrMsg = nullptr;

        // Open SQLite Database

        SQLiteConnection const connection(sqlite_filename);

        if (!connection) {
            return false;
        }

        sqlite3* dbHandle = connection.get();

        // Set full synchronous

        res = sqlite3_exec(dbHandle, "PRAGMA synchronous=2;", nullptr, nullptr, nullptr);
        if (res != SQLITE_OK) {
            std::cerr
                << format("[SQLite][Error][{}]\nFailed to set synchronous for {}\n", res, sqlite_filename.c_str());
            return false;
        }

        // Create Table

        if (!create_table(dbHandle)) {
            return false;
        }

        // Open CSV for reading

        std::ifstream csv_file(csv_filename.c_str(), std::ios::in);
        if (!csv_file.is_open()) {
            std::cerr << format("Error opening CSV file: {}\n", csv_filename);
            return false;
        }

        // Iterate CSV data, build INSERT statement, exec query

        // The number of columns which this version of wiuc knows about.
        // Keep in sync with the column list of sql_insert_stmt_tpl and of sql_table_schema (see create_table()).
        // Wikifolio adds columns to the investment universe from time to time, this check reports such a change.
        constexpr std::size_t expected_column_count = 13;

        // The statement is prepared once and reused for every record. The values
        // are bound as parameters, so they never have to be escaped or quoted.
        // Keep the column list in sync with sql_table_schema (see create_table()).
        static char const * sql_insert_stmt =
            "INSERT INTO Anlageuniversum ( ISIN, WKN, SecurityType, Bezeichnung, Emittent, "
            "Anlagegruppe1, Anlageuniversum1, Anlagegruppe2, Anlageuniversum2, Anlagegruppe3, Anlageuniversum3, "
            "Anlagegruppe4, Anlageuniversum4 ) "
            "VALUES ( ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ? );";

        sqlite3_stmt* insert_stmt = nullptr;

        res = sqlite3_prepare_v2(dbHandle, sql_insert_stmt, -1, &insert_stmt, nullptr);
        if (res != SQLITE_OK) {
            std::cerr << format(
                "[SQLite][Error][{}]\nFailed to prepare insert statement: {}\n", res, sqlite3_errmsg(dbHandle));
            return false;
        }

        // Owns the prepared statement and finalizes it on destruction.
        auto const insert_stmt_guard =
            std::unique_ptr<sqlite3_stmt, decltype(&sqlite3_finalize)>(insert_stmt, &sqlite3_finalize);

        std::vector<std::string> fields;

        // Read the CSV as a character stream, because a record may span several
        // lines when a value contains a line break.
        CsvReader reader(csv_file);

        // Read the first record and ignore it. It is the table header, which is set
        // in sql_table_schema (see create_table()).
        if (!reader.read(fields)) {
            std::cerr << format("Error: Malformed CSV header in \"{}\".\n", csv_filename);
            return false;
        }

        if (fields.size() != expected_column_count) {
            std::cerr << format(
                "Error: Unexpected number of columns in the header of \"{}\".\n"
                "Found {} columns, but wiuc {} expects {} columns.\n"
                "Wikifolio appears to have changed the columns of the investment universe.\n"
                "Please report this at: https://github.com/jakoch/wikifolio_universe_converter/issues\n",
                csv_filename,
                fields.size(),
                app_version::get_version(),
                expected_column_count);
            return false;
        }

        // wrap insert queries in transaction block
        res = sqlite3_exec(dbHandle, "BEGIN TRANSACTION;", nullptr, nullptr, &zErrMsg);

        if (res != SQLITE_OK) {
            std::cerr
                << format("[SQLite][Error][{}]\nBegin Transaction: {}, {}\n", res, zErrMsg, sqlite3_errmsg(dbHandle));
            sqlite3_free(zErrMsg);
            return false;
        }

        std::size_t record_number = 1; // the header record was read above

        while (true) {

            if (!reader.read(fields)) {
                // Reaching the end of the input is not an error, only a malformed record is.
                if (reader.error() != CsvError::None) {
                    std::cerr << format(
                        "Error: Malformed CSV record starting in line {} of \"{}\".\n"
                        "The record is not terminated by RFC 4180, the CSV is likely corrupt.\n",
                        reader.record_line(),
                        csv_filename);
                    return false;
                }
                break;
            }

            record_number++;

            // The column count must be verified before the values are assembled,
            // because a record with fewer than two fields would underflow the
            // size calculation of the trailing separator below.
            if (fields.size() != expected_column_count) {
                std::cerr << format(
                    "Error: Unexpected number of columns in record {} of \"{}\".\n"
                    "Found {} columns, but wiuc {} expects {} columns.\n"
                    "Wikifolio appears to have changed the columns of the investment universe.\n"
                    "Please report this at: https://github.com/jakoch/wikifolio_universe_converter/issues\n",
                    record_number,
                    csv_filename,
                    fields.size(),
                    app_version::get_version(),
                    expected_column_count);
                return false;
            }

            // Bind the values as parameters. The indexes are 1-based, in the order
            // of the column list of the statement above.
            for (std::size_t index = 0; index < fields.size(); ++index) {
                int const bind_index      = static_cast<int>(index) + 1;
                std::string const & field = fields.at(index);

                // SQLITE_TRANSIENT: SQLite copies the string, so the lifetime of
                // "field" does not matter here.
                res = sqlite3_bind_text(
                    insert_stmt, bind_index, field.c_str(), static_cast<int>(field.size()), SQLITE_TRANSIENT);

                if (res != SQLITE_OK) {
                    std::cerr
                        << format("[SQLite][Error][{}]\nFailed to bind value: {}\n", res, sqlite3_errmsg(dbHandle));
                    return false;
                }
            }

            res = sqlite3_step(insert_stmt);

            if (res != SQLITE_DONE) {
                std::cerr << format("[SQLite][Error][{}]\nQuery Exec: {}\n", res, sqlite3_errmsg(dbHandle));
                std::cerr << format("[SQLite][Error]\nQuery: {}\n", sql_insert_stmt);

                std::cerr << format(
                    "Error: Failed to store record {} of \"{}\" in the database.\n"
                    "This is most likely caused by a value which wiuc cannot store.\n"
                    "Please report this at: https://github.com/jakoch/wikifolio_universe_converter/issues\n",
                    record_number,
                    csv_filename);

                return false;
            }

            // Reset the statement for the next record, keeping the bindings intact.
            res = sqlite3_reset(insert_stmt);

            if (res != SQLITE_OK) {
                std::cerr
                    << format("[SQLite][Error][{}]\nFailed to reset statement: {}\n", res, sqlite3_errmsg(dbHandle));
                return false;
            }
        }

        res = sqlite3_exec(dbHandle, "END TRANSACTION;", nullptr, nullptr, &zErrMsg);

        if (res != SQLITE_OK) {
            std::cerr << format("[SQLite][Error][{}]\nEnd Transaction: {}\n", res, zErrMsg);
            sqlite3_free(zErrMsg);
            return false;
        }

        return true;
    }

    std::string getFile(std::string const & file_type, std::string const & output_folder)
    {
        static std::unordered_map<std::string, char const *> const files{
            {"xlsx", "Investment_Universe.de.xlsx"},
            {"csv_tmp", "Investment_Universe.tmp.csv"},
            {"csv", "Investment_Universe.csv"},
            {"sqlite", "Investment_Universe.sqlite"}};

        auto _it = files.find(file_type);
        if (_it == files.end()) {
            throw std::invalid_argument("Invalid fileType argument");
        }

        std::filesystem::path path(output_folder);
        path.append(_it->second);

        return path.string();
    }

    enum class Color : std::uint8_t
    {
        Red        = 31,
        Yellow     = 33,
        Green      = 32,
        Blue       = 34,
        Purple     = 35,
        Cyan       = 36,
        Light_Grey = 37
    };

    void print_status(
        std::string const & status_message = "Status update.", int indent_spaces = 0, Color color = Color::Light_Grey)
    {
        std::string const escape_code = "\033[0;" + std::to_string(static_cast<int>(color)) + "m";
        std::string const reset_code  = "\033[0m";
        std::string const indentation(indent_spaces, ' ');
        std::cout << indentation << escape_code << status_message << reset_code << '\n';
    }

    std::string format_status(
        std::string const & status_message = "Status update.", int indent_spaces = 0, Color color = Color::Light_Grey)
    {
        std::string const escape_code = "\033[0;" + std::to_string(static_cast<int>(color)) + "m";
        std::string const reset_code  = "\033[0m";
        std::string const indentation(indent_spaces, ' ');
        return indentation + escape_code + status_message + reset_code; // + "\n"
    }

    void printHelpText(std::string program_name)
    {
        std::string const help_text_header = fmt::format(
            "{} {}\n"
            "{}\n\n"
            "{} {} [OPTIONS] [ARGUMENTS]\n\n"
            "{}\n",
            format_status(app_version::get_nice_name(), 0, Color::Yellow).c_str(),
            format_status(app_version::get_version(), 0, Color::Yellow).c_str(),
            app_version::get_copyright(),
            format_status("Usage:", 0, Color::Yellow).c_str(),
            program_name,
            format_status("Options:", 0, Color::Yellow).c_str());

        std::string const help_text_body = fmt::format(
            "{}\t\tDisplay this help message\n"
            "{}\tConvert from XLSX to SQLite and CSV\n"
            "{}\tSet output folder (default is current directory)\n"
            "{}\tDisplay version information\n"
            "{}\tDisplay version information as JSON\n"
            "{}\tDisplay version number only\n",
            format_status("-h,   --help", 2, Color::Green).c_str(),
            format_status("-c,   --convert", 2, Color::Green).c_str(),
            format_status("-o,   --out <dir>", 2, Color::Green).c_str(),
            format_status("-V,   --version", 2, Color::Green).c_str(),
            format_status("-Vj,  --version-json", 2, Color::Green).c_str(),
            format_status("-Vo,  --version-only", 2, Color::Green).c_str());

        std::string const help_text = help_text_header + help_text_body;

        std::cout << format("{}\n", help_text.c_str());
    }

    // Returns the argument at "index", or an empty view if it does not exist.
    // The bound is checked here, so callers never index out of range.
    // std::span has no bounds-checked at() in C++20, hence the explicit check.
    std::string_view arg_at(std::span<char const * const> const & args, std::size_t const index)
    {
        if (index >= args.size()) {
            return {};
        }

        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
        char const * const argument = args[index];

        return argument != nullptr ? std::string_view(argument) : std::string_view();
    }

} // anonymous namespace

int main(int const argc, char const * argv[]) noexcept(false)
{
    std::string const name = "wiuc";

    std::span const args(argv, static_cast<std::size_t>(argc));

    // default action, no arguments → help
    if (args.size() <= 1) {
        printHelpText(name);
        return EXIT_SUCCESS;
    }

    // The "-o <folder>" option is only accepted together with "-c" / "--convert".
    // It is parsed up front, so the remaining flags can require a single-flag
    // invocation. Accepted invocations are a single flag, or "-c" followed by
    // "-o <folder>"; anything else prints the help text and fails.
    std::string_view const flag = arg_at(args, 1);

    std::string_view output_folder_option;

    if (args.size() == 4) {
        std::string_view const second_argument = arg_at(args, 2);
        std::string_view const third_argument  = arg_at(args, 3);

        if (second_argument == "-o" || second_argument == "--out") {
            output_folder_option = third_argument;
        }
    }

    bool const is_convert_flag = (flag == "-c" || flag == "--convert");

    bool const is_single_flag_invocation = (args.size() == 2);

    bool const is_convert_with_output_folder = (is_convert_flag && args.size() == 4 && !output_folder_option.empty());

    if (!(is_single_flag_invocation || is_convert_with_output_folder)) {
        printHelpText(name);
        return EXIT_FAILURE;
    }

    // Check for command line arguments
    if (flag == "-h" || flag == "--help") {
        printHelpText(name);
        return EXIT_SUCCESS;
    }

    if (flag == "-V" || flag == "--version") {
        std::cout << format("{} v{}\n", app_version::get_nice_name(), app_version::get_version());
        return EXIT_SUCCESS;
    }

    if (flag == "-Vo" || flag == "--version-only") {
        std::cout << format("{}\n", app_version::get_version());
        return EXIT_SUCCESS;
    }

    if (flag == "-Vj" || flag == "--version-json") {
        std::cout << app_version::get_version_json() << '\n';
        return EXIT_SUCCESS;
    }

    if (flag == "-c" || flag == "--convert") {
        std::string const app_header = format(
            "{} {}\n"
            "{}\n\n",
            format_status(app_version::get_nice_name(), 0, Color::Yellow).c_str(),
            format_status(app_version::get_version(), 0, Color::Yellow).c_str(),
            app_version::get_copyright());
        std::cout << app_header;

        // Output Folder

        // The output folder set via "-o" or "--out".
        // The default output folder is the current directory.
        std::string outputFolder = ".";

        if (!output_folder_option.empty()) {
            outputFolder = std::string(output_folder_option);

            if (!is_valid_folder_name(outputFolder)) {
                std::cerr << "Error: Invalid output folder name. Please use only these chars: 0-9a-zA-Z_-/.\n";
                return EXIT_FAILURE;
            }
        }

        if (!create_folder_if_not_exists(outputFolder)) {
            return EXIT_FAILURE;
        }

        print_status("Using output folder: " + outputFolder + "\n", 0, Color::Blue);

        auto xlsx_file    = getFile("xlsx", outputFolder);
        auto csv_file     = getFile("csv", outputFolder);
        auto csv_tmp_file = getFile("csv_tmp", outputFolder);
        auto sqlite_file  = getFile("sqlite", outputFolder);

        // Download

        bool universe_downloaded = false;

        if (file_exists(xlsx_file)) {
            std::cerr << "Download skipped. File already exists.\n";
            universe_downloaded = true;
        } else {
            // formerly "https://wikifolio.blob.core.windows.net/prod-documents/Investment_Universe.de.xlsx"
            char const * xlsx_url =
                "https://wikifoliostorage.blob.core.windows.net/prod-documents/Investment_Universe.de.xlsx";

            universe_downloaded = download(xlsx_url, xlsx_file);

            Timer::stop("Download");
        }

        // XLSX -> CSV

        if (!universe_downloaded) {
            std::cerr << "Error: Failed to download the investment universe. Aborting.\n";
            return EXIT_FAILURE;
        }

        {
            bool const converted_to_csv = xlsx_to_csv(xlsx_file, csv_file);

            Timer::stop("xlsx -> csv");

            if (!converted_to_csv) {
                return EXIT_FAILURE;
            }

            if (!rename_header_columns(csv_file, csv_tmp_file)) {
                return EXIT_FAILURE;
            }
        }

        // CSV -> SQLITE

        bool const converted_to_sqlite = csv_to_sqlite(csv_file, sqlite_file);

        if (!converted_to_sqlite) {
            std::cerr << "Error: Failed to convert CSV to SQLite\n";
            return EXIT_FAILURE;
        }

        Timer::stop("csv -> sqlite");

        Timer::stop("Total Runtime");

        // FINI

        print_status("Done.", 0, Color::Green);

        return EXIT_SUCCESS;
    }

    std::cerr << format("Error: Unknown option \"{}\".\n", flag);

    printHelpText(name);

    return EXIT_FAILURE;
}
