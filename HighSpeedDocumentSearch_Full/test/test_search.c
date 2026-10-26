#include "search.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <direct.h>
#define MKDIR(p) _mkdir(p)
#else
#include <sys/stat.h>
#define MKDIR(p) mkdir(p,0700)
#endif
static void write_file(const char *path, const char *contents)
{
    FILE *f = fopen(path, "wb");
    assert(f != NULL);

    if (f == NULL) {
        fprintf(stderr, "Failed to open file: %s\n", path);
        exit(EXIT_FAILURE);
    }

    if (fputs(contents, f) == EOF) {
        fprintf(stderr, "Failed to write file: %s\n", path);
        fclose(f);
        exit(EXIT_FAILURE);
    }

    if (fclose(f) != 0) {
        fprintf(stderr, "Failed to close file: %s\n", path);
        exit(EXIT_FAILURE);
    }
}
static void expect_query(SearchIndex *idx,const char *q,size_t expected){SearchResult *r=NULL;size_t n=hs_search(idx,q,&r);assert(n==expected);free(r);}
static void test_invalid_storage_paths(void)
{
    SearchIndex idx = {0};

    assert(hs_init(&idx) == 0);

    /* Invalid arguments */
    assert(hs_save(NULL, "build") == -1);
    assert(hs_save(&idx, NULL) == -1);

    assert(hs_load(NULL, "build") == -1);
    assert(hs_load(&idx, NULL) == -1);

    /* Directory does not exist */
    assert(hs_save(&idx, "/nonexistent_hs_test_dir") == -1);
    assert(hs_load(&idx, "/nonexistent_hs_test_dir") == 0);

    hs_destroy(&idx);
}

int main(void){MKDIR("build");MKDIR("build/test_data");const char *a="build/test_data/a.txt",*b="build/test_data/b.c",*c="build/test_data/c.md",*db="build/test_data";write_file(a,"search search search indexing engine\n");write_file(b,"search engine in c\n");write_file(c,"completely different content\n");SearchIndex idx;assert(hs_init(&idx)==0);assert(hs_supported_file("x.C"));assert(hs_supported_file("x.cpp"));assert(!hs_supported_file("x.png"));assert(hs_add(&idx,"missing.txt")<0);assert(hs_add(&idx,"x.png")<0);int ia=hs_add(&idx,a),ib=hs_add(&idx,b),ic=hs_add(&idx,c);assert(ia>0&&ib>0&&ic>0);assert(hs_add(&idx,a)==-2);assert(hs_active_documents(&idx)==3);assert(idx.unique_terms>0);expect_query(&idx,"search",2);expect_query(&idx,"SEARCH",2);expect_query(&idx,"search search",2);expect_query(&idx,"not_in_any_document",0);expect_query(&idx,"",0);expect_query(&idx,"!@#$",0);expect_query(&idx,"search indexing",2);SearchResult *r=NULL;size_t n=hs_search(&idx,"search",&r);assert(n==2);assert(r[0].doc_id==(uint32_t)ia);assert(r[0].score>=r[1].score);assert(idx.candidate_visits>0);free(r);assert(hs_search(&idx,NULL,&r)==0);assert(hs_search(NULL,"search",&r)==0);assert(hs_document(&idx,(uint32_t)ia)->size>0);write_file(b,"freshkeyword freshkeyword unique\n");assert(hs_update(&idx,(uint32_t)ib)==0);expect_query(&idx,"freshkeyword",1);expect_query(&idx,"search",1);assert(hs_update(&idx,99999)<0);assert(hs_delete(&idx,(uint32_t)ia)==0);assert(hs_delete(&idx,(uint32_t)ia)<0);expect_query(&idx,"search",0);assert(hs_active_documents(&idx)==2);assert(hs_save(&idx,db)==0);uint32_t expected_id=(uint32_t)ib;hs_destroy(&idx);assert(hs_init(&idx)==0);assert(hs_load(&idx,db)==0);assert(hs_document(&idx,expected_id)!=NULL);expect_query(&idx,"freshkeyword",1);assert(hs_active_documents(&idx)==2);hs_destroy(&idx);remove(a);remove(b);remove(c);remove("build/test_data/documents.dat");remove("build/test_data/statistics.dat");test_invalid_storage_paths();puts("PASS: indexing, supported extensions, keyword lookup, query normalization, empty/unknown queries, ranking, update, delete, persistence");return 0;}
