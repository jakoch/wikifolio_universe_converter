// SPDX-FileCopyrightText: 2021-2026 Jens A. Koch
// SPDX-License-Identifier: MIT
// This file is part of https://github.com/jakoch/wikifolio_universe_converter.

#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <fstream>
#include <string>

#include "paths.h"

namespace
{

    // A scratch directory that is removed again on destruction.
    class ScratchDir
    {
    public:
        explicit ScratchDir(std::string const & label)
        {
            // NOLINTNEXTLINE(misc-const-correctness)
            static int counter = 0;
            path = std::filesystem::temp_directory_path() / ("wiuc_paths_" + label + "_" + std::to_string(++counter));

            // These take a non-const std::error_code&, so "ignored" cannot be const.
            // NOLINTNEXTLINE(misc-const-correctness)
            std::error_code ignored;
            std::filesystem::remove_all(path, ignored);
            std::filesystem::create_directories(path, ignored);
        }

        ~ScratchDir()
        {
            // This takes a non-const std::error_code&, so "ignored" cannot be const.
            // NOLINTNEXTLINE(misc-const-correctness)
            std::error_code ignored;
            std::filesystem::remove_all(path, ignored);
        }

        ScratchDir(ScratchDir const &)            = delete;
        ScratchDir& operator=(ScratchDir const &) = delete;
        ScratchDir(ScratchDir&&)                  = delete;
        ScratchDir& operator=(ScratchDir&&)       = delete;

        [[nodiscard]] std::filesystem::path const & dir() const noexcept
        { return path; }

    private:
        std::filesystem::path path;
    };

    std::string as_string(std::filesystem::path const & path)
    { return path.string(); }

} // namespace

TEST_CASE("create_folder_if_not_exists() creates a nested path", "[paths]")
{
    ScratchDir const scratch("nested");
    auto const nested = scratch.dir() / "a" / "b" / "c";

    REQUIRE_FALSE(std::filesystem::exists(nested));

    // create_directory() would fail here, create_directories() must not.
    CHECK(create_folder_if_not_exists(as_string(nested)));
    CHECK(std::filesystem::is_directory(nested));
    CHECK(std::filesystem::is_directory(nested / ".."));
}

TEST_CASE("create_folder_if_not_exists() accepts an existing folder", "[paths]")
{
    ScratchDir const scratch("existing");

    CHECK(create_folder_if_not_exists(as_string(scratch.dir())));
    CHECK(std::filesystem::is_directory(scratch.dir()));
}

TEST_CASE("create_folder_if_not_exists() reports a folder it cannot create", "[paths]")
{
    ScratchDir const scratch("blocked");

    // A regular file blocks the path, so nothing can be created below it.
    std::ofstream blocker(scratch.dir() / "blocker");
    blocker << "not a directory";
    blocker.close();

    CHECK_FALSE(create_folder_if_not_exists(as_string(scratch.dir() / "blocker" / "sub")));
}

TEST_CASE("file_exists() finds a regular file", "[paths]")
{
    ScratchDir const scratch("exists");
    std::ofstream file(scratch.dir() / "file.txt");
    file << "content";
    file.close();

    CHECK(file_exists(as_string(scratch.dir() / "file.txt")));

    // A directory is not a regular file.
    CHECK_FALSE(file_exists(as_string(scratch.dir())));

    // Neither is a path that does not exist.
    CHECK_FALSE(file_exists(as_string(scratch.dir() / "missing.txt")));
}

TEST_CASE("file_exists() does not duplicate the output folder", "[paths]")
{
    ScratchDir const scratch("nodedup");
    std::ofstream file(scratch.dir() / "file.txt");
    file << "content";
    file.close();

    // A folder passed in, as getFile() produces it, must resolve to one path.
    std::string const combined = (scratch.dir() / "file.txt").string();
    CHECK(file_exists(combined));
}

TEST_CASE("is_valid_folder_name() accepts the whitelist", "[paths]")
{
    CHECK(is_valid_folder_name("."));
    CHECK(is_valid_folder_name("out"));
    CHECK(is_valid_folder_name("a/b/c"));
    CHECK(is_valid_folder_name("with_underscore-and-dash"));
    CHECK(is_valid_folder_name("MixedCase123"));
}

TEST_CASE("is_valid_folder_name() rejects illegal characters", "[paths]")
{
    CHECK_FALSE(is_valid_folder_name("bad folder"));
    CHECK_FALSE(is_valid_folder_name("wild*card"));
    CHECK_FALSE(is_valid_folder_name("semi;colon"));
    CHECK_FALSE(is_valid_folder_name("quote\""));
    CHECK_FALSE(is_valid_folder_name("back\\slash"));
    CHECK_FALSE(is_valid_folder_name("umlaut\303\244"));
}
