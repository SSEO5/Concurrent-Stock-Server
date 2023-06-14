#ifndef bst_h
#define bst_h

#include "csapp.h"

typedef struct{
    int ID;
    int left_stock;
    int price;
    int readcnt;
    sem_t mutex;
    sem_t write;
} item;

typedef struct node{
    item item;
    struct node* left;
    struct node* right;
} node;

node* insertNode(node* root, item item);
node* searchNode(node* root, int ID);
void freeNode(node* root);

#endif /* bst_h */
