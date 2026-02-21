// SPDX-FileCopyrightText: 2021-2026 Jens A. Koch
// SPDX-License-Identifier: MIT
// This file is part of https://github.com/jakoch/wikifolio_universe_converter.

#include "main.h"

#include <cinttypes>

#include <algorithm> // std::all_of
#include <cctype>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <filesystem> // NOLINT(build/c++17): <filesystem> unapproved C++17 header. sure.
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <ostream>
#include <span>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>

#include <fmt/format.h>
using fmt::format;
using fmt::vformat;

#include <sqlite3.h>

#include "download.h"

#include "xlsxio_read.h"

#include "version.h"

XLSXReader::XLSXReader(char const * filename) : handle(xlsxioread_open(filename))
{
    // init
}

XLSXReader::~XLSXReader()
{
    xlsxioread_close(handle);
}

std::unique_ptr<XLSXSheet> XLSXReader::OpenSheet(char const * sheetname, unsigned int flags)
{
    if (auto* sheet = xlsxioread_sheet_open(handle, sheetname, flags)) {
        return std::unique_ptr<XLSXSheet>(new XLSXSheet(sheet));
    }
    return nullptr;
}

XLSXSheet::XLSXSheet(xlsxioreadersheet sheet) noexcept : sheethandle(sheet) { }

XLSXSheet::~XLSXSheet()
{
    xlsxioread_sheet_close(sheethandle);
}

