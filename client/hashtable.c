#include "hashtable.h"

#define TABLE_SIZE 20000
#define MAX_VALUES 100

unsigned int hash(const char *key) {
    unsigned long int pos = 0;
    unsigned int key_len = strlen(key);

    // do several rounds of multiplication
    for (int i = 0; i < key_len; ++i) {
        pos = pos * 37 + key[i];
    }

    // make sure value is 0 <= value < TABLE_SIZE
    pos = pos % TABLE_SIZE;

    return pos;
}

entry_t *ht_pair(const char *key, const char *value) {
    // allocate the entry
    entry_t *entry = malloc(sizeof(entry_t) * 1);
    entry->key = malloc(strlen(key) + 1);
    entry->value[0] = malloc(strlen(value) + 1);

    // copy the key and value in place
    strcpy(entry->key, key);
    strcpy(entry->value[0], value);

    // next starts out null but may be set later on
    entry->next = NULL;

    // initialize elements
    entry->elements = 1;

    return entry;
}

ht_t *ht_create(void) {
    // allocate table
    ht_t *hashtable = malloc(sizeof(ht_t) * 1);

    // allocate table entries
    hashtable->entries = malloc(sizeof(entry_t*) * TABLE_SIZE);

    // set each to null (needed for proper operation)
    for (int i = 0; i < TABLE_SIZE; ++i) {
        hashtable->entries[i] = NULL;
    }

    return hashtable;
}

void ht_set(ht_t *hashtable, const char *key, const char *value) {
    unsigned int slot = hash(key);

    // try to look up an entry set
    entry_t *entry = hashtable->entries[slot];

    // no entry means slot empty, insert immediately
    if (entry == NULL) {
        hashtable->entries[slot] = ht_pair(key, value);
        return;
    }

    entry_t *prev;

    // walk through each entry until either the end is
    // reached or a matching key is found
    while (entry != NULL) {
        // check key
        if (strcmp(entry->key, key) == 0) {
            // match found, put the new value
            for(int i = 0; i < entry->elements; i++){
                if(strcmp(entry->value[i], value) == 0){
                    return;
                }
            }

            entry->value[entry->elements] = malloc(strlen(value) + 1);
            strcpy(entry->value[entry->elements], value);
            entry->elements++;
            return;
        }

        // walk to next
        prev = entry;
        entry = prev->next;
    }

    // end of chain reached without a match, add new
    prev->next = ht_pair(key, value);
}

entry_t *ht_get(const ht_t *hashtable, const char *key) {
    unsigned int slot = hash(key);

    // try to find a valid slot
    entry_t *entry = hashtable->entries[slot];

    // walk through each entry in the slot, which could just be a single thing
    while (entry != NULL) {
        // return value if found
        if (strcmp(entry->key, key) == 0) {
            return entry;
        }

        // proceed to next key if available
        entry = entry->next;
    }

    // reaching here means there were >= 1 entries but no key match
    return NULL;
}

void ht_del(ht_t *hashtable, const char *key) {
    unsigned int bucket = hash(key);

    // try to find a valid bucket
    entry_t *entry = hashtable->entries[bucket];

    // no bucket means no entry
    if (entry == NULL) {
        return;
    }

    entry_t *prev;
    int idx = 0;

    // walk through each entry until either the end is reached or a matching key is found
    while (entry != NULL) {
        // check key
        if (strcmp(entry->key, key) == 0) {
            // first item and no next entry
            if (entry->next == NULL && idx == 0) {
                hashtable->entries[bucket] = NULL;
            }

            // first item with a next entry
            if (entry->next != NULL && idx == 0) {
                hashtable->entries[bucket] = entry->next;
            }

            // last item
            if (entry->next == NULL && idx != 0) {
                prev->next = NULL;
            }

            // middle item
            if (entry->next != NULL && idx != 0) {
                prev->next = entry->next;
            }

            // free the deleted entry
            free(entry->key);
            // free all values
            for (int i = 0; i < entry->elements; i++) {
                free(entry->value[i]);
            }
            free(entry);

            return;
        }

        // walk to next
        prev = entry;
        entry = prev->next;

        ++idx;
    }
}

void ht_print_entry(const entry_t *entry) {
    printf("%s=[", entry->key);
    for (int j = 0; j < entry->elements - 1; j++){
        printf("%s,", entry->value[j]);
    }
    printf("%s]", entry->value[entry->elements - 1]);
    printf(" ELEMENTS: %d\n", entry->elements);
}

void ht_print(const ht_t *hashtable) {
    for (int i = 0; i < TABLE_SIZE; ++i) {
        entry_t *entry = hashtable->entries[i];

        while(entry != NULL) {
            printf("slot[%4d]: ", i); // slot
            ht_print_entry(entry);
            entry = entry->next;
        }
    }
}


// int main(int argc, char **argv) {
//     ht_t *ht = ht_create();

//     ht_set(ht, "name1", "em");
//     ht_set(ht, "name2", "russian");
//     ht_set(ht, "name3", "pizza");
//     ht_set(ht, "name4", "doge");
//     ht_set(ht, "name5", "pyro");
//     ht_set(ht, "name6", "joost");
//     ht_set(ht, "name7", "kalix");
//     ht_set(ht, "name1", "PEPE");
//     ht_set(ht, "name1", "GONZALITO");
//     ht_set(ht, "name1", "ROBERTITO");
//     ht_set(ht, "name1", "VILLARCITO");
//     ht_print(ht);

//     printf("\n\n");

//     entry_t * entry = ht_get(ht, "name1");
//     ht_print_entry(entry);
    

//     printf("\n\n");

//     ht_del(ht, "name1");
//     ht_del(ht, "name2");

//     ht_print(ht);

//     return 0;
// }