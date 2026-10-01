// SPDX-FileCopyrightText: 2021-2026 Jens A. Koch
// SPDX-License-Identifier: MIT
// This file is part of https://github.com/jakoch/wikifolio_universe_converter.

#include <catch2/catch_session.hpp>
#include <catch2/catch_test_macros.hpp>

#include <array>
#include <cstdio>
#include <cstdlib> // std::getenv
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>

namespace
{

    // Size of the pipe buffer used to drain the child process output.
    constexpr std::size_t PIPE_BUFFER_SIZE = 256;

    constexpr std::string_view XLSX_NAME = "Investment_Universe.de.xlsx";

    // The paths are passed via the environment by CTest, see CMakeLists.txt.
    std::filesystem::path& wiuc_exe()
    {
        static std::filesystem::path path;
        return path;
    }

    // The directory holding the error fixtures.
    std::filesystem::path& fixture_dir()
    {
        static std::filesystem::path path;
        return path;
    }

    // A scratch directory that is removed again on destruction.
    class ScratchDir
    {
    public:
        explicit ScratchDir(std::string const & label)
        {
            // NOLINTNEXTLINE(misc-const-correctness)
            static int counter = 0;
            path = std::filesystem::temp_directory_path() / ("wiuc_err_" + label + "_" + std::to_string(++counter));

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

        static void seed(std::filesystem::path const & scratch, std::filesystem::path const & fixture)
        { std::filesystem::copy_file(fixture, scratch / XLSX_NAME); }

    private:
        std::filesystem::path path;
    };

    // Runs wiuc and returns the exit code. wiuc writes its diagnostics to stdout and
    // stderr, so both are captured and returned for the caller to search.
    struct RunResult
    {
        int exit_code = -1;
        std::string output{};
    };

    RunResult run(std::string const & arguments)
    {
        // The two calls below build a command line from std::filesystem::path.
        // clang-tidy misreads the temporary returned by string() as pointer
        // arithmetic, so the check is suppressed for the line.
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        std::string const command = "\"" + wiuc_exe().string() + "\" " + arguments + " 2>&1";

        std::array<char, PIPE_BUFFER_SIZE> buffer{};
        // popen() needs a C string and a FILE*, and std::fgets reads into a raw
        // buffer, so pointer arithmetic here is unavoidable.
        // NOLINTNEXTLINE(cert-err33-c, cppcoreguidelines-pro-bounds-pointer-arithmetic)
        FILE* const pipe = popen(command.c_str(), "r"); // NOLINT
        if (pipe == nullptr) {
            return {};
        }

        RunResult result;
        while (std::fgets(buffer.data(), static_cast<int>(buffer.size()), pipe) != nullptr) {
            result.output += buffer.data();
        }

        result.exit_code = pclose(pipe);
        return result;
    }

