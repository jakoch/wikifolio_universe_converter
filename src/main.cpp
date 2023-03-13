// Copyright 2022 Jens A. Koch.
// SPDX-License-Identifier: MIT
// This file is part of jakoch/wikifolio_universe_converter.

#include "main.h"

XLSXReader::XLSXReader(char const *filename)
{
    handle = xlsxioread_open(filename);
}

XLSXReader::~XLSXReader()
{
    xlsxioread_close(handle);
}

class XLSXSheet *XLSXReader::OpenSheet(char const *sheetname, unsigned int flags)
{
    xlsxioreadersheet sheethandle = xlsxioread_sheet_open(handle, sheetname, flags);
    if (sheethandle == NULL) { return NULL; }
    return new XLSXSheet(sheethandle);
}

XLSXSheet::XLSXSheet(xlsxioreadersheet sheet) : sheethandle(sheet) { }

XLSXSheet::~XLSXSheet()
{
    xlsxioread_sheet_close(sheethandle);
}

bool XLSXSheet::GetNextRow()
{
    return (xlsxioread_sheet_next_row(sheethandle) != 0);
}

char *XLSXSheet::GetNextCell()
{
    return xlsxioread_sheet_next_cell(sheethandle);
}

bool XLSXSheet::GetNextCellString(char *&value)
{
    if (!xlsxioread_sheet_next_cell_string(sheethandle, &value)) {
        value = NULL;
        return false;
    }
    return true;
}

bool XLSXSheet::GetNextCellString(std::string &value)
{
    char *result;
    if (!xlsxioread_sheet_next_cell_string(sheethandle, &result)) {
        value.clear();
        return false;
    }
    value.assign(result);
    free(result);
    return true;
}

bool XLSXSheet::GetNextCellInt(int64_t &value)
{
    if (!xlsxioread_sheet_next_cell_int(sheethandle, &value)) {
        value = 0;
        return false;
    }
    return true;
}

bool XLSXSheet::GetNextCellFloat(double &value)
{
    if (!xlsxioread_sheet_next_cell_float(sheethandle, &value)) {
        value = 0;
        return false;
    }
    return true;
}

bool XLSXSheet::GetNextCellDateTime(time_t &value)
{
    if (!xlsxioread_sheet_next_cell_datetime(sheethandle, &value)) {
        value = 0;
        return false;
    }
    return true;
}

XLSXSheet &XLSXSheet::operator>>(char *&value)
{
    GetNextCellString(value);
    return *this;
}

XLSXSheet &XLSXSheet::operator>>(std::string &value)
{
    GetNextCellString(value);
    return *this;
}

XLSXSheet &XLSXSheet::operator>>(int64_t &value)
{
    GetNextCellInt(value);
    return *this;
}

XLSXSheet &XLSXSheet::operator>>(double &value)
{
    GetNextCellFloat(value);
    return *this;
}
/*
XLSXReaderSheet& XLSXReaderSheet::operator >> (time_t& value)
{
  GetNextCellDateTime(value);
  return *this;
}*/

std::string getYear()
{
    static const std::time_t time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time), "%Y");
    return ss.str();
}

class Timer
{
public:
    using Time = std::conditional_t<
        std::chrono::high_resolution_clock::is_steady,
        std::chrono::high_resolution_clock,
        std::chrono::steady_clock>;

private:
    inline static const Time::time_point start_time = Time::now();

public:
    Timer() = default;

    void stop(char const *time_point_name)
    {
        auto const stop_time   = Time::now();
        auto const time_diff   = stop_time - start_time;
        auto const ms_duration = std::chrono::duration_cast<std::chrono::milliseconds>(time_diff).count();
        char constexpr const* fmt = "[{}] Time Elapsed: {}.{} sec\n";
        std::ostringstream strStream;
        strStream << format(fmt, time_point_name, (ms_duration / 1000), (ms_duration % 1000));
        std::cout << strStream.str();
    }
};

