// SPDX-FileCopyrightText: 2021-2026 Jens A. Koch
// SPDX-License-Identifier: MIT
// This file is part of https://github.com/jakoch/wikifolio_universe_converter.

#include <catch2/catch_test_macros.hpp>

#include <sstream> // std::istringstream
#include <string>
#include <vector>

#include "csv_format.h"

TEST_CASE("replace() replaces every occurrence", "[csv]")
{
    SECTION("single occurrence")
    { CHECK(replace("Anlageuniversum 1", "Anlageuniversum 1", "Anlageuniversum1") == "Anlageuniversum1"); }

    SECTION("multiple occurrences")
    { CHECK(replace("aXbXc", "X", "-") == "a-b-c"); }

    SECTION("search string is absent")
    { CHECK(replace("abc", "X", "-") == "abc"); }

    SECTION("empty search string returns the input unchanged")
    {
        // Guards the early return, which also prevents an endless loop.
        CHECK(replace("abc", "", "-") == "abc");
    }

    SECTION("a replacement containing the search string terminates")
    {
        // Before the search advanced past each replacement, this looped forever.
        CHECK(replace("a", "a", "aa") == "aa");
        CHECK(replace("aa", "a", "aaa") == "aaaaaa");
    }

    SECTION("replaces the Wikifolio column names")
    {
        CHECK(replace("Anlageuniversum (Gruppe) 1", "Anlageuniversum (Gruppe) 1", "Anlagegruppe1") == "Anlagegruppe1");
        CHECK(replace("Anlageuniversum 4", "Anlageuniversum 4", "Anlageuniversum4") == "Anlageuniversum4");
    }
}

TEST_CASE("quote_csv_field() follows RFC 4180", "[csv]")
{
    SECTION("a plain value is enclosed in double quotes")
    { CHECK(quote_csv_field("Siemens AG") == "\"Siemens AG\""); }

    SECTION("an empty value becomes two double quotes")
    { CHECK(quote_csv_field("") == "\"\""); }

    SECTION("embedded double quotes are doubled")
    { CHECK(quote_csv_field("say \"hi\"") == "\"say \"\"hi\"\"\""); }

    SECTION("a value containing a comma is quoted")
    {
        // The Euronext price notation "EO -,01" prompted this behaviour.
        CHECK(quote_csv_field("EO -,01") == "\"EO -,01\"");
    }

    SECTION("a value containing a single quote is unaffected")
    { CHECK(quote_csv_field("O'Brien") == "\"O'Brien\""); }
}

TEST_CASE("parse_csv_record() splits and unquotes", "[csv]")
{
    std::vector<std::string> fields;

    SECTION("a single field")
    {
        REQUIRE(parse_csv_record("\"abc\"", fields));
        REQUIRE(fields.size() == 1);
        CHECK(fields.at(0) == "abc");
    }

    SECTION("several fields")
    {
        REQUIRE(parse_csv_record("\"a\",\"b\",\"c\"", fields));
        REQUIRE(fields.size() == 3);
        CHECK(fields.at(0) == "a");
        CHECK(fields.at(1) == "b");
        CHECK(fields.at(2) == "c");
    }

    SECTION("empty fields are preserved")
    {
        // A record like this must not collapse, otherwise the column count shifts.
        REQUIRE(parse_csv_record("\"a\",\"\",\"c\"", fields));
        REQUIRE(fields.size() == 3);
        CHECK(fields.at(0) == "a");
        CHECK(fields.at(1).empty());
        CHECK(fields.at(2) == "c");
    }

    SECTION("a doubled double quote is a literal one")
    {
        REQUIRE(parse_csv_record("\"say \"\"hi\"\"\"", fields));
        REQUIRE(fields.size() == 1);
        CHECK(fields.at(0) == "say \"hi\"");
    }

    SECTION("a comma inside a quoted field is not a separator")
    {
        REQUIRE(parse_csv_record("\"EO -,01\",\"b\"", fields));
        REQUIRE(fields.size() == 2);
        CHECK(fields.at(0) == "EO -,01");
        CHECK(fields.at(1) == "b");
    }

    SECTION("the output is cleared on every call")
    {
        REQUIRE(parse_csv_record("\"a\",\"b\"", fields));
        REQUIRE(parse_csv_record("\"c\"", fields));
        REQUIRE(fields.size() == 1);
        CHECK(fields.at(0) == "c");
    }

    SECTION("an unquoted field is rejected")
    { CHECK_FALSE(parse_csv_record("abc", fields)); }

    SECTION("an unterminated field is rejected")
    { CHECK_FALSE(parse_csv_record("\"abc", fields)); }

    SECTION("a missing separator is rejected")
    { CHECK_FALSE(parse_csv_record("\"a\" \"b\"", fields)); }
}