    // Runs "wiuc -c -o <scratch>" after seeding the scratch dir with "fixture".
    RunResult convert(std::string const & fixture, ScratchDir const & scratch)
    {
        ScratchDir::seed(scratch.dir(), fixture_dir() / fixture);
        // See the note in run(): this is std::string concatenation, not pointer arithmetic.
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        return run("-c -o \"" + scratch.dir().string() + "\"");
    }

} // namespace

TEST_CASE("wiuc reports a corrupt XLSX instead of crashing", "[e2e][error]")
{
    // A partial download leaves a file that is not a zip. xlsxioread_open() returns
    // null for it, which used to be dereferenced and crash the process.
    ScratchDir const scratch("corrupt");
    auto const result = convert("corrupt.xlsx", scratch);

    CHECK(result.exit_code != 0);
    CHECK(result.output.find("unable to open") != std::string::npos);

    // No half-written database is left behind.
    CHECK_FALSE(std::filesystem::exists(scratch.dir() / "Investment_Universe.sqlite"));
}

TEST_CASE("wiuc rejects a spreadsheet with the wrong number of columns", "[e2e][error]")
{
    // The fixture has 10 columns where wiuc expects 13, i.e. Wikifolio changed the
    // investment universe. This must be reported, not silently mis-mapped.
    ScratchDir const scratch("columns");
    auto const result = convert("too_few_columns.xlsx", scratch);

    CHECK(result.exit_code != 0);
    CHECK(result.output.find("Unexpected number of columns") != std::string::npos);
    CHECK(result.output.find("13") != std::string::npos);
}

TEST_CASE("wiuc rejects an output folder name with illegal characters", "[e2e][cli]")
{
    // A space is not in the whitelist. The check happens before anything is created.
    auto const result = run("-c -o \"bad folder\"");

    CHECK(result.exit_code != 0);
    CHECK(result.output.find("Invalid output folder name") != std::string::npos);
}

TEST_CASE("wiuc reports an output folder it cannot create", "[e2e][error]")
{
    ScratchDir const scratch("blocked");

    // A regular file blocks the path, so the directory below it cannot be created.
    std::ofstream blocker(scratch.dir() / "blocker");
    blocker << "not a directory";
    blocker.close();

    auto const result = run("-c -o \"" + (scratch.dir() / "blocker" / "sub").string() + "\"");

    CHECK(result.exit_code != 0);
    CHECK(result.output.find("Could not create output folder") != std::string::npos);
}

TEST_CASE("wiuc skips the download when the XLSX is already present", "[e2e][cli]")
{
    ScratchDir const scratch("skip");
    ScratchDir::seed(scratch.dir(), fixture_dir() / "Investment_Universe.de.xlsx");

    auto const result = run("-c -o \"" + scratch.dir().string() + "\"");

    CHECK(result.exit_code == 0);
    CHECK(result.output.find("Download skipped") != std::string::npos);
}

// The remaining cases cover the command line surface. None of them reaches the
// download, because none of them combines "-c" with a missing .xlsx.

TEST_CASE("wiuc prints help when invoked without arguments", "[e2e][cli]")
{
    auto const result = run("");

    CHECK(result.exit_code == 0);
    CHECK(result.output.find("Usage:") != std::string::npos);
    CHECK(result.output.find("--convert") != std::string::npos);
}

TEST_CASE("wiuc prints help for -h and --help", "[e2e][cli]")
{
    for (auto const * flag : {"-h", "--help"}) {
        INFO("flag: " << flag);

        auto const result = run(flag);

        CHECK(result.exit_code == 0);
        CHECK(result.output.find("Options:") != std::string::npos);
        CHECK(result.output.find("--out <dir>") != std::string::npos);
    }
}

TEST_CASE("wiuc prints the version for -V and -Vo", "[e2e][cli]")
{
    SECTION("-V shows the name and the version")
    {
        auto const result = run("-V");

        CHECK(result.exit_code == 0);
        CHECK(result.output.find("Wikifolio Investment Universe Converter") != std::string::npos);
    }

    SECTION("--version behaves the same")
    {
        auto const result = run("--version");

        CHECK(result.exit_code == 0);
        CHECK(result.output.find("Wikifolio Investment Universe Converter") != std::string::npos);
    }

    SECTION("-Vo shows the version number only")
    {
        auto const result = run("-Vo");

        CHECK(result.exit_code == 0);

        // Only digits and dots, no name, no escape sequences.
        auto const line = result.output.substr(0, result.output.find('\n'));
        INFO("line: [" << line << "]");
        CHECK_FALSE(line.empty());
        for (char const _char : line) {
            CHECK(((_char >= '0' && _char <= '9') || _char == '.'));
        }
    }
}

TEST_CASE("wiuc prints the version as JSON for -Vj", "[e2e][cli]")
{
    auto const result = run("-Vj");

    CHECK(result.exit_code == 0);
    CHECK(result.output.find("\"version\"") != std::string::npos);
    CHECK(result.output.find("\"license\"") != std::string::npos);
    CHECK(result.output.find("MIT") != std::string::npos);
}

TEST_CASE("wiuc rejects an unknown option", "[e2e][cli]")
{
    auto const result = run("--definitely-not-an-option");

    CHECK(result.exit_code != 0);
    CHECK(result.output.find("Unknown option") != std::string::npos);
    CHECK(result.output.find("--definitely-not-an-option") != std::string::npos);
}

TEST_CASE("wiuc rejects -o without -c", "[e2e][cli]")
{
    // The option is only accepted together with the convert flag.
    auto const result = run("-o out");

    CHECK(result.exit_code != 0);
    CHECK(result.output.find("Usage:") != std::string::npos);
}

TEST_CASE("wiuc rejects too many arguments", "[e2e][cli]")
{
    // Only a single flag, or "-c" followed by "-o <dir>", is accepted.
    auto const result = run("-c -o out extra");

    CHECK(result.exit_code != 0);
    CHECK(result.output.find("Usage:") != std::string::npos);
}

int main()
{
    // Catch2's Session parses positional arguments as test-name filters, so the
    // paths are read from the environment and main() forwards an argument-free list.
    if (char const * const exe = std::getenv("WIUC_EXE")) {
        wiuc_exe() = exe;
    }
    if (char const * const dir = std::getenv("WIUC_FIXTURE_DIR")) {
        fixture_dir() = dir;
    }

    std::array<char const *, 1> const catch_argv{"error_paths_test"};
    return Catch::Session().run(static_cast<int>(catch_argv.size()), catch_argv.data());
}
