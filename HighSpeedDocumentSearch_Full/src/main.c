#include "search.h"
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#ifdef _WIN32
#include <direct.h>
#define MKDIR(p) _mkdir(p)
#else
#include <sys/stat.h>
#define MKDIR(p) mkdir(p,0700)
#endif
static int read_line(char *s,size_t cap){if(!fgets(s,(int)cap,stdin))return -1;size_t n=strcspn(s,"\n");if(s[n]=='\n')s[n]=0;else{int c;while((c=getchar())!='\n'&&c!=EOF){}return -1;}return 0;}
static int parse_id(const char *s,uint32_t *out){if(!s||!*s||*s=='-'||!out)return -1;char *end;errno=0;unsigned long n=strtoul(s,&end,10);if(errno||*end||n==0||n>UINT32_MAX)return -1;*out=(uint32_t)n;return 0;}
static void list(const SearchIndex *idx){for(size_t i=0;i<idx->count;i++)if(idx->docs[i].active)printf("[%u] %s (%llu bytes, %u words)\n",idx->docs[i].id,idx->docs[i].path,(unsigned long long)idx->docs[i].size,idx->docs[i].words);}
static void history(const char *dir,const char *query,size_t results){char path[HS_PATH_MAX+64];int n=snprintf(path,sizeof(path),"%s/history.dat",dir);if(n<=0||(size_t)n>=sizeof(path))return;FILE *f=fopen(path,"ab");if(f){fprintf(f,"%lld\t%zu\t%s\n",(long long)time(NULL),results,query);fclose(f);}}
static void show_history(const char *dir){char path[HS_PATH_MAX+64];int n=snprintf(path,sizeof(path),"%s/history.dat",dir);if(n<=0||(size_t)n>=sizeof(path))return;FILE *f=fopen(path,"rb");if(!f){puts("No history yet.");return;}char line[HS_PATH_MAX+128];while(fgets(line,sizeof(line),f))fputs(line,stdout);fclose(f);}
int main(int argc,char **argv){const char *dir=argc>1?argv[1]:"build";if(MKDIR(dir)!=0&&errno!=EEXIST){perror("storage directory");return 1;}SearchIndex idx;if(hs_init(&idx)){fputs("Initialization failed\n",stderr);return 1;}if(hs_load(&idx,dir)){fputs("Could not load database\n",stderr);hs_destroy(&idx);return 1;}char line[HS_PATH_MAX+32];for(;;){puts("\n=== HIGH-SPEED DOCUMENT SEARCH ===\n1 Add file   2 Search     3 Update ID   4 Delete ID\n5 List       6 Statistics 7 Save        8 Query history\n0 Exit\nChoice:");if(read_line(line,sizeof(line)))break;char *end;long choice=strtol(line,&end,10);if(*end||end==line){puts("Invalid choice");continue;}if(choice==0)break;if(choice==1){puts("File path:");if(read_line(line,sizeof(line)))continue;int id=hs_add(&idx,line);if(id>0)printf("Indexed ID: %d\n",id);else if(id==-2)puts("Already indexed: this file path exists in the index.");else puts("Add failed: check that the file exists, is readable, is <=8 MiB, and has a supported extension.");}else if(choice==2){puts("Search keywords:");if(read_line(line,sizeof(line)))continue;SearchResult *r=NULL;size_t n=hs_search(&idx,line,&r);printf("%zu results\n",n);hs_print_results(&idx,r,n,stdout);free(r);history(dir,line,n);}else if(choice==3||choice==4){puts("Document ID:");if(read_line(line,sizeof(line)))continue;uint32_t id;if(parse_id(line,&id)){puts("Invalid document ID");continue;}int rc=choice==3?hs_update(&idx,id):hs_delete(&idx,id);puts(rc==0?"Operation completed":"Operation failed: check ID and file accessibility");}else if(choice==5)list(&idx);else if(choice==6)printf("Active documents: %zu, keywords: %zu, searches: %llu, avg CPU ms: %.3f, posting visits: %llu\n",hs_active_documents(&idx),idx.unique_terms,(unsigned long long)idx.searches,idx.searches?idx.total_ms/(double)idx.searches:0,(unsigned long long)idx.candidate_visits);else if(choice==7)puts(hs_save(&idx,dir)==0?"Saved":"Save failed");else if(choice==8)show_history(dir);else puts("Invalid choice");}if(hs_save(&idx,dir))fputs("Warning: save failed\n",stderr);hs_destroy(&idx);return 0;}
