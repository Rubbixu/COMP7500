#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>
#include "JobQueue.h"
/***************************************************************\
|    COM7500 Spring 2017 project 3 AuburnBatch. This file       |
|    containts all functions related to pthread. Global         |
|    variables are involved in them so I decided not to         |
|    write them in separate files.                              |
\***************************************************************/

/* declare global variables and initialize some of them*/

/* declare queue head and end */
jobInfo *head;
jobInfo *last;

/* declare condition vaiable and mutex */
pthread_mutex_t queue_mutex;
pthread_cond_t queue_not_empty;
pthread_cond_t queue_is_empty;
pthread_cond_t schedule_request;

/* initialize availalbe policies and default policy */
char *fcfs = "FCFS";
char *sjf = "SJF";
char *priority = "priority";
char *policy = "FCFS";

/* declare performance metrics */
Timestamp arrival_timestamp;
Timestamp start_timestamp;
Timestamp finish_timestamp;
Period avg_turnaround;
Period avg_wait;
Period max_wait;
Period avg_CPU;
double througput;
int expected_time = 0;

/* initialize queue contuers */
int queue_count = 0;
int total_count = 0;

/* initialize program flags */
int current_work = 0;
int termination = 0;
int reschedule = 0;

/* declare input storage base, which is essentially argv and argc*/
char command[10][20];
int input_n;

/*command table */
static struct {
	const char *name;
	char *function;
} cmdtable[] = {
	{"quit","quit"},
	{"q","quit"},
	{"h","help"},
	{"?","help"},
	{"help","help"},
	{"ls","list"},
	{"list","list"},
	{"run","run"},
	{"r","run"},
	{"test","test"},
	{"t","test"},
	{"fcfs","FCFS"},
	{"FCFS","FCFS"},
	{"sjf","SJF"},
	{"SJF","SJF"},
	{"priority","priority"},
	{"prio","priority"}
};
/***************************************************************\
|    COM7500 Spring 2017 project 3 showlist. Print out          |
|    current policy, job numbers and all job information:       |
|    name, expected CPU time, priority, arrival time, status.   |
\***************************************************************/
void *showlist() {
	pthread_mutex_lock(&queue_mutex);
	jobInfo *cursor = head->next;/*first jobs in head's next*/
	printf("Total number of jobs in the queue: %d\n",queue_count);
	printf("Scheduling Policy:%s\n", policy);
	printf("Name	CPU_Time	Pri	arrival_time	Progress\n");
	int i;
	for(i = 0;i<queue_count;i++) {
		/*until print out every job, same as cursor->next = NULL*/
		printf("%s	",cursor->name);
		printf("%d		",cursor->CPU_time);
		printf("%d	",cursor->prio);
		/*print out time in a specific form*/
		struct tm *ptm = localtime(&cursor->submit_time);
		printf("%02d:%02d:%02d	", ptm->tm_hour, ptm->tm_min, ptm->tm_sec);
		if (i==0) {
			printf("RUNNING\n"); /*the first job will always be running*/
		}else {
			printf("WAITING\n");
		}
		cursor = cursor->next; /*move on to next job*/
	}
	pthread_mutex_unlock(&queue_mutex);
	pthread_exit (NULL);
}


/***************************************************************\
|    COM7500 Spring 2017 project 3 jobsubmit. Its functionality |
|    is separated from scheduler: it will only submit job info  |
|    into the queue, and let scheduler do the sorting.          |
\***************************************************************/
void *jobSubmit(void *nothing) {
	/*parse the input*/
	char *name = command[1];
	int CPU_time = atoi(command[2]);
	int prio = 0;
	if (input_n == 4){/*priority is optional*/
		prio = atoi(command[3]);
	}
	/*create new job node based on job info, and add it to the end of queue*/
	jobInfo *newjob = createJob(name,prio, CPU_time);
	pthread_mutex_lock(&queue_mutex);/*modifying job queue, mutex lock*/
	last = addJob(last, newjob);
	queue_count++;
	total_count++;
	pthread_cond_signal(&queue_not_empty);/*signal dispatcher*/
	printf("Job %s was submitted\n",name);
	printf("Total number of jobs in the queue: %d\n",queue_count);
	reschedule = 1;
	pthread_cond_signal(&schedule_request);/*signal scheduler*/
	expected_time += CPU_time;
	pthread_mutex_unlock(&queue_mutex);
	printf("Expected waiting time: %ds\n",expected_time);
	printf("Scheduling Policy:%s\n", policy);
  pthread_exit (NULL);
}

