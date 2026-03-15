/* slinkedlist.h */
#ifndef SLINKEDLIST_H
#define SLINKEDLIST_H

/* Includes */
#include <stdio.h>
#include <stdlib.h>
#include <stm32f1xx.h>
#include <string.h>

#include "clock.h"
#include "app.h"
#include "timer.h"

/* Defines */

/* Type definitions */

typedef struct sllNode
{
    void                * data;
    struct sllNode      * nextNode;   
}sllNode_t;


typedef struct
{
    sllNode_t   * firstNode;
    sllNode_t   * lastNode;
    uint32_t    numberOfNodes;     
}sllCtrl_t;

/* Public functions */

extern sllCtrl_t * sllCreate (void);
extern STATUS sllInsertBack (sllCtrl_t * sll, void * data);


#endif /* SLINKEDLIST_H */