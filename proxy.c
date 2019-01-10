#include <stdio.h>

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 16777216 
#define MAX_OBJECT_SIZE 8388608 

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";


/* $begin tinymain */
/*
 * tiny.c - A simple, iterative HTTP/1.0 Web server that uses the 
 *     GET method to serve static and dynamic content.
 */

#include "csapp.h"

void doit(int fd);
void read_requesthdrs(rio_t *rp);
void Server_request(int fd, char *portOut, char *hostOut, char *filename);
void parse_uri(char *uri, char *filename, char *portOut, char *hostOut);
void serve_static(int fd, char *filename, int filesize);
void get_filetype(char *filename, char *filetype);
void serve_dynamic(int fd, char *filename, char *cgiargs);
void clienterror(int fd, char *cause, char *errnum, 
		 char *shortmsg, char *longmsg);



int main(int argc, char **argv) 
{
    int listenfd, connfd;
    char hostname[MAXLINE], port[MAXLINE];
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;

    /* Check command line args */
    if (argc != 2) {
	fprintf(stderr, "usage: %s <port>\n", argv[0]);
	exit(1);
    }

    Signal(SIGPIPE, SIG_IGN); //SIGPIPE "wrote to a pipe with no reader" is ignored
    listenfd = Open_listenfd(argv[1]); //see csapp.c
    
    //gathers cleint headers and calls doit
    while (1) {
	clientlen = sizeof(clientaddr);
	connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen); 
        Getnameinfo((SA *) &clientaddr, clientlen, hostname, MAXLINE, 
                    port, MAXLINE, 0);
        printf("Accepted connection from (%s, %s)\n", hostname, port);
	doit(connfd);                                          
	Close(connfd);                                          
    }
}


void doit(int fd) 
{
    struct stat;
    char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
    char filename[MAXLINE];
    char portOut[MAXLINE]; 
    char hostOut[MAXLINE];
    rio_t rio;
    
    strcpy(buf, "");

    /* Read request line and headers */
    rio_readinitb(&rio, fd); 
    if (!rio_readlineb(&rio, buf, MAXLINE))
      return;

    printf("%s\n", buf); //prints 3rd line Host: localhost:31287
    
    sscanf(buf, "%s %s %s", method, uri, version);
    if (strcasecmp(method, "GET")) {                     
        clienterror(fd, method, "501", "Not Implemented",
                    "Proxy does not implement this method");
        return;
    }
    //call cache here with uri
    //prints out headers, port and host are parsed from buffer
    read_requesthdrs(&rio);            

    /* Parse URI from GET request */
    parse_uri(uri, filename, portOut, hostOut);      

    //get static content from tiny
    if (strlen(hostOut) > 1) {
      Server_request(fd, portOut, hostOut, filename);
    }

    return;
}

void Server_request(int fd, char *portOut, char *hostOut, char *filename) {

      int proxyfd = Open_clientfd(hostOut, portOut);
      char bufret[MAXLINE]; 
      char Requestline[MAXLINE];
      char payload[MAX_OBJECT_SIZE];
      size_t n;
      rio_t rio;
      int sum;

      strcpy(bufret, "");
      strcpy(payload, "");
      strcpy(Requestline, "");

      snprintf(Requestline, MAXLINE, 
	       "GET %s HTTP/1.0\r\n" 
	       "Host: %s\r\n"     
	       "User-Agent: %s"
	       "Connection: close\r\n"
	       "Proxy-Connection: close\r\n"
	       "\r\n",
	       filename, hostOut, user_agent_hdr);

      rio_readinitb(&rio, proxyfd);

      rio_writen(proxyfd, Requestline, strlen(Requestline));
      
      //only prints out response headers
      /*while(strcmp(bufret, "\r\n")) {
	rio_readlineb(&rio, bufret, MAXLINE);
	printf("%s", bufret);
	}*/
      
      //sends everything to the client
      while((n=rio_readnb(&rio, payload, MAXLINE)) != 0) {
	sum += n;
	//strcat(payload, bufret);
	//memcpy(bufret, payload, n);
	rio_writen(fd, payload, MAXLINE);
	//Rio_readnb(&rio, bufret, MAXLINE);
	//printf("%s", payload);
      }
      
      //rio_writen(fd, payload, sum);
      
      printf("Forward respond %d bytes\n", sum);      

      Close(proxyfd);
}

