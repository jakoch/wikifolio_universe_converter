#!/usr/bin/env python3

# SPDX-FileCopyrightText: 2021-2026 Jens A. Koch
# SPDX-License-Identifier: MIT
# This file is part of https://github.com/jakoch/wikifolio_universe_converter.

"""Generates the XLSX fixtures which are not the plain investment universe.

Most files are deliberately malformed in exactly one way, so that a test can
assert wiuc reports the matching error and exits non-zero instead of crashing.
newline_in_value.xlsx is the exception: it is a valid file, kept here because it
covers a corner case of the CSV reader.

Run it from the repository root to regenerate the committed fixtures:

    python3 tests/fixtures/make_error_fixtures.py
"""

import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import make_fixture as base  # noqa: E402

FIXTURE_DIR = os.path.dirname(os.path.abspath(__file__))

# The 13 columns wiuc expects, but only the first 10 are present. This makes the
# generated CSV header have the wrong column count.
SHORT_HEADERS = base.HEADERS[:10]

# A value containing a newline. RFC 4180 allows a line break inside a quoted
# field, so the generated CSV record spans two lines. This is a valid input, it
# is generated here because it exercises the character stream CSV reader.
NEWLINE_ROWS = [
    [
        "DE0007236101",
        "723610",
        "Aktie",
        "Multi\nline Name",
        "Siemens",
        "Technologie",
        "Europa",
        "Industrie",
        "Large Cap",
        "Nachhaltig",
        "Ja",
        "Kern",
        "Standard",
    ],
]


def write_workbook(target, headers, rows):
    shared_strings = []
    sheet_xml = base.build_sheet_xml(shared_strings) if headers is base.HEADERS and rows is base.ROWS else None

    if sheet_xml is None:
        sheet_xml = build_custom_sheet(headers, rows, shared_strings)

    shared_strings_xml = base.build_shared_strings_xml(shared_strings)

    with __import__("zipfile").ZipFile(target, "w", __import__("zipfile").ZIP_DEFLATED) as archive:
        archive.writestr("[Content_Types].xml", base.CONTENT_TYPES)
        archive.writestr("_rels/.rels", base.ROOT_RELS)
        archive.writestr("xl/workbook.xml", base.WORKBOOK)
        archive.writestr("xl/_rels/workbook.xml.rels", base.WORKBOOK_RELS)
        archive.writestr("xl/worksheets/sheet1.xml", sheet_xml)
        archive.writestr("xl/sharedStrings.xml", shared_strings_xml)


def build_custom_sheet(headers, rows, shared_strings):
    """Same as make_fixture.build_sheet_xml, but with caller supplied headers/rows."""
    parts = [
        '<?xml version="1.0" encoding="UTF-8"?>',
        '<worksheet xmlns="http://schemas.openxmlformats.org/spreadsheetml/2006/main">',
        "<sheetData>",
    ]

    for row_index, row in enumerate([headers] + rows, start=1):
        parts.append('<row r="{}">'.format(row_index))
        for column_index, value in enumerate(row):
            reference = "{}{}".format(base.column_letter(column_index), row_index)
            if value not in shared_strings:
                shared_strings.append(value)
            parts.append('<c r="{}" t="s"><v>{}</v></c>'.format(reference, shared_strings.index(value)))
        parts.append("</row>")

    parts.append("</sheetData>")
    parts.append("</worksheet>")
    return "".join(parts)


def write_corrupt(target):
    """A file that is not a zip at all, so xlsxioread_open() returns null.

    This reproduces the case of an interrupted download leaving a partial file.
    """
    with open(target, "w", encoding="utf-8") as handle:
        handle.write("this is not a zip archive, it is a truncated download\n")


def write_too_few_columns(target):
    write_workbook(target, SHORT_HEADERS, [[str(index) for index in range(len(SHORT_HEADERS))]])


def write_newline_in_value(target):
    write_workbook(target, base.HEADERS, NEWLINE_ROWS)


def main():
    write_corrupt(os.path.join(FIXTURE_DIR, "corrupt.xlsx"))
    print("wrote corrupt.xlsx (not a zip)")

    write_too_few_columns(os.path.join(FIXTURE_DIR, "too_few_columns.xlsx"))
    print("wrote too_few_columns.xlsx ({} columns, wiuc expects {})".format(len(SHORT_HEADERS), len(base.HEADERS)))

    write_newline_in_value(os.path.join(FIXTURE_DIR, "newline_in_value.xlsx"))
    print("wrote newline_in_value.xlsx (a value contains a newline)")


if __name__ == "__main__":
    main()
