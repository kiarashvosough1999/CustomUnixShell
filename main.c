
//
//  main.c
//  os
//
//  Created by Kiarash Vosough on 9/4/1399 AP.
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/errno.h>
#include "messagingUtilities.h"
#include "logutilities.h"
#include "colors.h"
 
/* The array below will hold the arguments: args[0] is the command. */
static char* args[512];
pid_t pid;
int command_pipe[2];
 
#define READ  0
#define WRITE 1
 
static int analyizeCommand(char* cmd, int input, int first, int last);
static int executeCommand(int input_From_LastCommand, int is_first_command, int is_last_command);
static int analyizeCommand(char* command, int input, int is_first_command, int is_last_command);
static void executeBatchFile(char* fileName);
static void seperateCommands(char* command);
static void waitToTerminate(int n);
static char* ignoreWhiteSpaces(char* space);

static char line[1024];
static int number_of_procces = 0;
 
int main( int argc, char *argv[] )
{
    
    printf("%sKia SHELL: Type 'quit' or send EOF to terminate.\n",color_yellow);
    
    if (argc == 2) {
        executeBatchFile(argv[1]);
        return 0;
    }
    
    while (1) {
        /* Print the command prompt */
        char cwd[1024];
        // printf("%s\n", );
        getcwd(cwd,sizeof(cwd));
        cwd[0] = '~';
        printf("%s%s $-> %s",color_yellow,cwd,color_white);
        fflush(NULL);
        
        /* Read a command line */
        if (!fgets(line, 1024, stdin))
            return 0;
 
        /*
           'input' indicates that if we have more than one command, it will hold last command procces pipe read end
           'first' indicates the first command, it will be 0 after first command
        */
        int input_from_last_pipecommand = 0;
        int is_first_command = 1;
 
        
        char* command = line;
        
        char* next = strchr(command, '|'); /* Find first '|' */
        
        while (next != NULL) {
            /* 'next' points to '|' */
            *next = '\0';
            input_from_last_pipecommand = analyizeCommand(command, input_from_last_pipecommand, is_first_command, 0);
 
            command = next + 1;
            next = strchr(command, '|'); /* Find next '|' */
            is_first_command = 0;
        }
        input_from_last_pipecommand = analyizeCommand(command, input_from_last_pipecommand, is_first_command, 1);
        waitToTerminate(number_of_procces);
        number_of_procces = 0;
    }
    
    return 0;
}

static void executeBatchFile(char* fileName){
    FILE *fp;
    char* fullFileName = NULL;
    
    strcpy(fullFileName, fileName);
    strcat(fullFileName, ".txt");
    
    fp = fopen(fullFileName, "r");
    while (fgets(line, 1024, fp) != NULL) {
        /* Print the command prompt */
        printf("%s\n$-> %s\n",color_green,color_white);
        fflush(NULL);
        
        int input_from_last_pipecommand = 0;
        int is_first_command = 1;
        
        char* command = line;
        
        char* next = strchr(command, '|'); // Find first '|'
        
        while (next != NULL) {
            /* 'next' points to '|' */
            *next = '\0';
            input_from_last_pipecommand = analyizeCommand(command, input_from_last_pipecommand, is_first_command, 0);
            
            command = next + 1;
            next = strchr(command, '|'); // Find next '|'
            is_first_command = 0;
        }
        input_from_last_pipecommand = analyizeCommand(command, input_from_last_pipecommand, is_first_command, 1);
        waitToTerminate(number_of_procces);
        number_of_procces = 0;
    }
}

