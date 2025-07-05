# Changelog

All changes to the project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).
The project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).
The date format in this file is `YYYY-MM-DD`.
The upcomming release version is named `vNext` and links to the changes between latest version tag and git HEAD.

## [vNext] - unreleased

- "It was a bright day in April, and the clocks were striking thirteen." - 1984

## [1.0.9] - 2025-07-05

### Bugfix

- Wikifolio introduced 2 new database columns "Anlageuniversum (Gruppe) 3" and "Anlagegruppe 3"

## [1.0.8] - 2025-02-23

### Added

- compilation on ubuntu-24.04-arm, disabled because runner is never available

## [1.0.7] - 2024-04-28

### Changed

- updated the download URL of the Investment_Universe
- small fixes to cpplint and format.sh
- updated devcontainer version
- added missing includes to main.cpp to satisfiy cpplint

## [1.0.6] - 2024-04-28

### Bugfix

- Wikifolio introduced 2 new database columns "Anlageuniversum (Gruppe) 2" and "Anlagegruppe 2" to reflect that Kontron is listed in TecDax and SDax

## [1.0.5] - 2023-03-17

### Added

- added specify an output folder for content generation (wiuc -c -o data), close #3
- CLI color output

## [1.0.4] - 2023-03-14

### Added

- CLI options (-h,-V,-Vj,-Vo,-c)
  - help, version, version-only, version-json, convert

### Changed

- Project Cleanup
  - added EditorConfig and clang-format config
  - applied clang-format to source
- Simplified the Timer class, its no longer a template taking a chrono resolution
- reworked app_version
- added automatic Github Releases on tag

## [1.0.3] - 2022-05-21

## [1.0.2] - 2022-04-16

## [1.0.1] - 2022-01-27

## [0.2.0] - 2022-01-24

## [0.1.0] - 2021-09-03

<!-- Section for Reference Links -->

[vNext]: https://github.com/jakoch/wikifolio_universe_converter/compare/v1.0.9...HEAD
[1.0.9]: https://github.com/jakoch/wikifolio_universe_converter/compare/v1.0.8...v1.0.9
[1.0.8]: https://github.com/jakoch/wikifolio_universe_converter/compare/v1.0.7...v1.0.8
[1.0.7]: https://github.com/jakoch/wikifolio_universe_converter/compare/v1.0.6...v1.0.7
[1.0.6]: https://github.com/jakoch/wikifolio_universe_converter/compare/v1.0.5...v1.0.6
[1.0.5]: https://github.com/jakoch/wikifolio_universe_converter/compare/v1.0.4...v1.0.5
[1.0.4]: https://github.com/jakoch/wikifolio_universe_converter/compare/v1.0.3...v1.0.4
[1.0.3]: https://github.com/jakoch/wikifolio_universe_converter/compare/v1.0.2...v1.0.3
[1.0.2]: https://github.com/jakoch/wikifolio_universe_converter/compare/v1.0.1...v1.0.2
[1.0.1]: https://github.com/jakoch/wikifolio_universe_converter/compare/v0.2.0...v1.0.1
[0.2.0]: https://github.com/jakoch/wikifolio_universe_converter/compare/v0.1.0...v0.2.0
[0.1.0]: https://github.com/jakoch/wikifolio_universe_converter/compare/7223ede99...v0.1.0
