#include "user_manager.h"
#include <logger.h>
#include <crc32.h>
#include <string.h>
#include <stdlib.h>

DECLARE_LOGGER(User)

variant_stack_t *user_list;

int hash_password(const char *password)
{
    return crc32(0, password, strlen(password));
}

void delete_user_entry(void *arg)
{
    user_entry_t *e = (user_entry_t *)arg;
    free(e->username);
    free(e);
}

void user_manager_init()
{
    LOG_ADVANCED(User, "Initializing user manager");
    user_list = stack_create();
    LOG_INFO(User, "User manager initialized");
}

int user_manager_get_count()
{
    return user_list->count;
}

bool user_manager_add_user(const char *username, const char *password)
{
    stack_for_each(user_list, user_entry_variant)
    {
        user_entry_t *entry = VARIANT_GET_PTR(user_entry_t, user_entry_variant);
        if (strcmp(entry->username, username) == 0)
        {
            return false;
        }
    }

    user_entry_t *new_entry = malloc(sizeof(user_entry_t));
    new_entry->username = strdup(username);
    new_entry->password = hash_password(password);

    stack_push_back(user_list, variant_create_ptr(DT_PTR, new_entry, delete_user_entry));
    return true;
}

void user_manager_add_hashed_user(const char *username, const char *password)
{
    stack_for_each(user_list, user_entry_variant)
    {
        user_entry_t *entry = VARIANT_GET_PTR(user_entry_t, user_entry_variant);
        if (strcmp(entry->username, username) == 0)
        {
            return;
        }
    }

    user_entry_t *new_entry = malloc(sizeof(user_entry_t));
    new_entry->username = strdup(username);
    sscanf(password, "%d", &new_entry->password);
    stack_push_back(user_list, variant_create_ptr(DT_PTR, new_entry, delete_user_entry));
}

void user_manager_del_user(const char *username)
{
    stack_for_each(user_list, user_entry_variant)
    {
        user_entry_t *entry = VARIANT_GET_PTR(user_entry_t, user_entry_variant);
        if (strcmp(entry->username, username) == 0)
        {
            stack_remove(user_list, user_entry_variant);
            variant_free(user_entry_variant);
            break;
        }
    }
}

bool user_manager_authenticate(const char *username, const char *password)
{
    stack_for_each(user_list, user_entry_variant)
    {
        user_entry_t *entry = VARIANT_GET_PTR(user_entry_t, user_entry_variant);
        if (strcmp(entry->username, username) == 0)
        {
            if (hash_password(password) == entry->password)
            {
                return true;
            }
            else
            {
                return false;
            }
        }
    }

    return false;
}

void user_manager_for_each_user(void (*visitor)(user_entry_t *entry, void *arg), void *arg)
{
    stack_for_each(user_list, user_entry_variant)
    {
        user_entry_t *entry = VARIANT_GET_PTR(user_entry_t, user_entry_variant);
        visitor(entry, arg);
    }
}