bool XLSXSheet::GetNextRow()
{
    return (xlsxioread_sheet_next_row(sheethandle) != 0);
}

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

    std::string getYear()
    {
        static std::time_t const time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        std::stringstream stringstream;
        stringstream << std::put_time(std::localtime(&time), "%Y");
        return stringstream.str();
    }

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
        Timer() = default;

        static void stop(char const * time_point_name)
        {
            auto const stop_time       = Time::now();
            auto const time_diff       = stop_time - start_time;
            auto const ms_duration     = std::chrono::duration_cast<std::chrono::milliseconds>(time_diff).count();
            char constexpr const * fmt = "[{}] Time Elapsed: {}.{} sec\n";
            std::ostringstream strStream;
            int const msec = 1000;
            strStream << fmt::format(fmt, time_point_name, (ms_duration / msec), (ms_duration % msec));
            std::cout << strStream.str();
        }
    };

    inline std::string replace(std::string search_in, std::string const & search_for, std::string const & replace_with)
    {
        if (search_for.empty()) {
            return search_in;
        }

        std::size_t pos = 0;

        while ((pos = search_in.find(search_for)) != std::string::npos) {
            search_in.replace(pos, search_for.size(), replace_with);
        }

        return search_in;
    }

    std::string escape_string(std::string const & str)
    {
        std::ostringstream out;
        for (char const _char : str) {
            switch (_char) {
            // case '"': out << "\\\""; break;
            case '\'':
                out << "\'\'";
                break;
            /*case '\\': out << "\\\\"; break;
            case '\b': out << "\\b"; break;
            case '\f': out << "\\f"; break;
            case '\n': out << "\\n"; break;
            case '\r': out << "\\r"; break;
            case '\t': out << "\\t"; break;*/
            default:
                out << _char;
            }
        }
        return out.str();
    }

    bool xlsx_to_csv(std::string const & xlsx_filename, std::string const & csv_filename)
    {
        XLSXReader file(xlsx_filename.c_str());

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
            while (sheet->GetNextCellString(value)) {
                // printf("%s\t", value.c_str());
                csv_row.append("'");
                csv_row.append(escape_string(value));
                csv_row.append("'");
                csv_row.append(",");
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

        std::string line;
        bool replaced = false;

        while (!input_file.eof()) {
            getline(input_file, line);

            // change column names on first line of file only once
            if (!replaced) {
                line     = replace(line, "Anlageuniversum (Gruppe) 1", "Anlagegruppe1");
                line     = replace(line, "Anlageuniversum 1", "Anlageuniversum1");
                line     = replace(line, "Anlageuniversum (Gruppe) 2", "Anlagegruppe2");
                line     = replace(line, "Anlageuniversum 2", "Anlageuniversum2");
                line     = replace(line, "Anlageuniversum (Gruppe) 3", "Anlagegruppe3");
                line     = replace(line, "Anlageuniversum 3", "Anlageuniversum3");
                replaced = true;
            }

            output_file << line << '\n';
        }
        output_file.close();
        input_file.close();

        // delete old "Investment_Universe.csv"
        if (std::remove(csv_filename.c_str()) != 0) {
            std::cerr << format("Could not delete file.");
        }

        // rename "Investment_Universe.tmp.csv" -> "Investment_Universe.csv"
        if (std::rename(csv_tmp_filename.c_str(), csv_filename.c_str()) != 0) {
            std::cerr << format("Could not rename.");
        }

        return true;
    }

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
            "Anlageuniversum3 TEXT)";

        int const res = sqlite3_exec(_db, sql_table_schema, nullptr, nullptr, nullptr);

        if (res != SQLITE_OK) {
            std::cerr << format("[SQLite][Error][{}]\nFailed to create table: {}\n", res, sqlite3_errmsg(_db));
            sqlite3_close(_db);
            return false;
        }

        return res == SQLITE_OK;
    }

    bool csv_to_sqlite(std::string const & csv_filename, std::string const & sqlite_filename)
    {
        int res = 0;

        char* zErrMsg = nullptr;

        // Open SQLite Database

        sqlite3* dbHandle = nullptr;

        res = sqlite3_open(sqlite_filename.c_str(), &dbHandle);
        if (res != SQLITE_OK) {
            std::cerr << format("[SQLite][Error][{}]\nDB connection error: {}\n", res, sqlite3_errmsg(dbHandle));
            sqlite3_close(dbHandle);
            return false;
        }

        // Set full synchronous

        res = sqlite3_exec(dbHandle, "PRAGMA synchronous=2;", nullptr, nullptr, nullptr);
        if (res != SQLITE_OK) {
            std::cerr
                << format("[SQLite][Error][{}]\nFailed to set synchronous for {}\n", res, sqlite_filename.c_str());
            sqlite3_close(dbHandle);
            return false;
        }

        // Create Table

        if (!create_table(dbHandle)) {
            exit(EXIT_FAILURE);
        }

        // Open CSV for reading

        std::ifstream csv_file(csv_filename.c_str(), std::ios::in);
        if (!csv_file.is_open()) {
            std::cerr << format("Error opening CSV file.");
            exit(EXIT_FAILURE);
        }

        // Iterate CSV data, build INSERT statement, exec query

        static std::string const sql_insert_stmt_tpl =
            "INSERT INTO Anlageuniversum ( ISIN, WKN, SecurityType, Bezeichnung, Emittent, "
            "Anlagegruppe1, Anlageuniversum1, Anlagegruppe2, Anlageuniversum2, Anlagegruppe3, Anlageuniversum3 ) "
            "VALUES ( {} );";

        std::string sql_insert_values;
        std::string line;
        std::string field;

        // get first line and ignore it. it's the table header, which is set in sql_table_schema (see create_table()).
        std::getline(csv_file, line);

        // wrap insert queries in transaction block
        res = sqlite3_exec(dbHandle, "BEGIN TRANSACTION;", nullptr, nullptr, &zErrMsg);

        if (res != SQLITE_OK) {
            std::cerr
                << format("[SQLite][Error][{}]\nBegin Transaction: {}, {}\n", res, zErrMsg, sqlite3_errmsg(dbHandle));
            sqlite3_free(zErrMsg);
            return false;
        }

        while (std::getline(csv_file, line)) {
            if (line.empty()) {
                break;
            }

            std::stringstream lineStream(line);

            while (std::getline(lineStream, field, ',')) {
                sql_insert_values.append(field);
                sql_insert_values.append(", ");
            }
            sql_insert_values.pop_back(); // comma
            sql_insert_values.pop_back(); // nl?

            lineStream.clear();

            std::string sql_insert_values_str = sql_insert_values;
            // printf("%s\n", sql_insert_values_str);
            auto sql_insert_args = fmt::make_format_args(sql_insert_values_str);

            std::string sql_insert_stmt = vformat(sql_insert_stmt_tpl, sql_insert_args);

            sql_insert_values.clear();

            // printf("%s\n", sql_insert_stmt.c_str());

            res = sqlite3_exec(dbHandle, sql_insert_stmt.c_str(), nullptr, nullptr, &zErrMsg);
            if (res != SQLITE_OK) {
                std::cerr << format("[SQLite][Error][{}]\nQuery Exec: {}\n", res, zErrMsg);
                std::cerr << format("[SQLite][Error]\nQuery: {}\n", sql_insert_stmt.c_str());
                sqlite3_free(zErrMsg);
                return false;
            }

            sql_insert_stmt.clear();
        }

        res = sqlite3_exec(dbHandle, "END TRANSACTION;", nullptr, nullptr, &zErrMsg);

        if (res != SQLITE_OK) {
            std::cerr << format("[SQLite][Error][{}]\nEnd Transaction: {}\n", res, zErrMsg);
            sqlite3_free(zErrMsg);
            return false;
        }

        sqlite3_close(dbHandle);

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

    bool file_exists(std::string const & filename)
    {
        std::filesystem::path const file = std::filesystem::current_path() / filename;
        return std::filesystem::exists(file) && std::filesystem::is_regular_file(file);
    }

    void create_folder_if_not_exists(std::string const & folder_path)
    {
        std::filesystem::path const folder(folder_path);
        if (!std::filesystem::exists(folder)) {
            std::filesystem::create_directory(folder);
        }
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

    namespace Util
    {

        // requires <ranges> support
        /*auto is_alnum(std::string const& str) -> bool
        {
            return std::ranges::all_of(str.begin(), str.end(), [](char _char) {
                return std::isalnum(static_cast<unsigned char>(_char));
            });
        }*/

        bool is_alnum(std::string const & str)
        {
            return std::ranges::all_of(str, [](char _char) {
                return std::isalnum(static_cast<unsigned char>(_char));
            });
        }

        // requires <ranges> support
        /*bool is_valid_folder_name(std::string const& folder)
        {
            static std::string const char_whitelist =
        "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_-/."; return std::ranges::all_of(folder.begin(),
        folder.end(), [&](char _char) { return char_whitelist.find(_char) != std::string::npos;
            });
        }*/

        // conservative approach for chars in a folder name
        // allowed chars: 0-9,a-z,A-Z,_,-,/,.
        bool is_valid_folder_name(std::string const & folder)
        {
            static std::unordered_set<char> const char_whitelist = {
                '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f', 'g',
                'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x',
                'y', 'z', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O',
                'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', '_', '-', '/', '.'};

            return std::ranges::all_of(folder, [](char _char) {
                return char_whitelist.contains(_char);
            });
        }

    } // namespace Util

} // anonymous namespace

int main(int const argc, char const * argv[]) noexcept(false)
{
    std::string const name = "wiuc";

    std::span const args(argv, static_cast<size_t>(argc));

    // default action, no arguments → help
    if (args.size() <= 1) {
        printHelpText(name);
        return EXIT_SUCCESS;
    }

    // We only support single-flag invocation
    if (args.size() != 2) {
        printHelpText(name);
        return EXIT_SUCCESS;
    }

    std::string_view const flag = args[1];

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

        Timer const total_application_timer;

        // Output Folder

        // The output folder set via "-o" or "--out".
        // The default output folder is the current directory.
        std::string outputFolder = ".";

        if (args.size() == 4) {
            std::string_view const flag2 = args[2];
            if (flag2 == "-o" || flag2 == "--out") {
                outputFolder = std::string(args[3]);

                if (!Util::is_valid_folder_name(outputFolder)) {
                    std::cerr << "Error: Invalid output folder name. Please use only these chars: 0-9a-zA-Z_-/.\n";
                    return EXIT_FAILURE;
                }
            }
        }

        create_folder_if_not_exists(outputFolder);

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
            Timer const download_timer;

            // formerly "https://wikifolio.blob.core.windows.net/prod-documents/Investment_Universe.de.xlsx"
            char const * xlsx_url =
                "https://wikifoliostorage.blob.core.windows.net/prod-documents/Investment_Universe.de.xlsx";

            universe_downloaded = download(xlsx_url, xlsx_file);

            Timer::stop("Download");
        }

        // XLSX -> CSV

        if (universe_downloaded) {

            Timer const xlsx_to_csv_timer;

            bool const converted_to_csv = xlsx_to_csv(xlsx_file, csv_file);

            Timer::stop("xlsx -> csv");

            if (converted_to_csv) {
                rename_header_columns(csv_file, csv_tmp_file);
            }
        }

        // CSV -> SQLITE

        Timer const csv_to_sqlite_timer;

        bool const converted_to_sqlite = csv_to_sqlite(csv_file, sqlite_file);

        if (!converted_to_sqlite) {
            std::cerr << "Error: Failed to convert CSV to SQLite\n";
            return 1;
        }

        Timer::stop("csv -> sqlite");

        Timer::stop("Total Runtime");

        // FINI

        print_status("Done.", 0, Color::Green);

        return EXIT_SUCCESS;
    }

    return EXIT_SUCCESS;
}
