#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
  //Check num args etc
  printf("Helloo!");
  int i;
  char *inp;
  char *inputs[argc][3];

  for(i = 0; i < argc; i++) {
    printf("Argument %d: %s\n", i, argv[i]);
    int k=0;
    while( (inp = strsep(&argv[i], ",")) != NULL) {
        if(k == 3) {
		printf("Invalid number of arguments");
		exit(0);
	}
    	inputs[i][k++] = inp;
    }
    int j;
    for(j = 0; j < 3; j++) {
    	printf("%s ", inputs[i][j]);
    }
    printf("\n");
  }
  return 0;
}
