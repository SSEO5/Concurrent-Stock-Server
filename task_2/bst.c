#include "bst.h"

node* insertNode(node* root, item item){
    if(root == NULL){
        root = (node*)malloc(sizeof(node));
        root->left = root->right = NULL;
        root->item = item;
        root->item.readcnt = 0;
        Sem_init(&(root->item.mutex), 0, 1);
        Sem_init(&(root->item.write), 0, 1);
        return root;
    }
    if(root->item.ID > item.ID){
        root->left = insertNode(root->left, item);
    } else{
        root->right = insertNode(root->right, item);
    }
    return root;
}

node* searchNode(node* root, int ID){
    if(root == NULL) return root;
    if(root->item.ID == ID){
        P(&(root->item.mutex));
        root->item.readcnt++;
        if((root->item.readcnt) == 1) P(&(root->item.write));
        V(&(root->item.mutex));
        return root;
    }
    if(root->item.ID > ID){
        return searchNode(root->left, ID);
    } else{
        return searchNode(root->right, ID);
    }
}

void freeNode(node* root){
    if(root){
        freeNode(root->left);
        freeNode(root->right);
        free(root);
    }
}
