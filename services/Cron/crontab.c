#include "crontab.h"
#include "cron_config.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <logger.h>

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
    stack_free(e->time_list);
    free(e);
}

void delete_cron_action(void* arg)
{
    cron_action_t* action = (cron_action_t*)arg;
    variant_free(action->command);
    free(action);
}

/**
 * 
 * 
 * @author alex (11/18/2017)
 *  
 *  Find if time exists in existing entry times 
 *  
 * @param time 
 * @param existing_entry 
 * 
 * @return bool 
 */
bool crontab_has_time(int time, crontab_time_entry_t* existing_entry)
{
    stack_for_each(existing_entry->time_list, time_value_variant)
    {
        if(variant_get_int(time_value_variant) == time)
        {
            return true;
        }
    }

    return false;
}

bool    crontab_time_list_compare(variant_stack_t* left, variant_stack_t* right)
{
    if(left->count != right->count)
    {
        return false;
    }

    // If stacks are equal size, lets compare each time entry
    int index = 0;
    stack_for_each(left, left_time_entry)
    {
        variant_t* right_time_entry = stack_peek_at(right, index++);
        if(variant_get_int(left_time_entry) != variant_get_int(right_time_entry))
        {
            return false;
        }
    }

    return true;
}

/******************************************************************//**

Compare two time entries and return true if equal

***********************************************************************/
bool    crontab_time_compare(crontab_time_t left, crontab_time_t right)
{
    for(int i = 0; i < 5; i++)
    {
        if(!crontab_time_list_compare(left[i], right[i]))
        {
            return false;
        }
    }

    return true;
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

//                     array of time lists
//                                                         action type
//                                                                                 command
void    crontab_add_entry(crontab_time_t crontab_time, CronAction actionType, char* command)
{
    if(NULL == crontab)
    {
        crontab = stack_create();
    }
    variant_stack_t* root = crontab;
    variant_stack_t** time_list_ptr = (variant_stack_t**)crontab_time;
    bool node_found = false;

    for(int i = 0; i < 5; i++)
    {
        stack_for_each(root, time_entry_variant)
        {
            crontab_time_entry_t* e = (crontab_time_entry_t*)variant_get_ptr(time_entry_variant);
            
            if(crontab_time_list_compare(e->time_list, *time_list_ptr))
            {
                //printf("Time %d was found, go to next\n", e->time);
                root = e->child_array;
                time_list_ptr++;
                node_found = true;
                break;
            }

            node_found = false;
        }

        if(!node_found)
        {
            //printf("Alloc %d\n", sizeof(crontab_time_entry_t));
            crontab_time_entry_t* new_time_entry = malloc(sizeof(crontab_time_entry_t));
            new_time_entry->time_list = stack_create();

            stack_for_each((*time_list_ptr), time_value_variant)
            {
                stack_push_back(new_time_entry->time_list, variant_clone(time_value_variant));
            }

            time_list_ptr++;
            //printf("Adding time %d to root %p\n", new_time_entry->time, root);
            LOG_DEBUG(DT_CRON, "Adding time %d at position %d", variant_get_int(stack_peek_at(new_time_entry->time_list, 0)), i);
            new_time_entry->child_array = stack_create();
            stack_push_back(root, variant_create_ptr(DT_PTR, new_time_entry, &delete_time_entry));
            root = new_time_entry->child_array;
    
        }

    }

    // Here root points to the last child array which must hold the command
    //printf("Adding scene %s\n", scene);
    LOG_DEBUG(DT_CRON, "Adding scene %s", command);
    cron_action_t* action = malloc(sizeof(cron_action_t));
    action->type = actionType;
    action->command = variant_create_string(strdup(command));
    stack_push_back(root, variant_create_ptr(DT_PTR, action, &delete_cron_action));
}

typedef struct crontab_remove_candidate_t
{
    variant_t* data;
    variant_stack_t* stack;
} crontab_remove_candidate_t;

void    crontab_del_entry(crontab_time_t crontab_time, CronAction actionType, char* command)
{
    if(NULL == crontab)
    {
        return;
    }

    variant_stack_t* root = crontab;
    variant_stack_t** time_list_ptr = (variant_stack_t**)crontab_time;

    for(int i = 0; i < 5; i++)
    {
        stack_for_each(root, time_entry_variant)
        {
            //printf("Size of this level %p: %d\n", root, root->count);
            crontab_time_entry_t* e = (crontab_time_entry_t*)variant_get_ptr(time_entry_variant);
            if(crontab_time_list_compare(e->time_list, *time_list_ptr) || variant_get_int(stack_peek_at(e->time_list, 0)) == -1)
            {
                //printf("Time %d was found, go to next\n", e->time);
                if(i < 4)
                {
                    root = e->child_array;
                    time_list_ptr++;
                    break;
                }
                else
                {
                    // This is the last level - all child_arrays contains scene names!
                    stack_for_each(e->child_array, cron_action_variant)
                    {
                        cron_action_t* action = VARIANT_GET_PTR(cron_action_t, cron_action_variant);
                        //printf("Pushing back %s\n", variant_get_string(scene_name_variant));
                        if(strcmp(command, variant_get_string(action->command)) == 0 && actionType == action->type)
                        {
                            stack_remove(e->child_array, cron_action_variant);
                        }
                    }
                }
            }
        }
    }


    /*variant_stack_t* root = crontab;
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
                        //printf("Time %d has more than one child: %d\n", e->time, e->child_array->count);
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

    //printf("Remove candidates: %d\n", remove_count);
    for(int i = remove_count; i >= 0; i--)
    {
        stack_remove(remove_candidates[i].stack, remove_candidates[i].data);
        variant_free(remove_candidates[i].data);

        if(remove_candidates[i].stack->count == 0)
        {
            //printf("All stack items removed\n");
        }
    }*/
}

variant_stack_t*   crontab_get_action(current_time_value_t crontab_time)
{
    if(NULL == crontab)
    {
        return NULL;
    }

    variant_stack_t* root = crontab;
    int* time_ptr = (int*)crontab_time;
    bool node_found = false;

    variant_stack_t* scene_array = stack_create();

    for(int i = 0; i < 5; i++)
    {
        stack_for_each(root, time_entry_variant)
        {
            //printf("Size of this level %p: %d\n", root, root->count);
            crontab_time_entry_t* e = (crontab_time_entry_t*)variant_get_ptr(time_entry_variant);
            if(crontab_has_time(*time_ptr, e) || variant_get_int(stack_peek_at(e->time_list, 0)) == -1)
            {
                //printf("Time %d was found, go to next\n", e->time);
                if(i < 4)
                {
                    root = e->child_array;
                    time_ptr++;
                    node_found = true;
                    break;
                }
                else
                {
                    // This is the last level - all child_arrays contains scene names!
                    stack_for_each(e->child_array, cron_action_variant)
                    {
                        //printf("Pushing back %s\n", variant_get_string(scene_name_variant));
                        stack_push_back(scene_array, variant_clone(cron_action_variant));
                    }
                }
            }
            else
            {
                if(scene_array->count == 0)
                {
                    node_found = false;
                }
            }
        }

        if(!node_found)
        {
            //printf("Scene not found!\n");
            break;
        }
    }

    if(node_found)
    {
        //printf("Search found %d matches\n", scene_array->count);
        return scene_array;
    }
    else
    {
        stack_free(scene_array);
        return NULL;
    }
}

