## wikifolio_universe_converter [![Build](https://github.com/jakoch/wikifolio_universe_converter/actions/workflows/build.yml/badge.svg?branch=main)](https://github.com/jakoch/wikifolio_universe_converter/actions/)

Konvertiert das [wikifolio.com Anlageuniversum](https://www.wikifolio.com/de/de/hilfe/tutorials-trader/handel-hinweise/anlageuniversum) von XLSX zu CSV und SQLite.

### CI Build Matrix

[![](http://github-actions.40ants.com/jakoch/wikifolio_universe_converter/matrix.svg)](https://github.com/jakoch/wikifolio_universe_converter/actions/)

|   CI Service     | Compiler    | Target Triplet           | Status |
|:---------------- |:----------- |:------------------------ |:------:|
| GitHub Actions   | VC16 VS2019 | x64-windows-static       |   🟢   |
|                  | VC16 VS2019 | x64-windows              |   👷🏼   |
|                  | VC17 VS2022 | x64-windows-static       |   🟢   |
|                  | VC17 VS2022 | x64-windows              |   👷🏼   |
|                  | GCC-9       | x64-linux                |   ⚪🟢   |
|                  | GCC-10      | x64-linux                |   🟢  |
|                  | GCC-11      | x64-linux                |   🟢  |
|                  | Clang-12    | x64-linux                |   🟢  | 
|                  | Clang-14    | x64-linux                |   🟢  |

Symbols used: ⚪ Build disabled. 🟢 Build succeeds. 🔴 Build fails. 👷🏼 TODO.

### Todo
- [x] Download der Excel-Datei [Investment_Universe.de.xlsx](https://wikifolio.blob.core.windows.net/prod-documents/Investment_Universe.de.xlsx) 
- [x] Konvertierung in eine CSV Datei
- [x] Konvertierung in eine SQLite Datenbank