/***************************************************************\
|    COM7500 Spring 2017 project 3 scheduler. This program will |
|    do a new schedule whenever user submit a new job or issue  |
|    a policy change command.                                   |
\***************************************************************/
void *scheduler(void *nothing){
	while (1){/*always running*/
		pthread_mutex_lock(&queue_mutex);
		/* wait for the rechedule request signal */
		while (reschedule == 0 && termination == 0) {
			pthread_cond_wait(&schedule_request,&queue_mutex);
		}
		/*if user decided to quit, exit program*/
		if (termination == 1) {
			pthread_mutex_unlock(&queue_mutex);
			pthread_exit(NULL);
		}
		/*insertion sort based on differet values*/
		int i,j;
		jobInfo *temp1,*temp2,*sub_cursor;
		jobInfo *main_cursor = head->next;
		if (strcmp(policy,fcfs) == 0){/*arrival time*/
			for (i=0;i<queue_count-2;i++){
				temp1 = main_cursor;
				sub_cursor = main_cursor->next;
				/* find i's earliest job in the queue besides current running one*/
				for (j=i+1;j<queue_count-1;j++){
					if (difftime(sub_cursor->next->submit_time,temp1->next->submit_time)<0){
						temp1 = sub_cursor;
					}
					sub_cursor = sub_cursor->next;
				}
				/*swap earliest with current leftmost,closest to head*/
				temp2 = temp1->next;
		    temp1->next = temp1->next->next;
		    temp2->next = main_cursor->next;
		    main_cursor->next = temp2;
				/*shift leftmost cursor to right, as the base for next iteration*/
				main_cursor = main_cursor->next;
			}
		}
		if (strcmp(policy,sjf) == 0){/*expected CPU time*/
			for (i=0;i<queue_count-2;i++){
				temp1 = main_cursor;
				sub_cursor = main_cursor->next;
				for (j=i+1;j<queue_count-1;j++){
					if (sub_cursor->next->CPU_time < temp1->next->CPU_time){
						temp1 = sub_cursor;
					}
					if (sub_cursor->next->CPU_time ==temp1->next->CPU_time){
						if (difftime(sub_cursor->next->submit_time,temp1->next->submit_time)<0){
							temp1 = sub_cursor;/*arrival time is tie breaker*/
						}
					}
					sub_cursor = sub_cursor->next;
				}
				temp2 = temp1->next;
		    temp1->next = temp1->next->next;
		    temp2->next = main_cursor->next;
		    main_cursor->next = temp2;
				main_cursor = main_cursor->next;
			}
		}
		if (strcmp(policy,priority) == 0){/*priority*/
			for (i=0;i<queue_count-2;i++){
				temp1 = main_cursor;
				sub_cursor = main_cursor->next;
				for (j=i+1;j<queue_count-1;j++){
					if (sub_cursor->next->prio > temp1->next->prio){
						temp1 = sub_cursor;
					}
					if (sub_cursor->next->prio == temp1->next->prio){
						if (difftime(sub_cursor->next->submit_time,temp1->next->submit_time)<0){
							temp1 = sub_cursor;/*arrival time is tie breaker*/
						}
					}
					sub_cursor = sub_cursor->next;
				}
				temp2 = temp1->next;
		    temp1->next = temp1->next->next;
		    temp2->next = main_cursor->next;
		    main_cursor->next = temp2;
				main_cursor = main_cursor->next;
			}
		}
		/* after sorting, find the new queue end */
		last = head;
		for (i=0;i<queue_count;i++){
			last = last->next;
		}
		reschedule = 0;/* set reschedule flag to 0. Schedule request fullfill */
		pthread_mutex_unlock(&queue_mutex);
	}
}

