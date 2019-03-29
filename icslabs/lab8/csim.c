/**
* Name: Xingyue Qian
* LoginID: ics516072910066
**/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include "cachelab.h"

typedef struct {
int tag;
int valid;
int time_stamp;
} Cache_line;

typedef Cache_line* Cache_set;
typedef Cache_set* Cache;

//command parameters
static int v_flag = 0;
static int s_num = 0;
static int e_num = 0;
static int b_num = 0;
static char *t_name = 0;

// hit/miss/eviction number
static int hit_num = 0;
static int miss_num = 0;
static int evc_num = 0;

/*
*  print_help:
*   Prints help message.
*/
void print_help() {
    printf("Usage: ./csim [-hv] -s <num> -E <num> -b <num> -t <file>\n");
    printf("Options:\n");
    printf("\t-h         Print this help message.\n");
    printf("\t-v         Optional verbose flag.\n");
    printf("\t-s <num>   Number of set index bits.\n");
    printf("\t-E <num>   Number of lines per set.\n");
    printf("\t-b <num>   Number of block offset bits.\n");
    printf("\t-t <file>  Trace file.\n");
    printf("Examples:\n");
    printf("\tlinux>  ./csim -s 4 -E 1 -b 4 -t traces/yi.trace\n");
    printf("\tlinux>  ./csim -v -s 8 -E 2 -b 4 -t traces/yi.trace\n"); 
}

/*
*  get_args:
*   Gets command parameters.
*/
int get_args(int argc, char *argv[]) {
    char ch;
    int default_flag = 0;
    while ((ch = getopt(argc, argv, "hvs:E:b:t:"))!= -1) 
    {
        switch (ch)
        {
            case 'h':
                print_help();
                break;
            case 'v':
                v_flag = 1;
                break;
            case 's':
                s_num = atoi(optarg);
                break;
            case 'E':
                e_num = atoi(optarg);
                break;
            case 'b':
                b_num = atoi(optarg);
                break;
            case 't':
                t_name = optarg;
                break;
            default:
                print_help();
                default_flag = 1;
                break;
        }
    }
    if (s_num == 0 || e_num == 0 || b_num == 0 || t_name == NULL) {
        if (!default_flag) {                                        // if we don't check default_flag, help will be printed twice.
            printf("./csim: Missing required command line argument\n");
            print_help();
        }
        return -1;
    }
    return 1;
}

/*
*  init_mycache:
*   Initializes mycache memory to 0;
*/
void init_mycache(Cache *mycache) {
    int sets = 1 << s_num;
    *mycache = (Cache_set*)malloc(sizeof(Cache_set) * sets);
    for (int i = 0; i < sets; ++i) {
        (*mycache)[i] = (Cache_line*)malloc(sizeof(Cache_line) * e_num);
        for (int j = 0; j < e_num; ++j) {
            (*mycache)[i][j].valid = 0;
            (*mycache)[i][j].tag = -1;
            (*mycache)[i][j].time_stamp = 0;
        }
    }

}

/*
*  cache_deal:
*   Deals with cache behavior according to the address;
*/
void cache_deal(Cache *mycache, long unsigned int addr) {
    addr = addr >> b_num;
    long unsigned int mask = 0xffffffffffffffff >> (64 - s_num);
    long set_index = addr & mask;
    int cur_tag = (int)(addr >> s_num);

    Cache_set set = (*mycache)[set_index];
    for (int i = 0; i < e_num; i++) {
        if (set[i].valid == 1) {
            set[i].time_stamp++;
        } 
    }
    
    for (int i = 0; i < e_num; i++) {
        if (set[i].tag == cur_tag) {
            if (v_flag) printf("hit\n");
            set[i].time_stamp = 0;
            hit_num++;
            return;
        }
    }

    if (v_flag) printf("miss\n");
    miss_num++;

    for (int i = 0; i < e_num; i++) {
        if (set[i].valid == 0) {
            set[i].valid = 1;
            set[i].tag = cur_tag;
            set[i].time_stamp = 0;
            return;
        }
    }

    if (v_flag) printf("eviction\n");
    evc_num++;

    int time_max_index = 0;
    int max_time = 0;
    for (int i = 0; i < e_num; i++) {
        if (set[i].time_stamp > max_time) {
            time_max_index = i;
            max_time = set[i].time_stamp;
        }
    }
    set[time_max_index].tag = cur_tag;
    set[time_max_index].time_stamp = 0;

}

/*
*  cache:
*   Simulates the cache behavior.
*/
int cache(Cache *mycache, char operation, long unsigned int addr) {
    switch (operation) {
        case 'L':
        case 'S':
            cache_deal(mycache, addr);
            break;
        case 'M':
            cache_deal(mycache, addr);
            cache_deal(mycache, addr);
            break;
        default:
            return -1;
    }

    return 0;
}


/*
*  main:
*   Combines all together;
*/
int main(int argc, char *argv[])
{
    // Read arguments
    if (get_args(argc, argv) == -1) return -1;

    // Check the file opening
    FILE *fp = NULL;
    if ((fp = fopen(t_name, "r")) == NULL) {
        printf("%s: No such file or directory\n", t_name);
        return -1;
    } 

    // Initiate my cache
    Cache mycache;
    init_mycache(&mycache);


    // Simulate cache's behavior
    char line[80];
    char *pline;
    char operation;
    long unsigned int addr = 0;
    unsigned int size = 0;
    while (fgets(line, 80, fp) != NULL) {
        pline = line;
        if (*pline++ == 'I')
            continue;
        sscanf(pline, "%c %lx,%u", &operation, &addr, &size);
        cache(&mycache, operation, addr);
    }

    //print results
    printSummary(hit_num, miss_num, evc_num);

    //free mycache
    free(mycache);
    return 0;
}
