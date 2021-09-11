#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
  //Check num args etc
  printf("Helloo!");
  int i;

  char *inputs;
  char *inp[argc][3];
  for(i = 0; i < argc; i++){
    printf("Argument %d: %s\n", i, argv[i]);
    int k = 0;
    while( (inputs =  strsep(&argv[i], ",")) != NULL){
	if(k == 3) {
		printf("Invalid number of arguments\n");
		exit(0);
	}
    	inp[i][k++] = inputs;
    }
    int j ;
    for(j = 0; j < 3; j++)
        printf("%s ", inp[i][j]);
    printf("\n"); 
  }
  return 0;

}
