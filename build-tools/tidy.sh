#!/usr/bin/env bash

# SPDX-FileCopyrightText: 2021-2026 Jens A. Koch
# SPDX-License-Identifier: MIT
# This file is part of https://github.com/jakoch/wikifolio_universe_converter.

# Get the directory of the script
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Set the path to the compile_commands.json file
COMPILE_COMMANDS="${SCRIPT_DIR}/../out/build/clang20-x64-linux-rel/compile_commands.json"

# Check if the compile_commands.json file exists
if [ ! -f "$COMPILE_COMMANDS" ]; then
    echo "Please run cmake to generate compile_commands.json before running this script."
    exit 1
fi

# Change directory to the top-level directory
cd "${SCRIPT_DIR}/../" || exit

# This runs clang-tidy on all source files.
clang-tidy -p "${COMPILE_COMMANDS}" src/*.h src/*.cpp
