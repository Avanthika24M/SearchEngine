# Known limitations and next enhancements
1. No independent LRU file-content cache (the inverted index itself is cached in RAM).
2. No recursive folder scanner, PDF/DOCX parsing, or TCP client-server mode.
3. Update is not fully transactional if indexing fails after a file is opened.
4. No certified MISRA C conformance, RBAC, or security audit.
5. No measured 80-90% code coverage or large-scale throughput benchmark.
6. Relative paths are resolved against the current working directory; use project root for samples.
7. Persisted index metadata points to source files and requires them to remain accessible.
8. Source file paths are not canonicalized; different spellings of the same file can create duplicate entries.
