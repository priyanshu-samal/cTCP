#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#define TABLE_SIZE 16

typedef struct Entry
{
    char *key;
    char *value;

    struct Entry *next;
} Entry;

typedef struct
{
    Entry *buckets[TABLE_SIZE];

    pthread_mutex_t lock;

} HashTable;


/*
 * Simple hash function
 */
unsigned int hash(const char *key)
{
    unsigned int hash = 5381;

    while (*key)
    {
        hash = ((hash << 5) + hash) + *key;
        key++;
    }

    return hash % TABLE_SIZE;
}


/*
 * Initialize hash table
 */
void hashmap_init(HashTable *table)
{
    for (int i = 0; i < TABLE_SIZE; i++)
    {
        table->buckets[i] = NULL;
    }

    pthread_mutex_init(&table->lock, NULL);
}


/*
 * SET
 */
void hashmap_set(
    HashTable *table,
    const char *key,
    const char *value
)
{
    pthread_mutex_lock(&table->lock);

    unsigned int index = hash(key);

    Entry *entry = table->buckets[index];

    /*
     * Check whether key already exists.
     */
    while (entry != NULL)
    {
        if (strcmp(entry->key, key) == 0)
        {
            free(entry->value);

            entry->value = strdup(value);

            pthread_mutex_unlock(&table->lock);

            return;
        }

        entry = entry->next;
    }

    /*
     * Key doesn't exist.
     * Create new entry.
     */
    Entry *new_entry = malloc(sizeof(Entry));

    if (new_entry == NULL)
    {
        pthread_mutex_unlock(&table->lock);

        return;
    }

    new_entry->key = strdup(key);
    new_entry->value = strdup(value);

    new_entry->next = table->buckets[index];

    table->buckets[index] = new_entry;

    pthread_mutex_unlock(&table->lock);
}


/*
 * GET
 */
char *hashmap_get(
    HashTable *table,
    const char *key
)
{
    pthread_mutex_lock(&table->lock);

    unsigned int index = hash(key);

    Entry *entry = table->buckets[index];

    while (entry != NULL)
    {
        if (strcmp(entry->key, key) == 0)
        {
            char *result = strdup(entry->value);

            pthread_mutex_unlock(&table->lock);

            return result;
        }

        entry = entry->next;
    }

    pthread_mutex_unlock(&table->lock);

    return NULL;
}


/*
 * DELETE
 */
void hashmap_delete(
    HashTable *table,
    const char *key
)
{
    pthread_mutex_lock(&table->lock);

    unsigned int index = hash(key);

    Entry *entry = table->buckets[index];
    Entry *previous = NULL;

    while (entry != NULL)
    {
        if (strcmp(entry->key, key) == 0)
        {
            if (previous == NULL)
            {
                table->buckets[index] = entry->next;
            }
            else
            {
                previous->next = entry->next;
            }

            free(entry->key);
            free(entry->value);
            free(entry);

            pthread_mutex_unlock(&table->lock);

            return;
        }

        previous = entry;
        entry = entry->next;
    }

    pthread_mutex_unlock(&table->lock);
}


/*
 * Thread function
 */
void *worker(void *arg)
{
    HashTable *table = arg;

    hashmap_set(
        table,
        "name",
        "Priyanshu"
    );

    char *value = hashmap_get(
        table,
        "name"
    );

    if (value != NULL)
    {
        printf(
            "Thread %lu got: %s\n",
            pthread_self(),
            value
        );

        free(value);
    }

    return NULL;
}


int main(void)
{
    HashTable table;

    hashmap_init(&table);

    pthread_t thread1;
    pthread_t thread2;
    pthread_t thread3;

    pthread_create(
        &thread1,
        NULL,
        worker,
        &table
    );

    pthread_create(
        &thread2,
        NULL,
        worker,
        &table
    );

    pthread_create(
        &thread3,
        NULL,
        worker,
        &table
    );

    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);
    pthread_join(thread3, NULL);

    char *value = hashmap_get(
        &table,
        "name"
    );

    if (value != NULL)
    {
        printf("Final value: %s\n", value);

        free(value);
    }

    hashmap_delete(
        &table,
        "name"
    );

    pthread_mutex_destroy(&table.lock);

    return 0;
}