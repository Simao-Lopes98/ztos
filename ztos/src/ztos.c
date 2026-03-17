/*
 * ztos.c
 * ZTOS Real-Time Operating System Implementation
 */

/* Includes */
#include "ztos.h"

/* Private variables */
static sllCtrl_t * taskList = NULL;
static zTask_t * currentTask = NULL;
static zTask_t * idleTask = NULL;

/* Private function prototypes */
static void zIdleTask (void);
static STATUS createIdleTask (void);
static void setupTaskStack (zTask_t * task);

/* Public functions */
STATUS zSchedInit (void)
{
    /* 
    Set pendSV interruption level to the lowest.
    pendSV is triggered when the RTOS has to context switch.
    As to not interfere with the system and other system interrupts.
    */



    /* Start Timer for tick */
    (void) timerDrvInit(TIMER_DRV_2);
    
    /* Create SLL for task nodes */
    taskList = sllCreate ();
    if (taskList != OK)
    {
        return ERROR;
    }
    
    /* Create Idle Task */
    if (createIdleTask() != OK)
    {
        return ERROR;
    }

	(void) timerDrvEnable (TIMER_DRV_2);

    return OK;
}

STATUS zTaskCreate (char * name,
                    uint32_t stackSize,
                    zTaskHandler taskEntryFn)
{
    zTask_t * tasktoCreate = NULL;

    if (name == NULL)
    {
        return ERROR;
    }

    if (taskEntryFn == NULL)
    {
        return ERROR;
    }

    tasktoCreate = (zTask_t *) calloc (1, sizeof(zTask_t));
    if (tasktoCreate == NULL)
    {
        return ERROR;
    }

    /* Allocate stack size */
    tasktoCreate->stackPtr = calloc (stackSize, sizeof (uint32_t));
    if (tasktoCreate->stackPtr == NULL)
    {
        return ERROR;
    }

    tasktoCreate->stackSize = stackSize;
    (void) strncpy (tasktoCreate->name, name, MAX_TASK_NAME);
    tasktoCreate->entryFn = taskEntryFn;
    tasktoCreate->status = TASK_RUNNING;

    /* Fabricate entry point */
    setupTaskStack (tasktoCreate);

    if (sllInsertBack(taskList, (void *) tasktoCreate) != OK)
    {
        return ERROR;
    }
    
    return OK;
}

zTask_t * zGetTaskByName (char * name)
{
    sllNode_t * currNode = taskList->firstNode;
    zTask_t * currTask = NULL;

    if (name == NULL)
    {
        return NULL;
    }

    while (currNode != NULL)
    {
        currTask = (zTask_t*) currNode->data;

        if (strncmp (name, currTask->name, MAX_TASK_NAME) == 0)
        {
            return currTask;
        }

        currNode = currNode->nextNode;
    }

    return NULL;
}

/* 
IRQ from TIM2 serves as heart-beat.
Routine updates ticks and triggers pendSV.
*/
void TIM2_IRQHandler(void)
{
    /* Clear */
    TIM2->SR &= ~TIM_SR_UIF;

}

/* Private functions */

/* Triggers pendSV */
static zTaskYield (void)
{
    /* Trigger pendSV ISR */
    SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
}

/* Task Switcher - Routine goes through the task list */
static void ztaskSwitcher (void)
{    
    static sllNode_t *  currNode = NULL;
    sllNode_t *         startNode = NULL;
    zTask_t *           currTask = NULL;

    /* Point to the head node as the main task is the 1st to run */
    if (currNode == NULL)
    {
        currNode = taskList->firstNode;
    }

    /* Start from next task in round-robin order */
    currNode = (currNode->nextNode != NULL) ? 
                currNode->nextNode : taskList->firstNode;

    /* Remenber where the search started */
    startNode = currNode;
    
    currTask = (zTask_t *) currNode->data;

    while (currNode != startNode)
    {
        if (currTask->status == TASK_RUNNING)
        {
            /* Save the task to execute into the global var */
            currentTask = currTask;
            return;
        }

        /* Iterate */
        currNode = (currNode->nextNode != NULL) ? 
                    currNode->nextNode : taskList->firstNode;
    }
    
    /* No RUNNING task found — fallback to idle or first task */
    currentTask = idleTask;
}

/* Tick management routine - Decrements ticks from STOPPED tasks */
static void zTickManagment (void)
{
    
    sllNode_t * currNode = taskList->firstNode;
    zTask_t * currTask = NULL;

    /* Iterate each task */
    while (currNode != NULL)
    {
        currTask = (zTask_t *) currNode->data;

        if (currTask->status == TASK_STOPPED && currTask->ticks >= 0)
        {
            if (currTask->ticks == 0)
            {
                /* Mark the task as RUNNING */
                currTask->status = TASK_RUNNING;
            }
            else
            {
                /* Decrement ticks to delay */
                currTask->ticks--;
            }
            
        }
        currNode = currNode->nextNode;
    }

}

/*
Function creates a stack-ready when called by the exception.
This follows the stack on section 5.5.1 Stacking of ARM Cortex M3
TODO: The tasks should not return and have no input args. This could be improved.. To do so, LR and R0 have to be properly setup 
*/
static void setupTaskStack (zTask_t * task)
{
    uint32_t * stack = (uint32_t *) ((uint32_t *) task->stackPtr + task->stackSize);

    /* Align memory to 8 Bits */
    stack = (uint32_t *)(((uint32_t)stack) & ~0x07);

    /* Setup the PSR register with T bit set */
    * (-- stack) = 0x01000000;

    /* Setup the PC register to the function entry point */
    * (-- stack) = (uint32_t) task->entryFn;

    /* 
    Setup the LR - Tasks should not return 
    TODO: Add the feature for a task to return
    */
    * (-- stack) = 0xDEADBEEF;

    for (size_t i = 0; i < DUMMY_REGISTERS_NUM; i++)
    {
        /* Setup the GP Register (i.e. R4, R11, ...) with a dummy value */
        * (-- stack) = 0xDEADBEEF;
    }
    
    /* Save current stack pointer */
    task->currentStackPtr = stack;
}

static void zIdleTask (void)
{
    while (1)
    {
        /* Do nothing */
        __asm volatile ("nop");
        /* Never return */
    }
    
}

static STATUS createIdleTask (void)
{
    zTask_t * idleTcb = NULL;

    idleTcb = (zTask_t *) calloc (1, sizeof(zTask_t));
    if (idleTcb == NULL)
    {
        return ERROR;
    }

    /* Allocate space for idle task stack */
    idleTcb->stackPtr = (void *) calloc (   IDLE_TASK_STACK_SIZE, 
                                            sizeof (uint32_t));
    if (idleTcb->stackPtr == NULL)
    {
        return ERROR;
    }

    idleTcb->stackSize = IDLE_TASK_STACK_SIZE;
    (void) strncpy (idleTcb->name, IDLE_TASK_NAME, MAX_TASK_NAME);
    idleTcb->entryFn = zIdleTask;
    idleTcb->status = TASK_RUNNING; /* Idle task has to be ALWAYS running */

    /* Add idle task to the list */
    if (sllInsertBack (taskList, (void *) idleTcb) != OK)
    {
        return ERROR;
    }

    /* Save in the global context */
    idleTask = idleTcb;

    return OK;
}