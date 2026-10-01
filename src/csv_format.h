// SPDX-FileCopyrightText: 2021-2026 Jens A. Koch
// SPDX-License-Identifier: MIT
// This file is part of https://github.com/jakoch/wikifolio_universe_converter.

#pragma once

#include <cstddef>
#include <cstdint>
#include <istream>
#include <string>
#include <vector>

// Replaces every occurrence of "search_for" in "search_in" with "replace_with".
// The search continues behind each replacement, so a replacement containing
// the searched string does not cause an endless loop.
std::string replace(std::string search_in, std::string const & search_for, std::string const & replace_with);

// Quotes a value as a CSV field, as described by RFC 4180.
// Embedded double quotes are doubled. Wikifolio uses commas inside values,
// for example in the Euronext price notation "EO -,01", so fields must be quoted.
// A line break in a value is kept as-is, because RFC 4180 allows it in a quoted field.
std::string quote_csv_field(std::string const & value);

// The reason a record could not be read, CsvError::None if there was no error.
enum class CsvError : std::uint8_t
{
    None,
    UnquotedField,     // a field does not start with a double quote
    UnterminatedField, // the input ends inside a quoted field
    MissingSeparator,  // two fields are not separated by a comma
    ReadError          // the underlying stream reported a failure
};

// Reads CSV records from a character stream, as described by RFC 4180.
// A record may span several lines, because RFC 4180 allows a line break inside a
// quoted field, so the input must not be split up with std::getline() beforehand.
class CsvReader
{
public:
    explicit CsvReader(std::istream& input);

    // Reads the next record and unquotes it, the inverse of quote_csv_field().
    // Returns false at the end of the input and on a malformed record,
    // error() tells the two apart.
    bool read(std::vector<std::string>& fields);

    // The reason the last read() failed, CsvError::None after a successful
    // read and at the end of the input.
    [[nodiscard]] CsvError error() const
    { return error_; }

    // The 1-based line the last record started on, for diagnostics.
    [[nodiscard]] std::size_t record_line() const
    { return record_line_; }

private:
    int get(); // reads one character and counts the line breaks, for record_line()

    std::istream* input_;
    CsvError error_          = CsvError::None;
    std::size_t line_        = 1;
    std::size_t record_line_ = 1;
};

// Splits one CSV record into its fields and unquotes them, the inverse of quote_csv_field().
// The string must hold exactly one record and nothing else.
// Returns false, if the record is not well formed.
bool parse_csv_record(std::string const & record, std::vector<std::string>& fields);
