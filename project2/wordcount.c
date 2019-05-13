#include <stdio.h>
#include <string.h>


int wordcount(char content[], char word) {
	int counter;
	int j;

	counter = 0;

	if ((word != ' ' && word != '\n')
		&& (content[0] == ' ' || content[0] == '\n')) {
		counter++;
	}

	for (j = 1; j < strlen(content); j++) {
		if ((content[j-1] != ' ' && content[j-1] != '\n')
			&& (content[j] == ' ' || content[j] == '\n')) {
			counter++;
		}
	}
	return counter;
}
