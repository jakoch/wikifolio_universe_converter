// SPDX-FileCopyrightText: 2021-2026 Jens A. Koch
// SPDX-License-Identifier: MIT
// This file is part of https://github.com/jakoch/wikifolio_universe_converter.

#include "csv_format.h"

#include <sstream> // std::istringstream
#include <utility> // std::move

std::string replace(std::string search_in, std::string const & search_for, std::string const & replace_with)
{
    if (search_for.empty()) {
        return search_in;
    }

    std::size_t pos = 0;

    while ((pos = search_in.find(search_for, pos)) != std::string::npos) {
        search_in.replace(pos, search_for.size(), replace_with);
        // Continue the search behind the replacement, otherwise a replacement
        // which contains the searched string would be matched again (endless loop).
        pos += replace_with.size();
    }

    return search_in;
}

std::string quote_csv_field(std::string const & value)
{
    std::string quoted;
    quoted.reserve(value.size() + 2);

    quoted.push_back('"');

    for (char const _char : value) {
        if (_char == '"') {
            quoted.push_back('"');
        }
        quoted.push_back(_char);
    }

    quoted.push_back('"');

    return quoted;
}

namespace
{

    constexpr int eof = std::char_traits<char>::eof();

} // namespace

CsvReader::CsvReader(std::istream& input) : input_(&input) { }

int CsvReader::get()
{
    int const character = input_->get();

    if (character == '\n') {
        ++line_;
    }

    return character;
}

bool CsvReader::read(std::vector<std::string>& fields)
{
    fields.clear();
    error_ = CsvError::None;

    int character = get();

    // Skip the line break which ended the previous record, an empty line included.
    while (character == '\n' || character == '\r') {
        character = get();
    }

    if (character == eof) {
        error_ = input_->bad() ? CsvError::ReadError : CsvError::None;
        return false; // the end of the input is not an error
    }

    record_line_ = line_;

    while (true) {

        if (character != '"') {
            error_ = CsvError::UnquotedField; // a field must start with a double quote
            return false;
        }

        std::string field;
        character = get();

        while (true) {

            if (character == eof) {
                error_ = CsvError::UnterminatedField; // the field is not terminated
                return false;
            }

            if (character != '"') {
                // A line break inside the quotes is part of the value.
                field.push_back(static_cast<char>(character));
                character = get();
                continue;
            }

            character = get();

            if (character == '"') {
                field.push_back('"'); // a doubled double quote is a literal one
                character = get();
                continue;
            }

            break; // the field is closed
        }

        fields.push_back(std::move(field));

        if (character == eof) {
            error_ = input_->bad() ? CsvError::ReadError : CsvError::None;
            return true; // the last record need not be terminated
        }

        if (character == '\n' || character == '\r') {
            return true; // a line break terminates the record
        }

        if (character != ',') {
            error_ = CsvError::MissingSeparator; // fields must be separated by a comma
            return false;
        }

        character = get();
    }
}

bool parse_csv_record(std::string const & record, std::vector<std::string>& fields)
{
    std::istringstream input(record);
    CsvReader reader(input);

    if (!reader.read(fields)) {
        return false;
    }

    // The string must hold exactly one record, a second one is malformed input.
    input.get();

    return input.eof();
}
