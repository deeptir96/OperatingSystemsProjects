#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <fcntl.h>

int main(int argc, char *argv[]) {
	if(argc > 2) {
		printf("Nope\n");
	}

	
	while(1) {
  		char *str;
  		size_t len = 0;
    	printf("wish>");
    	getline(&str, &len, stdin);
		int rc = fork();
		if(rc == 0) {
			
			//printf("Child here with pid = %d\n", getpid());
			char path[] = "/bin/";
			char usrpath[] = "/usr/bin/";
			if(access(path, X_OK) == 0) {
				str[strlen(str)-1] = '\0';
				char *inp;
	  			int count = 1;
	  			char *ptr = str;
	  			while((ptr = strchr(ptr, ' ')) != NULL) {
	  				//printf("Ptr = %s\n", ptr);
	  				//printf("first ptr char = %c\n", ptr[1]);
	  				if(ptr[1] == '>') {
	  					//printf("dec count\n");
	  					count--;
	  				}
   				    count++;
    				ptr++;
				}
				
				//printf("Inside access str = %s\n", str);
				//strcat(path, str);
				char *myargs[count + 1];
				int i =0;
				ptr = str;
				int isCd = 0;
				int isRedirect = 0;
				while((inp = strsep(&str, " ")) != NULL) {
					printf("Inp = %s\n", inp);
   				    if(i == 0) {
   				    	printf("First if inp = %s\n", inp);
   				    	if(strcmp(inp, "exit") == 0) {
   				    		exit(0);
   				    	} else if(strcmp(inp, "cd") == 0) {
   				    		isCd = 1;

   				    	} else if(strcmp(inp, "path") == 0) {

   				    	} else {
   				    		strcat(path, inp);
   				    		inp = path;
   				    	}
   				    }
   				    if(strcmp(inp, ">") != 0 && isRedirect != 1) {
   				    	printf("Second if inp = %s\n", inp);
   				    	myargs[i] = inp;
    					i++;
   				    } else if (strcmp(inp, ">") == 0){
   				    	printf("In else bloc : %s \n", inp);
   				    	isRedirect = 1;
   				    	printf("redirect - %d\n", isRedirect);
   				    	
   				    	i++;
   				    	//count--;
   				    	
   				    	continue;
   				    }
   				    if(isRedirect == 1) {
   				    	printf("what is inp = %s\n", inp);
   				    	int lastArg = sizeof(myargs) / sizeof(myargs[0]) - 1;
   				    	printf("last arg val = %d\n", lastArg);
   				    	myargs[lastArg - 1] = inp;
   				    	printf("last arg = %s\n", myargs[lastArg - 1]);
   				    	close(STDOUT_FILENO);
   				    	open(inp, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
   				    	isRedirect = 0;
   				    }
   				    printf("Count = %d\n", count);
   				    
				}
				if(isCd == 1) {
					printf("in cd block\n");
					isCd = 0;
					int numArgs = sizeof(myargs) / sizeof(myargs[0]);
					if(numArgs != 3) {
						printf("Error numargs %d\n", numArgs);
					} else {
						if(chdir(myargs[1]) == 0) {
							//printf("changed \n");
						} else {
							//printf("Erorr\n");
						}
					}
				}
				
				//myargs[0] = strdup(path);
				//printf("path = %s\n", path);
				//printf("My args: %s\n", myargs[0]);
				else {
					printf("in exec block\n");

					myargs[count] = NULL;
					for(int itr = 0; itr <= count; itr++) {
						printf("Arg %d = %s\n", itr, myargs[itr]);
					}
					//execv(myargs[0], myargs);
					execv(myargs[0], myargs);
					printf("Execv failed\n");
				}
				
			}
			else if (access(usrpath, X_OK) == 0) {
				printf("Usr/bin set\n");
				strcat(usrpath, str);
				char *myargs[2];
				myargs[0] = strdup(usrpath);
				myargs[1] = NULL;
				execv(myargs[0], myargs);
				printf("Execv failed\n");
			} else {
				char *myargs[2];
				printf("Bin not set\n");
				myargs[0] = "";
			}
		
			//printf("This shouldn't print out\n");
		} else if (rc > 0) {
			int waitRc = wait(NULL);
			//printf("Parent here my pid is %d and my child is %d\n", getpid(), waitRc);
		} else {
			fprintf(stderr, "fork failed \n");
			exit(1);
	}
}
	return 0;
}