/* mycode2.c: your portion of the kernel
 *
 *   	Below are procedures that are called by other parts of the kernel. 
 * 	Your ability to modify the kernel is via these procedures.  You may
 *  	modify the bodies of these procedures any way you wish (however,
 *  	you cannot change the interfaces).  
 */

#include "aux.h"
#include "sys.h"
#include "mycode2.h"

#define TIMERINTERVAL 1	// in ticks (tick = 10 msec)

/* 	A sample process table. You may change this any way you wish. 
 */

static struct {
	int valid;		// is this entry valid: 1 = yes, 0 = no
	int pid;		// process ID (as provided by kernel)
	int request;	// request CPU time
	int stride;		// L/R
	int pass;		// The accumulated running counter
} proctab[MAXPROCS];

static int tail;	//The tail of the queue or top of the stack. Used for FIFO, LIFO, ROUNDROBIN.

static int sum_of_requested_CPU;		//Total requested CPU by all processes
static int num_of_requesting_process;	//# of processes that requires certain amount of CPU time
static int num_of_process;				//# of all processes that are valid
static int full_CPU;					//Whether CPU allocation is 100% 
static int stride_for_non_requesting;	

/* 	InitSched () is called when the kernel starts up.  First, set the
 *  	scheduling policy (see sys.h). Make sure you follow the rules
 *   	below on where and how to set it.  Next, initialize all your data
 * 	structures (such as the process table).  Finally, set the timer
 *  	to interrupt after a specified number of ticks. 
 */

void InitSched ()
{
	int i;

	/* First, set the scheduling policy. You should only set it
	 * from within this conditional statement.  While you are working
	 * on this assignment, GetSchedPolicy () will return NOSCHEDPOLICY. 
	 * Thus, the condition will be true and you may set the scheduling
	 * policy to whatever you choose (i.e., you may replace ARBITRARY).  
	 * After the assignment is over, during the testing phase, we will
	 * have GetSchedPolicy () return the policy we wish to test (and
	 * the policy WILL NOT CHANGE during the entirety of a test).  Thus
	 * the condition will be false and SetSchedPolicy (p) will not be
	 * called, thus leaving the policy to whatever we chose to test
	 * (and so it is important that you NOT put any critical code in
	 * the body of the conditional statement, as it will not execute when
	 * we test your program). 
	 */
	if (GetSchedPolicy () == NOSCHEDPOLICY) {	// leave as is
		SetSchedPolicy (LIFO);	// set policy here
	}
		
	/* Initialize all your data structures here */
	for (i = 0; i < MAXPROCS; i++) {
		proctab[i].valid = 0;
	}
	
	tail = -1;

	if (GetSchedPolicy () == PROPORTIONAL) {
		sum_of_requested_CPU = 0;
		num_of_requesting_process = 0;
		num_of_process = 0;
		full_CPU = 0;
		for (i = 0; i < MAXPROCS; i++){
			proctab[i].request = 0;	//Initialize all processes unrequested CPU time
			proctab[i].pass = 0;
		}
	}

	/* Set the timer last */
	SetTimer (TIMERINTERVAL);
}


/*  	StartingProc (p) is called by the kernel when the process
 * 	identified by PID p is starting. This allows you to record the
 * 	arrival of a new process in the process table, and allocate
 *  	any resources (if necessary).  Returns 1 if successful, 0 otherwise. 
 */

int StartingProc (p)
	int p;				// process that is starting
{
	int i;

	switch (GetSchedPolicy ()) {
		case ARBITRARY:
			for (i = 0; i < MAXPROCS; i++) {
				if (! proctab[i].valid) {
					proctab[i].valid = 1;
					proctab[i].pid = p;
					return (1);
				}
			}

			DPrintf ("Error in StartingProc: no free table entries\n");
			return (0);

/*The starting and ending of processes in FIFO, LIFO, ROUNDROBIN is the same:
 Starting: put the new process into the end of the process table.
 Ending: remove the process with given pid, and move processes behind it in the 
 array one cell ahead, to ensure that positions before tail are all valid processes. */
		case FIFO:

		case LIFO:
			
		case ROUNDROBIN:
			if (tail == MAXPROCS - 1){		//Number of processes reaches MAXPROCS.
				DPrintf ("Error in StartingProc: no free table entries\n");
				return (0);
			} else {
				tail = tail + 1;
				proctab[tail].valid = 1;
				proctab[tail].pid = p;
				DoSched ();	//After adding one process, os should do scheduling again.
				return (1);
			}
			break;

		case PROPORTIONAL:
			//Find an empty cell for the new process
			for (i = 0; i < MAXPROCS; i++) {	
				if (! proctab[i].valid) {	
					proctab[i].valid = 1;
					proctab[i].pid = p;
					num_of_process++;
					//The new process should be treated as a process without request
					//If CPU is fully allocated, it doesn't matter what its stride is.
					if (!full_CPU){
						stride_for_non_requesting = 100000 * (num_of_process - num_of_requesting_process) / (100 - sum_of_requested_CPU);
						//All processes without requests should be changed in their strides (the same).
						for (int j = 0; j < MAXPROCS; j++){
							if(proctab[j].valid && proctab[j].request == 0) proctab[j].stride = stride_for_non_requesting; 
						}
					}
					return (1);
				}
			}
			//Can't find an empty cell for the new process
			DPrintf ("Error in StartingProc: no free table entries\n");
			return (0);
			
	}
	
}
			

