/*
 * proxy.c - ICS Web proxy
 * 516260910013
 * Wang Tianxia
 */

#include "csapp.h"
#include <stdarg.h>
#include <sys/select.h>

// #define DEBUG


struct conn_param
{
    int* connfdp;
    struct sockaddr_storage* clientaddrp;
};


/*
 * Function prototypes
 */
int parse_uri(char *uri, char *target_addr, char *path, char *port);
void format_log_entry(char *logstring, struct sockaddr_in *sockaddr, char *uri, size_t size);
void* thread(void* vargp);
void doit(int fd, struct sockaddr_storage clientaddr);
char* read_requesthdrs(rio_t *rp, int *req_content_length);

ssize_t Rio_readn_w(int fd, void *ptr, size_t nbytes) 
{
    ssize_t n;
  
    if ((n = rio_readn(fd, ptr, nbytes)) < 0)
	{
        printf("Rio_readn error.\n");
        return 0;
    }
    return n;
}

ssize_t Rio_readnb_w(rio_t *rp, void *usrbuf, size_t n) 
{
    ssize_t rc;

    if ((rc = rio_readnb(rp, usrbuf, n)) < 0)
	{
        printf("Rio_readnb error: %s\n", strerror(errno));
        return 0;
    }
    // printf("Rio_readnb_w will return %d\n", rc);
    return rc;
}

ssize_t Rio_readlineb_w(rio_t *rp, void *usrbuf, size_t maxlen) 
{
    ssize_t rc;

    if ((rc = rio_readlineb(rp, usrbuf, maxlen)) < 0)
	{
        printf("Rio_readlineb error: %s\n", strerror(errno));
        return 0;
    }

    if (strstr(usrbuf, "\r\n") == NULL)
        return 0;

    return rc;
} 

ssize_t Rio_writen_w(int fd, void *usrbuf, size_t n) 
{
    ssize_t wc;
    
    if ((wc = rio_writen(fd, usrbuf, n)) != n)
	{
        printf("Rio_writen error.\n");
        return 0;
    }

    return wc;
}


/*
 * main - Main routine for the proxy program
 */
int main(int argc, char **argv)
{
    // printf("%d\n", MAXLINE);
    Signal(SIGPIPE, SIG_IGN);
    int listenfd;
    char hostname[MAXLINE], port[MAXLINE];
    socklen_t clientlen;
    // struct sockaddr_storage clientaddr;
    pthread_t tid;

    /* Check arguments */
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <port number>\n", argv[0]);
        exit(0);
    }

    listenfd = Open_listenfd(argv[1]);

    while(1) {
        struct conn_param* conn_param_ptr = (struct conn_param*)Malloc(sizeof(struct conn_param));
        conn_param_ptr->clientaddrp = (struct sockaddr_storage*)Malloc(sizeof(struct sockaddr_storage));
        conn_param_ptr->connfdp = (int*)Malloc(sizeof(int));
        clientlen = sizeof(struct sockaddr_storage);
        *(conn_param_ptr->connfdp) = Accept(listenfd, (SA *)(conn_param_ptr->clientaddrp), &clientlen);
        Getnameinfo((SA *)(conn_param_ptr->clientaddrp), clientlen, hostname, MAXLINE, 
                    port, MAXLINE, 0);
        Pthread_create(&tid, NULL, thread, conn_param_ptr);
    }

    exit(0);
}


/*
 * parse_uri - URI parser
 *
 * Given a URI from an HTTP proxy GET request (i.e., a URL), extract
 * the host name, path name, and port.  The memory for hostname and
 * pathname must already be allocated and should be at least MAXLINE
 * bytes. Return -1 if there are any problems.
 */
