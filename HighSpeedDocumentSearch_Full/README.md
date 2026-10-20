# High-Speed Document Search Framework (C11)

A complete **local CLI proof of concept** implementing the requested search features and a broader automated test suite. This is **not** a network client-server product, certified MISRA-C implementation, or security-audited production search engine.

## Requirements
- GCC (C11), Make (MSYS2 UCRT64 `mingw32-make` on Windows), math library.
- Windows: MSYS2 UCRT64 terminal inside Antigravity/VS Code. Linux: standard terminal.

## Build and run
```sh
mingw32-make
./build/search_app.exe
mingw32-make test
```
Linux: `make`, `./build/search_app`, `make test`.

**Run from the project root**, where `Makefile` and `samples/` exist. If the sample is already indexed from a previous run, menu option 1 reports that it is already indexed (duplicate code -2 in the API); use menu option 5 to list indexed files.

## Demo
1. Select `1` and enter `samples/example.txt`.
2. Select `2` and enter `index` (or `search`).
3. Select `5` for metadata; `6` for statistics; `8` for timestamped query history.
4. Select `3` to reindex a changed file by ID, `4` to remove an indexed document.
5. Select `7` to save; data is also saved on exit.

## Features
- Text and source code indexing for `.txt .md .c .h .cpp .hpp .cc .py .java .js .ts .json .xml .html .css .csv .log .sql .sh .go .rs` (case-insensitive extensions).
- In-memory FNV-1a hash table (8191 buckets), inverted posting lists, normalized keyword tokenization, OR-style multi-term queries, candidate-only TF-IDF scoring, descending ranking.
- Add, update, delete; ID/path/size/word-count metadata; statistics and query history; text-file persistence and rebuild on launch.
- The index itself acts as an in-memory search cache: queries don't reread document contents. **No separate LRU file cache is implemented.**
- File-size limit: 8 MiB per file; keyword length limited to 95 bytes; query limited to 32 unique terms.

## Tests and quality
- `mingw32-make test`: automated assertions for indexing, duplicate prevention, extension validation, case-insensitive queries, multiple docs, ranking order, empty/invalid/unknown queries, update/delete, and save/load.
- `mingw32-make coverage`: GCC/gcov instrumentation (coverage target depends on platform/toolchain). **No minimum 80-90% coverage is claimed.**
- `mingw32-make analyze`: requires `cppcheck` installed.
- `mingw32-make sanitize`: GCC sanitizers where supported (may require Linux).

## Project structure
```
HighSpeedDocumentSearch_Full/
  Makefile
  README.md
  include/search.h
  src/main.c
  src/index.c
  src/storage.c
  test/test_search.c
  samples/example.txt
  docs/REQUIREMENTS.md
  docs/LIMITATIONS.md
  build/  (created by Make)
```

## Important limitations
- Files are indexed individually; recursive folder crawler is not implemented.
- Text/source files only; binary formats (PDF/DOCX) are not parsed.
- Updating after a file-read failure or allocation failure can lose that document's previous postings; full transactional rollback is future work.
- `documents.dat` stores paths and IDs and reloads by rereading source files; source files must still exist at the saved paths.
- This is a single-process local CLI, not a TCP service, and does not include login/RBAC.
- Search candidate collection currently linearly checks already-matched results; it is more efficient than scanning all documents but can be further optimized with a doc-ID accumulator map.