/*   	EndingProc (p) is called by the kernel when the process
 * 	identified by PID p is ending.  This allows you to update the
 *  	process table accordingly, and deallocate any resources (if
 *  	necessary).  Returns 1 if successful, 0 otherwise. 
 */


int EndingProc (p)
	int p;				// process that is ending
{
	int i, j;

	switch (GetSchedPolicy ()) {
		case ARBITRARY:
			for (i = 0; i < MAXPROCS; i++) {
				if (proctab[i].valid && proctab[i].pid == p) {
					proctab[i].valid = 0;
					return (1);
				}
			}
			DPrintf ("Error in EndingProc: can't find process %d\n", p);
			return (0);

		case FIFO:

		case LIFO:
			
		case ROUNDROBIN:
			for (i = 0; i <= tail; i++){
				if (proctab[i].valid && proctab[i].pid == p) {	//Find the process with pid p
					proctab[i].valid = 0;
					for (j = i + 1; j <= tail; j++){			//Move processes after p one cell ahead
						proctab[j - 1].valid = 1;
						proctab[j - 1].pid = proctab[j].pid;
						proctab[j].valid = 0;
					}
					tail--;
					return (1);
				}
			}
			DPrintf ("Error in EndingProc: can't find process %d\n", p);
			return (0);

		case PROPORTIONAL:

			for (i = 0; i < MAXPROCS; i++) {
				if (proctab[i].valid && proctab[i].pid == p) {
					proctab[i].valid = 0;	
					proctab[i].pass = 0;		//Reset its pass to zero. (never running)
					num_of_process --;

					//Current process had request CPU time. 
					if (proctab[i].request) {
						num_of_requesting_process --;
						sum_of_requested_CPU -= proctab[i].request;	//return the CPU it was allocated back
						proctab[i].request = 0;			//Reset its request
						if (sum_of_requested_CPU < 100) full_CPU = 0;	//CPU allocation will not be full.
					}
					//If CPU times are not full, processes without requests should change in their stride.
					if (!full_CPU){
						stride_for_non_requesting = 100000 * (num_of_process - num_of_requesting_process) / (100 - sum_of_requested_CPU);
						for (int j = 0; j < MAXPROCS; j++){
							if (proctab[j].valid && proctab[j].request == 0) {
								proctab[j].stride = stride_for_non_requesting;
							}
						}

					}
					return (1);
				}
			}
			DPrintf ("Error in EndingProc: can't find process %d\n", p);
			return (0);
			

	}

	
}


/* 	SchedProc () is called by kernel when it needs a decision for
 * 	which process to run next. It calls the kernel function
 *  	GetSchedPolicy () which will return the current scheduling policy
 *   	which was previously set via SetSchedPolicy (policy).  SchedProc ()
 * 	should return a process PID, or 0 if there are no processes to run. 
 */

