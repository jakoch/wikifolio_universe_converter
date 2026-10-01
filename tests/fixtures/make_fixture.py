#!/usr/bin/env python3

# SPDX-FileCopyrightText: 2021-2026 Jens A. Koch
# SPDX-License-Identifier: MIT
# This file is part of https://github.com/jakoch/wikifolio_universe_converter.

"""Generates the minimal XLSX fixture used by the end-to-end tests.

Wikifolio publishes the investment universe as a real spreadsheet, but the tests
must not depend on the network. This script writes a small, valid XLSX with the
same 13 columns, so the converter takes its normal code path.

Run it from the repository root to regenerate the committed fixture:

    python3 tests/fixtures/make_fixture.py
"""

import os
import zipfile

# The column order must match the CREATE TABLE statement in src/main.cpp and the
# column list of the prepared INSERT in csv_to_sqlite(). The "(Gruppe) N" names
# are the ones Wikifolio actually publishes; rename_header_columns() rewrites them.
HEADERS = [
    "ISIN",
    "WKN",
    "SecurityType",
    "Bezeichnung",
    "Emittent",
    "Anlageuniversum (Gruppe) 1",
    "Anlageuniversum 1",
    "Anlageuniversum (Gruppe) 2",
    "Anlageuniversum 2",
    "Anlageuniversum (Gruppe) 3",
    "Anlageuniversum 3",
    "Anlageuniversum (Gruppe) 4",
    "Anlageuniversum 4",
]

# Values chosen to exercise the awkward cases on purpose:
#   - a comma, which is why fields are quoted (Euronext notation "EO -,01")
#   - a double quote, which must be doubled per RFC 4180
#   - an apostrophe, which must survive into SQLite
ROWS = [
    [
        "DE0007236101",
        "723610",
        "Aktie",
        "Siemens AG",
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
    [
        "DE0007100000",
        "710000",
        "Fonds",
        "Preis EUR -,01",
        "Anlagegesellschaft",
        "Renten",
        "Europa",
        "Anleihen",
        "Core",
        "Nachhaltig",
        "Nein",
        "Satellit",
        "Standard",
    ],
    [
        "US0378331005",
        "AAPL",
        "Aktie",
        'Sagt "Hallo"',
        "Apple Inc",
        "Technologie",
        "USA",
        "Industrie",
        "Large Cap",
        "Nachhaltig",
        "Ja",
        "Kern",
        "Standard",
    ],
    [
        "LU0000000000",
        "OBRIEN",
        "Fonds",
        "O'Brien Holdings",
        "O'Brien",
        "Aktien",
        "Global",
        "Aktien",
        "Satellit",
        "Nein",
        "Ja",
        "Kern",
        "Standard",
    ],
]


def xml_escape(text):
    return text.replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;")


def column_letter(index):
    """0 -> A, 25 -> Z, 26 -> AA."""
    letters = ""
    index += 1
    while index > 0:
        index, remainder = divmod(index - 1, 26)
        letters = chr(ord("A") + remainder) + letters
    return letters


def build_sheet_xml(shared_strings):
    """Builds worksheet XML, registering every value as a shared string."""
    parts = [
        '<?xml version="1.0" encoding="UTF-8"?>',
        '<worksheet xmlns="http://schemas.openxmlformats.org/spreadsheetml/2006/main">',
        "<sheetData>",
    ]

    for row_index, row in enumerate([HEADERS] + ROWS, start=1):
        parts.append('<row r="{}">'.format(row_index))
        for column_index, value in enumerate(row):
            reference = "{}{}".format(column_letter(column_index), row_index)
            if value not in shared_strings:
                shared_strings.append(value)
            shared_strings_index = shared_strings.index(value)
            parts.append(
                '<c r="{}" t="s"><v>{}</v></c>'.format(reference, shared_strings_index)
            )
        parts.append("</row>")

    parts.append("</sheetData>")
    parts.append("</worksheet>")
    return "".join(parts)


def build_shared_strings_xml(shared_strings):
    parts = [
        '<?xml version="1.0" encoding="UTF-8"?>',
        (
            '<sst xmlns="http://schemas.openxmlformats.org/spreadsheetml/2006/main" '
            'count="{0}" uniqueCount="{0}">'.format(len(shared_strings))
        ),
    ]
    for value in shared_strings:
        parts.append("<si><t>{}</t></si>".format(xml_escape(value)))
    parts.append("</sst>")
    return "".join(parts)


CONTENT_TYPES = (
    '<?xml version="1.0" encoding="UTF-8"?>'
    '<Types xmlns="http://schemas.openxmlformats.org/package/2006/content-types">'
    '<Default Extension="rels" ContentType="application/vnd.openxmlformats-package.relationships+xml"/>'
    '<Default Extension="xml" ContentType="application/xml"/>'
    '<Override PartName="/xl/workbook.xml" ContentType="application/vnd.openxmlformats-officedocument.spreadsheetml.sheet.main+xml"/>'
    '<Override PartName="/xl/worksheets/sheet1.xml" ContentType="application/vnd.openxmlformats-officedocument.spreadsheetml.worksheet+xml"/>'
    '<Override PartName="/xl/sharedStrings.xml" ContentType="application/vnd.openxmlformats-officedocument.spreadsheetml.sharedStrings+xml"/>'
    "</Types>"
)

ROOT_RELS = (
    '<?xml version="1.0" encoding="UTF-8"?>'
    '<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">'
    '<Relationship Id="rId1" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument" Target="xl/workbook.xml"/>'
    "</Relationships>"
)

WORKBOOK = (
    '<?xml version="1.0" encoding="UTF-8"?>'
    '<workbook xmlns="http://schemas.openxmlformats.org/spreadsheetml/2006/main" '
    'xmlns:r="http://schemas.openxmlformats.org/officeDocument/2006/relationships">'
    '<sheets><sheet name="Investment_Universe" sheetId="1" r:id="rId1"/></sheets>'
    "</workbook>"
)

WORKBOOK_RELS = (
    '<?xml version="1.0" encoding="UTF-8"?>'
    '<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">'
    '<Relationship Id="rId1" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/worksheet" Target="worksheets/sheet1.xml"/>'
    '<Relationship Id="rId2" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/sharedStrings" Target="sharedStrings.xml"/>'
    "</Relationships>"
)


def main():
    target_dir = os.path.join(os.path.dirname(os.path.abspath(__file__)))
    target = os.path.join(target_dir, "Investment_Universe.de.xlsx")

    shared_strings = []
    sheet_xml = build_sheet_xml(shared_strings)
    shared_strings_xml = build_shared_strings_xml(shared_strings)

    with zipfile.ZipFile(target, "w", zipfile.ZIP_DEFLATED) as archive:
        archive.writestr("[Content_Types].xml", CONTENT_TYPES)
        archive.writestr("_rels/.rels", ROOT_RELS)
        archive.writestr("xl/workbook.xml", WORKBOOK)
        archive.writestr("xl/_rels/workbook.xml.rels", WORKBOOK_RELS)
        archive.writestr("xl/worksheets/sheet1.xml", sheet_xml)
        archive.writestr("xl/sharedStrings.xml", shared_strings_xml)

    print("wrote {} ({} columns, {} data rows)".format(target, len(HEADERS), len(ROWS)))


if __name__ == "__main__":
    main()
