#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include "utilities.h"

functioncast_t
functioncast(void*generic) {
    functioncast_t* function = (functioncast_t*)&generic;
    return *function;
}

int
clamp(int value, int lbnd, int ubnd)
{
    if(value < lbnd)
        return lbnd;
    else if(value > ubnd)
        return ubnd;
    else
        return value;
}
