#ifdef _WIN32

static char buffer[2048];

char* readline(char* prompt){
    fputs(prompt, stdout);
    fgets(buffer, 2048, stdin);
    char* copy = malloc(stlren(buffer)+1);
    strcp(cpy, buffer);
    cpy[strlen(cpy)-1] = '\0';
    return cpy;
}

void add_history(char* unused){}

#else

#include <editline/readline.h>

#ifdef __APPLE__
#include <AvailabilityMacros.h>
#if !(MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_8)
#include <editline/history.h>
#endif
#else
#include <editline/history.h>
#endif
#endif