/***************************************************************\
|    COM7500 Spring 2017 project 3 scheduler. This program will |
|    process jobs using execv.  |
|    a policy change command.                                   |
\***************************************************************/
void *dispatcher(void *nothing) {
	char **myargs;
	Period job_wait;
	while (1){/*always running*/
		pthread_mutex_lock(&queue_mutex);
		/* wait for queue is not empty signal */
		while (queue_count == 0 && termination == 0) {
			pthread_cond_wait(&queue_not_empty,&queue_mutex);
		}
		/*if user decided to quit and queue is empty, exit program*/
		if (termination == 1 && queue_count == 0) {
			pthread_mutex_unlock(&queue_mutex);
			pthread_exit(NULL);
		}
		/*get input for job*/
		myargs = head->next->nargv;
		myargs[head->next->nargc] = NULL;
		arrival_timestamp = head->next->submit_timestamp;/*record job arrival time*/
		pthread_mutex_unlock(&queue_mutex);
		current_work = 1;/*flag there is a job in process*/
		start_timestamp = Now();/*record job start time*/
		/*use fork() to run execv*/
		pid_t pid;
		pid = fork();
		if (pid < 0) {
			printf("Fork failed\n");
			pthread_exit(NULL);
		}
		if (pid > 0) {
			wait(NULL);
		}
		else{
			execv(myargs[0],myargs);
		}
		finish_timestamp = Now();/*record job finish time*/
		/* calculate performance metrics */
		avg_turnaround += finish_timestamp - arrival_timestamp;
		job_wait = start_timestamp - arrival_timestamp;
		avg_wait += job_wait;
		if (job_wait > max_wait) {
			max_wait = job_wait;
		}
		avg_CPU += finish_timestamp - start_timestamp;
		current_work = 0;/*flag there is no job in process*/
		pthread_mutex_lock(&queue_mutex);
		expected_time -= head->next->CPU_time;
		head->next = head->next->next;/* set dispatcher to next job*/
		queue_count--;/*job will remain in queue until its finish processing*/
		/*if there is no job in queue, set end of queue to head*/
		if (queue_count == 0) {
			last = head;
			pthread_cond_signal(&queue_is_empty);
		}
		pthread_mutex_unlock(&queue_mutex);
	}
}

