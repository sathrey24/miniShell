/*******************************************************************************
 * Author : Sanjay Athrey
 * Pledge : I pledge my honor that I have abided by the Stevens Honor System.
 ******************************************************************************/
#include <setjmp.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>
#include <ctype.h>
#include <sys/wait.h>
#include <pwd.h>

#define BRIGHTBLUE "\x1b[34;1m"
#define DEFAULT    "\x1b[0m"
#define BUFSIZE 128

sigjmp_buf jmpbuf;

sig_atomic_t child_status = 0;

void catch_signal(int sig) {
    if (child_status == 1){
        write(STDOUT_FILENO, "\n", 1);
        siglongjmp(jmpbuf, 1);
    }

}

char * format_string(char *str) {
    char *dst = str;
    for (; *str; ++str) {
        *dst++ = *str;
        if (isspace(*str)) {
            do ++str; 
            while (isspace(*str));
            --str;
        }
    }
    *dst = 0;
    return dst;
}


int arg_counter(char *args){
    int count = 1;
    for (int i = 0; args[i] != '\0'; i++){   
            if (args[i] == ' '){             
                count++;
            }
        }
        if (args[strlen(args) - 1] == ' '){
            count -=1;
        }       
        return count;
}

char** parse(char *str, int argc){
        char **argv = malloc(256 * sizeof(char*));       
    	for(int i=0; i<argc; i++){
        	argv[i] = malloc(sizeof(char) * argc);
    	}
    	char* arg = strtok(str, " ");
    	int i = 0;
    	while(arg != NULL){
        	strcpy(argv[i], arg);
        	arg = strtok(NULL, " ");
        	i++;
    	}
   	argv[i] = NULL; 
	return argv;
}

void execute(char **argv, int argc){
	pid_t pid;  
	int status;
	
    if((pid = fork()) < 0) {
		fprintf(stderr,"Error: fork() failed. %s.\n", strerror(errno));
		for(int i=0; i<argc; i++){
            free(argv[i]);
		}
		free(argv);	
        exit(EXIT_FAILURE);
    

	} else if (pid == 0) { 
		if(execvp(argv[0], argv) < 0){
			fprintf(stderr, "Error: exec() failed. %s.\n", strerror(errno));
			for(int i=0; i<argc; i++){
              		  	free(argv[i]);
			}
            free(argv);	
			exit(EXIT_FAILURE);
            
        
		}	
	} else { 
        child_status = 0;
        if (wait(&status) == -1) {
            fprintf(stderr, "Error: wait() failed. %s\n",strerror(errno));
            for (int i=0; i<argc; i++){
                free(argv[i]);
		    }
		    free(argv);	
            exit(EXIT_FAILURE);
        child_status = 1;
      
        }   	
}
}


int main() {
    struct sigaction action;

    memset(&action, 0, sizeof(struct sigaction));
    action.sa_handler = catch_signal;
    action.sa_flags = SA_RESTART; 

    if (sigaction(SIGINT, &action, NULL) == -1) {
        fprintf(stderr, "Error: Cannot register signal handler. %s.\n",
            strerror(errno));
       
    }

    char path[PATH_MAX];
    char buf[BUFSIZE];
    char *args = buf;
    char empty[] = " ";
    
    
    sigsetjmp(jmpbuf, 1);
    do {
        if (getcwd(path, sizeof(path)) == NULL) {
            fprintf(stderr, "Error: Cannot get current working directory. %s.\n", strerror(errno));
            
        }
        printf("[%s%s%s]$ ", BRIGHTBLUE, path, DEFAULT);
        memset(path, 0, sizeof(char)*PATH_MAX);
        fflush(stdout);
        args[strlen(args)] = '\0';
        ssize_t bytes_read = read(STDIN_FILENO, buf, BUFSIZE);
        if (bytes_read > 0) {
            buf[bytes_read - 1] = '\0';
        }
        if (bytes_read <= 1){         
            continue;       
        }
       
        if (strncmp(buf, "exit", 5) == 0) {                              
            break;       
            }

        if (args[0] == ' '){
            int i;
            for(i =1;i<strlen(args);i++) {
		        args[i-1]=args[i];
	        }
            args[i-1]='\0';  
         
        }
        
   if (strlen(args) < 1){
       siglongjmp(jmpbuf, 1);
   }
    for (int i = 0; i < strlen(args); i++){
        if (strcmp(&buf[i], empty) == 0){
           siglongjmp(jmpbuf, 1);
        }
       
    }
       format_string(args);
       int argc= arg_counter(args);
       char **argv = parse(args, argc);

       
       if (argv == NULL){
            fprintf(stderr,"Error: malloc() failed. %s.\n", strerror(errno));
            for(int i=0; i<argc; i++){
                free(argv[i]);
		    }
		    free(argv);	
            
        }
       
    
  

   
    if (((strcmp(argv[0], "cd") == 0) && (argc <= 1)) || ((strcmp(argv[0], "cd") == 0) && (strcmp(argv[1], "~") == 0))){  
        uid_t uid = getuid();
        struct passwd* pwd;
        if ((pwd = getpwuid(uid)) == NULL) {
            fprintf(stderr, "Cannot get passwd entry. %s\n",strerror(errno));
                
        }
        if (pwd){
            char *homeDir = pwd->pw_dir;
            if (chdir(homeDir) == -1){
                fprintf(stderr, "Error: Cannot change directory to '~'. %s.\n", strerror(errno));
                for(int i=0; i<argc; i++){
                    free(argv[i]);
		        }
		        free(argv);	
                
            }
        }
    }
   
    else if ((strcmp(argv[0], "cd") == 0) && (strcmp(argv[1], "~") != 0)){
        if (argc == 2 || argc == 3){
            char* directory = argv[1];
            if (chdir(directory) == -1){
                fprintf(stderr, "Error: Cannot change directory to '%s'. %s.\n", directory, strerror(errno));
                
            }
            for(int i=0; i<argc; i++){
                free(argv[i]);
		    }
		    free(argv);	
        }
        else {
            fprintf(stderr, "Error: Too many arguments to cd.\n");
            for(int i=0; i<argc; i++){
                free(argv[i]);
		    }
		    free(argv);	
           
        }
    }
    else {
   
            format_string(args);
            execute(argv, argc);           
            memset(buf, 0, sizeof(char)*strlen(buf));
        }
    } while (true);
    
   return EXIT_SUCCESS;
}
