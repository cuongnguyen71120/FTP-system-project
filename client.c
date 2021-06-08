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
#include<sys/sendfile.h>
#include<sys/stat.h>

int sock_control;
#include"common/common.h"

//Option for address 
char *addrtype(int addr_type) { 
    switch(addr_type) {     //Option for IPV
        case AF_INET: 
            return "AF_INET"; //Option for IPv4 
        case AF_INET6: 
            return "AF_INET6"; //Option for IPv6
    }
    return "Unknown";
}


//read input from user 
void read_input(char* buffer, int size)
{
	char *nl = NULL;
	memset(buffer, 0, size);

	if ( fgets(buffer, size, stdin) != NULL ) {
		nl = strchr(buffer, '\n');
		if (nl) *nl = '\0'; // truncate, ovewriting newline
	}
}

// create function to send command that user typed 
int client_send_cmd(struct command *cmd)
{
	char buffer[MAXSIZE];
	int rc;

	printf(buffer, "%s %s", cmd->code, cmd->argum);
	
	// Sending command string to the server
	rc = send(sock_control, buffer, (int)strlen(buffer), 0);	
	if (rc < 0) {
		perror("Error sending command to server\n");
		return -1;
	}
	
	return 0;
}
int read_reply(){
	int retcode = 0;
	if (recv(sock_control, &retcode, sizeof(retcode), 0) < 0) {
		perror("Client: Error reading message from server\n");
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

    //sending user command to the LB
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
            perror("Error reading message from server\n");
            exit(1);
            break;

    }
}
//main function 
int main(int argc, char **argv){
    unsigned port =8784;
    struct hostent *hostname; 
    int i = 0;
    char domain[100];
    if (argc < 2) {
        printf("Enter the host:"); 
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
    while(1){
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
        // Login to LB on 8000 port 
        client_login();
       
    
        //Upload file to storage and download file from storage 
        int choice,size,status;
        char filename[20],buffer[100], *f;
        int filehandle;
        struct stat obj;
        printf("Get file\n");
        printf("Put file\n");
        printf("ls\n");
        printf("Quit\n");
        printf("Enter your choice:\n");
        scanf("%d",&choice);
        
        //Option  to choose put file or download file and ls  
        switch(choice){
            case 1:
                printf("Enter your filename to get:");
                scanf("%s", filename);
                strcpy(buffer,"get");
                strcat(buffer,filename);
                send(sockfd,buffer, sizeof(buffer),0);
                recv(sockfd,&size, sizeof(int),0);
                if(!size){
                    printf("No such file on the remote directory\n\n");
                    break;
                }
                f = malloc(size);
                recv(sockfd, f, size, 0);
                while(1){
                    filehandle = open (filename, O_CREAT | O_EXCL | O_WRONLY,0666);
                    if(filehandle == -1){
                        sprintf(filename+strlen(filename), "%d",i);//needed only if same directory is used for both server and client

                    }
                    break;
                }
                write(filehandle,f, size);
                close(filehandle);
                strcpy(buffer, "cat");
                strcat(buffer,filename);
                system(buffer);
                break;
            
            case 2:
                printf("Enter filename to put to server: ");
                scanf("%s", filename);
                filehandle = open(filename, O_RDONLY);
                if(filehandle == -1)
                {
                    printf("No such file on the local directory\n\n");
                    break;
                }
                strcpy(buffer, "put ");
                strcat(buffer, filename);
                send(sockfd, buffer, 100, 0);
                stat(filename, &obj);
                size = obj.st_size;
                send(sockfd, &size, sizeof(int), 0);
                sendfile(sockfd, filehandle, NULL, size);
                recv(sockfd, &status, sizeof(int), 0);
                if(status){
                    printf("File stored successfully\n");
                }
                else{
                    printf("File failed to be stored to remote machine\n");
                }
                break;

            case 3:
                strcpy(buffer, "ls");
                send(sockfd, buffer, sizeof(buffer), 0);
	            recv(sockfd, &size, sizeof(int), 0);
                f = malloc(size);
                recv(sockfd, f, size, 0);
	            filehandle = creat("temp.txt", O_WRONLY);
	            write(filehandle, f, size);
	            close(filehandle);
                printf("The remote directory listing is as follows:\n");
	            system("cat temp.txt");
	            break;
            case 4:
                strcpy(buffer, "quit");
                send(sockfd, buffer,sizeof(buffer), 0);
                recv(sockfd, &status, 100, 0);
	            if(status)
	            {
                    printf("Server closed\nQuitting..\n");
                    exit(0);
	            }
	            printf("Server failed to close connection\n");
	    }
    
   
    return 0;
}
}

