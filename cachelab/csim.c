#include "cachelab.h"
#include <getopt.h>
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>

typedef struct{
    int valid;
    unsigned tag;
    int counter;
}cache_line;

int opt,h,v,s,E,b,S;
char* filepath = NULL;
cache_line** cache = NULL;
char identifier;
unsigned address;
int size;
int hits=0,misses=0,evictions=0;
unsigned s_address, tag_address;

void run(){
    s_address = (address >> b) & (0xffffffff >> (32-s));
    tag_address = (address >> (s+b));
    int tmp=0;
    for(int i=0;i<E;i++){
        if(cache[s_address][i].valid == 1 && cache[s_address][i].tag == tag_address){
            cache[s_address][i].counter = 0;
            hits++;
            return;
        }
    }
    misses++;
    for(int i=0;i<E;i++){
        if(cache[s_address][i].valid == 0){
            cache[s_address][i].valid = 1;
            cache[s_address][i].tag = tag_address;
            cache[s_address][i].counter = 0;
            return;
        }else{
            if(cache[s_address][i].counter > cache[s_address][tmp].counter)
                tmp = i;
        }
    }
    evictions++;
    cache[s_address][tmp].tag = tag_address;
    cache[s_address][tmp].counter = 0;
}

void time(){
    for(int i=0;i<S;i++){
        for(int j=0;j<E;j++){
            if(cache[i][j].valid)
                cache[i][j].counter++;
        }
    }
}

int main(int argc, char* argv[])
{
    while( -1 != (opt = getopt(argc,argv,"hvs:E:b:t:")))
    {
        switch(opt){
            case 'h':
                h=1;
                break;
            case 'v':
                v=1;
                break;
            case 's':
                s = atoi(optarg);
                break;
            case 'E':
                E = atoi(optarg);
                break;
            case 'b':
                b = atoi(optarg);
                break;
            case 't':
                filepath = optarg;
                break;
            default:
                printf("wrong argument!!!\n");
                break;
        }
    }

    S = 1<<s;
    cache = (cache_line**) malloc(sizeof(cache_line*) * S);
    for(int i=0;i<S;i++){
        cache[i] = (cache_line*) malloc(sizeof(cache_line) * E);
        for(int j=0;j<E;j++){
            cache[i][j].valid = 0;
            cache[i][j].tag = 0xffffffff;
            cache[i][j].counter = 0;
        }
    }

    FILE * pFile = fopen(filepath,"r");
    while(fscanf(pFile," %c %x,%d\n",&identifier,&address,&size)>0)
    {
        switch (identifier) {
            case 'L':
                run();
                break;
            case 'S':
                run();
                break;
            case 'M':
                run();
                run();
                break;
        }
        time();
    }

    for(int i=0;i<S;i++){
        free(cache[i]);
    }
    free(cache);
    fclose(pFile);
    printSummary(hits, misses, evictions);
    return 0;
}
