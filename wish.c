#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <fcntl.h>
#include <ctype.h>

char error_message[30] = "An error has occurred\n";
char *userDefinedPath[30];
int save_out;

int runShell(char *command) {
	char *str;
	size_t len = 0;
	int isLoop = 0;
	int isRedirect1 = 0;
	int lastArg = -1;
	char *opFile;
		
	if(command == NULL && isLoop == 0) {
		printf("wish>");
		getline(&str, &len, stdin);
	} else {
		str = (char *) malloc(strlen(command) + 1);
		str = command;
	}
	
	while(isspace((unsigned char)*str)) str++;
	// printf("After:%s done\n", str);
	if(*str==0) {
		// printf("emptyyy\n");
		return -1;
	}

	char *end = str + strlen(str) - 1;
  	while(end > str && isspace((unsigned char)*end)) end--;
  	end[1] = '\0';
  	char *loopsy = strstr(str, "$loop");
  	int isLoopVar = 0;
  	if(loopsy) {
  		isLoopVar = 1;
  	}
  	char *redirect = strstr(str, ">");
  	int isRedirect = 0;
  	if(redirect){
  		isRedirect = 1;
  	}
  	int index = -1;
  	if (isRedirect == 1){
  		isRedirect = 0;
  		// printf("String before = %s\n", str);
  		const char *rPtr = strchr(str, '>');
		if(rPtr) {
   			index = rPtr - str;
   			// do something
   			// printf("Index = %d\n", index);
		}
		
		char *s1_start;
		char *s1_end;
		char *substr1;
		char *s2_start;
		char *s2_end;
		char *substr2;
		int f1 = 0;
		int f2 = 0;
		if(str[index - 1] != ' ') {
			s1_start = &str[0];
			s1_end = &str[index];
			substr1 = (char *) calloc(1, s1_end - s1_start + 3);
			memcpy(substr1, s1_start, s1_end - s1_start);
			
			strcat(substr1, " >");
			// printf("Substr1 = %s\n", substr1);
			f1 =1;
		}
		if(str[index + 1] != ' ') {
			s2_start = &str[index + 1];
			s2_end = &str[strlen(str)];
			char *substr2_tmp = (char *) calloc(1, s2_end - s2_start + 1);
			substr2 = (char *) calloc(1, s2_end - s2_start + 2);
			strcat(substr2, " ");
			memcpy(substr2_tmp, s2_start, s2_end - s2_start);
			strcat(substr2, substr2_tmp);
			// printf("Substr2 = %s\n", substr2);
			f2 =1;
		}
		if((f1 == f2) && (f1 == 1)) {
			strcat(substr1, substr2);
			// printf("Substringgg = %s\n", substr1);
			strcpy(str, substr1);
			// printf("String = %s\n", str);
		}

  	}
  	// printf("loopvar? = %d\n", isLoopVar);
	char path[] = "";
	
	char usrpath[] = "/usr/bin/";
	

	// if(access(path, X_OK) == 0) {
		// str[strlen(str)-1] = '\0';
		char *inp;
		int count = 1;
		char *ptr = str;
		if(ptr[0] == '>') {
			write(STDERR_FILENO, error_message, strlen(error_message));
			return -1;
		}
		while((ptr = strchr(ptr, ' ')) != NULL) {
	  				// printf("Ptr = %s\n", ptr);
	  				// printf("zero ptr char = %c\n", ptr[0]);
	  				// printf("first ptr char = %c\n", ptr[1]);
			if(ptr[0] == '>' || ptr[1] == '>') {
	  					// printf("dec count\n");
				isRedirect = 1;
				count = count - 1;
				// printf("Countoo %d\n", count);
				if(count <= 0) {
					// printf("ohohoh\n");
					write(STDERR_FILENO, error_message, strlen(error_message));
					return -1;
				}
			}
			count++;
			ptr++;
		}
		if(isRedirect == 1) {
			isRedirect = 0;
			count--;
			// printf("fin Count = %d\n", count);
		}
		// printf("Count = %d\n", count);

				//printf("Inside access str = %s\n", str);
				//strcat(path, str);

		char *myargs[count + 1];
		int i =0;
		ptr = str;
		int isCd = 0;
		
		int isExit = 0;
		int isPath = 0;
		int redirectIdx = -1;
			//int numItr = 0;
		while((inp = strsep(&str, " ")) != NULL) {
			// printf("Inp = %s\n", inp);
			if(i == 0) {
				if(strcmp(inp, "exit") == 0) {
					// printf("in exit\n");
					isExit = 1;
				} else if(strcmp(inp, "cd") == 0) {
					isCd = 1;
					// printf("in cd\n");
				} else if(strcmp(inp, "path") == 0) {
					isPath = 1;
					myargs[0] = inp;
					// printf("in path\n");
				} else if(strcmp(inp, "$loop") == 0) {
					// printf("IsLoopVar before= %d\n", isLoopVar);
					isLoopVar = 1;
					// printf("in loopv\n");
				} else if(strcmp(inp, "loop") == 0) {
					isLoop = 1;
					// printf("in loop\n");
				} 
				
			} 
				if(strcmp(inp, ">") != 0 && isRedirect != 1 ){//&& isLoop != 1) {
					// printf("Second if inp = %s\n", inp);
					myargs[i] = inp;
					i++;
				} else if (strcmp(inp, ">") == 0 ){
					// printf("In else bloc : %s \n", inp);
					isRedirect = 1;
					
					redirectIdx = i;
					// printf("redirectIdx - %d\n", redirectIdx);
					i++;
					continue;
				}
				if(isRedirect == 1) {
					// printf("what is inp = %s\n", inp);
					lastArg = sizeof(myargs) / sizeof(myargs[0]) - 1;
					// printf("last arg val = %d\n", lastArg - 1);
					// myargs[lastArg - 1] = inp;
					opFile = inp;
					// printf("opFile = %s\n", opFile);
					// printf("last arg = %s\n", myargs[lastArg - 1]);
					
					// save_out = dup(STDOUT_FILENO);
					// close(STDOUT_FILENO);
					// close(save_out);
					// printf("Clossedd \n");
					// printf("cl = %d\n", cl);
					// int op = open(myargs[lastArg - 1], O_WRONLY | O_APPEND | O_RDONLY | O_CREAT | O_TRUNC, 00777);//S_IRUSR | S_IWUSR);
					// printf("op = %d\n", op);
					isRedirect = 0;
					isRedirect1 = 1;
				}
			}
			if(isExit == 1) {
				isExit = 0;
				int numArgs = sizeof(myargs) / sizeof(myargs[0]);
				// printf("numargs = %d\n", numArgs);
				if(numArgs != 2) {
					write(STDERR_FILENO, error_message, strlen(error_message));
				} else {
					exit(0);
				}
				return -1;
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
					return -1;
				}
			} else if(isRedirect1 == 1) {
				// printf("isRedirect1 is set");
				isRedirect1 = 0;
				int numArgs = sizeof(myargs) / sizeof(myargs[0]);
				// int redirectIdx = -1;
				// for(int itr = 0; itr < numArgs; itr++) {
				// 	printf("redirect %d = %s\n", itr, myargs[itr]);
				// 	// if(strcmp(myargs[i], ">") == 0) {
				// 	// 	redirectIdx = itr;
				// 	// 	break;
				// 	// }
				// }
				// printf("redirectIdx = %d & numArgs = %d\n", redirectIdx, numArgs);
				if(redirectIdx >= numArgs) {
					write(STDERR_FILENO, error_message, strlen(error_message));
					return -1;
				}
				
				myargs[count] = NULL;
				int rc = fork();
					if(rc == 0) {
						
					for(int itr2 = 0; itr2 < 30; itr2++) {
						// printf("Myarg[0] = %s\n", myargs[0]);
						if(userDefinedPath[itr2] == NULL) {
							write(STDERR_FILENO, error_message, strlen(error_message));
							break;
						}

						char *executable = (char *)malloc(strlen(userDefinedPath[itr2]) + strlen(myargs[0]) + 1);
						strcat(executable, userDefinedPath[itr2]);
						// printf("executabl'es last char = %c\n", executable[strlen(executable) - 1]);
						// printf("executable before: %s inp: %s\n", executable, inp);
						if(strlen(executable) >=1 &&executable[strlen(executable) - 1] != '/') {
							strcat(executable, "/");
						}
						strcat(executable, myargs[0]);
						// printf("executable after: %s\n", executable);
						if((access(executable, X_OK) == 0)) {
							// printf("Accesible\n");
							strcpy(path, executable);
							// printf("path = %s\n", path);
							// for(int ix=0; ix < (sizeof(myargs) / sizeof(myargs[0])); ix++) {
							// 	printf("myargs[%d] = %s\n", ix, myargs[ix]);
							// }
							// printf("opFile again= %s\n", opFile);
							close(STDOUT_FILENO);
							int op = open(opFile, O_WRONLY | O_APPEND | O_RDONLY | O_CREAT | O_TRUNC, 00777);
							execv(path, myargs);
							write(STDERR_FILENO, error_message, strlen(error_message));
							break;
						}
					}
					} else if(rc > 0) {
						int wrc = wait(NULL);

					} else {
						fprintf(stderr, "fork failed \n");
						exit(1);
					}
				//return;
			} else if(isPath == 1) {
				isPath = 0;

				for(int itry = 0; itry< 30; itry++){
					if(userDefinedPath[itry] != NULL) {
						userDefinedPath[itry] = (char *) malloc(strlen("") + 1);
						strcpy(userDefinedPath[itry], "");
					} 
					//printf("udp[%d] = %s\n", itry, userDefinedPath[itry]);
					// userDefinedPath[itry] = NULL;
				}
				int numArgs = sizeof(myargs) / sizeof(myargs[0]);
				// printf("yooo%d\n",numArgs);
				// for(int itrx = 0; itrx < numArgs - 1; itrx++) {
				// 	printf("itrx = %d arg = %s\n", itrx, myargs[itrx]);
				// }
				if(numArgs <= 0) {
					write(STDERR_FILENO, error_message, strlen(error_message));
				}
				
				// strcpy(userDefinedPath, "PATH=");
				
				for(int itr1 = 1; itr1 <= numArgs - 2; itr1++) {
					// printf("myarged[itr1] = %s\n", myargs[itr1]);
					// strcat(userDefinedPath, myargs[itr1]);
					userDefinedPath[itr1 - 1] = (char *) malloc(strlen(myargs[itr1]) + 1);
					strcpy(userDefinedPath[itr1 - 1], myargs[itr1]);
					// printf("udp[%d] = %s\n", itr1-1, userDefinedPath[itr1-1]);
					// printf("hello");
					// strcat(userDefinedPath, "/:");
				}
				
			
				// printf("curr itr = %d\n",itr1);
				// for(int x = 0; x < 30; x++) {
				// 	printf("userDefinedPaths[%d] = %s\n",x, userDefinedPath[x]);
				// }
				// printf("-----------------------end of if-----------");
				
				return -1;
			} else {
				// printf("All other commands\n");
				myargs[count] = NULL;
				int didExec = 0;
				if(strcmp(myargs[0], "loop") == 0) {
					// printf("is in loop?\n");

					int numArgs = sizeof(myargs) / sizeof(myargs[0]);
					// printf("Num loop = %d\n", numArgs);
					// printf("Count = %d\n", count);
					if(count <= 1) {
						// printf("Count < 1\n");
						write(STDERR_FILENO, error_message, strlen(error_message));
						return -1;
					}
					// printf("hello again after loop\n");
					for (int itr = 0; myargs[1][itr]!= '\0'; itr++) {
						if (isdigit(myargs[1][itr]) == 0) {
							write(STDERR_FILENO, error_message, strlen(error_message));
							return -1;
						}
					}
					int numItr = atoi(myargs[1]);
					// printf("NumItr = %d\n", numItr);
					if(numItr <= 0) {
						write(STDERR_FILENO, error_message, strlen(error_message));
						return -1;
					}
					int loopIdx = -1;
					// printf("loopidx = %d\n", loopIdx);
					// printf("------------------");
					if(isLoopVar == 1) {
						// printf("Inside isloopvar\n");
						for(int itr = 0; itr < numArgs; itr++) {
							// printf("myarg = %s\n", myargs[itr]);
							if(strcmp(myargs[itr], "$loop") == 0) {
								loopIdx = itr;
								break;
							}
						}
						isLoopVar = 0;
					}
					// printf("loopIdx = %d\n", loopIdx);
					// printf("------------------");
					char *newargs[count - 1];
					// printf("loop path = %s\n", path);

					
					for(int itr = 2; itr <= count; itr++) {
						// printf("itr - 2 = %d\n", itr -2);
						// printf("my args at itr %d= %s\n", itr, myargs[itr]);
						//if(strcmp(myargs[itr], "$loop") == 0)
						newargs[itr-2] = myargs[itr];
						// printf("newargs[%d] = %s\n", itr-2, newargs[itr-2]);
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
						if(loopIdx-2 >= 0) {
							// printf("Before : %s\n", newargs[loopIdx-2]);
							sprintf(newargs[loopIdx-2], "%d", itr + 1);
							// printf("After newargs[%d] : %s\n",loopIdx-2, newargs[loopIdx-2]);
						}
						int loopRc = fork();
						if(loopRc == 0) {
							// printf("Loop forking\n");
							for(int itr3 = 0; itr3 < 30; itr3++) {
								if(userDefinedPath[itr3] == NULL) {
									write(STDERR_FILENO, error_message, strlen(error_message));
									return -1;
								}

								char *executable = (char *)malloc(strlen(userDefinedPath[itr3]) + strlen(newargs[0]) + 2);
								// printf("curr path = %s\n", userDefinedPath[itr3]);
								// printf("curr arg = %s\n", myargs[2]);
								strcat(executable, userDefinedPath[itr3]);
								strcat(executable, "/");
								strcat(executable, newargs[0]);
								int nSize = sizeof(newargs) / sizeof(newargs[0]);
								// for(int a = 0; a < nSize; a++) {
								// 	printf("newarg[%d] = %s\n", a, newargs[a]);
								// }
								// printf("executable after: %s\n", executable);
								if((access(executable, X_OK) == 0)) {
									// printf("Accesible\n");
									strcpy(path, executable);
									execv(path, newargs);
									write(STDERR_FILENO, error_message, strlen(error_message));
									// newargs[0] = path;
									// printf("neargs[0] = %s\n", newargs[0]);
									break;
								} else {

								}
						// else {
						// 	printf("Executable = %s\n", executable);
						// 	printf("oohlala\n");
						// }
							}
						} else if(loopRc > 0) {
							// printf("Loop parent\n");
							int lrc = wait(NULL);
						} else if(loopRc < 0) {
							write(STDERR_FILENO, error_message, strlen(error_message));
						}
						//write(STDERR_FILENO, error_message, strlen(error_message));
					}
					// printf("Loop exit\n");
					isLoop = 0;
				} else {
					// printf("Not loop\n");
					//int rc = fork();
					// if(rc == 0) {
					for(int itr2 = 0; itr2 < 30; itr2++) {
						// printf("Myarg[0] = %s\n", myargs[0]);
						// printf("in for loop\n");
						if(userDefinedPath[itr2] == NULL) {
							write(STDERR_FILENO, error_message, strlen(error_message));
							break;
						}

						char *executable = (char *)malloc(strlen(userDefinedPath[itr2]) + strlen(myargs[0]) + 1);
						strcat(executable, userDefinedPath[itr2]);
						// printf("executabl'es last char = %c\n", executable[strlen(executable) - 1]);
						// printf("executable before: %s inp: %s\n", executable, inp);
						if(strlen(executable) >=1 &&executable[strlen(executable) - 1] != '/') {
							strcat(executable, "/");
						}
						strcat(executable, myargs[0]);
						strcpy(path, executable);
						// printf("execu/table after: %s\n", executable);
						if((access(executable, X_OK) == 0)) {
							// printf("Accesible\n");
							// strcpy(path, executable);
							didExec = 1;
							// printf("path = %s\n", path);
							// for(int ix=0; ix < (sizeof(myargs) / sizeof(myargs[0])); ix++) {
							// 	printf("myargs[%d] = %s\n", ix, myargs[ix]);
							// }
							int frc = fork();
							if(frc == 0){
								execv(path, myargs);
								write(STDERR_FILENO, error_message, strlen(error_message));
							} else if(frc > 0) {
								int fwrc = wait(NULL);
							} else {
								fprintf(stderr, "fork failed \n");
								exit(1);
							}
							
							break;
						}// else {
							//write(STDERR_FILENO, error_message, strlen(error_message));
						//}
					}
					
						
						
					//} else if(rc > 0) {
						//int wrc = wait(NULL);

					//} else {
					//	fprintf(stderr, "fork failed \n");
					//	exit(1);
					//}
				}
			}	
			
			// printf("run shell exit\n");
		return redirectIdx;
}