/***************************************************************\
|    COM7500 Spring 2017 project 3 automated_evaluator. This    |
|    program cooperates with a benchmark taking input as time,  |
|    and simulates user's input.                                |
\***************************************************************/
void *automated_evaluator(void *nothing){
	char *benchmark = command[1];/* benchmark job name*/
  /* set policy*/
	char *policy_ev = command[2];
	if (strcmp(policy_ev,fcfs) == 0){
			policy = fcfs;
		}else {
			if (strcmp(policy_ev,sjf) == 0){
				policy = sjf;
			}else {
				if (strcmp(policy_ev,priority) == 0){
					policy = priority;
				}
			else {
				printf("Invalid policy. Please check help and try again.\n");
				pthread_exit(NULL);/*if input poicy is not valid*/
			}
		}
	}
	char *arrival_rate = command[3];
	int interval;
	/* simulate different arrival rate */
	if (strcmp(arrival_rate,"low") == 0){
			interval = 10;
		}else {
			if (strcmp(arrival_rate,"medium") == 0){
				interval = 5;
			}else {
				if (strcmp(arrival_rate,"high") == 0){
					interval = 2;
				}
			else {
				printf("Invalid arrival rate. Please check help and try again.\n");
				pthread_exit(NULL);/*if input arrival rate is not valid*/
			}
		}
	}
	int num_job = atoi(command[4]);
	int prio_range = atoi(command[5]);
	int min_CPU_time = atoi(command[6]);
	int max_CPU_time = atoi(command[7]);
	if (min_CPU_time > max_CPU_time){
		printf("Invalid CPU time range. Please check help and try again.\n");
		pthread_exit(NULL);/*if input CPU time is not valid*/
	}
	pthread_mutex_lock(&queue_mutex);
	printf("Starting evaluation. Please wait for current queue to finish.\n");
	/* evaluation will start when there is no job in curret queue */
	while (queue_count > 0){
		pthread_cond_wait(&queue_is_empty,&queue_mutex);
	}
	pthread_mutex_unlock(&queue_mutex);
	printf("Evaluation started. Please wait for the result.\n");
	int i;
	int rand_prio;
	int rand_CPU_time;
	char CPU_time_arg[num_job][30];
	/* use same seed to generate same sequence of jobs for different policies for comparison*/
	srand (1);
	/* performance need to be recorded for both general and automated reports*/
	/* a copy of previous information should be stored.                      */
	Period temp_turnaround_a = avg_turnaround;
	avg_turnaround = 0;
	Period temp_CPU_a = avg_CPU;
	avg_CPU = 0;
	Period temp_wait_a = avg_wait;
	avg_wait = 0;
	Period temp_max_wait = max_wait;
	max_wait = 0;
	for (i = 0;i<num_job;i++){
		/*generate random job info based on user input*/
		rand_prio = rand() % (prio_range + 1 - 0) + 0;
		rand_CPU_time = rand() % (max_CPU_time - min_CPU_time + 1) + min_CPU_time;
		jobInfo *newjob = createJob(benchmark,rand_prio, rand_CPU_time);
		/* benchmark needs input, which will be feed to dispatcher */
		sprintf(CPU_time_arg[i], "%d", rand_CPU_time);
		newjob->nargv[1] = CPU_time_arg[i];
		newjob->nargc += 1;
		/*rest part is similar to job submit*/
		pthread_mutex_lock(&queue_mutex);
		last = addJob(last, newjob);
		queue_count++;
		total_count++;
		pthread_cond_signal(&queue_not_empty);
		reschedule = 1;
		pthread_cond_signal(&schedule_request);
		expected_time += rand_CPU_time;
		pthread_mutex_unlock(&queue_mutex);
		sleep(interval);/*simulate job arrival rate, this should be adjustable*/
	}
	/* for report, wait all jobs to finish*/
	printf("All job submitted. Please wait for the result.\n");
	pthread_mutex_lock(&queue_mutex);
	while (queue_count > 0){
		pthread_cond_wait(&queue_is_empty,&queue_mutex);
	}
	Period temp_turnaround_b = avg_turnaround;
	Period temp_CPU_b = avg_CPU;
	Period temp_wait_b = avg_wait;
	avg_turnaround /= num_job;
	avg_CPU /= num_job;
	avg_wait /= num_job;
	througput = 1/avg_turnaround;
	printf("------------REPORT-------------\n");
	printf("Scheduling Policy:%s\n", policy);
	resultPrinter(num_job,avg_turnaround,avg_CPU,avg_wait,througput,max_wait);
	/*add benchmark job performance to global performance metrics*/
	avg_turnaround = temp_turnaround_a + temp_turnaround_b;
	avg_CPU = temp_CPU_a + temp_CPU_b;
	avg_wait = temp_wait_a + temp_wait_b;
	if (temp_max_wait > max_wait) {
		max_wait = temp_max_wait;
	}
	pthread_mutex_unlock(&queue_mutex);
	pthread_exit(NULL);
}

