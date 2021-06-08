#ifndef FTCLIENT_H
#define FTCLIENT_H

#include"../common/common.h"

// Option for IPv6 or IPv4 addresses
char *addrtype(int addr_type);

// To read data from keyboard 
void read_input(char *buffer, int size);

// sending the command 
int client_send_cmd(struct command *cmd);

// reading rely from server 
int read_rely();

//getting user and password from user 
void client_login();

#endif 