static int analyizeCommand(char* command, int input, int is_first_command, int is_last_command)
{
    seperateCommands(command);
    if (args[0] != NULL) {
        if (strcmp(args[0], "quit") == 0)
            exit(0);
        else if (strcmp(args[0], "msg") == 0) {
            sendMessage(args[1]);
            return 0;
        }
        else if (strcmp(args[0], "openmsg") == 0) {
            listenForMessages();
            return 0;
        }
        else if (strcmp(args[0], "gethis") == 0) {
            printHistory();
            return 0;
        }else if (strcmp(args[0], "help") == 0) {
            printHelp();
            return 0;
        }
        else if (strcmp(args[0], "cd") == 0) {
            if(args[1]==NULL) {
                char cwd[1024];
                printf("%s\n", getcwd(cwd,sizeof(cwd)));
                return 0;
            } else {
                if(chdir(args[1]) == -1) {
                    fprintf(stderr, "chdir error: %s\n", strerror(errno));
                    return 0;
                }
            }
            return 0;
        }
        
        number_of_procces += 1;
        saveCommand(args);
        return executeCommand(input, is_first_command, is_last_command);
    }
    return 0;
}

/*
 * Handle commands separatly
 * input: return value from previous command (useful for pipe file descriptor)
 * first: 1 if first command in pipe-sequence (no input from previous pipe)
 * last: 1 if last command in pipe-sequence (no input from previous pipe)
 *
 * EXAMPLE: If you type "ls | grep shell | wc" in your shell:
 *    fd1 = command(0, 1, 0), with args[0] = "ls"
 *    fd2 = command(fd1, 0, 0), with args[0] = "grep" and args[1] = "shell"
 *    fd3 = command(fd2, 0, 1), with args[0] = "wc"
 *
 * So if 'command' returns a file descriptor, the next 'command' has this
 * descriptor as its 'input'.
 */

static int executeCommand(int input_From_LastCommand, int is_first_command, int is_last_command)
{
    int pipeFileDescriptor[2];
 
    /* Invoke pipe */
    pipe(pipeFileDescriptor);
    /* call fork after creating a pipe, then the parent and child can communicate via the pipe */
    pid = fork();
 
    /*
     STDIN_FILENO    0     standard input file descriptor
     STDOUT_FILENO   1     standard output file descriptor
     STDIN --> O --> O --> O --> STDOUT
     */
    if (pid == 0) {
        if (is_first_command == 1 && is_last_command == 0 && input_From_LastCommand == 0) {
            /*
               It will write the output of first command on pipe
               so if there is second pipe command it can read data from it
               actually this line of code connect(copy, duplicate) std_out to write end of pipe
            */
            dup2(pipeFileDescriptor[WRITE], STDOUT_FILENO);
        } else if (is_first_command == 0 && is_last_command == 0 && input_From_LastCommand != 0) {
            /*
              read output of first command and use it in middle command
            */
            dup2(input_From_LastCommand, STDIN_FILENO);
            dup2(pipeFileDescriptor[WRITE], STDOUT_FILENO);
        } else {
            /*
              read output of middle command and use it in last command
            */
            dup2( input_From_LastCommand, STDIN_FILENO );
        }
 
        if (execvp( args[0], args) == -1){
            _exit(EXIT_FAILURE); // If child fails
        }
            
    }
 
    if (input_From_LastCommand != 0)
        close(input_From_LastCommand);
 
    // close write end of pipe
    close(pipeFileDescriptor[WRITE]);
 
    // If it is the last command, nothing more needs to be read from read end of pipe
    if (is_last_command == 1)
        close(pipeFileDescriptor[READ]);
 
    return pipeFileDescriptor[READ];
}
 
static char* ignoreWhiteSpaces(char* space)
{
    while (isspace(*space)) ++space;
    return space;
}
 
static void seperateCommands(char* command)
{
    command = ignoreWhiteSpaces(command);
    char* next = strchr(command, ' ');
    int i = 0;
 
    while(next != NULL) {
        next[0] = '\0';
        args[i] = command;
        ++i;
        command = ignoreWhiteSpaces(next + 1);
        next = strchr(command, ' ');
    }
 
    if (command[0] != '\0') {
        args[i] = command;
        next = strchr(command, '\n');
        next[0] = '\0';
        ++i;
    }
 
    args[i] = NULL;
}

/*
   'wait' for processes to terminate
*/
static void waitToTerminate(int n) {
    for (int i = 0; i < n; ++i)
        wait(NULL);
}