/***************************************************************\
|    COM7500 Spring 2017 project 3 main program. 				     		|
\***************************************************************/
int main(int argc,char* argv[]) {

	StartTime = Now();
	char *headname = "queue_head";
	/*initialize head and end of queue */
	head = createJob(headname,0,0);
	last = head;

	textPrinter("welcome.txt");

	int i;
	char *c_check;


	max_wait = 0;

	pthread_t threads[3];
  pthread_attr_t attr;

  /* initialize mutex and condition variable objects */
  pthread_mutex_init(&queue_mutex, NULL);
  pthread_cond_init(&queue_not_empty, NULL);
	pthread_cond_init(&queue_is_empty, NULL);
	pthread_cond_init(&schedule_request, NULL);

  /* for portability, explicitly create threads in a joinable state */
  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

	/*the following two threads will run until user quit*/
	pthread_create(&threads[0], &attr, dispatcher, NULL);/*create dispatcher thread*/
	pthread_create(&threads[1], &attr, scheduler, NULL);/*create scheduler thread*/


	while(1){
		printf(">");
		input_n=command_parser(command);
		c_check = "nothing";
		for (i=0;cmdtable[i].name; i++) {
			if (*cmdtable[i].name && !strcmp(command[0],cmdtable[i].name)){
				c_check = cmdtable[i].function;
				break;
			}
		}
		/* compare parsed command with specific string to use different functions,*/
		/* this is not as elegant as Dr'Qin's methond but similar, use a          */
		/* command table to check which functionality shoule be applied					  */
		/* if quit, end current loop and enter termination process */
		if (strcmp(c_check, "quit") == 0) {
			termination = 1;
			break;
		}
		/* if help, print help menu */
		if (strcmp(c_check, "help") == 0) {
			textPrinter("help.txt");
			continue;
		}
		/* if run, submit job */
		if (strcmp(c_check,"run") == 0){
			if (input_n == 4 || input_n == 3) {
				pthread_create(&threads[2], &attr, jobSubmit, NULL);
				pthread_join(threads[2], NULL);
			} else {
				printf("Arguments not supported. Please check help and try again.\n");
			}
			continue;
		}
		/* if list, print job info */
		if (strcmp(c_check,"list") == 0){
				pthread_create(&threads[2], &attr, showlist, NULL);
				pthread_join(threads[2], NULL);
				continue;
		}
		/* if policy, change queue to corresponding policy */
		if (strcmp(c_check,fcfs) == 0){
				policy = fcfs;
				reschedule = 1;
				pthread_cond_signal(&schedule_request);
				int remain = queue_count - current_work;
				printf("Scheduling policy is switched to %s. All the %d waiting jobs have been rescheduled.\n",policy,remain);
				continue;
		}
		if (strcmp(c_check,sjf) == 0 ){
				policy = sjf;
				reschedule = 1;
				pthread_cond_signal(&schedule_request);
				int remain = queue_count - current_work;
				printf("Scheduling policy is switched to %s. All the %d waiting jobs have been rescheduled.\n",policy,remain);
				continue;
		}
		if (strcmp(c_check,priority) == 0){
				policy = priority;
				reschedule = 1;
				pthread_cond_signal(&schedule_request);
				int remain = queue_count - current_work;
				printf("Scheduling policy is switched to %s. All the %d waiting jobs have been rescheduled.\n",policy,remain);
				continue;
		}
		/* if test, run automated_evaluator */
		if (strcmp(c_check,"test") == 0){
				if (input_n != 8) {
					printf("Arguments not supported. Please check help and try again.\n");
		}		else{
					pthread_create(&threads[2], &attr, automated_evaluator, NULL);
					pthread_join(threads[2], NULL);
		}
				continue;
	}
		printf("Commands not supported. Please check help and try again.\n");
	}
	/*signal dispatcher and scheduler to exit*/
	pthread_cond_signal(&queue_not_empty);
	pthread_cond_signal(&schedule_request);
	printf("Finishing jobs in queue. It may take a while. Please wait.\n");
	pthread_join(threads[0], NULL);
	pthread_join(threads[1], NULL);

	/*free attr, cv and mutex */
	pthread_attr_destroy(&attr);
	pthread_mutex_destroy(&queue_mutex);
	pthread_cond_destroy(&queue_not_empty);
	pthread_cond_destroy(&queue_is_empty);
	pthread_cond_destroy(&schedule_request);

	/*gernal performance metrics report*/
	avg_turnaround /= total_count;
	avg_CPU /= total_count;
	avg_wait /= total_count;
	througput = 1/avg_turnaround;
	printf("------------REPORT-------------\n");
	resultPrinter(total_count,avg_turnaround,avg_CPU,avg_wait,througput,max_wait);

	pthread_exit (NULL);
}
