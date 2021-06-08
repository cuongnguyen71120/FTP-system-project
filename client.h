#ifndef FTCLIENT_H
#define FTCLIENT_H

#include"../common/common.h"

// Option for IPv6 or IPv4 addresses
char *addrtype(int addr_type);

// To read data from keyboard 
void read_input(char *buffer, int size);

//Create this function to list command to help user are using easier and it has function look like linux command
//int client_command(char *buf, int size, struct command *structcmd );

// sending the command 
int client_send_cmd(struct command *cmd);

// reading rely from server 
int read_rely();

//getting user and password from user 
void client_login();

#endif 


