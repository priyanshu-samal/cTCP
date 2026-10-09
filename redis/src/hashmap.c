#include "hashmap.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


static unsigned long hash(const char *key)
{
    unsigned long hash = 5381;

    int c;

    while ((c = *key++))
    {
        hash = ((hash << 5) + hash) + c;
    }

    return hash % HASHMAP_SIZE;
}


void hashmap_init(HashMap *map)
{
    for (int i = 0; i < HASHMAP_SIZE; i++)
    {
        map->buckets[i] = NULL;
    }
}


void hashmap_set(
    HashMap *map,
    const char *key,
    const char *value
)
{
    unsigned long index = hash(key);

    Entry *entry = map->buckets[index];

    /*
     * Key already exists.
     */
    while (entry != NULL)
    {
        if (strcmp(entry->key, key) == 0)
        {
            free(entry->value);

            entry->value = strdup(value);

            return;
        }

        entry = entry->next;
    }


    /*
     * Create new entry.
     */
    Entry *new_entry = malloc(sizeof(Entry));

    if (new_entry == NULL)
    {
        return;
    }

    new_entry->key = strdup(key);
    new_entry->value = strdup(value);

    new_entry->next = map->buckets[index];

    map->buckets[index] = new_entry;
}


char *hashmap_get(
    HashMap *map,
    const char *key
)
{
    unsigned long index = hash(key);

    Entry *entry = map->buckets[index];

    while (entry != NULL)
    {
        if (strcmp(entry->key, key) == 0)
        {
            return strdup(entry->value);
        }

        entry = entry->next;
    }

    return NULL;
}


int hashmap_delete(
    HashMap *map,
    const char *key
)
{
    unsigned long index = hash(key);

    Entry *entry = map->buckets[index];

    Entry *previous = NULL;

    while (entry != NULL)
    {
        if (strcmp(entry->key, key) == 0)
        {
            if (previous == NULL)
            {
                map->buckets[index] = entry->next;
            }
            else
            {
                previous->next = entry->next;
            }

            free(entry->key);
            free(entry->value);
            free(entry);

            return 1;
        }

        previous = entry;
        entry = entry->next;
    }

    return 0;
}


int hashmap_exists(
    HashMap *map,
    const char *key
)
{
    unsigned long index = hash(key);

    Entry *entry = map->buckets[index];

    while (entry != NULL)
    {
        if (strcmp(entry->key, key) == 0)
        {
            return 1;
        }

        entry = entry->next;
    }

    return 0;
}


void hashmap_destroy(HashMap *map)
{
    for (int i = 0; i < HASHMAP_SIZE; i++)
    {
        Entry *entry = map->buckets[i];

        while (entry != NULL)
        {
            Entry *next = entry->next;

            free(entry->key);
            free(entry->value);
            free(entry);

            entry = next;
        }
    }
}