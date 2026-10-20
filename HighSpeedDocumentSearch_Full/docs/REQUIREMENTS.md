# Requirement traceability
| Requirement | Implementation | Test evidence |
|---|---|---|
| Document/source indexing | src/index.c hs_add | test_search.c |
| Keyword search | hs_search | test_search.c |
| Custom indexing | hash table + inverted posting lists | search tests |
| Add/update/delete | hs_add, hs_update, hs_delete | test_search.c |
| Ranked results | TF-IDF + qsort | ranking assertions |
| Metadata | Document struct, menu list | metadata assertions |
| Multiple file types | hs_supported_file | extension tests |
| Statistics/history | counters, history.dat | manual CLI check |
| Inverted index | Term/Posting | search tests |
| File I/O optimization | index retained in RAM; no query rereads | architectural property; not benchmarked |
| Hash table | FNV-1a | search tests |
| Memory cleanup | hs_destroy | sanitizer recommended |
| Modularization | main/index/storage/test | Makefile |
| Large dataset performance | candidate-only postings search | benchmark not supplied |

The project meets core local POC functionality but not every production-grade or performance-validation requirement.
