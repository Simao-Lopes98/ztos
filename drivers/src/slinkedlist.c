/*
 * slinkedlist.c
 * Singly Linked List Implementation
 */

/* Includes */
#include "sllinkedlist.h"

/* Private variables */

/* Private function prototypes */

/* Public functions */
sllCtrl_t * sllCreate (void)
{
    sllCtrl_t * sll = NULL;
    sll = (sllCtrl_t *) calloc (1, sizeof(sllCtrl_t));
    if (sll == NULL)
    {
        return NULL;
    }
    
    return sll;
}

STATUS sllInsertBack (sllCtrl_t * sll, void * data)
{
    sllNode_t * nodeToAdd = NULL;

    if (sll == NULL || data == NULL)
    {
        return ERROR;
    }

    nodeToAdd = (sllNode_t *) calloc (1, sizeof(sllNode_t));
    if (nodeToAdd == NULL)
    {
        return ERROR;
    }

    nodeToAdd->data = data;
    nodeToAdd->nextNode = NULL;

    /* Its the 1st node to be inserted */
    if (sll->numberOfNodes == 0)
    {
        sll->firstNode = nodeToAdd;
        sll->lastNode = nodeToAdd;
    }
    else
    {
        sll->lastNode->nextNode = nodeToAdd;
        sll->lastNode = nodeToAdd;
    }

    sll->numberOfNodes++;
    
    return OK;
}


/*
TODO: Add more API functions
*/