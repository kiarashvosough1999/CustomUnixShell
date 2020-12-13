//
//  logutilities.h
//  os
//
//  Created by Kiarash Vosough on 9/13/1399 AP.
//

#ifndef logutilities_h
#define logutilities_h

#include "colors.h"
#include <stdio.h>
#include <stdlib.h>

static void saveCommand(char* args[]){
    FILE * fp;
    int i = 0;
    fp = fopen ("commands.txt","a+");
    while (args[i] != NULL) {
        fprintf(fp, "%s ", args[i]);
        i++;
    }
    fprintf(fp, "\n");
    fclose (fp);
}

static void printHistory(){
    FILE * fp;
    char * line = NULL;
    size_t len = 0;
    ssize_t read;
    
    fp = fopen("commands.txt", "r");
    if (fp == NULL)
        exit(EXIT_FAILURE);
    printf("Command Histories are: \n");
    while ((read = getline(&line, &len, fp)) != -1) {
        printf("\t-> %s", line);
    }
    fclose(fp);
    if (line)
        free(line);
}

void printHelp(){
    printf("%ssin this shell you can run:",color_magenta);
    printf("\n  quit\t\t\t to exit from shell");
    printf("\n  msg\t\t\t take on argument that send to other shells (Other shells should listen for message by executing 'openmsg' command)");
    printf("\n  openmsg\t\t listening for messages");
    printf("\n  gethis\t\t get history of commnads\n");
}

#endif /* logutilities_h */