inline std::string replace(std::string search_in, std::string const &search_for, std::string const &replace_with)
{
    if (search_for.empty()) { return search_in; }

    std::size_t pos;
    while ((pos = search_in.find(search_for)) != std::string::npos) {
        search_in.replace(pos, search_for.size(), replace_with);
    }

    return search_in;
}

std::string escape_string(std::string const &s)
{
    std::ostringstream o;
    for (auto c = s.cbegin(); c != s.cend(); c++) {
        switch (*c) {
        // case '"': o << "\\\""; break;
        case '\'':
            o << "\'\'";
            break;
        /*case '\\': o << "\\\\"; break;
        case '\b': o << "\\b"; break;
        case '\f': o << "\\f"; break;
        case '\n': o << "\\n"; break;
        case '\r': o << "\\r"; break;
        case '\t': o << "\\t"; break;*/
        default:
            o << *c;
        }
    }
    return o.str();
}

char const *investment_universe_URL =
    "https://wikifolio.blob.core.windows.net/prod-documents/Investment_Universe.de.xlsx";

char const *xlsx_filename    = "Investment_Universe.de.xlsx";
char const *csv_tmp_filename = "Investment_Universe.tmp.csv";
char const *csv_filename     = "Investment_Universe.csv";
char const *sqlite_filename  = "Investment_Universe.sqlite";

bool xlsx_to_csv()
{
    XLSXReader *file = new XLSXReader(xlsx_filename);
    XLSXSheet *sheet = file->OpenSheet(NULL, XLSXIOREAD_SKIP_EMPTY_ROWS);

    if (sheet == nullptr) {
        std::printf("xlsxio was unable to find the first sheet.");
        return false;
    }

    if (sheet) {
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
        delete sheet;
    }
    delete file;

    return true;
};

bool rename_header_columns()
{
    std::fstream input_file(csv_filename, std::ios::in);
    std::ofstream output_file(csv_tmp_filename);

    std::string line;
    bool replaced = 0;

    while (!input_file.eof()) {
        getline(input_file, line);

        // change column names on first line of file only once
        if (replaced == 0) {
            line     = replace(line, "Anlageuniversum(Gruppe)", "Anlagegruppe");
            replaced = 1;
        }

        output_file << line << '\n';
    }
    output_file.close();
    input_file.close();

    // delete old "Investment_Universe.csv"
    if (std::remove(csv_filename)) { printf("Could not delete file."); }

    // rename "Investment_Universe.tmp.csv" -> "Investment_Universe.csv"
    if (std::rename(csv_tmp_filename, csv_filename)) { printf("Could not rename."); }

    return true;
}

size_t write_callback(void *ptr, size_t size, size_t nmemb, FILE *stream)
{
    size_t written = fwrite(ptr, size, nmemb, stream);
    return written;
}

bool download(char const *url, char const *save_as_filename)
{
    CURLcode res;

    curl_global_init(CURL_GLOBAL_DEFAULT);
    CURL *curl = curl_easy_init();

    if (curl) {
        // struct curl_slist *dns;
        // dns = curl_slist_append(NULL, "wikifolio.blob.core.windows.net:443:191.239.203.0");
        // curl_easy_setopt(curl, CURLOPT_RESOLVE, dns);

        struct curl_slist *headers = NULL;
        headers                    = curl_slist_append(headers, "Content-Type:application/octet-stream");
        headers                    = curl_slist_append(headers, "Content-Transfer-Encoding: Binary");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);

        curl_easy_setopt(curl, CURLOPT_BUFFERSIZE, 102400L); // 100Kb
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_AUTOREFERER, 1L);
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
        curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "gzip, deflate, br"); // enable all supported built-in
                                                                              // compressions

        // curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

        curl_easy_setopt(curl, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_1);

        curl_easy_setopt(curl, CURLOPT_USE_SSL, CURLUSESSL_ALL);
        curl_easy_setopt(curl, CURLOPT_USE_SSL, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, false);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, false);

        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);

        FILE *fp = fopen(save_as_filename, "wb");
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);

        char error_buffer[CURL_ERROR_SIZE];
        error_buffer[0] = '\0';
        curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, error_buffer);

        long http_code = 0;
        res            = curl_easy_perform(curl);
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

        if (res == CURLE_OK) {
            // printf("Request was successful with status %i", http_code);
        } else {
            // file has size "0", because downloading it's content failed.
            remove(save_as_filename);
            printf("Request failed: %s", error_buffer);
        }

        fclose(fp);
    }

    curl_easy_cleanup(curl);
    curl_global_cleanup();

    return (res == CURLE_OK) ? 1 : 0;
}

