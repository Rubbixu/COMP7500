/***************************************************************\
|    COM7500 Spring 2017 project 3 header                       |
\***************************************************************/

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/select.h>
#include <sys/time.h>

/***************************************************************\
|   Timestamp developed by Dr.Saad Biaz for COMP7300 Fall 2017, |
|   which is used to record performance.                        |
\***************************************************************/
typedef double Timestamp;
typedef double Period;

Timestamp StartTime;

Timestamp Now(){
  struct timeval tv_CurrentTime;
  gettimeofday(&tv_CurrentTime,NULL);
  return( (Timestamp) tv_CurrentTime.tv_sec + (Timestamp) tv_CurrentTime.tv_usec / 1000000.0-StartTime);
}

/***************************************************************\
|   Job queue are realized by linked list. Each node represents  |
|   a job, and contains vital information inside.                |
\***************************************************************/

typedef struct node {
  char *name; /* job name*/
  int prio;/* job priority*/
  int CPU_time;/* expected CPU time*/
  time_t submit_time;/* arrival time used to sort queue*/
  Timestamp submit_timestamp;/* arrival time used to evaluate performance*/
  struct node * next;/* next job node*/
  int nargc;
  char *nargv[5];
} jobInfo;


/***************************************************************\
|   Initilize new job node                                      |
\***************************************************************/
jobInfo* createJob(char *name,int prio,int CPU_time){
  jobInfo* newjob = (jobInfo*)malloc(sizeof(jobInfo));
  newjob->name = name;
  newjob->prio = prio;
  newjob->CPU_time = CPU_time;
  newjob->next = NULL;
  newjob->submit_time = time(NULL);
  newjob->submit_timestamp = Now();
  newjob->nargc = 1;
  newjob->nargv[0] = name;
  return newjob;
}

/***************************************************************\
|  Add new job node to end of queue, and return new job as      |
|  end of queue.                                                 |
\***************************************************************/
jobInfo* addJob(jobInfo *last, jobInfo *newjob) {
  last->next = newjob;
  return newjob;
}
