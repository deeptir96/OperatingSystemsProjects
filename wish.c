#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
	if(argc > 2) {
		printf("Nope\n");
	}

	//printf("Hello, my pid is %d\n", getpid());
	while(1) {
  		char *str;
  		size_t len = 0;
    	printf("wish>");
    	getline(&str, &len, stdin);
		int rc = fork();
		if(rc == 0) {
			
			printf("Child here with pid = %d\n", getpid());
			char path[] = "/bin/";
			char usrpath[] = "/usr/bin/";
			if(access(path, X_OK) == 0) {
				str[strlen(str)-1] = '\0';
				char *inp;
				//while( (inp = strsep(str, ",")) != NULL) {
      			//	if(k == 3) {
				//		printf("bad command");
				//	break;
	  			//}
	  			int count = 1;
	  			char *ptr = str;
	  			while((ptr = strchr(ptr, ' ')) != NULL) {
	  				printf("Ptr = %s\n", ptr);
   				    count++;
    				ptr++;
				}
				printf("Count = %d\n", count);
				//printf("Inside access str = %s\n", str);
				//strcat(path, str);
				char *myargs[count + 1];
				int i =0;
				ptr = str;
				while((inp = strsep(&str, " ")) != NULL) {
   				    //count++;
   				    if(i == 0) {
   				    	strcat(path, inp);
   				    	inp = path;
   				    }
   				    myargs[i] = inp;
    				//ptr++;
    				i++;
				}
				
				//myargs[0] = strdup(path);
				//printf("path = %s\n", path);
				printf("My args: %s\n", myargs[0]);
				myargs[count] = NULL;
				//execv(myargs[0], myargs);
				execv(myargs[0], myargs);
			}
			else if (access(usrpath, X_OK) == 0) {
				printf("Usr/bin set\n");
				strcat(usrpath, str);
				char *myargs[2];
				myargs[0] = strdup(usrpath);
				myargs[1] = NULL;
				execv(myargs[0], myargs);
			} else {
				char *myargs[2];
				printf("Bin not set\n");
				myargs[0] = "";
			}
		
			printf("This shouldn't print out\n");
		} else if (rc > 0) {
			int waitRc = wait(NULL);
			printf("Parent here my pid is %d and my child is %d\n", getpid(), waitRc);
		} else {
			fprintf(stderr, "fork failed \n");
			exit(1);
	}
}
	return 0;
}