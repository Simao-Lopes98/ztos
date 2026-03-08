/* ztos.h */
#ifndef ZTOS_H
#define ZTOS_H

/* Includes */
#include <stdio.h>
#include <stm32f1xx.h>
#include <string.h>
#include "clock.h"
#include "app.h"

/* Defines */

#define MAX_TASK_NAME 16

/* Type definitions */

/* Task Function pointer */
typedef void (* zTaskHandler) ();

typedef enum
{
    TASK_RUNNING,
    TASK_STOPPED
}zTaskStatus_t;

typedef struct
{
    void * stackPtr;
    uint32_t stackSize;
    char name [MAX_TASK_NAME];
    zTaskHandler entryFn;
    zTaskStatus_t status;
    uint64_t ticks;
} zTask_t;
 
/* Private variables */

/* Private function prototypes */

/* Public functions */


#endif /* ZTOS_H */