#!/usr/bin/env bash

# SPDX-License-Identifier: MIT
# SPDX-FileCopyrightText: 2024 Jens A. Koch.
# This file is part of https://github.com/jakoch/wikifolio_universe_converter.

# Allow ENV.CLANG_FORMAT to define the path to the binary or default to clang-format
CLANG_FORMAT=${CLANG_FORMAT:-clang-format}

# Check if the binary exists and matches a supported version
function check_version() {
    local binary=$1
    if command -v "$binary" &> /dev/null; then
        local version=$("$binary" --version)
        if [[ $version =~ "version 17" || $version =~ "version 18" ]]; then
            echo "$binary"
            return 0
        fi
    fi
    return 1
}

# Try to find a valid clang-format binary
if ! CLANG_FORMAT=$(check_version "$CLANG_FORMAT") &&
   ! CLANG_FORMAT=$(check_version "clang-format-17") &&
   ! CLANG_FORMAT=$(check_version "clang-format-18"); then
    echo "Error: No compatible clang-format version (17 or 18) found."
    exit 1
fi

# Display the binary and version being used
VERSION=$("$CLANG_FORMAT" --version)
echo "Using clang-format ($VERSION)"

# Scan the top-level directory and subdirectories for .h and .cpp files

# first convert line endings to Unix format
# this step is skipped in CI environments
if [[ -z "$CI" && -z "$GITHUB_ACTION" ]]; then
    find . -type f \( -name "*.hpp" -o -name "*.cpp" \) -exec dos2unix {} \;
fi

# Apply clang-format in-place to .hpp and .cpp files
find . -type f \( -name "*.hpp" -o -name "*.cpp" \) -exec "$CLANG_FORMAT" -i -style=file {} \;

# In the CI context, we run `git diff --exit-code`.
# After clang-format finishes, we check for changes with `git diff`.
# If there are changes, we exit with a non-zero status code, causing the CI job to fail.
# This ensures that code formatting is enforced.
if [[ -n "$CI" || -n "$GITHUB_ACTION" ]]; then
    if ! git diff --exit-code; then
        echo "Error: Code formatting issues detected. Please run ./build-tools/format.sh and commit the changes."
        exit 1
    fi
fi
