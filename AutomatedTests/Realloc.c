#include "Test.h"

#include <MacMemory.h>
#include <stdlib.h>
#include <string.h>

enum
{
    initialSize = 64 * 1024,
    grownSize = 128 * 1024,
    blockerSize = 8 * 1024,
    blockerCount = 96,
    releasedBlockers = (grownSize / blockerSize) + 4,
    freeMemoryTolerance = 1024
};

int main(void)
{
    Size freeBefore = FreeMem();
    unsigned char *allocation = malloc(initialSize);
    void *blockers[blockerCount];
    unsigned char *grown;
    Size freeAfter;
    int failure = 0;
    int numBlockers = 0;
    int ok = allocation != NULL;
    int i;

    memset(blockers, 0, sizeof(blockers));

    if(!ok)
        failure = 1;

    if(ok)
    {
        for(i = 0; i < initialSize; ++i)
            allocation[i] = (unsigned char)(i * 37 + 11);

        while(numBlockers < blockerCount &&
            (blockers[numBlockers] = malloc(blockerSize)) != NULL)
        {
            memset(blockers[numBlockers], 0x5a, blockerSize);
            ++numBlockers;
        }

        if(numBlockers <= releasedBlockers)
        {
            ok = 0;
            failure = 6;
        }

        for(i = numBlockers - releasedBlockers; i < numBlockers; ++i)
        {
            if(i >= 0)
            {
                free(blockers[i]);
                blockers[i] = NULL;
            }
        }

        grown = realloc(allocation, grownSize);
        if(!grown)
        {
            ok = 0;
            failure = 2;
        }
        else if(grown == allocation)
        {
            ok = 0;
            failure = 3;
        }

        if(grown)
        {
            for(i = 0; ok && i < initialSize; ++i)
            {
                if(grown[i] != (unsigned char)(i * 37 + 11))
                {
                    ok = 0;
                    failure = 4;
                }
            }
            free(grown);
        }
        else
        {
            free(allocation);
        }

    }
    else
    {
        free(allocation);
    }

    for(i = 0; i < numBlockers; ++i)
        free(blockers[i]);

    freeAfter = FreeMem();
    if(ok && freeAfter + freeMemoryTolerance < freeBefore)
    {
        ok = 0;
        failure = 5;
    }

    if(ok)
    {
        TEST_LOG_OK();
    }
    else
    {
        char result[3];
        result[0] = 'N';
        result[1] = (char)('0' + failure);
        result[2] = '\0';
        TEST_LOG_SIZED(result, 2);
    }

    return ok ? 0 : 1;
}
