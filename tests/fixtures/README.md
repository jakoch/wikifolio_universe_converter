# Test Fixtures

## Investment_Universe.de.xlsx

A minimal, valid XLSX that mirrors the structure of the file Wikifolio publishes at
<https://wikifoliostorage.blob.core.windows.net/prod-documents/Investment_Universe.de.xlsx>,
so the converter runs its normal code path in tests without network access.

It is a **generated** file. Do not edit it by hand. Regenerate it with:

```bash
python3 tests/fixtures/make_fixture.py
```

The generator is `make_fixture.py`, which writes the sheet XML, the shared-string
table and the OPC package parts by hand. It uses only the Python standard library,
so it runs anywhere the tests do.

### Why it is committed

A binary fixture cannot be reviewed in a diff, and a test that generates its own
input on every run risks the test passing because the *generator* changed rather
than because the code is correct. Committing the file pins the input: if a
regression appears, the input is known to be unchanged.

Regenerate and commit together, so the two never drift:

```bash
python3 tests/fixtures/make_fixture.py
git add tests/fixtures/Investment_Universe.de.xlsx
```

### Contents

13 columns, matching the `CREATE TABLE` statement in `src/main.cpp`:

`ISIN`, `WKN`, `SecurityType`, `Bezeichnung`, `Emittent`, then
`Anlageuniversum (Gruppe) 1` … `Anlageuniversum (Gruppe) 4` and
`Anlageuniversum 1` … `Anlageuniversum 4`.

The `(Gruppe) N` spellings are what Wikifolio actually publishes, so
`rename_header_columns()` has something real to rewrite.

Four data rows, each chosen to exercise an awkward case:

| ISIN | Exercises |
|---|---|
| `DE0007236101` | the plain, boring case |
| `DE0007100000` | a comma in a value (`Preis EUR -,01`, Euronext notation) |
| `US0378331005` | an embedded double quote (`Sagt "Hallo"`) |
| `LU0000000000` | an apostrophe in two fields (`O'Brien`) |

The comma is why every field is quoted, the double quote is what RFC 4180 doubles,
and the apostrophe is what the SQL escaping and prepared-statement binding must
preserve. If a change to the CSV or SQLite layer breaks quoting or binding, this
fixture catches it.

To see the expected output, run the converter against a copy:

```bash
cp tests/fixtures/Investment_Universe.de.xlsx /tmp/fixture-check/
cd /tmp/fixture-check && wiuc -c
```
