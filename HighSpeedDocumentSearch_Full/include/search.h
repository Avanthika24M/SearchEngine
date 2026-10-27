#ifndef SEARCH_H
#define SEARCH_H
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#define HS_PATH_MAX 1024
#define HS_WORD_MAX 96
#define HS_BUCKETS 8191
#define HS_MAX_FILE (8u*1024u*1024u)
typedef struct { uint32_t id; char path[HS_PATH_MAX]; uint64_t size; uint32_t words; int active; } Document;
typedef struct { uint32_t doc_id; uint32_t frequency; } Posting;
typedef struct Term { char *word; Posting *postings; size_t count; size_t capacity; struct Term *next; } Term;
typedef struct { Document *docs; size_t count; size_t capacity; Term **buckets; size_t unique_terms; uint32_t next_id; uint64_t searches; double total_ms; uint64_t candidate_visits; } SearchIndex;
typedef struct { uint32_t doc_id; double score; } SearchResult;
int hs_init(SearchIndex *idx);
void hs_destroy(SearchIndex *idx);
int hs_add(SearchIndex *idx, const char *path);
int hs_update(SearchIndex *idx, uint32_t id);
int hs_delete(SearchIndex *idx, uint32_t id);
size_t hs_search(SearchIndex *idx, const char *query, SearchResult **out);
const Document *hs_document(const SearchIndex *idx, uint32_t id);
int hs_save(const SearchIndex *idx, const char *directory);
int hs_load(SearchIndex *idx, const char *directory);
void hs_print_results(const SearchIndex *idx, const SearchResult *results, size_t n, FILE *out);
int hs_supported_file(const char *path);
size_t hs_active_documents(const SearchIndex *idx);
#endif
