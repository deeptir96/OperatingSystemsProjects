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
  FILE *fpr;
  FILE *fp2;
  FILE *fp;
  ssize_t read;
  size_t len = 0;
  char *line = NULL;

  for(i = 1; i < argc; i++) {
//    printf("Argument %d: %s\n", i, argv[i]);
    int k=0;
    while( (inp = strsep(&argv[i], ",")) != NULL) {
        if(k == 3) {
		printf("Invalid number of arguments/bad command");
		break;
		//Handle this somehow outside as well
	}
    	inputs[i][k++] = inp;
    }
    int ctr;
  //  printf("Inputs\n");
    for(ctr = 0; ctr < 3; ctr++) {
//	    printf("input %d = %s ", ctr, inputs[i][ctr]);
    }
    printf("\n");
    if(k > 3) {
     printf("Bad command\n");
    }
    char command = (char) inputs[i][0][0];
    switch(command) {
    	case 'p': 
	  //Put/update key value pair into map
	  //TODO: handle mem allocation
	  key = atoi(inputs[i][1]);
//	  printf("Input [2] = %s\n", inputs[i][2]);
	  if(inputs[i][2] == NULL) {
		  printf("Bad command\n");
		  break;
	  }
	  strcpy(value, inputs[i][2]);
	  if(value == NULL) {
   	    printf("Bad command\n");
	    break;
	  }
	  fpr = fopen("key_value.csv", "r");
	  if(fpr == NULL) {
//		  printf("fpr null\n");
	    fp = fopen("key_value.csv", "a");
//	    printf("Open key_value\n");
	    fprintf(fp, "%s,", inputs[i][1]);
            fprintf(fp, "%s \n", inputs[i][2]);
	    fclose(fp);
	  } else {
//            printf("fpr not null\n");
   	    fp2 = fopen("temp.csv", "a");
            char currKey[30];
            while( getline(&line, &len, fpr) != -1 ) {
              strcpy(currKey, line);
//              printf("Line: %s\n", currKey);
	      char *token = strtok(currKey, ",");
//	      printf("Key %s\n", token);
//              printf("inputs[i][1] %s\n", inputs[i][1]);
//              printf("Line again: %s\n", line);
              if(atoi(token) != atoi(inputs[i][1])) {
//               printf("Current line: %s\n", line);
               fprintf(fp2, "%s", line);
              } else {
                continue;
              }
            }
	    fprintf(fp2, "%s,", inputs[i][1]);
            fprintf(fp2, "%s \n", inputs[i][2]);
	    fclose(fpr);
	    fclose(fp2);
	    remove("key_value.csv");
            rename("temp.csv", "key_value.csv");
	  }
	  break;
	case 'g': 
	  //Gets specific key value pair
	  key = atoi(inputs[i][1]);
	  if(inputs[i][2] != NULL) {
	  	printf("Bad command\n");
                break;
	  }
	  break;
	case 'c': 
	  //Clears all key value pairs stored so far
          if(inputs[i][1] !=NULL || inputs[i][2] != NULL) {
                printf("Bad command\n");
                break;
          }
	  fp = fopen("key_value.csv", "w");
	  printf("Cleared all key value pairs");
	  fclose(fp);
	  break;
 	case 'd': 
	  //Deletes specific key value pair
	  key = atoi(inputs[i][1]);
          if(inputs[i][2] != NULL) {
                printf("Bad command\n");
                break;
          }
	  break;
	case 'a': 
	  //Prints all the key value pairs
	  if(inputs[i][1] !=NULL || inputs[i][2] != NULL) {
                printf("Bad command\n");
                break;
          }
	  printf("Displaying all key value pairs:\n");
	  fp = fopen("key_value.csv", "r");
	  while( getline(&line, &len, fp) != -1 ) {
	      printf("%s", line);
          }
	  fclose(fp);
	  break;
	default: 
	  printf("Bad command\n");
	  break;
    }
  }
  return 0;
}