int main(int argc, char *argv[]) {
		if(argc > 2) {
			write(STDERR_FILENO, error_message, strlen(error_message));
			exit(1);
		} else if (argc == 1) {
			userDefinedPath[0]= "/bin/";
			for(int i = 1; i < 30; i++) {
				userDefinedPath[i] = NULL;
			}
			while(1) {
				// printf("Times =\n");
				runShell(NULL);
			}
		} else if(argc == 2) {
			size_t len = 0;
			char *line;
			FILE *fp;
			
			userDefinedPath[0]= "/bin/";
			for(int i = 1; i < 30; i++) {
				userDefinedPath[i] = (char *) malloc(sizeof(char) * 50);
				userDefinedPath[i] = NULL;
			}
			if((fp = fopen(argv[1], "r"))) {
				// printf("File open\n");
				int ct = 0;

				while(getline(&line, &len, fp) > 1) {
					// close(STDOUT_FILENO);

					ct++;
					// printf("Line = %s", line);
					// printf("final--------------------\n");
					// for(int i = 0; i < 30; i++) {
						// printf("udp = %s\n", userDefinedPath[i]);
					// }
					// printf("end ----------------------\n");
					int rdrct = runShell(line);
					// if(rdrct != -1)
						// dup2(save_out, STDOUT_FILENO);
				}
				// printf("main exit\n");
				// printf("Ct = %d\n", ct);
				fclose(fp);
			} else {
				write(STDERR_FILENO, error_message, strlen(error_message));
				exit(1);
			}	
		}
		return 0;
}
