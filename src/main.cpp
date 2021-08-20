#include "main.h"

XLSXReader::XLSXReader(const char* filename)
{
  handle = xlsxioread_open(filename);
}

XLSXReader::~XLSXReader()
{
  xlsxioread_close(handle);
}

class XLSXSheet* XLSXReader::OpenSheet(const char* sheetname, unsigned int flags)
{
  xlsxioreadersheet sheethandle = xlsxioread_sheet_open(handle, sheetname, flags);
  if (sheethandle == NULL) {
    return NULL;
  }
  return new XLSXSheet(sheethandle);
}

XLSXSheet::XLSXSheet(xlsxioreadersheet sheet)
: sheethandle(sheet)
{
}

XLSXSheet::~XLSXSheet()
{
  xlsxioread_sheet_close(sheethandle);
}

bool XLSXSheet::GetNextRow()
{
  return (xlsxioread_sheet_next_row(sheethandle) != 0);
}

char* XLSXSheet::GetNextCell()
{
  return xlsxioread_sheet_next_cell(sheethandle);
}

bool XLSXSheet::GetNextCellString(char*& value)
{
  if (!xlsxioread_sheet_next_cell_string(sheethandle, &value)) {
    value = NULL;
    return false;
  }
  return true;
}

bool XLSXSheet::GetNextCellString(std::string& value)
{
  char* result;
  if (!xlsxioread_sheet_next_cell_string(sheethandle, &result)) {
    value.clear();
    return false;
  }
  value.assign(result);
  free(result);
  return true;
}

bool XLSXSheet::GetNextCellInt(int64_t& value)
{
  if (!xlsxioread_sheet_next_cell_int(sheethandle, &value)) {
    value = 0;
    return false;
  }
  return true;
}

bool XLSXSheet::GetNextCellFloat(double& value)
{
  if (!xlsxioread_sheet_next_cell_float(sheethandle, &value)) {
    value = 0;
    return false;
  }
  return true;
}

bool XLSXSheet::GetNextCellDateTime(time_t& value)
{
  if (!xlsxioread_sheet_next_cell_datetime(sheethandle, &value)) {
    value = 0;
    return false;
  }
  return true;
}

XLSXSheet& XLSXSheet::operator >> (char*& value)
{
  GetNextCellString(value);
  return *this;
}

XLSXSheet& XLSXSheet::operator >> (std::string& value)
{
  GetNextCellString(value);
  return *this;
}

XLSXSheet& XLSXSheet::operator >> (int64_t& value)
{
  GetNextCellInt(value);
  return *this;
}

XLSXSheet& XLSXSheet::operator >> (double& value)
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

#include <type_traits>
#include <sstream>
#include <iostream>

template <class Resolution = std::chrono::milliseconds>
class Timer {
public:
    using Clock = std::conditional_t<std::chrono::high_resolution_clock::is_steady,
        std::chrono::high_resolution_clock,
        std::chrono::steady_clock>;

private:
    const Clock::time_point _start = Clock::now();

public:
    Timer() = default;

    inline void stop(const char *time_point_name) {
        const auto _end = Clock::now();
        const auto duration = std::chrono::duration_cast<Resolution>(_end - _start).count();
        std::ostringstream strStream;
        strStream << "[" << time_point_name << "] Time Elapsed: "
            << format("{:01}.{:04}", (duration % 1'000'000) / 1'000, duration % 1'000)
            << " s" << std::endl;
        std::cout << strStream.str() << std::endl;
    }
};

inline std::string replace(std::string search_in, const std::string& search_for, const std::string& replace_with)
{
    if (search_for.empty()) {
      return search_in;
    }

    std::size_t pos;
    while ((pos = search_in.find(search_for)) != std::string::npos) {
      search_in.replace(pos, search_for.size(), replace_with);
    }

    return search_in;
}

