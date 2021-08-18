#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>

#include <string>
#include <stdexcept>
#include <fstream>
#include <ostream>

#include "xlsxio_read.h"

#include <sqlite3.h>

#include <curl/curl.h>
#include <curl/easy.h>

class XLSXReader
{
 private:
   xlsxioreader handle;
 public:
  XLSXReader(const char* filename);
  ~XLSXReader();

  class XLSXSheet* OpenSheet(const char* sheetname, unsigned int flags);
};

class XLSXSheet
{
private:
  xlsxioreadersheet sheethandle;
  XLSXSheet(xlsxioreadersheet sheet);
  XLSXSheet(xlsxioreadersheet xlsxhandle, const char* sheetname, unsigned int flags);

public:

  ~XLSXSheet();
  bool GetNextRow();
  char* GetNextCell();
  bool GetNextCellString(char*& value);
  bool GetNextCellString(std::string& value);
  bool GetNextCellInt(int64_t& value);
  bool GetNextCellFloat(double& value);
  bool GetNextCellDateTime(time_t& value);

  XLSXSheet& operator >> (char*& value);
  XLSXSheet& operator >> (std::string& value);
  XLSXSheet& operator >> (int64_t& value);
  XLSXSheet &operator>>(double &value);

  friend class XLSXReader;
};
