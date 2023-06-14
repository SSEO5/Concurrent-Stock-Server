// Task2 : Thread-based Approach with pthread
#include "csapp.h"
#include "bst.h"

#define NTHREADS 100
#define SBUFSIZE 8192

typedef struct{
    int *buf;
    int n;
    int front;
    int rear;
    sem_t mutex;
    sem_t slots;
    sem_t items;
} sbuf_t;

void sbuf_init(sbuf_t *sp, int n);
void sbuf_deinit(sbuf_t *sp);
void sbuf_insert(sbuf_t *sp, int item);
int sbuf_remove(sbuf_t *sp);

sbuf_t sbuf;
void *thread(void *vargp);

node* root = NULL;
void load(FILE* fp);
void save(FILE* fp, struct node* cur);
void sigint_handler(int sig);

char sendBuf[MAXLINE];
void show(int connfd, node* cur);
void command(int connfd, char* buf);

int main(int argc, char **argv){
    int i, listenfd, connfd;
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;
    char client_hostname[MAXLINE], client_port[MAXLINE];
    pthread_t tid;

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
    sbuf_init(&sbuf, SBUFSIZE);
    
    for(i=0; i<NTHREADS; i++){
        Pthread_create(&tid, NULL, thread, NULL);
    }
    while (1) {
        clientlen = sizeof(struct sockaddr_storage);
        connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
        Getnameinfo((SA *) &clientaddr, clientlen, client_hostname, MAXLINE, client_port, MAXLINE, 0);
        printf("Connected to (%s, %s)\n", client_hostname, client_port);
        sbuf_insert(&sbuf, connfd);
    }
}

void *thread(void *vargp){
    Pthread_detach(pthread_self());
    while(1){
        int connfd = sbuf_remove(&sbuf);
        
        int n;
        char buf[MAXLINE];
        rio_t rio;
        
        Rio_readinitb(&rio, connfd);
        while ((n = Rio_readlineb(&rio, buf, MAXLINE)) != 0 && strcmp(buf, "exit\n")) {
            //printf("server received %d bytes on fd %d\n", n, connfd);
            printf("fd %d : %s", connfd, buf); // 명령어 확인용 서버 출력
            command(connfd, buf); // 명령어 실행
        }
        Close(connfd);
    }
}


void show(int connfd, node* cur){
    if(cur == NULL) return;
    char tmp[MAXLINE];
    show(connfd, cur->left);
    
    P(&(cur->item.mutex));
    cur->item.readcnt++;
    if(cur->item.readcnt == 1) P(&(cur->item.write));
    V(&(cur->item.mutex));
    
    sprintf(tmp, "%d %d %d\n", cur->item.ID, cur->item.left_stock, cur->item.price);
    strcat(sendBuf, tmp);
    
    P(&(cur->item.mutex));
    cur->item.readcnt--;
    if(cur->item.readcnt == 0) V(&(cur->item.write));
    V(&(cur->item.mutex));
    
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
        
        P(&(cur->item.mutex));
        cur->item.readcnt--;
        if(cur->item.readcnt == 0) V(&(cur->item.write));
        V(&(cur->item.mutex));
        
        if(cur->item.left_stock < stock_cnt){
            strcpy(sendBuf, "Not enough left stocks\n");
        } else{
            P(&(cur->item.write));
            cur->item.left_stock -= stock_cnt;
            V(&(cur->item.write));
            
            strcpy(sendBuf, "[buy] success\n");
        }
    }
    else if(!strcmp(cmd, "sell")){
        cur = searchNode(root, stock_id);
        if(cur == NULL) return;
        
        P(&(cur->item.mutex));
        cur->item.readcnt--;
        if(cur->item.readcnt == 0) V(&(cur->item.write));
        V(&(cur->item.mutex));
        
        P(&(cur->item.write));
        cur->item.left_stock += stock_cnt;
        V(&(cur->item.write));
        
        strcpy(sendBuf, "[sell] success\n");
    }
    else return;
    
    //printf("connfd : %d\n", connfd);
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
    
    sbuf_deinit(&sbuf);
    freeNode(root);
    
    //printf("buy_sell_cnt: %d\n", buy_sell_cnt);
    //printf("show_cnt: %d\n", show_cnt);
    
    exit(0);
}


void sbuf_init(sbuf_t *sp, int n){
    sp->buf = Calloc(n, sizeof(int));
    sp->n = n;
    sp->front = sp->rear = 0;
    Sem_init(&sp->mutex, 0, 1);
    Sem_init(&sp->slots, 0, n);
    Sem_init(&sp->items, 0, 0);
}

void sbuf_deinit(sbuf_t *sp){
    Free(sp->buf);
}

void sbuf_insert(sbuf_t *sp, int item){
    P(&sp->slots);
    P(&sp->mutex);
    sp->buf[(++sp->rear)%(sp->n)] = item;
    V(&sp->mutex);
    V(&sp->items);
}

int sbuf_remove(sbuf_t *sp){
    int item;
    P(&sp->items);
    P(&sp->mutex);
    item = sp->buf[(++sp->front)%(sp->n)];
    V(&sp->mutex);
    V(&sp->slots);
    return item;
}