std::string escape_string(const std::string &s) {
    std::ostringstream o;
    for (auto c = s.cbegin(); c != s.cend(); c++) {
        switch (*c) {
        //case '"': o << "\\\""; break;
        case '\'': o << "\'\'"; break;
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

const char* investment_universe_URL = "https://wikifolio.blob.core.windows.net/prod-documents/Investment_Universe.de.xlsx";

const char* xlsx_filename    = "Investment_Universe.de.xlsx";
const char* csv_tmp_filename = "Investment_Universe.tmp.csv";
const char *csv_filename     = "Investment_Universe.csv";
const char* sqlite_filename  = "Investment_Universe.sqlite";

bool xlsx_to_csv()
{
  XLSXReader *file = new XLSXReader(xlsx_filename);
  XLSXSheet *sheet = file->OpenSheet(NULL, XLSXIOREAD_SKIP_EMPTY_ROWS);

  if (sheet == nullptr) {
    std::printf("xlsxio was unable to find the first sheet.");
    return false;
  }

  if (sheet)
  {
    std::ofstream csvfile;
    csvfile.open(csv_filename);

    std::string csv_row;
    std::string value;
    while (sheet->GetNextRow()) {
      csv_row.clear();
      while (sheet->GetNextCellString(value)) {
        //printf("%s\t", value.c_str());
        csv_row.append("'");
        csv_row.append(escape_string(value));
        csv_row.append("'");
        csv_row.append(",");
      }
      csv_row.append("\n");
      //printf("\n");
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

  while (!input_file.eof())
  {
    getline(input_file, line);

    // change column names on first line of file only once
    if(replaced == 0) {
      line = replace(line, "Anlageuniversum(Gruppe)", "Anlagegruppe");
      line = replace(line, "Whitelist für institutionelle Produkte – Schweiz", "WhitelistSchweiz");
      line = replace(line, "IC20 Whitelist", "WhitelistIC20");
      replaced = 1;
    }

    output_file << line << std::endl;
  }
  output_file.close();
  input_file.close();

  // delete old "Investment_Universe.csv"
  if (std::remove(csv_filename)) {
    printf("Could not delete file.");
  }

  // rename "Investment_Universe.tmp.csv" -> "Investment_Universe.csv"
  if (std::rename(csv_tmp_filename, csv_filename)) {
    printf("Could not rename.");
  }

  return true;
}

size_t write_callback(void *ptr, size_t size, size_t nmemb, FILE *stream) {
    size_t written = fwrite(ptr, size, nmemb, stream);
    return written;
}

bool download(const char *url, const char *save_as_filename)
{
  CURLcode res;

  curl_global_init(CURL_GLOBAL_DEFAULT);
  CURL *curl = curl_easy_init();

  if (curl) {
    struct curl_slist *dns;
    dns = curl_slist_append(NULL, "wikifolio.blob.core.windows.net:443:191.239.203.0");
    curl_easy_setopt(curl, CURLOPT_RESOLVE, dns);

    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Content-Type:application/octet-stream");
    headers = curl_slist_append(headers, "Content-Transfer-Encoding: Binary");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);

    curl_easy_setopt(curl, CURLOPT_BUFFERSIZE, 102400L); //100Kb
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_AUTOREFERER, 1L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
    curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "gzip, deflate, br"); // enable all supported built-in compressions

    //curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

    curl_easy_setopt(curl, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_1);

    curl_easy_setopt(curl, CURLOPT_USE_SSL, CURLUSESSL_ALL);
    curl_easy_setopt(curl, CURLOPT_USE_SSL, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, false);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, false);

    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);

    FILE* fp = fopen(save_as_filename, "wb");
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);

    char error_buffer[CURL_ERROR_SIZE];
    error_buffer[0] = '\0';
    curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, error_buffer);

    long http_code = 0;
    res = curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

    if (res == CURLE_OK) {
        //printf("Request was successful with status %d", http_code);
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

void createTable(sqlite3 *db) {
  // Create Schema
  // --------------------------------------------------------------
  // CREATE TABLE Anlageuniversum ("ISIN TEXT, WKN TEXT, SecurityType TEXT, Bezeichnung TEXT, Anlagegruppe TEXT, Anlageuniversum TEXT, WhitelistSchweiz TEXT,WhitelistIC20 TEXT)
  static const char *sql_table_schema = "CREATE TABLE Anlageuniversum ("
                                        "ISIN TEXT,"
                                        "WKN TEXT,"
                                        "SecurityType TEXT,"
                                        "Bezeichnung TEXT,"
                                        "Anlagegruppe TEXT,"
                                        "Anlageuniversum TEXT,"
                                        "WhitelistSchweiz TEXT,"
                                        "WhitelistIC20 TEXT)";

  sqlite3_exec(db, sql_table_schema, nullptr, nullptr, nullptr);
}

bool csv_to_sqlite()
{
  int r;
  char *zErrMsg = 0;

  // Open SQLite Database
  // --------------------

  sqlite3 *db;

  r = sqlite3_open(sqlite_filename, &db);
  if(r != SQLITE_OK) {
    printf("DB connection error: %s\n", sqlite3_errmsg(db));
    sqlite3_close(db);
	}

  // set full synchronous
  r = sqlite3_exec(db, "PRAGMA synchronous=2;", nullptr, nullptr, nullptr);
  if (r != SQLITE_OK) {
      printf("Failed(%d) to set synchronous by %s", r, sqlite_filename);
      return 0;
  }

  // Create Table
  // --------------------

  createTable(db);

  // Open CSV for reading
  // --------------------

  std::ifstream csv_file(csv_filename, std::ios::in);
  if (!csv_file.is_open()) {
		printf("Error opening CSV file.");
	}

  // Iterate CSV data, build INSERT statement, exec query
  // --------------------

  std::string sql_insert_stmt_tpl =
      "INSERT INTO Anlageuniversum ( ISIN, WKN, SecurityType, Bezeichnung, Anlagegruppe, Anlageuniversum, WhitelistSchweiz, WhitelistIC20 ) VALUES( {} );";

  std::string sql_insert_values, sql_insert_stmt, line, field;

  // get first line - and ignore it, because it's expressed in the sql_table_schema (see createTable())
  std::getline(csv_file, line);

  // warp insert queries in an transaction block
  r = sqlite3_exec(db, "BEGIN TRANSACTION;", nullptr, nullptr, &zErrMsg);

  if( r != SQLITE_OK )
	{
    fprintf(stderr, "[SQLite][Begin Transaction] Error: %d, %s, %s\n", r, zErrMsg, sqlite3_errmsg(db));
    sqlite3_free(zErrMsg);
		return 0;
	}

  while(std::getline(csv_file, line))
  {
      if(line.empty()) {
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

      //printf("%s\n", sql_insert_values.c_str());

      sql_insert_stmt = format(sql_insert_stmt_tpl, sql_insert_values.c_str());

      sql_insert_values.clear();

      //printf("%s\n", sql_insert_stmt.c_str());

      r= sqlite3_exec(db, sql_insert_stmt.c_str(), nullptr, nullptr, &zErrMsg);

      if( r != SQLITE_OK )
      {
        fprintf(stderr, "[SQLite][Query Exec] Error: %d, %s\n", r, zErrMsg);
        fprintf(stderr, "[SQLite][Query]: %s\n", sql_insert_stmt.c_str());
        sqlite3_free(zErrMsg);
        return 0;
      }

      sql_insert_stmt.clear();
  }

  r = sqlite3_exec(db, "END TRANSACTION;", nullptr, nullptr, &zErrMsg);

  if( r != SQLITE_OK )
	{
		fprintf(stderr, "[SQLite][End Transaction] Error: %s\n", zErrMsg);
    sqlite3_free(zErrMsg);
		return 0;
	}

  sqlite3_close(db);

  return 1;
}

int main(int argc, char *argv[])
{
  printf("Wikifolio Investment Universe Converter v1.0.0\nCopyright (c) Jens A. Koch, 2021.\n\n");

  Timer<> total_application_timer;

  Timer<> download_timer;

  bool universe_downloaded = download(investment_universe_URL, xlsx_filename);

  download_timer.stop("Download");

  if (universe_downloaded) {

    Timer<> xlsx_to_csv_timer;

    bool converted_to_csv = xlsx_to_csv();

    xlsx_to_csv_timer.stop("xlsx -> csv");

    if (converted_to_csv) {
      rename_header_columns();
    }
  }

  Timer<> csv_to_sqlite_timer;

  bool converted_to_sqlite = csv_to_sqlite();

  csv_to_sqlite_timer.stop("csv -> sqlite");

  total_application_timer.stop("Total Runtime");

  printf("Done.");

  return EXIT_SUCCESS;
}
