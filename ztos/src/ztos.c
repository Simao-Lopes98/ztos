/*
 * ztos.c
 * ZTOS Real-Time Operating System Implementation
 */

/* Includes */
#include "ztos.h"

/* Private variables */
static sllCtrl_t * taskList = NULL;
zTask_t * volatile currentTask = NULL;
zTask_t * volatile newTaskToRun = NULL;
static zTask_t * idleTask = NULL;

/* Private function prototypes */
static void zIdleTask (void);
static STATUS zCreateIdleTask (void);
static void zSetupTaskStack (zTask_t * task);
static void zSetupPendSV (void);
static void zTaskYield (void);
static void zTickManagment (void);
static void ztaskSwitcher (void);
// static void zUpdateCurrNewTask (void);

/* Public functions */
STATUS zSchedInit (void)
{
    /* Start Timer for tick */
    (void) timerDrvInit(TIMER_DRV_2);
    
    /* Create SLL for task nodes */
    taskList = sllCreate ();
    if (taskList == NULL)
    {
        return ERROR;
    }
    
    /* Create Idle Task */
    if (zCreateIdleTask() != OK)
    {
        return ERROR;
    }

    /* Setup PendSV interrupt */
    zSetupPendSV();

    /* Set the PSP to a valid RAM stack. Using the currentTask (i.e. IdleTask) */
    __set_PSP((uint32_t)currentTask->currentStackPtr);
    
    /* From now on, use PSP in Thread mode */
    __set_CONTROL(0x02); 
    __ISB();             // Flush pipeline

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
    zSetupTaskStack (tasktoCreate);

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

STATUS zTaskDelay (uint64_t ticks)
{
    if (ticks == 0U || currentTask == NULL)
    {
    return ERROR;
    }

    /* Set to number of ticks to wait */
    currentTask->ticks = ticks;
    
    /* Set the task to STOPPED */
    currentTask->status = TASK_STOPPED;
    
    /* Trigger context switch */
    zTaskYield();

    return OK;
}

/* 
IRQ from TIM2 serves as heart-beat routine.
Routine updates ticks, switches tasks and if a new task is
ready to run, triggers PendSV.
*/
void TIM2_IRQHandler(void)
{
    /* Clear TIM2 Interrupt Flag */
    TIM2->SR &= ~TIM_SR_UIF;

    /* Update ticks on each task */
    zTickManagment ();

    ztaskSwitcher ();

    if (currentTask != newTaskToRun)
    {
        zTaskYield();
    }

}

/* 
PendSV is triggered when the RTOS has to context switch 
We need the "naked" compiler attribute to let the compiler know
not to push/pop additional registers to/from the stack.

Important - On ARM Cortex M3, when the processor invokes an exception, 
it automatically pushes the following eight registers to the SP in the 
following order: PC, xPSR, R0-R3, R12, LR.

Internal logic should avoid using registers R4 to R11 as it could corrupt the task stack.
As such, one should just use R0-R3.
*/
__attribute__((naked))
void PendSV_Handler(void)
{
    __asm volatile (/* Disable lower prio interrupts */
                    "MOVS R0, #0x55 \t\n"   /* Disables interrupt with priority */
                    "MSR BASEPRI, R0\t\n"   /* 0x55-0xFF using the CMSIS-CORE function*/
                    "ISB            \t\n"   /* Instruction Synchronization Barrier */
                    
                    /* Save the context */
                    "MRS R0, PSP                \t\n"   /* Get the Task's Stack Pointer into R0 */
                    "STMDB R0!, {R4-R11}     \t\n"      /* Push the "Software Frame" onto the task stack - This pushes registers R4 to R11 into PSP (now R0) */
                    "LDR R1, =currentTask        \t\n"  /* Get the ADDRESS pointed by currentTask. Equivalent to R1 = &currentTask */
                    "LDR R2, [R1]        \t\n"          /* Derreference currentTask. Equivalent to R2 = *R1 i.e. R2 = currentTask */
                    "STR R0, [R2]               \t\n"   /* Save the contents of R0 into the MEMORY location pointed by R2. Equivalent to *R2 = PSP */


                    /* Update current task */
                    "LDR R0, =newTaskToRun       \t\n"
                    "LDR R1, =currentTask        \t\n"
                    "LDR R2, [R0]                \t\n"
                    "STR R2, [R1]                \t\n"

                    /* Restore the context of the new task */
                    "LDR R0, =currentTask       \t\n"   /* Get the new Task's Stack Pointer into R3 */
                    "LDR R1, [R0]                \t\n"  /* R1 = currentTask */
                    "LDR R1, [R1]                \t\n"  /* R1 = currentTask->stackPtr (Actual SP!)*/
                    "LDMIA R1!, {R4-R11}        \t\n"   /* Pop registers, R1 moves up 32 bytes */
                    "MSR PSP, R1                \t\n"   /* Load the PSP into the currentTask->stackPtr */

                    /* Enable lower prio interrupts */
                    "MOVS R0, #0x00 \t\n" 
                    "MSR BASEPRI, R0\t\n"   

                    "BX LR          \t\n"   /* Exeception return */
                ); 
}

/* Private functions */

/* 
Enable and set PendSV interruption level to its lowest.
As to not interfere with the system and other system interrupts.
*/
static void zSetupPendSV (void)
{
    /* Enable IRQ */
    NVIC_EnableIRQ (PendSV_IRQn);

    /* Set PendSV prio to its minimum */
    NVIC_SetPriority(PendSV_IRQn, 0xFF);
}

/* Triggers pendSV */
static void zTaskYield (void)
{
    /* Trigger PendSV ISR */
    SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
    __DSB ();
    __ISB ();
}

/* Task Switcher - Routine goes through the task list */
static void ztaskSwitcher (void)
{    
    static sllNode_t *  currNode = NULL;
    sllNode_t *         startNode = NULL;
    zTask_t *           currTask = NULL;

    /* Only idle task in the system */
    if (taskList->numberOfNodes == 0)
    {
        newTaskToRun = idleTask;
        return;
    }

    /* Point to the head node as the main task is the 1st to run */
    if (currNode == NULL)
    {
        currNode = taskList->firstNode;
    }

    /* Remenber where the search started */
    startNode = currNode;

    do
    {
        /* Start from next task in round-robin order */
        currNode = (currNode->nextNode != NULL) ? 
                    currNode->nextNode : taskList->firstNode;
                    
        currTask = (zTask_t *) currNode->data;

        if (currTask->status == TASK_RUNNING)
        {
            /* Save the task to execute into the global var */
            newTaskToRun = currTask;
            return;
        }

    } while (currNode != startNode);
    
    /* No RUNNING task found — fallback to idle or first task */
    newTaskToRun = idleTask;
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
static void zSetupTaskStack (zTask_t * task)
{
    /* Get the top of the stack */
    uint32_t * stack = (uint32_t *)((uint8_t *)task->stackPtr + task->stackSize);

    /* Align to 8 bytes */
    stack = (uint32_t *)(((uint32_t)stack) & ~0x07);

    /* Hardware Frame (Popped by BX LR) */
    *(--stack) = 0x01000000;                // xPSR (Must have bit 24 set)
    *(--stack) = (uint32_t)task->entryFn;   // PC (Points to the actual function)
    *(--stack) = 0xFFFFFFFD;                // LR (Task Return Address)
    *(--stack) = 0x12121212;                // R12
    *(--stack) = 0x03030303;                // R3
    *(--stack) = 0x02020202;                // R2
    *(--stack) = 0x01010101;                // R1
    *(--stack) = 0x00000000;                // R0

    /* Software Frame (Popped by LDMIA {R4-R11}) */
    *(--stack) = 0x11111111;             // R11
    *(--stack) = 0x10101010;             // R10
    *(--stack) = 0x09090909;             // R9
    *(--stack) = 0x08080808;             // R8
    *(--stack) = 0x07070707;             // R7
    *(--stack) = 0x06060606;             // R6
    *(--stack) = 0x05050505;             // R5
    *(--stack) = 0x04040404;             // R4

    /* Point the TCB to the bottom of this 16-word block */
    task->currentStackPtr = stack;
}

/* Default task routine when no other task is running */
static void zIdleTask (void)
{
    while (1)
    {
        /* Do nothing */
        __asm volatile ("nop    \t\n");
        /* Never return */
    }
    
}

static STATUS zCreateIdleTask (void)
{
    zTask_t * idleTcb = NULL;

    idleTcb = (zTask_t *) calloc (1, sizeof(zTask_t));
    if (idleTcb == NULL)
    {
        return ERROR;
    }

    /* Allocate space for idle task stack */
    idleTcb->stackPtr = (void *) malloc (IDLE_TASK_STACK_SIZE);
    if (idleTcb->stackPtr == NULL)
    {
        return ERROR;
    }

    (void) memset (idleTcb->stackPtr, 0, IDLE_TASK_STACK_SIZE);

    idleTcb->stackSize = IDLE_TASK_STACK_SIZE;
    (void) strncpy (idleTcb->name, IDLE_TASK_NAME, MAX_TASK_NAME);
    idleTcb->entryFn = zIdleTask;
    idleTcb->status = TASK_RUNNING; /* Idle task has to be ALWAYS running */

    zSetupTaskStack (idleTcb);

    /* Save in the global context */
    idleTask = idleTcb;

    /* Set current Task to the idle one */
    currentTask = idleTask;

    return OK;
}