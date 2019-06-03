/*
 * proxy.c - ICS Web proxy
 *
 *
 */

#include "csapp.h"
#include <stdarg.h>
#include <sys/select.h>


struct args{
    int fd;
    struct sockaddr_storage sockaddr;
};
/*
 * Function prototypes
 */
int parse_uri(char *uri, char *target_addr, char *path, char *port);
void format_log_entry(char *logstring, struct sockaddr_in *sockaddr, char *uri, size_t size);
void *thread(void *vargp);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg);
void doit(int fd, struct sockaddr_storage clientaddr);
ssize_t Rio_readn_w(int fd, void *ptr, size_t nbytes);
ssize_t Rio_readnb_w(rio_t *rp, void *usrbuf, size_t n);
ssize_t Rio_readlineb_w(rio_t *rp, void *usrbuf, size_t maxlen);
ssize_t Rio_writen_w(int fd, void *usrbuf, size_t n);

sem_t mute_x;

/*
 * main - Main routine for the proxy program
 */
int main(int argc, char **argv)
{
    int listenfd, connfd;
    pthread_t tid;
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;
    char client_hostname[MAXLINE], client_port[MAXLINE];

    Sem_init(&mute_x, 0, 1);

    /* Check arguments */
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <port number>\n", argv[0]);
        exit(0);
    }

    listenfd = Open_listenfd(argv[1]);
    while(1) {
        clientlen = sizeof(struct sockaddr_storage);
        connfd = Accept(listenfd, (SA *)&clientaddr,  &clientlen);
        Getnameinfo((SA *) &clientaddr, clientlen, client_hostname, MAXLINE, client_port, MAXLINE, 0);
        // printf("Accepted connection from (%s, %s)\n", client_hostname, client_port);
        
        struct args *p = (struct args*)Malloc(sizeof(struct args));
        p->fd = connfd;
        p->sockaddr = clientaddr;

        Pthread_create(&tid, NULL, thread, p);
        
    }

    exit(0);
}

/* rio functions */
ssize_t Rio_readn_w(int fd, void *ptr, size_t nbytes) {
    ssize_t n;
  
    if ((n = rio_readn(fd, ptr, nbytes)) < 0) {
        P(&mute_x);
        printf("Rio_readn error.\n");
        V(&mute_x);
        return 0;
    }
    return n;
}

ssize_t Rio_readnb_w(rio_t *rp, void *usrbuf, size_t n) {
    ssize_t rc;

    if ((rc = rio_readnb(rp, usrbuf, n)) < 0) {
        P(&mute_x);
        printf("Rio_readnb error: %s\n", strerror(errno));
        V(&mute_x);
        return 0;
    }
    return rc;
}

ssize_t Rio_readlineb_w(rio_t *rp, void *usrbuf, size_t maxlen) {
    ssize_t rc;

    if ((rc = rio_readlineb(rp, usrbuf, maxlen)) < 0) {
        P(&mute_x);
        printf("Rio_readlineb error: %s\n", strerror(errno));
        V(&mute_x);
        return 0;
    }

    if (strstr(usrbuf, "\r\n") == NULL)
        return 0;

    return rc;
} 

