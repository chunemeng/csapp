#include "cachelab.h"
#include <getopt.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
// 滕昊益 522031910118

typedef int *line_t;

struct lru_node
{
    line_t line;
    unsigned long long int tag;
};

struct lru
{
    int max_size;
    int current_size;
    struct lru_node *head;
};

void init_lru(struct lru **l, int set_size, int line_size)
{
    *l = (struct lru *)malloc(sizeof(struct lru) * set_size);
    for (int i = 0; i < set_size; ++i)
    {
        (*l)[i].current_size = 0;
        (*l)[i].max_size = line_size;
        (*l)[i].head = (struct lru_node *)malloc(sizeof(struct lru_node) * line_size);
    }
};

void update_lru(struct lru *l, int set_offset, int line_offset)
{
    struct lru *cur = &l[set_offset];
    unsigned long long int tag = cur->head[line_offset].tag;

    line_t line = cur->head[line_offset].line;
    for (int i = line_offset; i > 0; --i)
    {
        cur->head[i].line = cur->head[i - 1].line;
        cur->head[i].tag = cur->head[i - 1].tag;
    }
    cur->head[0].line = line;
    cur->head[0].tag = tag;
}
void update_set_lru(struct lru *l, int set_offset, int line_offset, int block_offset, int bytes)
{
    // printf("use\n");
    struct lru *cur = &l[set_offset];
    unsigned long long int tag = cur->head[line_offset].tag;

    line_t line = cur->head[line_offset].line;

    for (int i = line_offset; i > 0; --i)
    {
        cur->head[i].line = cur->head[i - 1].line;
        cur->head[i].tag = cur->head[i - 1].tag;
    }
    memset(line + block_offset, 1, (bytes - block_offset) * 4);
    cur->head[0].line = line;
    cur->head[0].tag = tag;
    // printf("hepp%lld\n",tag);
}

int put_lru(struct lru *l, line_t *line, unsigned long long int tag, int set_offset)
{
    struct lru *cur = &l[set_offset];
    // printf("%d",cur->current_size);
    int flag = cur->current_size == cur->max_size;

    if (!flag)
    {
        cur->current_size++;
    }
    else
    {
        free(cur->head[cur->current_size - 1].line);
    }

    for (int i = cur->current_size - 1; i > 0; --i)
    {
        cur->head[i].line = cur->head[i - 1].line;
        cur->head[i].tag = cur->head[i - 1].tag;
    }
    cur->head[0].line = *line;
    cur->head[0].tag = tag;
    // printf("%p\n",*line);
    //     printf("%ld\n",sizeof(*line));

    //  printf("%p\n",&(l[4].current_size));
    //          printf("%ld\n",sizeof(l[4]));

    // printf("\nhkl%d\n",l[4].current_size);

    // printf("tag %lld\n", tag);
    return flag;
}

int find(struct lru *l, unsigned long long int tag, int set_offset, int block_offset, int bytes, int *offset)
{
    struct lru *cur = &l[set_offset];
    struct lru_node node;

    for (int i = 0; i < cur->current_size; ++i)
    {
        node = cur->head[i];

        if (tag == node.tag)
        {

            *offset = i;
            for (int j = 0; j < bytes; ++j)
            {
                if (!node.line[j + block_offset])
                {
                    return 2;
                }
            }
            return 1;
        }
    }
    return 0;
}

void clean_lru(struct lru *l, int set_size)
{
    for (int i = 0; i < set_size; ++i)
    {
        // printf("ddd %d \n",l[i].current_size);
        for (int j = 0; j < l[i].current_size; ++j)
        {
            // printf("ttttag %lld\n", l[i].head[j].tag);
            free(l[i].head[j].line);
        }
        // printf("dsjkksa %lld\n", l[i].head[0].tag);
        free(l[i].head);
    }
    free(l);
}

