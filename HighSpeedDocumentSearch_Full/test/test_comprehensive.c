#define _POSIX_C_SOURCE 200809L
#include "search.h"
#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static char root[HS_PATH_MAX];
static void path(char *out, size_t n, const char *name) {
    int k = snprintf(out, n, "%s/%s", root, name);
    assert(k > 0 && (size_t)k < n);
}
static void write_text(const char *p, const char *s) {
    FILE *f = fopen(p, "wb"); assert(f);
    assert(fputs(s, f) >= 0); assert(fclose(f) == 0);
}
static void query_count(SearchIndex *idx, const char *q, size_t want) {
    SearchResult *r = NULL;
    size_t n = hs_search(idx, q, &r);
    assert(n == want); free(r);
}
static void test_api_and_search(void) {
    SearchIndex idx = {0};
    assert(hs_init(NULL) == -1);
    hs_destroy(NULL);
    assert(hs_active_documents(NULL) == 0);
    assert(hs_document(NULL, 1) == NULL);
    assert(hs_supported_file(NULL) == 0);
    assert(hs_supported_file("README") == 0);
    assert(hs_supported_file("file.PY") == 1);
    assert(hs_supported_file("file.TXT") == 1);
    assert(hs_supported_file("file.exe") == 0);
    assert(hs_supported_file("file.txt.exe") == 0);
    assert(hs_init(&idx) == 0);
    assert(hs_active_documents(&idx) == 0);
    assert(hs_add(NULL, "x.txt") == -1);
    assert(hs_add(&idx, NULL) == -1);
    assert(hs_add(&idx, "") == -1);
    assert(hs_add(&idx, "missing.txt") == -1);
    assert(hs_add(&idx, "invalid.png") == -1);
    assert(hs_update(NULL, 1) == -1);
    assert(hs_delete(NULL, 1) == -1);
    assert(hs_update(&idx, 999) == -1);
    assert(hs_delete(&idx, 999) == -1);
    assert(hs_document(&idx, 999) == NULL);
    SearchResult *r = (SearchResult *)1;
    assert(hs_search(NULL, "abc", &r) == 0 && r == NULL);
    assert(hs_search(&idx, NULL, &r) == 0 && r == NULL);
    assert(hs_search(&idx, "abc", NULL) == 0);
    query_count(&idx, "", 0);
    query_count(&idx, "???", 0);
    FILE *capture = tmpfile(); assert(capture);
    hs_print_results(&idx, NULL, 0, capture);
    rewind(capture);
    char line[2048]; assert(fgets(line, sizeof(line), capture));
    assert(strstr(line, "No matching documents.") != NULL);
    assert(fclose(capture) == 0);
    hs_print_results(&idx, NULL, 0, NULL);

    char a[HS_PATH_MAX], b[HS_PATH_MAX], c[HS_PATH_MAX];
    path(a, sizeof(a), "alpha.txt"); path(b, sizeof(b), "beta.c"); path(c, sizeof(c), "gamma.md");
    write_text(a, "Search search SEARCH index engine alpha\n");
    write_text(b, "search engine beta\n");
    write_text(c, "gamma unique token\n");
    int ia = hs_add(&idx, a), ib = hs_add(&idx, b), ic = hs_add(&idx, c);
    assert(ia > 0 && ib > 0 && ic > 0);
    assert(hs_add(&idx, a) == -2);
    assert(hs_active_documents(&idx) == 3);
    assert(idx.unique_terms > 0);
    query_count(&idx, "SEARCH", 2);
    query_count(&idx, "search search", 2);
    query_count(&idx, "search alpha", 2);
    query_count(&idx, "absentword", 0);
    assert(hs_search(&idx, "search", &r) == 2);
    assert(r[0].doc_id == (uint32_t)ia);
    assert(r[0].score >= r[1].score);
    capture = tmpfile(); assert(capture);
    hs_print_results(&idx, r, 2, capture);
    rewind(capture);
    assert(fgets(line, sizeof(line), capture));
    assert(strstr(line, "alpha.txt") != NULL);
    assert(fclose(capture) == 0);
    free(r);
    assert(idx.searches > 0 && idx.candidate_visits > 0);
    assert(hs_document(&idx, (uint32_t)ia)->words > 0);

    write_text(b, "freshkeyword freshkeyword\n");
    assert(hs_update(&idx, (uint32_t)ib) == 0);
    query_count(&idx, "freshkeyword", 1);
    query_count(&idx, "search", 1);
    assert(hs_update(&idx, 0) == -1);
    assert(hs_delete(&idx, (uint32_t)ia) == 0);
    assert(hs_delete(&idx, (uint32_t)ia) == -1);
    assert(hs_document(&idx, (uint32_t)ia) == NULL);
    query_count(&idx, "search", 0);
    assert(hs_active_documents(&idx) == 2);
    assert(remove(b) == 0);
    assert(hs_update(&idx, (uint32_t)ib) == -1);
    query_count(&idx, "freshkeyword", 1); /* failed update preserves index */
    assert(hs_delete(&idx, (uint32_t)ib) == 0);
    assert(hs_delete(&idx, (uint32_t)ic) == 0);
    assert(hs_active_documents(&idx) == 0);
    hs_destroy(&idx);
    hs_destroy(&idx);
    assert(remove(a) == 0 && remove(c) == 0);
}
static void test_limits_and_special_files(void) {
    SearchIndex idx = {0}; assert(hs_init(&idx) == 0);
    char longpath[HS_PATH_MAX + 10];
    memset(longpath, 'x', sizeof(longpath));
    memcpy(longpath + sizeof(longpath) - 5, ".txt", 5);
    assert(hs_add(&idx, longpath) == -1);
    char p[HS_PATH_MAX]; path(p, sizeof(p), "empty.txt");
    write_text(p, "");
    int id = hs_add(&idx, p); assert(id > 0);
    assert(hs_document(&idx, (uint32_t)id)->words == 0);
    query_count(&idx, "anything", 0);
    assert(remove(p) == 0);
    path(p, sizeof(p), "no_file.txt");
    assert(hs_add(&idx, p) == -1);
    idx.next_id = UINT32_MAX;
    assert(hs_add(&idx, p) == -1);
    idx.next_id = (uint32_t)INT_MAX + 1u;
    assert(hs_add(&idx, p) == -1);
    hs_destroy(&idx);
}
static void load_fixture(const char *content, int expected) {
    char p[HS_PATH_MAX]; path(p, sizeof(p), "documents.dat");
    write_text(p, content);
    SearchIndex idx = {0}; assert(hs_init(&idx) == 0);
    assert(hs_load(&idx, root) == expected);
    hs_destroy(&idx);
    assert(remove(p) == 0);
}
static void test_storage(void) {
    SearchIndex idx = {0}; assert(hs_init(&idx) == 0);
    assert(hs_save(NULL, root) == -1);
    assert(hs_save(&idx, NULL) == -1);
    assert(hs_load(NULL, root) == -1);
    assert(hs_load(&idx, NULL) == -1);
    assert(hs_load(&idx, root) == 0); /* absent database */
    assert(hs_save(&idx, "/unlikely_nonexistent_hs_dir_abc") == -1);
    assert(hs_load(&idx, "/unlikely_nonexistent_hs_dir_abc") == 0);
    assert(hs_save(&idx, root) == 0);
    assert(hs_load(&idx, root) == 0);
    char p[HS_PATH_MAX]; path(p, sizeof(p), "documents.dat");
    assert(remove(p) == 0);
    path(p, sizeof(p), "statistics.dat");
    assert(remove(p) == 0);
    hs_destroy(&idx);

    load_fixture("BAD\n1\n", -1);
    load_fixture("HSDS2\n", -1);
    load_fixture("HSDS2\n0\n", -1);
    load_fixture("HSDS2\nnotanumber\n", -1);
    load_fixture("HSDS2\n4294967296\n", -1);
    load_fixture("HSDS2\n2\nmalformedline\n", -1);
    load_fixture("HSDS2\n2\nabc\tfile.txt\n", -1);
    load_fixture("HSDS2\n2\n0\tfile.txt\n", -1);
    load_fixture("HSDS2\n2\n1\t\n", -1);
    load_fixture("HSDS1\n3\n1\tmissing_file.txt\n", 0);
    load_fixture("HSDS2\n3\n2\tmissing_file.txt\n1\tother.txt\n", -1);

    char a[HS_PATH_MAX], content[HS_PATH_MAX + 100];
    path(a, sizeof(a), "persist.txt"); write_text(a, "persist search\n");
    assert(hs_init(&idx) == 0);
    int id = hs_add(&idx, a); assert(id > 0);
    query_count(&idx, "persist", 1);
    assert(hs_save(&idx, root) == 0);
    hs_destroy(&idx);
    assert(hs_init(&idx) == 0);
    assert(hs_load(&idx, root) == 0);
    assert(hs_document(&idx, (uint32_t)id));
    query_count(&idx, "persist", 1);
    hs_destroy(&idx);
    path(p, sizeof(p), "documents.dat"); assert(remove(p) == 0);
    path(p, sizeof(p), "statistics.dat"); assert(remove(p) == 0);
    assert(remove(a) == 0);

    /* A directory at the target filename forces rename failure on POSIX. */
    path(p, sizeof(p), "documents.dat"); assert(mkdir(p, 0700) == 0);
    assert(hs_init(&idx) == 0);
    assert(hs_save(&idx, root) == -1);
    hs_destroy(&idx);
    assert(rmdir(p) == 0);
    /* A directory at statistics.dat prevents the statistics file from opening. */
    path(p, sizeof(p), "statistics.dat"); assert(mkdir(p, 0700) == 0);
    assert(hs_init(&idx) == 0);
    assert(hs_save(&idx, root) == -1);
    hs_destroy(&idx);
    assert(rmdir(p) == 0);
    path(p, sizeof(p), "documents.dat"); assert(remove(p) == 0);
    (void)content;
}
int main(void) {
    char pattern[] = "/tmp/hs_comprehensive_XXXXXX";
    char *p = mkdtemp(pattern); assert(p);
    assert(strlen(p) < sizeof(root)); strcpy(root, p);
    test_api_and_search();
    test_limits_and_special_files();
    test_storage();
    assert(rmdir(root) == 0);
    puts("PASS: comprehensive API, search, persistence, malformed database and boundary tests");
    return 0;
}