int parse_uri(char *uri, char *hostname, char *pathname, char *port)
{
    #ifdef DEBUG
        printf(uri);
        printf("\n");
    #endif // DEBUG
    char *hostbegin;
    char *hostend;
    char *pathbegin;
    int len;

    if (strncasecmp(uri, "http://", 7) != 0) {
        hostname[0] = '\0';
        return -1;
    }
    /* Extract the host name */
    hostbegin = uri + 7;
    hostend = strpbrk(hostbegin, " :/\r\n\0");
    if (hostend == NULL)
        return -1;
    len = hostend - hostbegin;
    strncpy(hostname, hostbegin, len);
    hostname[len] = '\0';

    /* Extract the port number */
    if (*hostend == ':') {
        char *p = hostend + 1;
        while (isdigit(*p))
            *port++ = *p++;
        *port = '\0';
    } else {
        strcpy(port, "80");
    }

    /* Extract the path */
    pathbegin = strchr(hostbegin, '/');
    if (pathbegin == NULL) {
        pathname[0] = '\0';
    }
    else {
        pathbegin++;
        strcpy(pathname, pathbegin);
    }

    return 0;
}

/*
 * format_log_entry - Create a formatted log entry in logstring.
 *
 * The inputs are the socket address of the requesting client
 * (sockaddr), the URI from the request (uri), the number of bytes
 * from the server (size).
 */
void format_log_entry(char *logstring, struct sockaddr_in *sockaddr,
                      char *uri, size_t size)
{
    time_t now;
    char time_str[MAXLINE];
    unsigned long host;
    unsigned char a, b, c, d;

    /* Get a formatted time string */
    now = time(NULL);
    strftime(time_str, MAXLINE, "%a %d %b %Y %H:%M:%S %Z", localtime(&now));

    /*
     * Convert the IP address in network byte order to dotted decimal
     * form. Note that we could have used inet_ntoa, but chose not to
     * because inet_ntoa is a Class 3 thread unsafe function that
     * returns a pointer to a static variable (Ch 12, CS:APP).
     */
    host = ntohl(sockaddr->sin_addr.s_addr);
    a = host >> 24;
    b = (host >> 16) & 0xff;
    c = (host >> 8) & 0xff;
    d = host & 0xff;

    /* Return the formatted log entry string */
    sprintf(logstring, "%s: %d.%d.%d.%d %s %zu", time_str, a, b, c, d, uri, size);
}



void* thread(void* vargp) {
    Pthread_detach(Pthread_self());
    struct conn_param* conn_param_ptr = (struct conn_param*) vargp;
    int connfd = *(conn_param_ptr->connfdp);
    struct sockaddr_storage clientaddr = *(conn_param_ptr->clientaddrp);
    Free(conn_param_ptr->clientaddrp);
    Free(conn_param_ptr->connfdp);
    Free(conn_param_ptr);
    doit(connfd, clientaddr);
    #ifdef DEBUG
        printf("After doit\n");
        printf("Before close\n");
    #endif
    Close(connfd);
    #ifdef DEBUG
        printf("After close\n");
    #endif
    return NULL;
}

