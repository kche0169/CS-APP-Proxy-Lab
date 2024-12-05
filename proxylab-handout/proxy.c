#include <stdio.h>
#include "csapp.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";
void doit(int fd);
void read_requesthdrs(rio_t *rp, char *headers);
int parse_uri(char *uri, char *filename, char *cgiargs);
void clienterror(int fd, char *cause, char *errnum, 
		 char *shortmsg, char *longmsg);

int main(int argc, char** argv)
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
    listenfd = Open_listenfd(argv[1]);
    while (1) {
        clientlen = sizeof(clientaddr);
        connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen); //line:netp:tiny:accept
        Getnameinfo((SA *) &clientaddr, clientlen, hostname, MAXLINE, 
                    port, MAXLINE, 0);
        printf("Accepted connection from (%s, %s)\n", hostname, port);
        doit(connfd);                                             //line:netp:tiny:doit
        Close(connfd);                                            //line:netp:tiny:close
    }
    return 0;
}

/*
 * doit - handle one HTTP request/response transaction
 */
/* $begin doit */
void doit(int fd) {
    char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
    char path[MAXLINE], Host[MAXLINE], headers[MAXLINE];
    rio_t rio_input, rio_output;
    int clientfd_to_tiny;
    printf("buf data:\n");
    
    /* Read request line and headers */
    Rio_readinitb(&rio_input, fd); // 初始化写入客户端的RIO
    if (!Rio_readlineb(&rio_input, buf, MAXLINE))  // Read request line
        return;
    printf("%s", buf);
    
    sscanf(buf, "%s %s %s", method, uri, version);
    printf("method = %s\n", method);

    if (strcmp(method, "GET")) {  // Only handle GET requests
        return;
    }
    read_requesthdrs(&rio_input, headers);
    printf("headers = %s\n", headers);
    return;
    

    // /* Parse URI to extract Host and Path */

    // char* hostbegin = strstr(uri, "\\") + 2;
    // char *hostend = strpbrk(hostbegin, " :/\r\n");
    // int hostlen = hostend - hostbegin;
    // strncpy(Host, hostbegin, hostlen);
    // Host[hostlen] = '\0';

    // char *portptr = strchr(hostbegin, ':');
    // char port[10] = "80"; // 默认端口

    // if (portptr) {
    //     sscanf(portptr, ":%s", port);
    // }

    // char *pathptr = strchr(hostbegin, '/');
    // if (pathptr) {
    //     strcpy(path, pathptr);
    // } else {
    //     strcpy(path, "/");
    // }

    // printf("Host: %s, Path: %s\n", Host, path);  // Debugging output

    // /* Connect to the target server */
    // clientfd = Open_clientfd(Host, port);  // Using default HTTP port 80
    // if (clientfd < 0) {
    //     fprintf(stderr, "Failed to connect to %s:80\n", Host);
    //     return;
    // }

    // Rio_readinitb(&rio_output, clientfd);

    // /* Send request to the target server */
    // snprintf(buf, MAXLINE, "GET %s HTTP/1.0\r\n", path);
    // Rio_writen(clientfd, buf, strlen(buf));

    // snprintf(buf, MAXLINE, "Host: %s\r\n", Host);
    // Rio_writen(clientfd, buf, strlen(buf));

    // snprintf(buf, MAXLINE, "%s", user_agent_hdr);
    // Rio_writen(clientfd, buf, strlen(buf));

    // snprintf(buf, MAXLINE, "Connection: close\r\n");
    // Rio_writen(clientfd, buf, strlen(buf));

    // snprintf(buf, MAXLINE, "Proxy-Connection: close\r\n");
    // Rio_writen(clientfd, buf, strlen(buf));

    // Rio_writen(clientfd, headers, strlen(headers));

    // /* Send empty line to end headers */
    // snprintf(buf, MAXLINE, "\r\n");
    // Rio_writen(clientfd, buf, strlen(buf));

    // /* Send response body from target server back to client */
    // int n;
    // while ((n = Rio_readnb(&rio_output, buf, MAXLINE)) > 0) {
    //     Rio_writen(fd, buf, n);
    // }

    // Close(clientfd);
    // return;
}

/* $end doit */

/*
 * read_requesthdrs - read HTTP request headers
 */
/* $begin read_requesthdrs */
void read_requesthdrs(rio_t *rp, char *headers) {
    char buf[MAXLINE];
    headers[0] = '\0';

    Rio_readlineb(rp, buf, MAXLINE);
    while(strcmp(buf, "\r\n")) {
        if (!strstr(buf, "Host:") && !strstr(buf, "User-Agent:") &&
            !strstr(buf, "Connection:") && !strstr(buf, "Proxy-Connection:")) {
            strcat(headers, buf);
        }
        Rio_readlineb(rp, buf, MAXLINE);
    }
}
/* $end read_requesthdrs */

/*
 * parse_uri - parse URI into filename and CGI args
 *             return 0 if dynamic content, 1 if static
 */
/* $begin parse_uri */
int parse_uri(char *uri, char *filename, char *cgiargs) 
{
    char *ptr;

    if (!strstr(uri, "cgi-bin")) {  /* Static content */ //line:netp:parseuri:isstatic
	strcpy(cgiargs, "");                             //line:netp:parseuri:clearcgi
	strcpy(filename, ".");                           //line:netp:parseuri:beginconvert1
	strcat(filename, uri);                           //line:netp:parseuri:endconvert1
	if (uri[strlen(uri)-1] == '/')                   //line:netp:parseuri:slashcheck
	    strcat(filename, "home.html");               //line:netp:parseuri:appenddefault
	return 1;
    }
    else {  /* Dynamic content */                        //line:netp:parseuri:isdynamic
	ptr = index(uri, '?');                           //line:netp:parseuri:beginextract
	if (ptr) {
	    strcpy(cgiargs, ptr+1);
	    *ptr = '\0';
	}
	else 
	    strcpy(cgiargs, "");                         //line:netp:parseuri:endextract
	strcpy(filename, ".");                           //line:netp:parseuri:beginconvert2
	strcat(filename, uri);                           //line:netp:parseuri:endconvert2
	return 0;
    }
}
/* $end parse_uri */

/*
 * clienterror - returns an error message to the client
 */
/* $begin clienterror */
void clienterror(int fd, char *cause, char *errnum, 
		 char *shortmsg, char *longmsg) 
{
    char buf[MAXLINE];

    /* Print the HTTP response headers */
    sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-type: text/html\r\n\r\n");
    Rio_writen(fd, buf, strlen(buf));

    /* Print the HTTP response body */
    sprintf(buf, "<html><title>Tiny Error</title>");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "<body bgcolor=""fffffe"">\r\n");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "%s: %s\r\n", errnum, shortmsg);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "<p>%s: %s\r\n", longmsg, cause);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "<hr><em>The Proxy Web server</em>\r\n");
    Rio_writen(fd, buf, strlen(buf));
}
/* $end clienterror */