int create_table(sqlite3 *db)
{
    static char const *sql_table_schema =
        "CREATE TABLE Anlageuniversum ("
        "ISIN TEXT,"
        "WKN TEXT,"
        "SecurityType TEXT,"
        "Bezeichnung TEXT,"
        "Emittent TEXT,"
        "Anlagegruppe TEXT,"
        "Anlageuniversum TEXT)";

    int r = sqlite3_exec(db, sql_table_schema, nullptr, nullptr, nullptr);

    if (r != SQLITE_OK) {
        fprintf(stderr, "[SQLite][Error][%i]\nFailed to create table: %s\n", r, sqlite3_errmsg(db));
        sqlite3_close(db);
        return 0;
    }

    return r == SQLITE_OK;
}

bool csv_to_sqlite()
{
    int r;
    char *zErrMsg = 0;

    // Open SQLite Database

    sqlite3 *db;

    r = sqlite3_open(sqlite_filename, &db);
    if (r != SQLITE_OK) {
        fprintf(stderr, "[SQLite][Error][%i]\nDB connection error: %s\n", r, sqlite3_errmsg(db));
        sqlite3_close(db);
        return 0;
    }

    // Set full synchronous

    r = sqlite3_exec(db, "PRAGMA synchronous=2;", nullptr, nullptr, nullptr);
    if (r != SQLITE_OK) {
        fprintf(stderr, "[SQLite][Error][%i]\nFailed to set synchronous for %s\n", r, sqlite_filename);
        sqlite3_close(db);
        return 0;
    }

    // Create Table

    if (!create_table(db)) {
        exit(EXIT_FAILURE);
    };

    // Open CSV for reading

    std::ifstream csv_file(csv_filename, std::ios::in);
    if (!csv_file.is_open()) {
        fprintf(stderr, "Error opening CSV file.");
        exit(EXIT_FAILURE);
    }

    // Iterate CSV data, build INSERT statement, exec query

    static const std::string sql_insert_stmt_tpl =
        "INSERT INTO Anlageuniversum ( ISIN, WKN, SecurityType, Bezeichnung, Emittent, Anlagegruppe, Anlageuniversum ) "
        "VALUES ( {} );";

    std::string sql_insert_values, sql_insert_stmt, line, field;

    // get first line and ignore it. it's the table header, which is set in sql_table_schema (see create_table()).
    std::getline(csv_file, line);

    // wrap insert queries in transaction block
    r = sqlite3_exec(db, "BEGIN TRANSACTION;", nullptr, nullptr, &zErrMsg);

    if (r != SQLITE_OK) {
        fprintf(stderr, "[SQLite][Error][%i]\nBegin Transaction: %s, %s\n", r, zErrMsg, sqlite3_errmsg(db));
        sqlite3_free(zErrMsg);
        return 0;
    }

    while (std::getline(csv_file, line)) {
        if (line.empty()) { break; }

        std::stringstream lineStream(line);

        while (std::getline(lineStream, field, ',')) {
            sql_insert_values.append(field);
            sql_insert_values.append(", ");
        }
        sql_insert_values.pop_back(); // comma
        sql_insert_values.pop_back(); // nl?

        lineStream.clear();

        // printf("%s\n", sql_insert_values.c_str());

        sql_insert_stmt = vformat(sql_insert_stmt_tpl, make_format_args(sql_insert_values.c_str()));

        sql_insert_values.clear();

        // printf("%s\n", sql_insert_stmt.c_str());

        r = sqlite3_exec(db, sql_insert_stmt.c_str(), nullptr, nullptr, &zErrMsg);
        if (r != SQLITE_OK) {
            fprintf(stderr, "[SQLite][Error][%i]\nQuery Exec: %s\n", r, zErrMsg);
            fprintf(stderr, "[SQLite][Error]\nQuery: %s\n", sql_insert_stmt.c_str());
            sqlite3_free(zErrMsg);
            return 0;
        }

        sql_insert_stmt.clear();
    }

    r = sqlite3_exec(db, "END TRANSACTION;", nullptr, nullptr, &zErrMsg);

    if (r != SQLITE_OK) {
        fprintf(stderr, "[SQLite][Error][%i]\nEnd Transaction: %s\n", r, zErrMsg);
        sqlite3_free(zErrMsg);
        return 0;
    }

    sqlite3_close(db);

    return 1;
}

