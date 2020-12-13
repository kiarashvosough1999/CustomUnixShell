//
//  messagingUtilities.h
//  os
//
//  Created by Kiarash Vosough on 9/13/1399 AP.
//

#ifndef messagingUtilities_h
#define messagingUtilities_h

#include "colors.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>


void listenForMessages(){
    // FIFO file path
    int fd1;
    char * myfifo = "/tmp/myfifo";
    mkfifo(myfifo, 0666);
    
    char str1[80];
    
    while (1) {
        fd1 = open(myfifo,O_RDONLY);
        read(fd1, str1, 80);
        
        printf("%sBroadcasted: %s\n",color_cyan, str1);
        close(fd1);
        unlink(myfifo);
        break;
    }
}

void sendMessage(char* msg){
    int fd;
    char * myfifo = "/tmp/myfifo";
    
    mkfifo(myfifo, 0666);
    
//    if (0 != mkfifo(myfifo, 0666)){
//        printf("%sno other shell are listening, read help for instruction\n",KRED);
//        return;
//    }
    
    fd = open(myfifo, O_WRONLY | O_CREAT);
    if (fd < 0) {
        printf("%sno other shell are listening, read help for instruction\n",color_red);
        return;
    }
    
    if (-1 == write(fd, msg, strlen(msg)+1)){
        printf("%sno other shell are listening, read help for instruction\n",color_red);
        return;
    }
    close(fd);
    unlink(myfifo);
}

#endif /* messagingUtilities_h */