void doit(int fd, struct sockaddr_storage clientaddr) {
    char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
    char server_request_line[5*MAXLINE];
    char logstring[MAXLINE];
    char buf_client[MAXLINE], buf_server[MAXLINE];
    char hostname[MAXLINE], pathname[MAXLINE], port[MAXLINE];
    char request_body[MAXLINE];
    int client_content_length = -1;
    int server_content_length = -1;
    int serverfd = -1;
    char headers[MAXLINE];
    buf_client[0] = '\0';
    request_body[0] = '\0';
    size_t receive_size = 0;
    ssize_t n = 0;
    rio_t rio_client, rio_server;

    Rio_readinitb(&rio_client, fd);

    // Read request line

    if (Rio_readlineb_w(&rio_client, buf_client, MAXLINE) == 0)
    {
        printf("No request line.\n");
        return;
    }

    // parse request line

    if (sscanf(buf_client, "%s %s %s", method, uri, version) != 3)
    {
        printf("Parsing request line error.\n");
        return;
    }

    // parse uri

    if (parse_uri(uri, hostname, pathname, port) != 0)
    {
        printf("Parsing uri error.\n");
        return;
    }

    // connect to the remote server 

    if ((serverfd = open_clientfd(hostname, port)) < 0)
    {
        printf("open_clientfd error.\n");
        return;
    }
    Rio_readinitb(&rio_server, serverfd);

    // construct a request line to server

    sprintf(server_request_line, "%s%s%s%s%s%s", method, " /", pathname, " ", version, "\r\n");
    if (strlen(server_request_line) != Rio_writen_w(serverfd, server_request_line, strlen(server_request_line)))
    {
        printf("Error constructing a request line to server.\n");
        Close(serverfd);
        return;
    }

    // read the request header and forward it to server

    while((n = Rio_readlineb_w(&rio_client, buf_client, MAXLINE)) != 0)
    {
        if (Rio_writen_w(serverfd, buf_client, n) != n)
        {
            printf("Forwarding error.\n");
            Close(serverfd);
            return;
        }

        if (strcmp("\r\n", buf_client) == 0) 
        {
            break;
        }

        if (strncasecmp(buf_client, "Content-Length:", strlen("Content-Length:")) == 0) 
        {
            sscanf(buf_client+16, "%d", &client_content_length);
        }
    }

    // get body content and forward it to the server

    if (client_content_length != -1)
    {
        /*int i;
        for(i = 0;i < client_content_length;++i)
        {
            if (Rio_readnb_w(&rio_client, buf_client, 1) == 0) 
            {
                printf("Error getting body content.\n");
                Close(serverfd);
                printf("serverfd has been closed.\n");
                return;
            }
            else
            {
                if (Rio_writen_w(serverfd, buf_client, 1) == 0)
                {
                    printf("Error forwarding body content to the server.\n");
                    Close(serverfd);
                    return;
                }
            }
            
        }*/
        int content_left = client_content_length;
        while(content_left > 0)
        {
            int max_num;
            int read_num;
            if (content_left <= MAXLINE)
                max_num = content_left;
            else
                max_num = MAXLINE;
            // content_left -= max_num;
            if ((read_num = Rio_readnb_w(&rio_client, buf_client, max_num)) == 0)
            {
                printf("Error getting body content.\n");
                Close(serverfd);
                printf("serverfd has been closed.\n");
                return;
            }
            else
            {
                if (Rio_writen_w(serverfd, buf_client, read_num) == 0)
                {
                    printf("Error forwarding body content to the server.\n");
                    Close(serverfd);
                    return;
                }
            }
            content_left -= read_num;
        }
    }

    // read and forward the headers in server fd
    while((n = Rio_readlineb_w(&rio_server, buf_server, MAXLINE)) != 0)
    {
        if (Rio_writen_w(fd, buf_server, n) == 0)
        {
            printf("Error writing to the client.\n");
            Close(serverfd);
            return;
        }

        receive_size += n;

        if (strcmp("\r\n", buf_server) == 0)
        {
            break;
        }

        if (strncasecmp(buf_server, "Content-Length:", strlen("Content-Length:")) == 0) 
        {
            sscanf(buf_server+16, "%d", &server_content_length);
        }
    }

    // Read and forward the response body
    if (server_content_length != -1)
    {
        int i;
        for (i = 0;i < server_content_length;++i)
        {
            if (Rio_readnb_w(&rio_server, buf_server, 1) == 0)
            {
                printf("Error reading response body from server.\n");
                Close(serverfd);
                return;
            }
            else
            {
                if (Rio_writen_w(fd, buf_server, 1) == 0)
                {
                    printf("Error writing response body to client.\n");
                    Close(serverfd);
                    return;
                }
            }
            receive_size++;
        }
    }

    format_log_entry(logstring, (struct sockaddr_in *)&clientaddr, uri, receive_size);

    printf("%s\n", logstring);
    Close(serverfd);
    return;
}

char* read_requesthdrs(rio_t *rp, int *req_content_length) {
    char* headers = (char*)Malloc(MAXLINE);
    headers[0] = '\0';
    char local_buf[MAXLINE];
    Rio_readlineb(rp, local_buf, MAXLINE);
    while(strcmp(local_buf, "\r\n")) {  
        if (!strncasecmp(local_buf, "Content-Length:", strlen("Content-Length:")))
        {
            sscanf(local_buf+16, "%d", req_content_length);
        }
        strcat(headers, local_buf);
	    Rio_readlineb(rp, local_buf, MAXLINE);
    }
    strcat(headers, local_buf);
    return headers;
}
