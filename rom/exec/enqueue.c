/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc: Add a node into a sorted list
    Lang: english
*/
/* I want the macros */
#define AROS_ALMOST_COMPATIBLE
#include "exec_intern.h"
#include <exec/lists.h>
#include <proto/exec.h>

/*****************************************************************************

    NAME */

	AROS_LH2I(void, Enqueue,

/*  SYNOPSIS */
	AROS_LHA(struct List *, list, A0),
	AROS_LHA(struct Node *, node, A1),

/*  LOCATION */
	struct ExecBase *, SysBase, 45, Exec)

/*  FUNCTION
	Sort a node into a list. The sort-key is the field node->ln_Pri.

    INPUTS
	list - Insert into this list. The list has to be in descending
		order in respect to the field ln_Pri of all nodes.
	node - This node is to be inserted. Note that this has to
		be a complete node and not a MinNode !

    RESULT
	The new node will be inserted before nodes with the same or lower
	priority.

    NOTES
	The list has to be in descending order in respect to the field
	ln_Pri of all nodes.

    EXAMPLE
	struct List * list;
	struct Node * node;

	node->ln_Pri = 5;

	// Sort the node at the correct place into the list
	Enqueue (list, node);

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
	26-08-95    digulla created after EXEC-Routine
	26-10-95    digulla adjusted to new calling scheme

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct Node * next;

    assert (list);
    assert (node);

    /* Look through the list */
    ForeachNode (list, node)
    {
	/*
	    Look for the first node with a lower pri as the node
	    we have to insert into the list.
	*/
	if (node->ln_Pri > next->ln_Pri)
	    break;
    }

    /* Insert the node. The situation looks like this:

	    A<->next<->B *<-node->*

	ie. next->ln_Pred points to A, A->ln_Succ points to next,
	next->ln_Succ points to B, B->ln_Pred points to next.
	ln_Succ and ln_Pred of node contain illegal pointers.
    */

    /* Let node point to B: A<->next<->B *<->node->B */
    node->ln_Pred	   = next->ln_Pred;

    /* Let B point to node */
    next->ln_Pred->ln_Succ = node;

    /* Make next point to node: A<->next->node<->B */
    next->ln_Pred	   = node;

    /* Make node point to next: A<->next<->node<->B */
    node->ln_Succ	   = next;

    AROS_LIBFUNC_EXIT
} /* Enqueue */

