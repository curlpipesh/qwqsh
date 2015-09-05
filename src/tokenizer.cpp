#include <cstring>
#include <iostream>
#include <string>
#include <vector>

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "conf.hpp"
#include "colors.hpp"

const char* trim(const char* str);

std::vector<std::string> split(const char* inStr, char c = ' ') {
    const char* str = trim(inStr);
    std::vector<std::string> result;
    do {
        const char *begin = str;
        while(*str != c && *str) {
            str++;
        }
        result.push_back(std::string(begin, str));
    } while(0 != *str++);
    if(strncmp(inStr, str, strlen(inStr)) == 0) {
        // Can't actually free a const, so have to cast like this
        free((char*)str);
    }
    return result;
}

// See http://stackoverflow.com/a/122974 - My original implementation of this
// wouldn't shift the string back to be the original pointer, which caused 
// stuff to break. 
const char *trim(const char *origStr) {
    char* str = (char*) origStr;
    size_t len = 0;
    char *frontp = str;
    char *endp = NULL;

    if( str == NULL ) {
        return NULL;
    }
    if( str[0] == '\0' ) {
        return str;
    }

    len = strlen(str);
    endp = str + len;

    /* Move the front and back pointers to address the first non-whitespace
     * characters from each end.
     */
    while( isspace(*frontp) ) {
        ++frontp;
    }
    if( endp != frontp ) {
        while( isspace(*(--endp)) && endp != frontp ) {}
    }

    if( str + len - 1 != endp )
        *(endp + 1) = '\0';
    else if( frontp != str &&  endp == frontp )
        *str = '\0';

    /* Shift the string so that it starts at str so that if it's dynamically
     * allocated, we can still free it on the returned pointer.  Note the reuse
     * of endp to mean the front of the string buffer now.
     */
    endp = str;
    if( frontp != str ) {
        while( *frontp ) {
            *endp++ = *frontp++;
        }
        *endp = '\0';
    }


    return (const char*) str;
}
