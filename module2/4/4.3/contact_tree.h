#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "contact.h"


typedef struct TreeNode
{
    Contact *contact;
    struct TreeNode *left;
    struct TreeNode *right;
    unsigned int height;
} TreeNode;

typedef struct ContactTree
{
    struct TreeNode *root;
} ContactTree;


ContactTree *createContactTree();

ContactTree *removeContactTree(TreeNode *root);

TreeNode *createTreeNode();

TreeNode *findTreeNode(TreeNode *root, unsigned int id);

TreeNode *insertTreeNode(TreeNode *root, Contact *contact);

TreeNode* editTreeNode(TreeNode *root, Contact *contact, unsigned int id);

TreeNode *removeTreeNode(TreeNode *root, unsigned int id);

int height(TreeNode *node);

int getBalance(TreeNode *node);

TreeNode *rightRotate(TreeNode *y);

TreeNode *leftRotate(TreeNode *x);

TreeNode *balanceTree(TreeNode *node);

TreeNode *findMinNode(TreeNode *root);

int max(int a, int b);
