/* ztos.h */
#ifndef ZTOS_H
#define ZTOS_H

/* Includes */
#include <stdio.h>
#include <stdlib.h>
#include <stm32f1xx.h>
#include <core_cm3.h>
#include <string.h>

#include "clock.h"
#include "app.h"
#include "timer.h"
#include "sllinkedlist.h"

/* Defines */

#define TICKS_PER_SEC 100

#define MAX_TASK_NAME 16
#define DUMMY_REGISTERS_NUM 13

#define IDLE_TASK_STACK_SIZE 256
#define IDLE_TASK_NAME "idleTask"

/* Type definitions */

/* Task Function pointer */
typedef void (* zTaskHandler) ();

typedef enum
{
    TASK_RUNNING,
    TASK_READY,
    TASK_STOPPED
}zTaskStatus_t;

typedef struct
{
    void * currentStackPtr; /* Has to be 1st element for easy access on ASM */
    void * stackPtr;        
    uint32_t stackSize;
    char name [MAX_TASK_NAME];
    zTaskHandler entryFn;
    zTaskStatus_t status;
    uint64_t ticks;
} zTask_t;

/* Public functions */
STATUS zSchedInit (void);
STATUS zTaskCreate (char * name,
                    uint32_t stackSize,
                    zTaskHandler taskEntryFn);
zTask_t * zGetTaskByName (char * name);
STATUS zTaskDelay (uint64_t ticks);

#endif /* ZTOS_H */