ssize_t Rio_writen_w(int fd, void *usrbuf, size_t n) {
    ssize_t wc;
    
    if ((wc = rio_writen(fd, usrbuf, n)) != n) {
        P(&mute_x);
        printf("Rio_writen error.\n");
        V(&mute_x);
        return 0;
    }

    return wc;
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


void *thread(void *vargp) {
    int fd;
    struct sockaddr_storage clientaddr;

    Pthread_detach(Pthread_self());
    fd = ((struct args*)vargp)->fd;
    clientaddr = ((struct args*)vargp)->sockaddr;
    Free(vargp);

    doit(fd, clientaddr);
    
    close(fd);
    
    // printf("exit\n");
    return NULL;
}

void doit(int fd, struct sockaddr_storage clientaddr) {
    rio_t rio, rio_prxc;
    char buf[MAXLINE], buf_prxc[MAXLINE];
    char method[MAXLINE], uri[MAXLINE], version[MAXLINE];
    char hostname[MAXLINE], pathname[MAXLINE], port[MAXLINE];
    char logstring[MAXLINE];
    char request[MAXLINE];
    int c_content_len = 0;
    int s_content_len = 0;
    int n;

    Rio_readinitb(&rio, fd);
    if (Rio_readlineb_w(&rio, buf, MAXLINE) == 0) {
        return;
    }
    
    if (sscanf(buf, "%s %s %s", method, uri, version) != 3) {
        return;
    }

    // printf("method: %s, uri: %s, version: %s\n", method, uri, version);

    int result = parse_uri(uri, hostname, pathname, port);
    if (result == -1) {
        fprintf(stderr, "error request\n");
        return;
    }

    // printf("hostname: %s, pathname: %s, port: %s\n", hostname, pathname, port);

    int prx_clientfd = Open_clientfd(hostname, port);

    if (prx_clientfd < 0) {
        // printf("NG\n");
        return;
    }

    /* build the request header */
	sprintf(request, "%s /%s %s\r\n", method, pathname, version);

    // printf("buf: %s\n", )
	
	while ((n = Rio_readlineb_w (&rio, buf, MAXLINE))) {
		if (n <= 0){
			return;
		}
	
		if (strncasecmp(buf, "Content-Length:", strlen("Content-Length:")) == 0) {
            sscanf(buf+16, "%d", &c_content_len);
        }

		sprintf(request, "%s%s", request, buf);
		
		if (!strcmp(buf, "\r\n")){
			break;
		}
	}

    /* send the request header to server */
	Rio_readinitb(&rio_prxc, prx_clientfd);

    // printf("write request:\n%s", request);

	if (Rio_writen_w(prx_clientfd, request, strlen(request)) != strlen(request)) {
        Close(prx_clientfd);
		return;
	}

    /* send the request body to server */
	if (c_content_len != 0) {
		int size_tmp = c_content_len;
		while(size_tmp > 0) {
            int read_num;
            if ((read_num = Rio_readnb_w(&rio, buf, MAXLINE)) == 0) {
                Close(prx_clientfd);
                return;
            }
            else {
                if (Rio_writen_w(prx_clientfd, buf, read_num) == 0) {
                    Close(prx_clientfd);
                    return;
                }
            }
            size_tmp -= read_num;
        }
	}

    int size = 0;
    Rio_readinitb(&rio_prxc, prx_clientfd);

    /* receive the response header from server */
	while ((n = Rio_readlineb_w(&rio_prxc, buf_prxc, MAXLINE))) {
		if (n <= 0) {
			Close(prx_clientfd);
			return;
		}

        size += n;

		if (Rio_writen_w(fd, buf_prxc, strlen(buf_prxc)) != strlen(buf_prxc)) {
        	close(prx_clientfd);
			return;
		}

		if (!strcmp(buf_prxc, "\r\n")) {
			break;
		}

        if (strncasecmp(buf_prxc, "Content-Length:", strlen("Content-Length:")) == 0) {
            sscanf(buf_prxc+16, "%d", &s_content_len);
        }
	}


    /* receive the response body from server */
	if (s_content_len != 0) {
        int i;
        for (i = 0;i < s_content_len;++i) {
            if (Rio_readnb_w(&rio_prxc, buf_prxc, 1) == 0) {
                Close(prx_clientfd);
                return;
            }
            else {
                if (Rio_writen_w(fd, buf_prxc, 1) == 0) {
                    Close(prx_clientfd);
                    return;
                }
            }
            size++;
        }
    }
    
    format_log_entry(logstring, (struct sockaddr_in *)&clientaddr, uri, size);

    P(&mute_x);
    printf("%s\n", logstring);
    V(&mute_x);
    close(prx_clientfd);
    return;
}




