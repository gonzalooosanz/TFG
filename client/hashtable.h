#ifndef _HashTable_H
#define _HashTable_H

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TABLE_SIZE 20000
#define MAX_VALUES 100

typedef struct entry_t {
    char *key;
    char *value[MAX_VALUES];
    int elements;
    struct entry_t *next;
} entry_t;

typedef struct {
    entry_t **entries;
} ht_t;

unsigned int hash(const char *key);

entry_t *ht_pair(const char *key, const char *value);

ht_t *ht_create(void);

void ht_set(ht_t *hashtable, const char *key, const char *value);

entry_t *ht_get(const ht_t *hashtable, const char *key);

void ht_del(ht_t *hashtable, const char *key);

void ht_print_entry(const entry_t *entry);

void ht_print(const ht_t *hashtable);

#endif
