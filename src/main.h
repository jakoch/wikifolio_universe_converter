// SPDX-FileCopyrightText: 2021-2026 Jens A. Koch
// SPDX-License-Identifier: MIT
// This file is part of https://github.com/jakoch/wikifolio_universe_converter.

#pragma once

#include <ctime> // time_t
#include <cstdint> // int64_t
#include <memory>
#include <string>

#include "xlsxio_read.h"

class XLSXSheet
{
private:
    xlsxioreadersheet sheethandle;
    explicit XLSXSheet(xlsxioreadersheet sheet) noexcept;

public:
    ~XLSXSheet();

    XLSXSheet(XLSXSheet const &)            = delete;
    XLSXSheet& operator=(XLSXSheet const &) = delete;

    // A sheet owns an open xlsxio handle, so it is neither movable nor copyable.
    XLSXSheet(XLSXSheet&&)            = delete;
    XLSXSheet& operator=(XLSXSheet&&) = delete;

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

    // A reader owns an open xlsxio handle, so it is neither movable nor copyable.
    XLSXReader(XLSXReader&&)            = delete;
    XLSXReader& operator=(XLSXReader&&) = delete;

    // xlsxioread_open() returns null if the file cannot be opened,
    // for example when it is not a valid XLSX (zip) file.
    [[nodiscard]] bool is_open() const noexcept;

    std::unique_ptr<XLSXSheet> OpenSheet(char const * sheetname, unsigned int flags);
};