bool file_exists(std::string filename) {
    std::filesystem::path file = std::filesystem::current_path() / filename;
    return std::filesystem::exists(file) && std::filesystem::is_regular_file(file);
}

static std::string printHelpText(char const * const program_name)
{
    char help_text[400];
    sprintf(help_text,
            "%s\n"
            "%s\n\n"
            "Usage: %s [OPTIONS]\n\n"
            "Options:\n"
            "  -c,  --convert        Convert from XLSX to SQLite and CSV\n"
            "  -h,  --help           Display this help message\n"
            "  -v,  --version        Display version information\n"
            "  -vj, --version-json   Display version information as JSON\n"
            "  -vo, --version-only   Display version number only\n",
            app_version::get_nice_name(),
            app_version::get_copyright(),
            program_name);

    return std::string(help_text);
}


int main(int const argc, char const *argv[])
{
    char const *name = argv[0];
    (void)argc;

    // default action
    if (argc <= 1) {
        printHelpText(name);
        return EXIT_SUCCESS;
    }

    // Check for command line arguments
    if (argc == 2 && (std::string(argv[1]) == "-h" || std::string(argv[1]) == "--help"))
    {
        std::cout << printHelpText(name) << std::endl;
        return EXIT_SUCCESS;
    }
    else if (argc == 2 && (std::string(argv[1]) == "-v" || std::string(argv[1]) == "--version"))
    {
        printf("%s v%s\n", app_version::get_nice_name(), app_version::get_version());
        return EXIT_SUCCESS;
    }
    else if (argc == 2 && std::string(argv[1]) == "-vo" || std::string(argv[1]) == "--version-only")
    {
        printf("%s\n", app_version::get_version());
        return EXIT_SUCCESS;
    }
    else if (argc == 2 && std::string(argv[1]) == "-vj" || std::string(argv[1]) == "--version-json")
    {
        std::cout << app_version::get_version_json() << std::endl;
        return EXIT_SUCCESS;
    }
    else if (argc == 2 && std::string(argv[1]) == "-c" || std::string(argv[1]) == "--convert")
    {

        printf("%s v%s\n%s.\n\n", app_version::get_nice_name(), app_version::get_version(), app_version::get_copyright());

        Timer total_application_timer;

        // Download

        bool universe_downloaded = false;

        if (file_exists(xlsx_filename)) {
            std::cerr << "Download skipped. File already exists.\n";
            universe_downloaded = true;
        } else {
            Timer download_timer;

            universe_downloaded = download(investment_universe_URL, xlsx_filename);

            download_timer.stop("Download");
        }

        // XLSX -> CSV

        if (universe_downloaded) {

            Timer xlsx_to_csv_timer;

            bool converted_to_csv = xlsx_to_csv();

            xlsx_to_csv_timer.stop("xlsx -> csv");

            if (converted_to_csv) { rename_header_columns(); }
        }

        // CSV -> SQLITE

        Timer csv_to_sqlite_timer;

        bool converted_to_sqlite = csv_to_sqlite();

        csv_to_sqlite_timer.stop("csv -> sqlite");

        total_application_timer.stop("Total Runtime");

        // FINI

        std::cout << "Done.";

        return EXIT_SUCCESS;
    }

    return EXIT_SUCCESS;
}
