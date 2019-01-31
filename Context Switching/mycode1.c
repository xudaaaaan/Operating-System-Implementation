/* mycode1.c: your portion of the kernel
 *
 *   	Below are routines that are called by other parts of the kernel. 
 * 	Your ability to modify the kernel is via these routines.  You may
 *  	modify the bodies of these routines, and add code outside or them,
 *  	in any way you wish (however, you cannot change their interfaces).  
 */

#include <strings.h>
#include "aux.h"
#include "sys.h"
#include "mycode1.h"

/* 	NewContext (p, c) will be called by the kernel whenever a new
 * 	process	is created. This is essentially a notification (which you
 *  	will make use of) that this newly created process has an ID of p,
 *   	and that its initial context is pointed to by c. Make sure you
 * 	make a copy of the contents of this context (i.e., don't copy the
 *  	pointer, but the contents pointed to by the pinter), as the pointer
 *  	will become useless after this routine returns.
 */
static CONTEXT contexts[MAXPROCS];
static int last_pid;

void NewContext (p, c)
	int p;				// ID of new process just created
	CONTEXT *c;			// initial context for this process
{
	/* your code here */
	memcpy (contexts + p - 1, c, sizeof(CONTEXT));
	return;
}

/*  	MySwitchContext (p) should cause a context switch from the calling
 *  	process to process p.  It should return the ID of the calling
 *  	process. The ID of the calling process can be determined by calling
 *  	GetCurProc (), which returns the currently running process's ID.  
 *  	The routine SwitchContext (p) is an internal working version of
 *  	context switching.  This is provided so that the kernel works
 *  	without modification, to allow the other exercises to execute and
 *  	to illustrate proper behavior. For Exercise F, the call to
 *  	SwitchContext (p) MUST be removed. 
 */
 
int MySwitchContext (p)
	int p;				// process to switch to
{
	int magic = 0;
	CONTEXT current_ctxt;
	last_pid = GetCurProc();

	SaveContext(&current_ctxt);  //save current context to variable current_ctxt
	contexts[GetCurProc() - 1] = current_ctxt; //save the current_ctxt to the global variable.

	if (magic == 1){
		return last_pid;
	}else{
		magic = 1;
		RestoreContext(&contexts[p - 1]);
	}
	
}