TEST_CASE("parse_csv_record() inverts quote_csv_field()", "[csv]")
{
    std::vector<std::vector<std::string>> const values{
        {"Siemens AG"},
        {""},
        {"EO -,01"},
        {"say \"hi\""},
        {"O'Brien"},
        {"a", "b", "c"},
        {"a", "", "c"},
        {"back\\slash", "100%"},
        {"line1", "with \"quotes\" and , comma"},
        {"Multi\nline Name", "b"},
        {"\n", "\r\n"},
    };

    for (auto const & value : values) {
        std::string record;
        for (std::size_t index = 0; index < value.size(); ++index) {
            if (index != 0) {
                record += ',';
            }
            record += quote_csv_field(value.at(index));
        }

        std::vector<std::string> parsed;
        INFO("record: " << record);
        REQUIRE(parse_csv_record(record, parsed));
        CHECK(parsed == value);
    }
}

TEST_CASE("CsvReader reads every record of a stream", "[csv]")
{
    std::istringstream input("\"a\",\"b\"\n\"c\",\"d\"\n");
    CsvReader reader(input);

    std::vector<std::string> fields;

    REQUIRE(reader.read(fields));
    CHECK(fields == std::vector<std::string>{"a", "b"});
    CHECK(reader.error() == CsvError::None);

    REQUIRE(reader.read(fields));
    CHECK(fields == std::vector<std::string>{"c", "d"});

    // The end of the input is not an error, it only ends the loop.
    CHECK_FALSE(reader.read(fields));
    CHECK(reader.error() == CsvError::None);
}

TEST_CASE("CsvReader keeps a newline inside a quoted field", "[csv]")
{
    // This is the round trip which used to break: the writer emits the newline
    // verbatim, so a record spans two lines of the CSV.
    std::string const record = quote_csv_field("Multi\nline") + "," + quote_csv_field("b");
    std::istringstream input(record + "\n" + quote_csv_field("next") + "\n");

    CsvReader reader(input);
    std::vector<std::string> fields;

    REQUIRE(reader.read(fields));
    REQUIRE(fields.size() == 2);
    CHECK(fields.at(0) == "Multi\nline");
    CHECK(fields.at(1) == "b");

    REQUIRE(reader.read(fields));
    REQUIRE(fields.size() == 1);
    CHECK(fields.at(0) == "next");
}

TEST_CASE("CsvReader accepts the line terminators of RFC 4180", "[csv]")
{
    std::istringstream input("\"a\",\"b\"\r\n\"c\"\r\"d\"\n");
    CsvReader reader(input);

    std::vector<std::string> fields;

    REQUIRE(reader.read(fields));
    CHECK(fields == std::vector<std::string>{"a", "b"});

    REQUIRE(reader.read(fields));
    CHECK(fields == std::vector<std::string>{"c"});

    REQUIRE(reader.read(fields));
    CHECK(fields == std::vector<std::string>{"d"});
}

TEST_CASE("CsvReader reports why a record is malformed", "[csv]")
{
    std::vector<std::string> fields;

    SECTION("an unquoted field")
    {
        std::istringstream input("abc\n");
        CsvReader reader(input);
        CHECK_FALSE(reader.read(fields));
        CHECK(reader.error() == CsvError::UnquotedField);
    }

    SECTION("an unterminated field")
    {
        std::istringstream input("\"abc\n");
        CsvReader reader(input);
        CHECK_FALSE(reader.read(fields));
        CHECK(reader.error() == CsvError::UnterminatedField);
    }

    SECTION("a missing separator")
    {
        std::istringstream input("\"a\" \"b\"\n");
        CsvReader reader(input);
        CHECK_FALSE(reader.read(fields));
        CHECK(reader.error() == CsvError::MissingSeparator);
    }

    SECTION("an empty string holds no record")
    {
        std::istringstream input("");
        CsvReader reader(input);
        CHECK_FALSE(reader.read(fields));
        CHECK(reader.error() == CsvError::None);
    }
}

TEST_CASE("CsvReader reports the line a record starts on", "[csv]")
{
    // The record on line 2 contains a line break, so it ends on line 3. The
    // diagnostic must point at the start of the record, not at its end.
    std::istringstream input("\"header\"\n\"Multi\nline\"\n");
    CsvReader reader(input);

    std::vector<std::string> fields;

    REQUIRE(reader.read(fields));
    CHECK(reader.record_line() == 1);

    REQUIRE(reader.read(fields));
    CHECK(reader.record_line() == 2);
}
