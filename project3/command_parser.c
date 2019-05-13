/***************************************************************\
|    COM7500 Spring 2017 project 3 command parser. Simply       |
|    store input in char array. The decision will be made in    |
|    main.                                                      |
\***************************************************************/

#include <stdio.h>
#include <string.h>

int command_parser(char target[10][20]) {
	int n=0, j=0;
	char base;
	while((base = getchar()) != '\n'){
		if(base!=' '){
			target[n][j++]=base;
		}
		else{
			target[n][j++]='\0';
			n++;
			j=0;
		}
	target[n][j] = '\0';
	}
	return n+1;
}
