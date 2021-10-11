#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <fcntl.h>
#include <ctype.h>

char error_message[30] = "An error has occurred\n";

void runShell(char *command) {

	char *str;
	size_t len = 0;
	int isLoop = 0;
	
	if(command == NULL && isLoop == 0) {
		printf("wish>");
		getline(&str, &len, stdin);
	} else {
		str = command;
	}
	// printf("Len = %d Before:%s done\n", strlen(str), str);
	// if(str[0] == 10){//strcmp(str,"\n") == 0 || (strcmp(str, "\r") == 0)) {
	// 	// printf("New line \n");
	// 	return;
	// }
	while(isspace((unsigned char)*str)) str++;
	// printf("After:%s done\n", str);
	if(*str==0) {
		// printf("emptyyy\n");
		return;
	}

	char *end = str + strlen(str) - 1;
  	while(end > str && isspace((unsigned char)*end)) end--;
  	end[1] = '\0';
	
	// if(rc == 0) {


			// printf("Child here with pid = %d\n", getpid());
	 // printf("String:%s\n", str);
	char path[] = "/bin/";
	char usrpath[] = "/usr/bin/";
	if(access(path, X_OK) == 0) {
		// str[strlen(str)-1] = '\0';
		char *inp;
		int count = 1;
		char *ptr = str;
		if(ptr[0] == '>') {
			write(STDERR_FILENO, error_message, strlen(error_message));
			return;
		}
		while((ptr = strchr(ptr, ' ')) != NULL) {
	  				// printf("Ptr = %s\n", ptr);
	  				// printf("zero ptr char = %c\n", ptr[0]);
	  				// printf("first ptr char = %c\n", ptr[1]);
			if(ptr[0] == '>' || ptr[1] == '>') {
	  					// printf("dec count\n");
				count--;
				if(count <= 0) {
					// printf("ohohoh\n");
					write(STDERR_FILENO, error_message, strlen(error_message));
					return;
				}
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
		int isExit = 0;
		int redirectIdx = -1;
			//int numItr = 0;
		while((inp = strsep(&str, " ")) != NULL) {
				// printf("Inp = %s\n", inp);
			if(i == 0) {
					// printf("First if inp = %s\n", inp);
				if(strcmp(inp, "exit") == 0) {
					isExit = 1;
				} else if(strcmp(inp, "cd") == 0) {
					isCd = 1;

				} else if(strcmp(inp, "path") == 0) {

				} else if(strcmp(inp, "loop") == 0) {
					isLoop = 1;
				} else {
					strcat(path, inp);
						//inp = path;
				}
			}
				if(strcmp(inp, ">") != 0 && isRedirect != 1 ){//&& isLoop != 1) {
					// printf("Second if inp = %s\n", inp);
					myargs[i] = inp;
					i++;
				} else if (strcmp(inp, ">") == 0 ){
					// printf("In else bloc : %s \n", inp);
					isRedirect = 1;
					// printf("redirect - %d\n", isRedirect);
					redirectIdx = i;
					i++;
					continue;
				}
				if(isRedirect == 1) {
					//printf("what is inp = %s\n", inp);
					int lastArg = sizeof(myargs) / sizeof(myargs[0]) - 1;
					// printf("last arg val = %d\n", lastArg);
					myargs[lastArg - 1] = inp;
					//printf("last arg = %s\n", myargs[lastArg - 1]);
					close(STDOUT_FILENO);
					open(inp, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
					isRedirect = 0;
				}
			}
			if(isExit == 1) {
				isExit = 0;
				int numArgs = sizeof(myargs) / sizeof(myargs[0]);
				if(numArgs != 2) {
					write(STDERR_FILENO, error_message, strlen(error_message));
				} else {
					exit(0);
				}
				return;
			}
			else if(isCd == 1) {
				// printf("in cd block\n");
				isCd = 0;
				int numArgs = sizeof(myargs) / sizeof(myargs[0]);
				if(numArgs != 3) {
					write(STDERR_FILENO, error_message, strlen(error_message));
				} else {
					if(chdir(myargs[1]) == 0) {
							//printf("changed \n");
					} else {
							//printf("Erorr\n");
					}
					return;
				}
			} else if(isRedirect == 1) {
				int numArgs = sizeof(myargs) / sizeof(myargs[0]);
				// int redirectIdx = -1;
				// for(int itr = 0; itr < numArgs; itr++) {
				// 	printf("redirect %d = %s\n", itr, myargs[itr]);
				// 	// if(strcmp(myargs[i], ">") == 0) {
				// 	// 	redirectIdx = itr;
				// 	// 	break;
				// 	// }
				// }
				//printf("redirectIdx = %d & numArgs = %d\n", redirectIdx, numArgs);
				if(redirectIdx >= numArgs - 1) {
					write(STDERR_FILENO, error_message, strlen(error_message));
					return;
				}
				return;
			}

				//myargs[0] = strdup(path);
				//printf("path = %s\n", path);
				//printf("My args: %s\n", myargs[0]);
			else {
				// printf("in exec block\n");

				myargs[count] = NULL;
				for(int itr = 0; itr <= count; itr++) {
					// printf("Arg %d = %s\n", itr, myargs[itr]);
				}
					//execv(myargs[0], myargs);
				if(strcmp(myargs[0], "loop") == 0) {
					int numArgs = sizeof(myargs) / sizeof(myargs[0]);
					// printf("Num loop = %d\n", numArgs);
					// printf("Count = %d\n", count);
					if(count <= 1) {
						write(STDERR_FILENO, error_message, strlen(error_message));
						return;
					}
					for (int itr = 0; myargs[1][itr]!= '\0'; itr++) {
						if (isdigit(myargs[1][itr]) == 0) {
							write(STDERR_FILENO, error_message, strlen(error_message));
							return;
						}
					}
					int numItr = atoi(myargs[1]);
					if(numItr <= 0) {
						write(STDERR_FILENO, error_message, strlen(error_message));
						return;
					}
					char *newargs[count - 1];
					if((access(path, X_OK) == 0)) {
						newargs[0] = strcat(path, myargs[2]);
					} else if((access(usrpath, X_OK) == 0)) {
						newargs[0] = strcat(usrpath, myargs[2]);
					} else {
						newargs[0] = "";
					}
					for(int itr = 3; itr <= count; itr++) {
						// printf("itr - 2 = %d\n", itr -2);
						// printf("my args at itr = %s\n", myargs[itr]);
						newargs[itr-2] = myargs[itr];
					}
					int newArgSize = sizeof(newargs) / sizeof(newargs[0]);
					// printf("size newarg = %d\n", newArgSize);
					for(int itr = 0; itr < newArgSize; itr++) {
						// printf("lala %d\n", itr);
						// printf("new Arg %d = %s\n", itr, newargs[itr]);
					}
					// execv(newargs[0], newargs);
					for(int itr = 0; itr < numItr; itr++) {
						// printf("In for loop %d\n", itr);
						int loopRc = fork();
						if(loopRc == 0) {
							execv(newargs[0], newargs);
							write(STDERR_FILENO, error_message, strlen(error_message));
						} else if(loopRc > 0) {
							int lrc = wait(NULL);
						} else if(loopRc < 0) {
							write(STDERR_FILENO, error_message, strlen(error_message));
						}
						//write(STDERR_FILENO, error_message, strlen(error_message));
					}
					isLoop = 0;
				} else {
					//printf("Else final %s\n", myargs[0]);
					//printf("Path = %s\n", path);
					for(int itr = 0; itr < sizeof(myargs) / sizeof(myargs[0]); itr++) {
						// printf("Arg %d = %s\n", itr, myargs[itr]);
					}
					int rc = fork();
					if(rc == 0) {
						execv(path, myargs);
						write(STDERR_FILENO, error_message, strlen(error_message));
					} else if(rc > 0) {
						int wrc = wait(NULL);

					} else {
						fprintf(stderr, "fork failed \n");
						exit(1);
					}
				}
				
			}

		}
		else if (access(usrpath, X_OK) == 0) {
			// printf("Usr/bin set\n");
			strcat(usrpath, str);
			char *myargs[2];
			myargs[0] = strdup(usrpath);
			myargs[1] = NULL;
			execv(myargs[0], myargs);
			write(STDERR_FILENO, error_message, strlen(error_message));
		} else {
			char *myargs[2];
			// printf("Bin not set\n");
			myargs[0] = "";
		}
		
			//printf("This shouldn't print out\n");
	// } 
	// else if (rc > 0) {
	// 	int waitRc = wait(NULL);
	// 		//printf("Parent here my pid is %d and my child is %d\n", getpid(), waitRc);
	// } else {
	// 	fprintf(stderr, "fork failed \n");
	// 	exit(1);
	// }
		return;
	}


	int main(int argc, char *argv[]) {
		if(argc > 2) {
			write(STDERR_FILENO, error_message, strlen(error_message));
			exit(1);
		} else if (argc == 1) {
		// int f = fork();
		// if(f == 0) {
		// 	char* xargs[2];
		// 	xargs[0] = "ls";
		// 	xargs[1] = NULL;
		// 	printf("Hello\n");
		// 	execv("/bin/ls", xargs);
		// 	printf("Failed\n");
		// }
			while(1) {
				runShell(NULL);
			}
		} else if(argc == 2) {
			size_t len = 0;
			char *line;
			FILE *fp;
			if((fp = fopen(argv[1], "r"))) {
				// printf("File open\n");
				int ct = 0;
				while(getline(&line, &len, fp) > 1) {
					ct++;
					// printf("Line = %s", line);
					runShell(line);
				}
				// printf("Ct = %d\n", ct);
				fclose(fp);
			} else {
				write(STDERR_FILENO, error_message, strlen(error_message));
				exit(1);
			}	
		}
		return 0;
	}
