#pragma once

#define CHECK_RB(cond, msg) \
    if(!(cond)) \
    { \
        perror(msg); \
        return false; \
    }

#define CHECK_RV(cond, msg) \
    if(!(cond)) \
    { \
        perror(msg); \
        return; \
    }

#define CHECK_RI(cond, msg) \
    if(!(cond)) \
    { \
        perror(msg); \
        return -1; \
    }