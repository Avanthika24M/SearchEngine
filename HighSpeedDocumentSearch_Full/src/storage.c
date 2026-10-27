#include "search.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static int join(char *dst,size_t cap,const char *dir,const char *name){int n=snprintf(dst,cap,"%s/%s",dir,name);return n>0&&(size_t)n<cap?0:-1;}
int hs_save(const SearchIndex *idx,const char *dir){if(!idx||!dir)return -1;char path[HS_PATH_MAX+64],tmp[HS_PATH_MAX+64];if(join(path,sizeof(path),dir,"documents.dat")||join(tmp,sizeof(tmp),dir,"documents.tmp"))return -1;FILE *f=fopen(tmp,"wb");if(!f)return -1;if(fprintf(f,"HSDS2\n%u\n",idx->next_id)<0)goto fail;for(size_t i=0;i<idx->count;i++){const Document *d=&idx->docs[i];if(d->active&&fprintf(f,"%u\t%s\n",d->id,d->path)<0)goto fail;}if(fclose(f)!=0){remove(tmp);return -1;}
#ifdef _WIN32
/* Windows rename does not replace an existing destination. */
if(remove(path)!=0&&errno!=ENOENT){remove(tmp);return -1;}
#endif
if(rename(tmp,path)!=0){remove(tmp);return -1;}if(join(path,sizeof(path),dir,"statistics.dat"))return -1;f=fopen(path,"wb");if(!f)return -1;int ok=fprintf(f,"%llu %.9f %llu\n",(unsigned long long)idx->searches,idx->total_ms,(unsigned long long)idx->candidate_visits)>0;if(fclose(f)!=0)ok=0;return ok?0:-1;
fail:
    if (fclose(f) != 0) {
        /* File closing failed; the save operation already failed. */
    }

    if (remove(tmp) != 0) {
        /* Temporary file cleanup failed. */
    }

    return -1;
}
int hs_load(SearchIndex *idx,const char *dir){if(!idx||!dir)return -1;char path[HS_PATH_MAX+64];if(join(path,sizeof(path),dir,"documents.dat"))return -1;FILE *f=fopen(path,"rb");if(!f)return errno==ENOENT?0:-1;char line[HS_PATH_MAX+64];if(!fgets(line,sizeof(line),f)||(strcmp(line,"HSDS2\n")!=0&&strcmp(line,"HSDS1\n")!=0)){fclose(f);return -1;}if(!fgets(line,sizeof(line),f)){fclose(f);return -1;}char *end=NULL;unsigned long next=strtoul(line,&end,10);if(end==line||next==0||next>UINT32_MAX){fclose(f);return -1;}while(fgets(line,sizeof(line),f)){char *tab=strchr(line,'\t');if(!tab){fclose(f);return -1;}*tab=0;unsigned long id=strtoul(line,&end,10);if(end==line||*end||id==0||id>UINT32_MAX||id<idx->next_id){fclose(f);return -1;}char *p=tab+1;p[strcspn(p,"\r\n")]=0;if(!*p){fclose(f);return -1;}idx->next_id=(uint32_t)id;if(hs_add(idx,p)<0){fprintf(stderr,"Warning: skipping unavailable file: %s\n",p);idx->next_id=(uint32_t)id+1;}}if(ferror(f)){fclose(f);return -1;}fclose(f);if(next>idx->next_id)idx->next_id=(uint32_t)next;if(join(path,sizeof(path),dir,"statistics.dat"))return -1;f=fopen(path,"rb");if(f){unsigned long long searches=0,visits=0;double ms=0;int n=fscanf(f,"%llu %lf %llu",&searches,&ms,&visits);if(n>=2){idx->searches=searches;idx->total_ms=ms;if(n==3)idx->candidate_visits=visits;}fclose(f);}return 0;}
