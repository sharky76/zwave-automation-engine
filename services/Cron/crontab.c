#include "crontab.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

variant_stack_t*    crontab;

void delete_time_entry(void* arg)
{
    crontab_time_entry_t* e = (crontab_time_entry_t*)arg;

    stack_for_each(e->child_array, child_time_entry_variant)
    {
        stack_remove(e->child_array, child_time_entry_variant);
        variant_free(child_time_entry_variant);
    }

    stack_free(e->child_array);
    free(e);
}

/**
 * o o o o 
 * | 
 * o o 
 *   |
 *   o o o
 *   |
 *   o o o
 *     |
 *     o o
 *       |
 *       C C C
 *  
 */


void    crontab_add_entry(crontab_time_t crontab_time, char* scene)
{
    if(NULL == crontab)
    {
        crontab = stack_create();
    }
    variant_stack_t* root = crontab;
    int* time_ptr = (int*)crontab_time;
    bool node_found = false;

    for(int i = 0; i < 5; i++)
    {
        stack_for_each(root, time_entry_variant)
        {
            crontab_time_entry_t* e = (crontab_time_entry_t*)variant_get_ptr(time_entry_variant);
            if(e->time == *time_ptr)
            {
                printf("Time %d was found, go to next\n", e->time);
                root = e->child_array;
                time_ptr++;
                node_found = true;
                break;
            }

            node_found = false;
        }

        if(!node_found)
        {
            crontab_time_entry_t* new_time_entry = calloc(1, sizeof(crontab_time_entry_t));
            new_time_entry->time = *time_ptr;
            time_ptr++;
            printf("Adding time %d\n", new_time_entry->time);
            new_time_entry->child_array = stack_create();
            stack_push_back(root, variant_create_ptr(DT_PTR, new_time_entry, &delete_time_entry));
            root = new_time_entry->child_array;
    
        }

    }

    // Here root points to the last child array which must hold the command
    printf("Adding scene %s\n", scene);
    stack_push_back(root, variant_create_string(strdup(scene)));
}

typedef struct crontab_remove_candidate_t
{
    variant_t* data;
    variant_stack_t* stack;
} crontab_remove_candidate_t;

void    crontab_del_entry(crontab_time_t crontab_time, char* scene)
{
    if(NULL == crontab)
    {
        return;
    }

    variant_stack_t* root = crontab;
    int* time_ptr = (int*)crontab_time;
    crontab_remove_candidate_t  remove_candidates[6] = {0};
    int remove_count = 0;

    for(int i = 0; i < 6; i++)
    {
        stack_for_each(root, time_entry_variant)
        {
            if(time_entry_variant->type == DT_PTR)
            {
                crontab_time_entry_t* e = (crontab_time_entry_t*)variant_get_ptr(time_entry_variant);
                if(e->time == *time_ptr)
                {
                    // if this is the only entry at this level, delete it and it will delete all its children recursively
                    if(e->child_array->count == 1)
                    {
                        remove_candidates[remove_count].data = time_entry_variant;
                        remove_candidates[remove_count].stack = root;
                        remove_count++;
                    }
                    else
                    {
                        printf("Time %d has more than one child: %d\n", e->time, e->child_array->count);
                        // clear all remove candidates!
                        memset(remove_candidates, 0, sizeof(remove_candidates));
                        remove_count = 0;
                    }
    
                    root = e->child_array;
                    time_ptr++;
                    break;
                }
            }
            else
            {
                const char* s = (const char*)variant_get_string(time_entry_variant);
                if(strcmp(s, scene) == 0)
                {
                    if(root->count > 1)
                    {
                        // clear all remove candidates!
                        memset(remove_candidates, 0, sizeof(remove_candidates));
                        remove_count = 0;
                    }

                    remove_candidates[remove_count].data = time_entry_variant;
                    remove_candidates[remove_count].stack = root;
                }
            }
        }
    }

    printf("Remove candidates: %d\n", remove_count);
    for(int i = remove_count; i >= 0; i--)
    {
        stack_remove(remove_candidates[i].stack, remove_candidates[i].data);
        variant_free(remove_candidates[i].data);

        if(remove_candidates[i].stack->count == 0)
        {
            printf("All stack items removed\n");
        }
    }
}

variant_stack_t*   crontab_get_scene(crontab_time_t crontab_time)
{
    if(NULL == crontab)
    {
        return NULL;
    }

    variant_stack_t* root = crontab;
    int* time_ptr = (int*)crontab_time;
    bool node_found = false;

    for(int i = 0; i < 5; i++)
    {
        stack_for_each(root, time_entry_variant)
        {
            crontab_time_entry_t* e = (crontab_time_entry_t*)variant_get_ptr(time_entry_variant);
            if(e->time == *time_ptr)
            {
                printf("Time %d was found, go to next\n", e->time);
                root = e->child_array;
                time_ptr++;
                node_found = true;
                break;
            }

            node_found = false;
        }

        if(!node_found)
        {
            printf("Scene not found!\n");
            break;
        }
    }

    if(node_found)
    {
        return root;
    }
    else
    {
        return NULL;
    }
}

