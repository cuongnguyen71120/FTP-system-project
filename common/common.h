#ifndef COMMON_H
#define COMMON_H

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/socket.h>
#include<netdb.h>
#include<errno.h>
#include<sys/types.h>
#include<fcntl.h>
#include<unistd.h>
#include<arpa/inet.h>
#include<netinet/in.h>

//constant 
#define PORT 8000;
#define MAX 1000 


/* This struct holds command code and argument */
struct command{
    char argum[260];
    char code[10];
};

#endif
