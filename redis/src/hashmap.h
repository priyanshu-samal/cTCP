#ifndef HASHMAP_H
#define HASHMAP_H

#define HASHMAP_SIZE 1024

typedef struct Entry
{
    char *key;
    char *value;

    struct Entry *next;

} Entry;


typedef struct
{
    Entry *buckets[HASHMAP_SIZE];

} HashMap;


void hashmap_init(HashMap *map);

void hashmap_set(
    HashMap *map,
    const char *key,
    const char *value
);

char *hashmap_get(
    HashMap *map,
    const char *key
);

int hashmap_delete(
    HashMap *map,
    const char *key
);

int hashmap_exists(
    HashMap *map,
    const char *key
);

void hashmap_destroy(
    HashMap *map
);

#endif