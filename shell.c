#include "shell.h"
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>

int main(int argc, char *argv[]) {

	FILE * fp;
	char  line[1024];
	int numArgs = argc;
	struct command *result;
	// File name was included, open the file
	if (numArgs > 1) {
		fp = fopen(argv[1], "r");
	}
	while (1) {
		// File name was included, read in a line from the file
		if (numArgs > 1) {
			if (fgets(line, 1024, fp) == '\0') {
                                exit(1);
                        }
		}
		// File name was not given, read commands in from stdin
		else {
			fprintf(stderr, "shell> ");
			if (fgets(line, 1024, stdin) == '\0') {
				exit(1);
			}
		}
		// Parse the line for commands and store in a struct called result
		result = parse_command(line);
		// There is a command to be executed, decide which command
        	if (result->args[0] != '\0') {
			// Command is exit, exit the shell
			if (strcmp(result->args[0], "exit") == 0) {
        			exit(0);
		}
			// Command is cd, change directories
			else if (strcmp(result->args[0], "cd") == 0) {
				// New directory is given, change to that directory
				if (result->args[1] != '\0') {
					int b = chdir(result->args[1]);
					// Change directory command fails, print a message to stderr
					if (b == -1) {
						fprintf(stderr, "Could not chdir to ");
						fprintf(stderr, "%s", result->args[0]);
						fprintf(stderr, ": No such file or directory");
					}
				}
				// No new directory is given, get the home directory
				else {
					char *home = getenv("HOME");
					// A home directory cannot be found, print an error to stderr
					if (home == '\0') {
						fprintf(stderr, "Directory not found.\n");
					}
					// A home directory can be found, change to that directory
					else {
						int c = chdir(home);
						// Chdir fail
						if (c == -1) {
							perror("Could not chdir.");
						}
					}
				}
			}
			// Command is setenv
			else if (strcmp(result->args[0], "setenv") == 0) {
				// No variable is given, print a message to stderr
				if (result->args[1] == '\0') {
					fprintf(stderr, "Could not setenv: No argument\n");
				}
				// Variable is given, unset environment variable
				else if (result->args[2] == '\0') {
					int u = unsetenv(result->args[1]);
					// Unsetenv fail
					if (u == -1) {
						perror("Could not unsetenv.");
					}
				}
				// Variable and value are given, set environment variable with value
				else {
					int s = setenv(result->args[1], result->args[2], 1);
					// Setenv fail
					if (s == -1) {
						perror("Could not setenv: Invalid argument\n");
					}
				}
         		}
			else {
				// Fork a child process
				pid_t a = fork();
     				int status;
				// Fork failed
     				if (a < 0) {
          				perror("Fork failed\n");
          				exit(1);
     				}
				// Child process
     				else if (a == 0) {
					if (result->in_redir != '\0') {
						// Open the file
						int in = open(result->in_redir, O_RDONLY);
						// File open fails, print a message to stderr and exit
						if (in == -1) {
                                                        fprintf(stderr, "Could not open ");
                                                        fprintf(stderr, "%s", result->in_redir); 
                                                        fprintf(stderr, " for input: No such file or directory\n");
                                                	exit(1);
						}
						// Duplicate the file to stdin
						else {
							int d1 = dup2(in, 0);
							// Duplicate fail
							if (d1 == -1) {
								perror("File could not be duplicated.");
								exit(1);
							}
						}
						// Close the file
						close(in);
					}
					if (result->out_redir != '\0') {
						// Open the file
						int out = open(result->out_redir, O_WRONLY | O_CREAT | O_TRUNC, 0666);
						// File open fails, print a message to stderr and exit
						if (out == -1) {
							fprintf(stderr, "Could not open ");
							fprintf(stderr, "%s", result->out_redir);
							fprintf(stderr, " for output: No such file or directory\n");
							exit(1);
						}
						// Duplicate the file to stdout
						else {
							int d2 = dup2(out, 1);
							// Duplicate fail
							if (d2 == -1) {
								perror("File could not be duplicated.");
								exit(1);
							}
						}
						// Close the file
						close(out);
					}
					// Execute file, if it returns then print a message to stderr and exit
          				execvp(*result->args, result->args);
						fprintf(stderr, "Error executing ");
						fprintf(stderr, "%s", result->args[0]);
						fprintf(stderr, ": No such file or directory\n");
               					exit(1);
          				
     				}
				// Parent process
     				else {
					// Ignore keyboard interrupt signal
					signal(SIGINT, SIG_IGN);
					// Wait on child process to finish
                                        pid_t w = wait(&status);
					// Reset the handler to the default
					signal(SIGINT, SIG_DFL);
					// Wait fail
					if (w < 0) {
						perror("Wait failed.");
						exit(1);
					}
					// Child process exited normally
                                        if (WIFEXITED(status)) {
						// Get status of child process when it exited, print a message to stderr
						if (WEXITSTATUS(status) > 0) {
                                            		fprintf(stderr, "Command returned ");
					    		fprintf(stderr, "%d", WEXITSTATUS(status));
                                            		fprintf(stderr, "\n");
						}
					}
					// Child process was killed by a signal, print a message to stderr
                                        if (WIFSIGNALED(status)) {
						int sig = WTERMSIG(status);
                                               	char *com = strsignal(sig);
						fprintf(stderr, "Command killed: ");
						fprintf(stderr, "%s", com);
                                       		fprintf(stderr, "\n");
					}
     				}
			}
		}
		free_command(result);
	}
	fclose(fp);
}
