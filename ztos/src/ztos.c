/*
 * ztos.c
 * ZTOS Real-Time Operating System Implementation
 */

/* Includes */
#include "ztos.h"

/* Private variables */

/* Private function prototypes */

/* Public functions */
STATUS zSchedInit ()
{
    /* Start Timer for tick */

    /* Create SLL for task nodes */

    /* Create Idle Task */


}

STATUS zTaskCreate (char * name,
                    uint32_t stackSize,
                    zTaskHandler taskEntryFn)
{
    if (name == NULL)
    {
        return ERROR;
    }

    if (taskEntryFn == NULL)
    {
        return ERROR;
    }

    /* Allocate stack size */

    /* Fabricate entry point */

    /* Insert task on list node */
    
    return OK;
}