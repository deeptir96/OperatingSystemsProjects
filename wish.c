#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <fcntl.h>
#include <ctype.h>

char error_message[30] = "An error has occurred\n";
char *userDefinedPath[30];

void runShell(char *command) {
	char *str;
	size_t len = 0;
	int isLoop = 0;
	int isRedirect1 = 0;
	char *opFile;

	if(command == NULL && isLoop == 0) {
		printf("wish>");
		getline(&str, &len, stdin);
	} else {
		str = (char *) malloc(strlen(command) + 1);
		str = command;
	}
	
	while(isspace((unsigned char)*str)) str++;
	if(*str==0) {
		return;
	}

	char *end = str + strlen(str) - 1;
	while(end > str && isspace((unsigned char)*end)) end--;
	end[1] = '\0';
	char *containsLoop = strstr(str, "$loop");
	int isLoopVar = 0;
	if(containsLoop) {
		isLoopVar = 1;
	}
	char *containsRedirect = strstr(str, ">");
	int isRedirect = 0;
	if(containsRedirect){
		isRedirect = 1;
	}
	int index = -1;
	if (isRedirect == 1){
		isRedirect = 0;
		const char *rPtr = strchr(str, '>');
		if(rPtr) {
			index = rPtr - str;
		}
		
		char *s1_start;
		char *s1_end;
		char *substr1;
		char *s2_start;
		char *s2_end;
		char *substr2;
		int isSpaceBefore = 0;
		int isSpaceAfter = 0;
		if(str[index - 1] != ' ') {
			s1_start = &str[0];
			s1_end = &str[index];
			substr1 = (char *) calloc(1, s1_end - s1_start + 3);
			memcpy(substr1, s1_start, s1_end - s1_start);
			strcat(substr1, " >");
			isSpaceBefore = 1;
		}
		if(str[index + 1] != ' ') {
			s2_start = &str[index + 1];
			s2_end = &str[strlen(str)];
			char *substr2_tmp = (char *) calloc(1, s2_end - s2_start + 1);
			substr2 = (char *) calloc(1, s2_end - s2_start + 2);
			strcat(substr2, " ");
			memcpy(substr2_tmp, s2_start, s2_end - s2_start);
			strcat(substr2, substr2_tmp);
			isSpaceAfter = 1;
		}
		if((isSpaceBefore == isSpaceAfter) && (isSpaceBefore == 1)) {
			strcat(substr1, substr2);
			strcpy(str, substr1);
		}

	}

	char path[] = "";
	char usrpath[] = "/usr/bin/";
	if(usrpath[0] == '\0')
		return;
	
	char *inp;
	int count = 1;
	char *ptr = str;
	if(ptr[0] == '>') {
		write(STDERR_FILENO, error_message, strlen(error_message));
		return;
	}
	while((ptr = strchr(ptr, ' ')) != NULL) {
		if(ptr[0] == '>' || ptr[1] == '>') {
			isRedirect = 1;
			count = count - 1;
			if(count <= 0) {
				write(STDERR_FILENO, error_message, strlen(error_message));
				return;
			}
		}
		count++;
		ptr++;
	}
	if(isRedirect == 1) {
		isRedirect = 0;
		count--;
	}

	char *myargs[count + 1];
	int i =0;
	ptr = str;
	int isCd = 0;

	int isExit = 0;
	int isPath = 0;
	int redirectIdx = -1;
	while((inp = strsep(&str, " ")) != NULL) {
		if(i == 0) {
			if(strcmp(inp, "exit") == 0) {
				isExit = 1;
			} else if(strcmp(inp, "cd") == 0) {
				isCd = 1;
			} else if(strcmp(inp, "path") == 0) {
				isPath = 1;
				myargs[0] = inp;
			} else if(strcmp(inp, "$loop") == 0) {
				isLoopVar = 1;
			} else if(strcmp(inp, "loop") == 0) {
				isLoop = 1;
			} 

		} 
		if(strcmp(inp, ">") != 0 && isRedirect != 1 ){
			myargs[i] = inp;
			i++;
		} else if (strcmp(inp, ">") == 0 ){
			isRedirect = 1;
			redirectIdx = i;
			i++;
			continue;
		}
		if(isRedirect == 1) {
			opFile = inp;
			isRedirect = 0;
			isRedirect1 = 1;
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
		isCd = 0;
		int numArgs = sizeof(myargs) / sizeof(myargs[0]);
		if(numArgs != 3) {
			write(STDERR_FILENO, error_message, strlen(error_message));
		} else {
			chdir(myargs[1]);
			return;
		}
	} else if(isRedirect1 == 1) {
		isRedirect1 = 0;
		int numArgs = sizeof(myargs) / sizeof(myargs[0]);
		if(redirectIdx >= numArgs) {
			write(STDERR_FILENO, error_message, strlen(error_message));
			return;
		}

		myargs[count] = NULL;
		int rc = fork();
		if(rc == 0) {

			for(int itr2 = 0; itr2 < 30; itr2++) {
				if(userDefinedPath[itr2] == NULL) {
					write(STDERR_FILENO, error_message, strlen(error_message));
					break;
				}
				char *executable = (char *)malloc(strlen(userDefinedPath[itr2]) + strlen(myargs[0]) + 1);
				strcat(executable, userDefinedPath[itr2]);
				if(strlen(executable) >=1 &&executable[strlen(executable) - 1] != '/') {
					strcat(executable, "/");
				}
				strcat(executable, myargs[0]);
				if((access(executable, X_OK) == 0)) {
					strcpy(path, executable);
					close(STDOUT_FILENO);
					open(opFile, O_WRONLY | O_APPEND | O_RDONLY | O_CREAT | O_TRUNC, 00777);
					execv(path, myargs);
					write(STDERR_FILENO, error_message, strlen(error_message));
					break;
				}
			}
		} else if(rc > 0) {
			(void) wait(NULL);

		} else {
			fprintf(stderr, "fork failed \n");
			exit(1);
		}
	} else if(isPath == 1) {
		isPath = 0;

		for(int itry = 0; itry< 30; itry++){
			if(userDefinedPath[itry] != NULL) {
				userDefinedPath[itry] = (char *) malloc(strlen("") + 1);
				strcpy(userDefinedPath[itry], "");
			} 
		}
		int numArgs = sizeof(myargs) / sizeof(myargs[0]);
		if(numArgs <= 0) {
			write(STDERR_FILENO, error_message, strlen(error_message));
		}

		for(int itr1 = 1; itr1 <= numArgs - 2; itr1++) {
			userDefinedPath[itr1 - 1] = (char *) malloc(strlen(myargs[itr1]) + 1);
			strcpy(userDefinedPath[itr1 - 1], myargs[itr1]);
		}
		
		return;
	} else {
		myargs[count] = NULL;
		if(strcmp(myargs[0], "loop") == 0) {

			int numArgs = sizeof(myargs) / sizeof(myargs[0]);
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
			int loopIdx = -1;
			if(isLoopVar == 1) {
				for(int itr = 0; itr < numArgs; itr++) {
					if(strcmp(myargs[itr], "$loop") == 0) {
						loopIdx = itr;
						break;
					}
				}
				isLoopVar = 0;
			}
			char *newargs[count - 1];
			for(int itr = 2; itr <= count; itr++) {
				newargs[itr-2] = myargs[itr];
			}
			for(int itr = 0; itr < numItr; itr++) {
				if(loopIdx-2 >= 0) {
					sprintf(newargs[loopIdx-2], "%d", itr + 1);
				}
				int loopRc = fork();
				if(loopRc == 0) {
					for(int itr3 = 0; itr3 < 30; itr3++) {
						if(userDefinedPath[itr3] == NULL) {
							write(STDERR_FILENO, error_message, strlen(error_message));
							return;
						}

						char *executable = (char *)malloc(strlen(userDefinedPath[itr3]) + strlen(newargs[0]) + 2);
						strcat(executable, userDefinedPath[itr3]);
						strcat(executable, "/");
						strcat(executable, newargs[0]);
						if((access(executable, X_OK) == 0)) {
							strcpy(path, executable);
							execv(path, newargs);
							write(STDERR_FILENO, error_message, strlen(error_message));
							break;
						} 
					}
				} else if(loopRc > 0) {
					(void) wait(NULL);
				} else if(loopRc < 0) {
					write(STDERR_FILENO, error_message, strlen(error_message));
				}
			}
			isLoop = 0;
		} else {
			for(int itr2 = 0; itr2 < 30; itr2++) {
				if(userDefinedPath[itr2] == NULL) {
					write(STDERR_FILENO, error_message, strlen(error_message));
					break;
				}

				char *executable = (char *)malloc(strlen(userDefinedPath[itr2]) + strlen(myargs[0]) + 1);
				strcat(executable, userDefinedPath[itr2]);
				if(strlen(executable) >=1 &&executable[strlen(executable) - 1] != '/') {
					strcat(executable, "/");
				}

				strcat(executable, myargs[0]);
				strcpy(path, executable);
				if((access(executable, X_OK) == 0)) {
					int frc = fork();
					if(frc == 0){
						execv(path, myargs);
						write(STDERR_FILENO, error_message, strlen(error_message));
					} else if(frc > 0) {
						(void) wait(NULL);
					} else {
						fprintf(stderr, "fork failed \n");
						exit(1);
					}

					break;
				}
			}
		}
	}	

	return;
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
			while(getline(&line, &len, fp) > 1) {
				runShell(line);
			}
			fclose(fp);
		} else {
			write(STDERR_FILENO, error_message, strlen(error_message));
			exit(1);
		}	
	}
	return 0;
}
