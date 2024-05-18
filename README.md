## wikifolio_universe_converter [![Build](https://github.com/jakoch/wikifolio_universe_converter/actions/workflows/build.yml/badge.svg?branch=main)](https://github.com/jakoch/wikifolio_universe_converter/actions/)

Konvertiert das [wikifolio.com Anlageuniversum](https://help.wikifolio.com/article/102-welche-werte-kann-ich-im-wikifolio-handeln) von XLSX zu CSV und SQLite.

### CI Build Matrix

[![](http://github-actions.40ants.com/jakoch/wikifolio_universe_converter/matrix.svg)](https://github.com/jakoch/wikifolio_universe_converter/actions/)

|   CI Service        | OS Image     | Compiler    | Target Triplet      | Build Type     | Status |
|:------------------- |:------------ |:----------- |:------------------- |:--------------:|:------:|
|  GitHub Actions     | Windows 2019 | VC16 VS2019 | x64-windows         | Release        |   👷🏼   |
|                     | Windows 2019 | VC16 VS2019 | x64-windows-static  | Release        |   🟢   |
|                     | Windows 2022 | VC17 VS2022 | x64-windows         | Release        |   👷🏼   |
|                     | Windows 2022 | VC17 VS2022 | x64-windows-static  | Release        |   🟢   |
|                     | Windows 2022 | VC17 VS2022 | x64-windows-static  | RelWithDebInfo |   🟢   |
|                     | Ubuntu 20.04 | GCC-9       | x64-linux           | Release        |   🟢   |
|                     |              | to          | x64-linux           | Release        |   ⚪🟢|
|                     | Ubuntu 22.04 | GCC-13      | x64-linux           | Release        |   🟢   |
|                     | Ubuntu 24.04 | GCC-14      | x64-linux           | Release        |   🟢   |
|                     | Ubuntu 20.04 | Clang-10    | x64-linux           | Release        |   🟢   |
|                     |              | to          | x64-linux           | Release        |   ⚪🟢|
|                     | Ubuntu 22.04 | Clang-15    | x64-linux           | Release        |   🟢   |
| [jakoch/cpp-devbox] | Debian 12    | Clang-17    | x64-linux           | Release        |   🟢   |
|                     | Ubuntu 24.04 | Clang-18    | x64-linux           | Release        |   🟢   |

Symbols used: ⚪ Build disabled. 🟢 Build succeeds. 🔴 Build fails. 👷🏼 TODO.

### Todo
- [x] Download der Excel-Datei [Investment_Universe.de.xlsx](https://wikifolio.blob.core.windows.net/prod-documents/Investment_Universe.de.xlsx)
- [x] Konvertierung in eine CSV Datei
- [x] Konvertierung in eine SQLite Datenbank


[jakoch/cpp-devbox]: https://github.com/jakoch/cpp-devbox