/*
void cache_mustache(char *payload) {

  struct Node {
    int data;
    int descriptor;
    int size;
    struct Node* next;
    struct Node* prev;
  }

  void push(struct Node** head_ref, int new_data) 
  { 
    Node* new_node = (struct Node*)malloc(sizeof(struct Node)); 
    new_node->data = new_data; 
    new_node->next = (*head_ref); 
    new_node->prev = NULL; 
    if ((*head_ref) != NULL) 
        (*head_ref)->prev = new_node; 
    (*head_ref) = new_node; 
	return;
  } 
  
  void insertAfter(struct Node* prev_node, int new_data) 
  { 
    if (prev_node == NULL) { 
        printf("the given previous node cannot be NULL"); 
        return; 
    } 
    struct Node* new_node = (struct Node*)malloc(sizeof(struct Node)); 
    new_node->data = new_data; 
    new_node->next = prev_node->next; 
    prev_node->next = new_node; 
    new_node->prev = prev_node; 
    if (new_node->next != NULL) 
        new_node->next->prev = new_node; 
	return;
  } 
  
  void append(struct Node** head_ref, int new_data) 
  { 
    struct Node* new_node = (struct Node*)malloc(sizeof(struct Node)); 
  	struct Node* last = *head_ref;

    new_node->data = new_data; 
    new_node->next = NULL; 
    if (*head_ref == NULL) { 
        new_node->prev = NULL; 
        *head_ref = new_node; 
        return; 
    } 
    while (last->next != NULL) 
        last = last->next; 
    last->next = new_node; 
    new_node->prev = last; 
  
    return; 
  } 
  
  void printList(struct Node* node) 
  { 
    struct Node* last; 
    printf("\nTraversal in forward direction \n"); 
    while (node != NULL) { 
        printf(" %d ", node->data); 
        last = node; 
        node = node->next; 
    } 
  
    printf("\nTraversal in reverse direction \n"); 
    while (last != NULL) { 
        printf(" %d ", last->data); 
        last = last->prev; 
    } 
  } 


}*/



void read_requesthdrs(rio_t *rp) 
{
  //input here is typically line 2 "GET /pepe.gif HTTP/1.1"
  char buf2[MAXLINE];

    if (!rio_readlineb(rp, buf2, MAXLINE))
      return;
    printf("%s", buf2);
    //This loop prints out lines from "Accept: */* to Response headers:"
    while(strcmp(buf2, "\r\n")) {        
      rio_readlineb(rp, buf2, MAXLINE);
      printf("%s", buf2);
    }
    return;
}

//set to global vars port and host
//port has extra newline char, not sure how to remove

//Super messy, some redundant assignments
void parse_uri(char *uri, char *filename, char *portOut, char *hostOut) 
{   
  
  int count = 0;
  char *tempHost;
  char *tempPort;
  char *tempfilename;

  
  tempPort = strtok(uri, ":");
  
  count = 0;
  while(tempPort != NULL) {
    
    if(count == 1) {
      tempHost = tempPort;
    }
    if(count == 2) {
      break;
    }
    count++;
    tempPort = strtok(NULL, ":");
  }

  if(tempPort == NULL) {
	strcpy(portOut, "80");
     
	tempfilename = strtok(tempHost, "/");
	
	//deals with no port and no filepath.
	if(strlen(tempfilename)+5 > strlen(tempHost)) {
	  tempHost = strtok(tempHost, "/");
	  strcpy(hostOut, tempHost);
	  strcpy(filename, "/ \0");
	}
	else {
	  tempHost = strtok(tempHost, "/");
	  strcpy(hostOut, tempHost); 
	
	  tempfilename = strtok(NULL, "/");
	  if(tempfilename != NULL) {
	    strcpy(filename, "/");
	    strcat(filename, tempfilename);
	    strcat(filename, "\0");
	  }
	}
  }
  else {
	tempfilename = strtok(tempPort, "/");
	tempfilename = strtok(NULL, "/");
	tempPort = strtok(tempPort, "/");
	tempHost = strtok(tempHost, "/");
	strcpy(hostOut, tempHost);
	strcpy(portOut, tempPort);
	
	strcpy(filename, "/");
	if(tempfilename != NULL) {
	  strcat(filename, tempfilename);
	  strcat(filename, "\0");
	}
      }
  
  
  // printf("Parse Output: %s %s %s\n", hostOut, portOut, filename);
  return;
}



void get_filetype(char *filename, char *filetype) 
{
    if (strstr(filename, ".html"))
	strcpy(filetype, "text/html");
    else if (strstr(filename, ".gif"))
	strcpy(filetype, "image/gif");
    else if (strstr(filename, ".png"))
	strcpy(filetype, "image/png");
    else if (strstr(filename, ".jpg"))
	strcpy(filetype, "image/jpeg");
    else if (strstr(filename, ".mp4"))
        strcpy(filetype, "video/mp4");
    else if (strstr(filename, ".mp3"))
        strcpy(filetype, "audio/mp3");
    else
	strcpy(filetype, "text/plain");
}  



void clienterror(int fd, char *cause, char *errnum, 
		 char *shortmsg, char *longmsg) 
{
    char buf[MAXLINE], body[MAXBUF];

    /* Build the HTTP response body */
    sprintf(body, "<html><title>Tiny Error</title>");
    sprintf(body, "%s<body bgcolor=""ffffff"">\r\n", body);
    sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
    sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
    sprintf(body, "%s<hr><em>The Tiny Web server</em>\r\n", body);

    /* Print the HTTP response */
    sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
    sprintf(buf, "%sContent-type: text/html\r\n", buf);
    sprintf(buf, "%sContent-length: %d\r\n\r\n", buf, (int)strlen(body));
    rio_writen(fd, buf, strlen(buf));
    rio_writen(fd, body, strlen(body));
}
/* $end clienterror */