int main(int argc, char *argv[])
{
    int set_bits;
    int line_size;
    int block_bits;
    FILE *fp = NULL;
    int o;
    if (argc != 9)
    {
        printf("Invaild Input");
        return 0;
    }

    while ((o = getopt(argc, argv, "s:E:b:t:")) != -1)
    {
        switch (o)
        {
        case 's':
            set_bits = atoi(optarg);
            break;
        case 'E':
            line_size = atoi(optarg);
            break;
        case 'b':
            block_bits = atoi(optarg);
            break;
        case 't':
            if ((fp = fopen(optarg, "r")) == NULL)
            {
                printf("文件打开失败。\n");
                return -1;
            }
            break;
        case '?':
            printf("error optopt: %c\n", optopt);
            printf("error opterr: %d\n", opterr);
            return -1;
        }
    }

    char l[20];
    int hit, evi;
    int set_size = pow(2, set_bits);
    int block_size = pow(2, block_bits);
    // int offset = set_bits + block_bits;
    int b_address, s_address;
    unsigned long long int tag;
    // int tag_bits = 64 - offset;
    // unsigned long long int tag_size = pow(2, tag_bits);
    int hits = 0, misses = 0, evictions = 0;
    void *ret;
    unsigned long long int address;
    char *endptr;
    int offset;
    struct lru *lru;
    init_lru(&lru, set_size, line_size);
    // struct set *s = (struct set *)malloc(sizeof(struct set) * set_size);

    // for (int i = 0; i < set_size; ++i)
    // {
    //     s[i].lines = (line_t *)malloc(sizeof(line_t) * line_size);
    //     s[i].timestamp = 0;
    //     for (int j = 0; j < line_size; ++j)
    //     {
    //         memset(s[i].lines, 0, sizeof(s[i].lines));
    //         unsigned char *ptr = (unsigned char *)malloc(block_size);
    //         memset(ptr, 0, block_size);
    //         s[i].lines->data = ptr;
    //     }
    // }
    while (1)
    {
        memset(l, 0, 20);
        ret = fgets(l, 20, fp);
        if (ret == NULL)
            break;

        if (l[0] != ' ')
        {
            continue;
        }
        evi = 0;
        // need_store = l[1] != 'L';
        address = strtoul(l + 3, &endptr, 16);
        b_address = (block_size - 1) & address;
        s_address = (set_size - 1) & (address >> block_bits);
        tag = (address >>= (set_bits + block_bits));
        address = strtoul(endptr + 1, &endptr, 10);
        printf("b%d , s%d , tag%lld , size %lld", b_address, s_address, tag, address);
        hit = find(lru, tag, s_address, b_address, address, &offset);

        // printf("%d\n", hits);
        if (!hit)
        {
            line_t t = (line_t)malloc(sizeof(int) * block_size);

            memset(t, 0x1, block_size * sizeof(int));
            // memset(t + b_address, 1, address);
            evi = put_lru(lru, &t, tag, s_address);
            offset = 0;
        }
        else
        {
            if (hit == 1)
            {
                update_lru(lru, s_address, offset);
                offset = 1;
            }
            if (hit == 2)
            {
                update_set_lru(lru, s_address, offset, b_address, block_size);
                hit = 0;
                offset = 2;
            }
        }
        printf(" %c H%d, E %d\n", l[1], offset, evi);
        switch (l[1])
        {
        case 'M':
            if (hit)
            {
                hits += 2;
            }
            else
            {
                if (evi)
                {
                    ++evictions;
                }
                ++misses;
                ++hits;
            }
            break;
        case 'L':
        case 'S':
            if (hit)
            {
                ++hits;
            }
            else
            {
                if (evi)
                {
                    ++evictions;
                    ++misses;
                }
                else
                {
                    ++misses;
                }
            }
        default:
            break;
        }
    }
    // for (int i = 0; i < set_size; ++i)
    // {
    //     for (int j = 0; j < line_size; ++j)
    //     {
    //         free(s[i].lines->data);
    //     }
    //     free(s[i].lines);
    // }
    // free(s);
    clean_lru(lru, set_size);
    printSummary(hits, misses, evictions);

    return 0;
}
