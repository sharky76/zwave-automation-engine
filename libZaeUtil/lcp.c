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
char*   lcp_compare_strings(const char* str1, const char* str2, char* result)
{
    if(NULL == str1 || NULL == str2)
    {
        return NULL;
    }

    int result_index = 0;
    int n1 = strlen(str1);
    int n2 = strlen(str2);

    //char* lcp_result = calloc(n1+1, sizeof(char));
 
    for (int i=0, j=0; i < n1 && j < n2; i++,j++)
    {
        if (str1[i] != str2[j])
            break;
        result[result_index++] = str1[i];
    }

    result[result_index] = 0;

    //printf("LCP result of %s and %s is %s\n", str1, str2, lcp_result);
    return result;
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

    if(string_list->count == 1)
    {
        return strdup(variant_get_string(stack_peek_at(string_list, 0)));
    }
    
    stack_iterator_t* it = stack_iterator_begin(string_list);
    const char* prefix = variant_get_string(stack_iterator_data(it));
    it = stack_iterator_next(it);
    char result[512];
    memcpy(result, prefix, strlen(prefix));

    while(!stack_iterator_is_end(it))
    {
        const char* next = variant_get_string(stack_iterator_data(it));
        lcp_compare_strings(result, next, result);
        it = stack_iterator_next(it);
    }
    
    return strdup(result);
}
