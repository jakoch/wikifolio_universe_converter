// SPDX-FileCopyrightText: 2021-2026 Jens A. Koch
// SPDX-License-Identifier: MIT
// This file is part of https://github.com/jakoch/wikifolio_universe_converter.

#include "paths.h"

#include <algorithm> // std::ranges::all_of
#include <filesystem>
#include <iostream>
#include <ranges>
#include <unordered_set>

#include <fmt/format.h>
using fmt::format;

bool file_exists(std::string const & filename)
{
    std::filesystem::path const file(filename);
    return std::filesystem::exists(file) && std::filesystem::is_regular_file(file);
}

bool create_folder_if_not_exists(std::string const & folder_path)
{
    std::error_code error_code;
    std::filesystem::path const folder(folder_path);

    if (std::filesystem::exists(folder)) {
        return true;
    }

    // create_directories() creates all missing parent folders,
    // create_directory() would fail for a nested path like "a/b".
    if (std::filesystem::create_directories(folder, error_code)) {
        return true;
    }

    std::cerr << format("Error: Could not create output folder \"{}\": {}\n", folder_path, error_code.message());
    return false;
}

bool is_valid_folder_name(std::string const & folder)
{
    static std::unordered_set<char> const char_whitelist = {
        '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l',
        'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
        'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', '_', '-', '/', '.'};

    return std::ranges::all_of(folder, [](char _char) {
        return char_whitelist.contains(_char);
    });
}
