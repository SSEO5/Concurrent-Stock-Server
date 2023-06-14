/*
 * echoclient.c - An echo client
 */
/* $begin echoclientmain */
#include "csapp.h"
#include <time.h>

#define ORDER_PER_CLIENT 3
#define STOCK_NUM 5
#define BUY_SELL_MAX 10

int main(int argc, char **argv) 
{
    int clientfd;
    char *host, *port, buf[MAXLINE], tmp[3];
    rio_t rio;

    if (argc != 3) {
	fprintf(stderr, "usage: %s <host> <port>\n", argv[0]);
	exit(0);
    }
    host = argv[1];
    port = argv[2];

    clientfd = Open_clientfd(host, port);
    Rio_readinitb(&rio, clientfd);
    srand((unsigned int) getpid());

//    while (Fgets(buf, MAXLINE, stdin) != NULL) {
//	Rio_writen(clientfd, buf, strlen(buf));
//	Rio_readlineb(&rio, buf, MAXLINE);
//	Fputs(buf, stdout);
//    }
    
    for(int i=0;i<ORDER_PER_CLIENT;i++){
        int option = rand() % 3;
        
        if(option == 0){//show
            strcpy(buf, "show\n");
        }
        else if(option == 1){//buy
            int list_num = rand() % STOCK_NUM + 1;
            int num_to_buy = rand() % BUY_SELL_MAX + 1;//1~10

            strcpy(buf, "buy ");
            sprintf(tmp, "%d", list_num);
            strcat(buf, tmp);
            strcat(buf, " ");
            sprintf(tmp, "%d", num_to_buy);
            strcat(buf, tmp);
            strcat(buf, "\n");
        }
        else if(option == 2){//sell
            int list_num = rand() % STOCK_NUM + 1;
            int num_to_sell = rand() % BUY_SELL_MAX + 1;//1~10
            
            strcpy(buf, "sell ");
            sprintf(tmp, "%d", list_num);
            strcat(buf, tmp);
            strcat(buf, " ");
            sprintf(tmp, "%d", num_to_sell);
            strcat(buf, tmp);
            strcat(buf, "\n");
        }
        //strcpy(buf, "buy 1 2\n");
    
        Rio_writen(clientfd, buf, strlen(buf));
        // Rio_readlineb(&rio, buf, MAXLINE);
        Rio_readnb(&rio, buf, MAXLINE);
        Fputs(buf, stdout);

        usleep(1000000);
    }
    
    Close(clientfd); //line:netp:echoclient:close
    exit(0);
}
/* $end echoclientmain */
