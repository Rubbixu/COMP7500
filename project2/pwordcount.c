/**
 * COMP7500 Project 2

 main program
 */

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>
#include <sys/select.h>
#include <sys/time.h>

#define READ_END	0
#define WRITE_END	1

typedef double Timestamp;
typedef double Period;

/* Timestamp developed by Dr.Saad Biaz for COMP7300 Fall 2017 */
/* Used to record performance */
Timestamp StartTime;

Timestamp Now(){
  struct timeval tv_CurrentTime;
  gettimeofday(&tv_CurrentTime,NULL);
  return( (Timestamp) tv_CurrentTime.tv_sec + (Timestamp) tv_CurrentTime.tv_usec / 1000000.0-StartTime);
}

int main(int argc, char *argv[])
{
	int BUFFER_SIZE;
	int file_size;
	FILE *file;
	char lastword;
	int write_result;
	int read_result;
	int fd1[2],fd2[2];
	int i,endcontent;
	pid_t pid;
	Timestamp StartInitialize;
	Period RunningTime;

	StartTime = Now();
	/* argc should be 2 for correct execution */
	if ( argc != 2 ) {
		printf( "Please enter a file name.\n");
	  printf( "usage: %s, <file_name>\n", argv[0]);
		return 1;
	}

	/* Configuration mechanism */
	BUFFER_SIZE = inputBuff();
	printf("You entered: %d\n", BUFFER_SIZE);

	/* Buffer less than 4 will break the program */
	if (BUFFER_SIZE < 4) {
		BUFFER_SIZE = 4;
		printf("Your buffer size is too small, set to 4.\n");
	}
	StartInitialize = Now();
	char write_content[BUFFER_SIZE];
	char read_content[BUFFER_SIZE];

	/* create the pipes */
	if (pipe(fd1) == -1) {
		fprintf(stderr,"First Pipe failed");
		return 1;
	}

	if (pipe(fd2) == -1) {
		fprintf(stderr,"Second Pipe failed");
		return 1;
	}

	/* now fork a child process */
	pid = fork();
	if (pid < 0) {
		fprintf(stderr, "Fork failed");
		return 1;
	}

	if (pid > 0) {  /* parent process */

		/* close the unused end of the pipes */
		close(fd1[READ_END]);
		close(fd2[WRITE_END]);

		printf("Process 1 is reading file '%s' now ...\n",argv[1]);

		/* Read file for process 1 */
		file = fopen(argv[1],"r");
		/* fopen returns 0, the NULL pointer, on failure */
		if ( file == 0 ) {
			printf("Could not open file: %s.\n",argv[1]);
			return 1;
		}
		struct stat st;
		fstat(fileno(file), &st);
		file_size = st.st_size;

		/* write to the first pipe using multiple transfer mechanism */
		printf("Process 1 starts sending data to Process 2 ...\n");
		endcontent = 1;
		while (endcontent) {
			for(i=0;i<BUFFER_SIZE-1;i++) {
				if (feof(file)) {
					write_content[i-1] = ' ';
					write_content[i] = 0;
					endcontent = 0;
					break;
				}
				write_content[i] = fgetc(file);
			}
			write_content[BUFFER_SIZE-1] = 0;
			write(fd1[WRITE_END], write_content, strlen(write_content)+1);
		}

		/* close the write end of the first pipe and file handler*/
		printf("Process 1 stops sending data to Process 2 ...\n");
		fclose(file);
		close(fd1[WRITE_END]);

		/* read from the second pipe and close it*/
		read(fd2[READ_END], &read_result, BUFFER_SIZE);
		close(fd2[READ_END]);
		printf("Process 1: The total number of words is %d.\n",read_result);
		/*create or append evluation file */
		RunningTime = Now() - StartInitialize;
		file = fopen("PerformanceEvaluation.txt","a");
		fprintf(file,"*******Performance Evaluation********\n");
		fprintf(file,"File Name: %s\n",argv[1]);
		fprintf(file,"File Size: %d bytes\n",file_size);
		fprintf(file,"Buffer Size: %d\n",BUFFER_SIZE);
		fprintf(file,"Total Word: %d\n",read_result);
		fprintf(file,"Running time: %7.3f\n",RunningTime);
		fprintf(file,"************End of Record***********\n\n\n\n");
		fclose(file);

		printf("Process 1 finishes writing performance evaluation.\n");
	}

	else { /* child process */
		/* close the unused end of the pipes */
		close(fd1[WRITE_END]);
		close(fd2[READ_END]);

		/* read from the first pipe */
		printf("Process 2 starts reveiving data and counting words now.\n");
		write_result = 0;
		lastword = 0;
		while (read(fd1[READ_END], read_content, sizeof(read_content))){
			write_result +=  wordcount(read_content,lastword);
			lastword = read_content[BUFFER_SIZE-2];
		}
		printf("Process 2 finishes receiving data from Process 1 ...\n");
		/* close the read end of the first pipe */
		close(fd1[READ_END]);

		/* write to the second pipe */
		printf("Process 2 is sending the result back to Process 1 ...\n");
		write(fd2[WRITE_END], &write_result,sizeof(write_result));

		/* close the write end of the second pipe */
		close(fd2[WRITE_END]);
	}

	return 0;
}
