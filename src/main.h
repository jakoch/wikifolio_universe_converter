// SPDX-FileCopyrightText: 2021-2026 Jens A. Koch
// SPDX-License-Identifier: MIT
// This file is part of https://github.com/jakoch/wikifolio_universe_converter.

#pragma once

#include <memory>
#include <string>

#include "xlsxio_read.h"

class XLSXSheet
{
private:
    xlsxioreadersheet sheethandle;
    explicit XLSXSheet(xlsxioreadersheet sheet) noexcept;

    XLSXSheet(xlsxioreadersheet xlsxhandle, char const * sheetname, unsigned int flags);

public:
    ~XLSXSheet();

    XLSXSheet(XLSXSheet const &)            = delete;
    XLSXSheet& operator=(XLSXSheet const &) = delete;

    XLSXSheet(XLSXSheet&&) noexcept;
    XLSXSheet& operator=(XLSXSheet&&) noexcept;

    bool GetNextRow();
    bool GetNextCellString(char*& value);
    bool GetNextCellString(std::string& value);
    bool GetNextCellInt(int64_t& value);
    bool GetNextCellFloat(double& value);
    bool GetNextCellDateTime(time_t& value);

    friend class XLSXReader;
};

class XLSXReader
{
private:
    xlsxioreader handle{};

public:
    explicit XLSXReader(char const * filename);
    ~XLSXReader();

    XLSXReader(XLSXReader const &)            = delete;
    XLSXReader& operator=(XLSXReader const &) = delete;

    XLSXReader(XLSXReader&&) noexcept;
    XLSXReader& operator=(XLSXReader&&) noexcept;

    std::unique_ptr<XLSXSheet> OpenSheet(char const * sheetname, unsigned int flags);
};
