#include <stdio.h>
#include <stdlib.h>
#include <string.h> //strlen 
#include <errno.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>  //inet_addr
#include<fcntl.h>
#define PORT 8000;
#define MAXSIZE 1000 

int sock_control;
#include"common/common.h"


char *addrtype(int addr_type) { 
    switch(addr_type) {     //Option for IPV
        case AF_INET: 
            return "AF_INET"; //Option for IPv4 
        case AF_INET6: 
            return "AF_INET6"; //Option for IPv6
    }
    return "Unknown";
}

void read_input(char* buffer, int size)
{
	char *nl = NULL;
	memset(buffer, 0, size);

	if ( fgets(buffer, size, stdin) != NULL ) {
		nl = strchr(buffer, '\n');
		if (nl) *nl = '\0'; // truncate, ovewriting newline
	}
}

int client_command(char *buf, int size,struct command *structcmd){
    memset(structcmd->code,0,sizeof(structcmd->code));
    memset(structcmd->argum,0,sizeof(structcmd->argum));

    printf("[Client]: "); //Prompt for input 
    fflush(stdout);
    //waiting for user to enter a command  
    read_input(buf,size);
    
    char *arg = NULL;
    arg = strtok(buf, " ");
    arg = strtok(NULL, " ");

    if(arg != NULL){
        //Stroring the argument if there is one 
        strncpy(structcmd->argum,arg,strlen(arg));
    }
    //buf=command 
    if(strcmp(buf, "ls")==0){
        strcpy(structcmd->code,"LIST");
    }
    else if(strcmp(buf, "get")==0){
        strcpy(structcmd->code,"RETR");
    }
    else if(strcmp(buf, "bye")==0){
        strcpy(structcmd->code,"QUIT");
    }
    else if(strcmp(buf, "put")==0){
        strcpy(structcmd->code, "SEND");
    }
    else { // invalid 
        return -1;
    }

    //Storing the code in beginning of the buffer
    memcpy(buf,0,sizeof(buf));
    strcpy(buf, structcmd->code);

    //if there's an arg, then appending it to the buffer
    if(arg != NULL){
        strcat(buf," ");
        strncat(buf, structcmd->argum, strlen(structcmd->argum));
    }
    return 0;
}
int client_send_cmd(struct command *cmd)
{
	char buffer[MAXSIZE];
	int rc;

	sprintf(buffer, "%s %s", cmd->code, cmd->argum);
	
	// Sending command string to the server
	rc = send(sock_control, buffer, (int)strlen(buffer), 0);	
	if (rc < 0) {
		perror("Error sending command to server");
		return -1;
	}
	
	return 0;
}
int read_reply(){
	int retcode = 0;
	if (recv(sock_control, &retcode, sizeof retcode, 0) < 0) {
		perror("client: error reading message from server\n");
		return -1;
	}	
	return ntohl(retcode);
}


//get login details from the user and sending it to the server for authentication 
void client_login(){
    struct command cmd;
    char user[256];
    memset(user,0,256);

    //getting username from user 
    printf("Username: ");
    fflush(stdout);
    read_input(user,256);

    //sending user command to the server
    strcpy(cmd.code, "USER");
    strcpy(cmd.argum, user);
    client_send_cmd(&cmd);

    //Waiting for go ahead to send the password 
    int wait;
    recv(sock_control, &wait, sizeof(wait), 0);

    //getting the password from the user 
    fflush(stdout);
    char *pass = getpass("Password: ");

    //send pass command to the server
    strcpy(cmd.code, "PASS");
    strcpy(cmd.argum, pass);
    client_send_cmd(&cmd);

    //waiting for the response 
    int code = read_reply();
    switch(code){
        case 430:
            printf("Invalid username/password.\n");
            exit(0);
        case 230:
            printf("Successful login.\n");
            break;
        default:
            perror("Error reading message from server");
            exit(1);
            break;

    }
}

int main(int argc, char **argv){
    unsigned port =8000;
    struct hostent *hostname; 
    int i = 0;
    char domain[100];
    if (argc < 2) {
        printf("Enter the domain:"); 
        scanf("%s",domain);
        hostname=gethostbyname(domain); 
    }
    else{
        hostname = gethostbyname(argv[1]);
        }
        
    if (!hostname) {
        printf("Lookup Failed: %s\n", hstrerror(h_errno));
        exit(0);
        }
    while(hostname ->h_addr_list[i] != NULL) {
        printf("The IP address:");
        printf("%s\n", inet_ntoa((struct in_addr)*((struct in_addr *) hostname->h_addr_list[i])));
        i++;
        }
    // Client for socket
    struct sockaddr_in addr;
    int sockfd = socket(AF_INET,SOCK_STREAM,0);
    if (sockfd<0){
        printf("Erro socket\n");
        exit(0);
    }
    printf("Socket created\n");
    memset(&addr,0,sizeof(addr));
    addr.sin_family=AF_INET;
    memcpy((char *) &addr.sin_addr.s_addr, hostname->h_addr_list[0], hostname->h_length);
    addr.sin_port = htons(port);
    //connect with LB
    int connect_lb=connect(sockfd,(struct sockaddr *)&addr,sizeof(addr));
    if (connect_lb<0){
        printf("Error connect\n");
        exit(0);
    }
    else{
        printf("Connected with LB\n");
        //reuse address 
        setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,&(int){1}, sizeof(int));
        //enable nonblocking 
        fcntl(sockfd,F_SETFL,O_NONBLOCK);
    }
    client_login();

    return 0;

}