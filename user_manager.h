#include <stack.h>

typedef struct user_entry_t
{
    char* username;
    int   password;
} user_entry_t;

void    user_manager_init();
int     user_manager_get_count();
bool    user_manager_add_user(const char* username, const char* password);
void    user_manager_add_hashed_user(const char* username, const char* password);
void    user_manager_del_user(const char* username);
bool    user_manager_authenticate(const char* username, const char* password);
void    user_manager_for_each_user(void (*visitor)(user_entry_t* entry, void* arg), void* arg);

