/* Based on the code example here:
 * https://www.daniweb.com/programming/software-development/code/216517/strings-search-and-replace
 *
 * Mostly used for stripping color from output strings.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "console.h"

char *strip_colors(const char* str) {
    char *next, *current;
    current = str;

    next = replace(current, RESET, "");
    current = next;

    next = replace(current, BLACK, "");
    free(current);
    current = next;

    next = replace(current, RED, "");
    free(current);
    current = next;

    next = replace(current, GREEN, "");
    free(current);
    current = next;

    next = replace(current, YELLOW, "");
    free(current);
    current = next;

    next = replace(current, BLUE, "");
    free(current);
    current = next;

    next = replace(current, MAGENTA, "");
    free(current);
    current = next;

    next = replace(current, CYAN, "");
    free(current);
    current = next;

    next = replace(current, WHITE, "");
    free(current);
    current = next;

    return next;
}

/* Free the return buffer yourself. */
char *replace(const char *src, const char *p_from, const char *p_to)
{
   size_t size    = strlen(src) + 1;
   size_t fromlen = strlen(p_from);
   size_t tolen   = strlen(p_to);
   char *value = malloc(size);
   char *dst = value;
   if ( value != NULL ) {
      while ( 1 ) {
         const char *match = strstr(src, p_from);
         if ( match != NULL ) {
            size_t count = match - src;
            char *temp;
            size += tolen - fromlen;
            temp = realloc(value, size);
            if ( temp == NULL )
            {
               free(value);
               return NULL;
            }
            dst = temp + (dst - value);
            value = temp;
            memmove(dst, src, count);
            src += count;
            dst += count;
            memmove(dst, p_to, tolen);
            src += fromlen;
            dst += tolen;
         }
         else {
            strcpy(dst, src);
            break;
         }
      }
   }
   return value;
}
