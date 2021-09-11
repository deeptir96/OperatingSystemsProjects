#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
  //Check num args etc
  int i;
  char *inp;
  char *inputs[argc][3];
  char value[30];
  int key;

  for(i = 1; i < argc; i++) {
    printf("Argument %d: %s\n", i, argv[i]);
    int k=0;
    while( (inp = strsep(&argv[i], ",")) != NULL) {
        if(k == 3) {
//		printf("Invalid number of arguments/bad command");
		break;
		//Handle this somehow outside as well
	}
    	inputs[i][k++] = inp;
    }
    if(k > 3) {
     printf("Bad command\n");
    }
    char command = (char) inputs[i][0][0];
    switch(command) {
    	case 'p': 
	  //do something
	  key = atoi(inputs[i][1]);
	  if(inputs[i][2] == NULL) {
		  printf("Argh bad command\n");
		  break;
	  }
	  strcpy(value, inputs[i][2]);
	  if(value == NULL) {
   	    printf("Bad command\n");
	  }
	  break;
	case 'g': //do something

	  break;
	case 'c': //do something
	  break;
 	case 'd': //do something
	  break;
	case 'a': //do something
	  break;
	default: 
	  printf("Bad command\n");
	  break;
    }
  }
  return 0;
}
