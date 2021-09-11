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
  FILE *fp;

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
    int ctr;
    printf("Inputs\n");
    for(ctr = 0; ctr < 3; ctr++) {
	    printf("input %d = %s ", ctr, inputs[i][ctr]);
    }
    printf("\n");
    if(k > 3) {
     printf("Bad command\n");
    }
    char command = (char) inputs[i][0][0];
    switch(command) {
    	case 'p': 
	  //put/update key value pair into map
	  key = atoi(inputs[i][1]);
	  printf("Input [2] = %s\n", inputs[i][2]);
	  if(inputs[i][2] == NULL) {
		  printf("Put arghh bad command\n");
		  break;
	  }
	  strcpy(value, inputs[i][2]);
	  if(value == NULL) {
   	    printf("Put Bad command\n");
	    break;
	  }
	  fp = fopen("key_value.csv", "a");
//	  char finalString = inputs[i][1];
//	  strcat(finalString, ",");
//	  strcat(finalString, inputs[i][2]);
  //        printf("Helloooofinal = %s\n", finalString);
	  printf("Helloooo inputs[i][1] = %s\n", inputs[i][1]);
          printf("Helloooo inputs[i][2] = %s\n", inputs[i][2]);
	  fprintf(fp, "%s,", inputs[i][1]);
          fprintf(fp, "%s \n", inputs[i][2]);
	  fclose(fp);
	  break;
	case 'g': 
	  //do something
	  key = atoi(inputs[i][1]);
	  if(inputs[i][2] != NULL) {
	  	printf("Get Argh bad command\n");
                break;
	  }
	  break;
	case 'c': 
	  //do something
	  key = atoi(inputs[i][1]);
          if(inputs[i][1] !=NULL || inputs[i][2] != NULL) {
                printf("Clear Argh bad command\n");
                break;
          }
	  break;
 	case 'd': 
	  //do something
	  key = atoi(inputs[i][1]);
          if(inputs[i][2] != NULL) {
                printf("DeleteArgh bad command\n");
                break;
          }
	  break;
	case 'a': 
	  //do something
	  if(inputs[i][1] !=NULL || inputs[i][2] != NULL) {
                printf("All argh bad command\n");
                break;
          }
	  break;
	default: 
	  printf("Bad command\n");
	  break;
    }
  }
  return 0;
}
