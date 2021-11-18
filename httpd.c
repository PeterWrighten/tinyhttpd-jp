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

#define ISspace(x) isspace((int)(x));
//#include<ctype.h>に定義される関数。
//' ', '\r', '\t', '\n', '\v', '\f'（空白）であれば０以外の値を返し、そうではないと０を返す。
#define SERVER_STRING "Server: jdbhttpd/0.1.0\r\n"

void accept_request(int);
void bad_request(int);
void cat(int, FILE *);
void cannot_execute(int);
void error_die(const char *);
void execute_cgi(int, const char *, const char *, const char *);
int get_line(int, char *, int);
void headers(int, const char *);
void not_found(int);
void server_file(int, const char *);
int startup(u_short *);
void unimplemented(int);

/**************************************************
 * 
 * 
 * 
 * ************************************************/

void accept_request(int client)
{
    char buf[1024];
    int numchars;
    char method[255];
    char url[255];
    char path[512];
    size_t i, j;
    struct stat st;// #include<sys/stat.h>に定義される構造体
    int cgi = 0;
    char *query_string = NULL;
    numchars = get_line(client, buf, sizeof(buf));
    // C標準ライブラリ(#include<stdio.h>)のgets(char *buff) 及び fgets(char * buf, int buffersize, *fstream)と似てる;
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



}