#include "lcp.h"
#include "variant.h"
#include "string.h"
#include "stdlib.h"
#include "stdio.h"

/**
 * Compare 2 strings and return NULL-terminated string with 
 * longest common prefix 
 * 
 * @author alex (12/24/2017)
 * 
 * @param str1 
 * @param str2 
 * 
 * @return char* 
 */
char*   lcp_compare_strings(const char* str1, const char* str2)
{
    if(NULL == str1 || NULL == str2)
    {
        return NULL;
    }

    int result_index = 0;
    char* lcp_result = calloc(strlen(str1)+1, sizeof(char*));

    int n1 = strlen(str1);
    int n2 = strlen(str2);
 
    for (int i=0, j=0; i <= n1-1 && j <= n2-1; i++,j++)
    {
        if (str1[i] != str2[j])
            break;
        lcp_result[result_index++] = str1[i];
    }

    //printf("LCP result of %s and %s is %s\n", str1, str2, lcp_result);
    return lcp_result;
}



/**
 * Find longest common prefix of all strings in the list
 * 
 * @author alex (12/24/2017)
 * 
 * @param string_list 
 * @param length 
 * 
 * @return char* 
 */
char* lcp(variant_stack_t* string_list)
{
    if(NULL == string_list || string_list->count == 0)
    {
        return NULL;
    }

    //printf("LCP start with list size: %d\n", string_list->count);

    if(string_list->count == 1)
    {
        //printf("LCP Single value: %s\n", variant_get_string(stack_peek_at(string_list, 0)));
        return strdup(variant_get_string(stack_peek_at(string_list, 0)));
    }

    int mid = string_list->count / 2;

    //printf("LCP first stack from %d to %d\n", 0, mid);
    variant_stack_t* st1 = stack_splice(string_list, 0, mid);
    //printf("LCP first stack size: %d\n", st1->count);
    char* str1 = lcp(st1);
    stack_free(st1);

    //printf("LCP second stack from %d to %d\n", mid, string_list->count);

    variant_stack_t* st2 = stack_splice(string_list, mid, string_list->count);

    //printf("LCP second stack size: %d\n", st2->count);

    char* str2 = lcp(st2);
    stack_free(st2);


    //printf("Str1: %s, Str2: %s\n", str1, str2);

    char* result = lcp_compare_strings(str1, str2);
    free(str1);
    free(str2);
    return result;
}
