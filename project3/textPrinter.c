#include <stdio.h>
#include <string.h>

/***************************************************************\
|    COM7500 Spring 2017 project 3 textPrinter. This is used    |
|    to print out help and welcome menue, which is stored as    |
|    text files.                                                |
\***************************************************************/
void textPrinter(char *txtname) {
	char c;
	FILE *txt;
	txt = fopen(txtname,"r");
	if ( txt == 0 ) {
		printf("%s is missing\n",txtname);
		} else {
	 		while ((c= fgetc(txt))!= EOF){
			 printf ("%c", c);
	 		}
			printf("\n");
			fclose(txt);
	}
}

/***************************************************************\
|    COM7500 Spring 2017 project 3 result printer. Print out    |
|    the performance metrics.                                   |
\***************************************************************/
void *resultPrinter(int tc,double tt,double ct,double wt,double tp,double xw){

	printf("Total number of job submitted: %d\n",tc);
	printf("Average turnaround time:%7.3f seconds\n",tt);
	printf("Average CPU time:	%7.3f seconds\n",ct);
	printf("Average waiting time:	%7.3f seconds\n",wt);
	printf("Maximum waiting time:	%7.3f seconds\n",xw);
	printf("Throughput:		%7.3f No./second\n\n\n",tp);
}