int SchedProc ()
{
	int i;
	int min_pass;

	switch (GetSchedPolicy ()) {

	case ARBITRARY:

		for (i = 0; i < MAXPROCS; i++) {
			if (proctab[i].valid) {
				return (proctab[i].pid);
			}
		}
		break;

	//Choose the process from the head of the queue if the queue is not empty.
	case FIFO:
		if (tail > -1 && proctab[0].valid) {
			return (proctab[0].pid);
		}
		break;

	//Choose the process from the tail of the queue (or top of the stack) if not empty.
	case LIFO:

		if (tail > -1 && proctab[tail].valid) {
			return (proctab[tail].pid);
		}

		break;

	//Choose the process from the head of the queue. The same as FIFO.
	case ROUNDROBIN:

		if (tail > -1 && proctab[0].valid) {
			return (proctab[0].pid);
		}

		break;

	case PROPORTIONAL:

		min_pass = 2147483647;

		//Find the process with minimal pass value.
		int process_with_min_pass = -1;

		//If CPU allocation is full, we don't choose from processes without requests.
		if (full_CPU){	
			for(i = 0; i < MAXPROCS; i++){
				if(proctab[i].valid && proctab[i].request && proctab[i].pass < min_pass){
					min_pass = proctab[i].pass;
					process_with_min_pass = i;
				}
			}
		//If CPU is not full, we should count in the processes without requests.
		}else{
			for (i = 0; i < MAXPROCS; i++){
				if(proctab[i].valid && proctab[i].pass < min_pass){
					min_pass = proctab[i].pass;
					process_with_min_pass = i;
				}
			}
		}
		if (process_with_min_pass > -1) {
			//Update the pass value of the chosen process by adding its stride.
			proctab[process_with_min_pass].pass += proctab[process_with_min_pass].stride;
			return (proctab[process_with_min_pass].pid);
		}
		break;

	}
	
	return (0);
}


/*  	HandleTimerIntr () is called by the kernel whenever a timer
 *  	interrupt occurs.  Timer interrupts should occur on a fixed
 * 	periodic basis.
 */

void HandleTimerIntr ()
{
	SetTimer (TIMERINTERVAL);

	switch (GetSchedPolicy ()) {	// is policy preemptive?

	case ROUNDROBIN:		// ROUNDROBIN is preemptive
		//Move the current running process to the back of the queue, and move others forward.
		if (tail > -1 && proctab[0].valid) {
			int current = proctab[0].pid;
			int i;
			for (i = 0; i < tail; i++) {
				proctab[i].pid = proctab[i + 1].pid;
			}
			proctab[tail].pid = current;
		}
		DoSched ();		
		break;

	case PROPORTIONAL:		// PROPORTIONAL is preemptive

		DoSched ();		// make scheduling decision
		break;

	default:			// if non-preemptive, do nothing
		break;
	}
}

/* 	MyRequestCPUrate (p, n) is called by the kernel whenever a process
 *  	identified by PID p calls RequestCPUrate (n).  This is a request for
 *   	n% of CPU time, i.e., requesting a CPU whose speed is effectively
 * 	n% of the actual CPU speed. Roughly n out of every 100 quantums
 *  	should be allocated to the calling process. n must be at least
 *  	0 and must be less than or equal to 100.  MyRequestCPUrate (p, n)
 * 	should return 0 if successful, i.e., if such a request can be
 * 	satisfied, otherwise it should return -1, i.e., error (including if
 *  	n < 0 or n > 100). If MyRequestCPUrate (p, n) fails, it should
 *   	have no effect on scheduling of this or any other process, i.e., AS
 * 	IF IT WERE NEVER CALLED.
 */

int MyRequestCPUrate (p, n)
	int p;				// process whose rate to change
	int n;				// percent of CPU time
{
	//Check if the value of n is valid
	if (n < 0 || n > 100) return (-1);
	int i;

	for(i = 0; i < MAXPROCS; i++){
		if(proctab[i].pid == p && proctab[i].valid){	//find the refered process in proctab
			if(proctab[i].request){	//The process already have CPU requests in the past
				if(sum_of_requested_CPU - proctab[i].request + n > 100) return (-1);
				else{
					sum_of_requested_CPU -= proctab[i].request;
					sum_of_requested_CPU += n;
					proctab[i].request = n;

					if (!n == 0) proctab[i].stride = 100000/n;
					else num_of_requesting_process --;					
					}
			}else {	//The process didn't have CPU requests in the past
				if (n==0) return (-1);
				if (sum_of_requested_CPU + n > 100) return (-1);
				num_of_requesting_process++;
				sum_of_requested_CPU += n;
				proctab[i].request = n;
				proctab[i].stride = 100000/n;
			}
			//Now think about those processes without requests
			if (sum_of_requested_CPU == 100) full_CPU = 1;
			else{
				stride_for_non_requesting = 100000 * (num_of_process - num_of_requesting_process) / (100 - sum_of_requested_CPU);
				for (int j = 0; j < MAXPROCS; j++){
					if(proctab[j].valid && proctab[j].request == 0)
						proctab[j].stride = stride_for_non_requesting;
				}
			}

			return (0);
		}
	}
	return (-1);
}

