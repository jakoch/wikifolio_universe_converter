// Copyright 2021-2024 Jens A. Koch.
// SPDX-License-Identifier: MIT
// This file is part of jakoch/wikifolio_universe_converter.

#pragma once

#if defined(__linux__)
#define _OS_LINUX_
#elif defined(_WIN32) || defined(_WIN64)
#define _OS_WINDOWS_
#endif

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>

#include <algorithm> // std::all_of
#include <chrono>
#include <filesystem> // NOLINT(build/c++17): <filesystem> unapproved C++17 header. sure.
#include <fstream>
#include <iomanip>
#include <iostream>
#include <ostream>
// #include <ranges>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <unordered_map>

#include <fmt/format.h>
using fmt::format;
using fmt::make_format_args;
using fmt::vformat;

#include "xlsxio_read.h"

#include <sqlite3.h>

#include <curl/curl.h>
#include <curl/easy.h>

#include "version.h"

class XLSXReader
{
private:
    xlsxioreader handle;

public:
    explicit XLSXReader(char const * filename);
    ~XLSXReader();

    class XLSXSheet* OpenSheet(char const * sheetname, unsigned int flags);
};

class XLSXSheet
{
private:
    xlsxioreadersheet sheethandle;
    explicit XLSXSheet(xlsxioreadersheet sheet);
    XLSXSheet(xlsxioreadersheet xlsxhandle, char const * sheetname, unsigned int flags);

public:
    ~XLSXSheet();
    bool GetNextRow();
    char* GetNextCell();
    bool GetNextCellString(char*& value);
    bool GetNextCellString(std::string& value);
    bool GetNextCellInt(int64_t& value);
    bool GetNextCellFloat(double& value);
    bool GetNextCellDateTime(time_t& value);

    XLSXSheet& operator>>(char*& value);
    XLSXSheet& operator>>(std::string& value);
    XLSXSheet& operator>>(int64_t& value);
    XLSXSheet& operator>>(double& value);
    void* operator new(std::size_t size);
    void operator delete(void* ptr);

    friend class XLSXReader;
};
