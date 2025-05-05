
#include "webserv.hpp"

unsigned int    findRnrn(const char *s, unsigned int sz)
{

    for (unsigned int i = 0; i < (sz - 3);  i ++)
    {
        if (s[i] == '\r')
        {
            if (!strncmp(&s[i], "\r\n\r\n", 4))
                return (i);
        }
    }
    return (0);
}

int get_time(struct timeval *initial_time)
{
    struct timeval  currtimeval;

    gettimeofday(&currtimeval, NULL);
    return (currtimeval.tv_sec - initial_time->tv_sec);
}

