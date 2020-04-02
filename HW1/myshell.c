#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#define BUFFER_SIZE 100

int main(void) {
	close(2);
	dup(1);
	char command[BUFFER_SIZE];
	char **argv;
	char *cmd;
	int i,c,cnt;
	while (1) {
		i = 0;
		cnt = 1;
		fprintf(stdout, "my-shell> ");
		memset(command, 0, BUFFER_SIZE);
		fgets(command, BUFFER_SIZE, stdin);
        if(strncmp(command, "exit", 4) == 0){ //exit command
			break;
		}
		for (j = 0; j < strlen(command); j++) {
			if (commant[j] == ' ') {
				cnt++;
			}
		}
		cnt++;
		argv = (char**)malloc(cnt * sizeof(char*));
		if (argv == NULL) {
			fprintf(stdout,"ERROR! failed to allocate memory");
		}
		argv[i] = strtok(command, " \n");
		while (argv[i] != NULL){//extracting parameters from command
			i++;
			argv[i] = strtok(NULL, " \n");
		}
		cmd = argv[0];
		if (strncmp(argv[i - 1], "&", 1) == 0) {//if need to run in background
			argv[i - 1] = NULL;
			pid_t pid=fork();//forking
			if (pid < 0) {//fork has failed
				fprintf(stdout, "fork faild!\n");
			}
			else if (pid == 0) {//fork succeed, we are in child process
				execvp(cmd, argv);//executing the command in background
			}
		}
		else {//if need to run in foreground
			pid_t pid = fork();//forking
			if (pid < 0) {//fork has failed
				fprintf(stdout, "fork faild!\n");
			}
			else if (pid == 0) {//fork succeed, we are in child process
				if (execvp(cmd, argv) < 0) {//executing the command in foreground
					fprintf(stdout,"Error: in exec!\n");//execvp failed
					exit(1);
				}
				else {
					exit(0);
				}
			}
			else {//in parent process
				int value;
				waitpid(pid,&value,0);//waiting to child process to finish
			}
		}
		free(argv);


	}

	return 0;
}
