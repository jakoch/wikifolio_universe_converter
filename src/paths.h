// SPDX-FileCopyrightText: 2021-2026 Jens A. Koch
// SPDX-License-Identifier: MIT
// This file is part of https://github.com/jakoch/wikifolio_universe_converter.

#pragma once

#include <string>

// Returns true, if the path exists and is a regular file.
// The path is used as given. A relative path is resolved against the current
// working directory by the filesystem operations themselves, so prepending
// current_path() here would duplicate an already included output folder.
bool file_exists(std::string const & filename);

// Returns true, if the folder exists, or could be created.
// Nested paths like "a/b/c" are created in full.
bool create_folder_if_not_exists(std::string const & folder_path);

// Conservative check of the characters allowed in a folder name.
// Allowed: 0-9, a-z, A-Z, _, -, /, .
bool is_valid_folder_name(std::string const & folder);
