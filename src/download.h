// SPDX-FileCopyrightText: 2021-2026 Jens A. Koch
// SPDX-License-Identifier: MIT
// This file is part of https://github.com/jakoch/wikifolio_universe_converter.

#ifndef SRC_DOWNLOAD_H_
#define SRC_DOWNLOAD_H_

#include <string>

bool download(char const * url, std::string const & save_as_filename);

#endif // SRC_DOWNLOAD_H_
