// Task1 : Event-driven Approach with select()
#include "csapp.h"
#include "bst.h"

typedef struct{
    int maxfd;
    fd_set read_set;
    fd_set ready_set;
    int nready;
    int maxi;
    int clientfd[FD_SETSIZE];
    rio_t clientrio[FD_SETSIZE];
} pool;

void init_pool(int listenfd, pool *p);
void add_client(int connfd, pool *p);
void check_clients(pool *p);

node* root = NULL;
void load(FILE* fp);
void save(FILE* fp, struct node* cur);
void sigint_handler(int sig);

char sendBuf[MAXLINE];
void show(int connfd, node* cur);
void command(int connfd, char* buf);

int main(int argc, char **argv){
    int listenfd, connfd;
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;
    char client_hostname[MAXLINE], client_port[MAXLINE];
    static pool pool;
    
    if (argc != 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(0);
    }
    
    Signal(SIGINT, sigint_handler);
    // file load
    FILE* fp = Fopen("stock.txt", "r");
    load(fp);
    Fclose(fp);
    
    listenfd = Open_listenfd(argv[1]);
    init_pool(listenfd, &pool);
    
    while (1) {
        pool.ready_set = pool.read_set;
        pool.nready = Select(pool.maxfd+1, &pool.ready_set, NULL, NULL, NULL);
        
        if(FD_ISSET(listenfd, &pool.ready_set)){
            clientlen = sizeof(struct sockaddr_storage);
            connfd = Accept(listenfd, (SA*)&clientaddr, &clientlen);
            Getnameinfo((SA *) &clientaddr, clientlen, client_hostname, MAXLINE, client_port, MAXLINE, 0);
            printf("Connected to (%s, %s)\n", client_hostname, client_port);
            add_client(connfd, &pool);
        }
        check_clients(&pool);
    }
}

void init_pool(int listenfd, pool *p){
    int i;
    p->maxi = -1;
    for (i=0; i<FD_SETSIZE; i++) p->clientfd[i] = -1;
    
    p->maxfd = listenfd;
    FD_ZERO(&p->read_set);
    FD_SET(listenfd, &p->read_set);
}

void add_client(int connfd, pool *p){
    int i;
    p->nready--;
    for (i = 0; i<FD_SETSIZE; i++)
        if (p->clientfd[i] < 0) {
            p->clientfd[i] = connfd;
            Rio_readinitb(&p->clientrio[i], connfd);
            
            FD_SET(connfd, &p->read_set);

            if (connfd > p->maxfd)
                p->maxfd = connfd;
            if (i > p->maxi)
                p->maxi = i;
            break;
        }
    if (i == FD_SETSIZE)
        app_error("add_client error: Too many clients");
}

void check_clients(pool *p){
    int i, connfd, n;
    char buf[MAXLINE];
    rio_t rio;
    
    for(i = 0; (i <= p->maxi) && (p->nready > 0); i++) {
        connfd = p->clientfd[i];
        rio = p->clientrio[i];
        
        if ((connfd > 0) && (FD_ISSET(connfd, &p->ready_set))) {
            p->nready--;
            if ((n = Rio_readlineb(&rio, buf, MAXLINE)) != 0 && strcmp(buf, "exit\n")) {
                //printf("server received %d bytes on fd %d\n", n, connfd);
                printf("fd %d : %s", connfd, buf); // 명령어 확인용 서버 출력
                command(connfd, buf); // 명령어 실행
            }
            else {
                Close(connfd);
                FD_CLR(connfd, &p->read_set);
                p->clientfd[i] = -1;
                
                // file save
                //printf("save...\n");
                //FILE* fp = fopen("stock.txt", "w");
                //save(fp, root);
                //fclose(fp);
            }
        }
    }
}


void show(int connfd, node* cur){
    if(cur == NULL) return;
    char tmp[MAXLINE];
    show(connfd, cur->left);
    sprintf(tmp, "%d %d %d\n", cur->item.ID, cur->item.left_stock, cur->item.price);
    strcat(sendBuf, tmp);
    show(connfd, cur->right);
}


void command(int connfd, char* buf){
    char cmd[5]; int stock_id, stock_cnt;
    sscanf(buf, "%s %d %d", cmd, &stock_id, &stock_cnt);
    
    node* cur = NULL;
    
    if(!strcmp(cmd, "show")){
        //strcpy(sendBuf, "[show] success\n");
        show(connfd, root);
    }
    else if(!strcmp(cmd, "buy")){
        cur = searchNode(root, stock_id);
        if(cur == NULL) return;
        if(cur->item.left_stock < stock_cnt){
            strcpy(sendBuf, "Not enough left stocks\n");
        } else{
            cur->item.left_stock -= stock_cnt;
            strcpy(sendBuf, "[buy] success\n");
        }
    }
    else if(!strcmp(cmd, "sell")){
        cur = searchNode(root, stock_id);
        if(cur == NULL) return;
        cur->item.left_stock += stock_cnt;
        strcpy(sendBuf, "[sell] success\n");
    }
    else return;
    
    Rio_writen(connfd, sendBuf, MAXLINE); // 명령어 결과 출력
    memset(sendBuf, '\0', sizeof(sendBuf));
}


void load(FILE* fp){
    //printf("load...\n");
    item item;
    int ID, left_stock, price;
    while(!feof(fp)){
        if(fscanf(fp, "%d %d %d", &ID, &left_stock, &price) != EOF){
            item.ID = ID; item.left_stock = left_stock; item.price = price;
            root = insertNode(root, item);
        }
    }
}

void save(FILE* fp, struct node* cur){
    if(cur == NULL) return;
    save(fp, cur->left);
    fprintf(fp, "%d %d %d\n", cur->item.ID, cur->item.left_stock, cur->item.price);
    save(fp, cur->right);
}

void sigint_handler(int sig){
    // file save
    //printf("save...\n");
    FILE* fp = Fopen("stock.txt", "w");
    save(fp, root);
    Fclose(fp);
    
    freeNode(root);
    
    exit(0);
}
