/**********************************
    
    注釈者：GitHub：peterwrighten

***********************************/

#include<stdio.h>
#include<sys/socket.h>
#include<sys/types.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<unistd.h>
#include<ctype.h>
#include<strings.h>
#include<string.h>
#include<sys/stat.h>
#include<sys/wait.h>
#include<stdlib.h>


#define isSpace(x) isspace((int)(x));
//#include<ctype.h>に定義される関数。
//' ', '\r', '\t', '\n', '\v', '\f'（空白）であれば０以外の値を返し、そうではないと０を返す。
#define SERVER_STRING "Server: jdbhttpd/0.1.0\r\n"

int get_line(int, char *, int);
void not_found(int);
void headers(int, const char *);
void server_file(int, const char *);
void execute_cgi(int, const char *, const char *, const char *);
void unimplemented(int);
void accept_request(int);
void bad_request(int);
void cat(int, FILE *);
void cannot_execute(int);
void error_die(const char *);
int startup(u_short *);


/**************************************************
 * 
 * 
 * 
 * 
 * ************************************************/

int get_line(int sock, char *buf, int size)
{
    int i = 0;
    char c = '\0';
    int n;

    while((i < size - 1) && (c != '\n'))
    {
        n = recv(sock, &c, 1, 0);//recv()関数は<sys/socket.h>に定義される。
        if(n > 0)
        {
            if(c == '\r')
            {
                n = recv(sock, &c, 1, MSG_PEEK);
                if((n > 0) && (c == '\n'))
                    recv(sock, &c, 1, 0);
                else
                    c = '\n';
            }
            buf[i] = c;
            i++;
        }
        else
            c = '\n';
    
    }
    buf[i] = '\0';

    return(i);
}

/**************************************************
 * 
 * Clientに'404 Not Found'をお知らせする。
 * 
 * ************************************************/

void not_found(int client)
{
    char buf[1024];

    sprintf(buf, "HTTP/1.0 404 NOT FOUND\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, SERVER_STRING);
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "Content-Type: text/html\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "<HTML><TITLE>Not Found</TITLE>\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "<BODY><P>The server could not fulfill\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "your request because the resource specified\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "is unavailable or nonexistent.\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "</BODY></HTML>\r\n");
    send(client, buf, strlen(buf), 0);
}

/**************************************************
 * 
 * 
 * 
 * 
 * ************************************************/

void headers(int client, const char *filename)
{
    char buf[1024];
    (void)filename;

    strcpy(buf, "HTTP/1.0 200 OK\r\n");
    send(client, buf, strlen(buf), 0);
    strcpy(buf, SERVER_STRING);
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "Content-type: text/html\r\n");
    send(client, buf, strlen(buf), 0);
    strcpy(buf, "\r\n");
    send(client, buf, strlen(buf), 0);
}

/**************************************************
 * 
 * 
 * 
 * 
 * ************************************************/

void serve_file(int client, const char *filename)
{
    FILE *resource = NULL;
    int numchars = 1;
    char buf[1024];

    buf[0] = 'A'; buf[1] = '\0';
    while((numchars > 0) && strcmp("\n", buf))
        numchars = get_line(client, buf, sizeof(buf));

    resource = fopen(filename, "r");
    if(resource == NULL)
        not_found(client);
    else 
    {
        headers(client, filename);
        cat(client, resource);
    }
    fclose(resource);
}


/**************************************************
 * 
 * 
 * 
 * 
 * ************************************************/


void accept_request(int client)
{
    char buf[1024];//バファのサイズを定義する（最大1024バイト）
    int numchars;
    char method[255];
    char url[255];
    char path[512];
    size_t i, j;
    struct stat st;// #include<sys/stat.h>に定義される構造体
    int cgi = 0;
    char *query_string = NULL;
    numchars = get_line(client, buf, sizeof(buf));
    //後には定義される。
    i = 0; j = 0;
    while(!ISspace(buf[j]) && (i < sizeof(method) - 1))
    {
        method[i] = buf[j];
        i++; j++;
    }
    method[i] = '\0';

    if(strcasecmp(method, "GET") && strcasecmp(method, "POST"))
    {
        unimplemented(client);
        return;
    }
    //strcasecmpは、<strings.h>により定義され、大文字小文字を区別しないString比較。等しいなら０を返す。
    if(strcasecmp(method, "POST") == 0)
    {
        cgi = 1;
    }

    i = 0;

    while(ISspace(buf[j]) && (j < sizeof(buf)))
        j++;
    
    while(!ISspace(buf[j]) && (i < sizeof(url) - 1) && (j < sizeof(buf)))
    {
        url[i] = buf[j];
        i++; j++;
    }
    url[i] = '\0';

    if(strcasecmp(method, "GET") == 0)
    {
        query_string = url;

        while((*query_string != '?') && (*query_string != '\0'))
            query_string++;
        
        if(*query_string == '?')
        {

            cgi = 1;

            *query_string = '\0';

            query_string++;
        }


    }

    sprintf(path, "htdocs%s", url);

    if(path[strlen(path) - 1] == '/')
        strcat(path, "index.html");

    if(stat(path, &st) == -1)
    {
        while((numchars > 0) && strcmp("\n", buf))
            numchars = get_line(client, buf, sizeof(buf));

        not_found(client);
    }
    else 
    {
        if((st.st_mode & S_IFMT) == S_IFDIR)
            strcat(path, "/index.html");

        if( (st.st_mode & S_IXUSR) ||
            (st.st_mode & S_IXGRP) ||
            (st.st_mode & S_IXOTH)    )
            cgi = 1;
        
        if(!cgi)
        {
            serve_file(client, path);
        }
        else
        {
            execute_cgi(client, path, method, query_string);
        }
       
    }
    close(client);
}

/**************************************************
 * 
 * 
 * 
 * 
 * ************************************************/

void bad_request(int client)
{
    char buf[1024];

    sprintf(buf, "HTTP/1.0 400 BAD REQUEST\r\n");
    send(client, buf, sizeof(buf), 0);//ssize_t send(int, const void*, size_t, int)は、#include<sys/socket.h>に定義される。
    sprintf(buf, "Content-type: text/html\r\n");
    send(client, buf, sizeof(buf), 0);
    sprintf(buf, "\r\n");
    send(client, buf, sizeof(buf), 0);
    sprintf(buf, "<P>Your browser sent a bad request, ");
    send(client, buf, sizeof(buf), 0);
    sprintf(buf, "such as a POST without a Content-Length.\r\n");
    send(client, buf, sizeof(buf), 0);

}

/**************************************************
 * 
 * 
 * 
 * 
 * ************************************************/

void cat(int client, FILE *resource)
{
    char buf[1024];
    fgets(buf, sizeof(buf), resource);
    while(!feof(resource))
    {
        send(client, buf, strlen(buf), 0);
        fgets(buf, sizeof(buf), resource);
    }
}

/**************************************************
 * 
 * 
 * 
 * 
 * ************************************************/

void cannot_execute(int client)
{
    char buf[1024];
    sprintf(buf, "HTTP/1.0 500 Internal Server Error\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "Content-type: text/html\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "<P>Error prohibited CGI execution.\r\n");
    send(client, buf, strlen(buf), 0);
}

/**************************************************
 * 
 * 
 * 
 * 
 * ************************************************/

