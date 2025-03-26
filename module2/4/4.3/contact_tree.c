#include "contact_tree.h"

ContactTree *createContactTree()
{
    ContactTree *tree = (ContactTree *)malloc(sizeof(ContactTree));
    if (tree == NULL)
        return NULL;

    tree->root = NULL;
    return tree;
}

ContactTree *removeContactTree(TreeNode *root)
{
    if (root == NULL)
        return NULL;

    removeContactTree(root->left);

    removeContactTree(root->right);

    removeContact(root->contact);

    free(root);
    return NULL;
}

TreeNode *createTreeNode()
{
    TreeNode *new_node = (TreeNode *)malloc(sizeof(TreeNode));
    if (new_node == NULL)
        return NULL;

    new_node->height = 1;
    new_node->left = NULL;
    new_node->right = NULL;
    return new_node;
}

TreeNode *findTreeNode(TreeNode *root, unsigned int id)
{
    if (root == NULL)
        return NULL;

    if (root->contact->id < id)
    {
        return findTreeNode(root->left, id);
    }
    else if (root->contact->id > id)
    {
        return findTreeNode(root->right, id);
    }

    return root;
}

TreeNode *insertTreeNode(TreeNode *root, Contact *contact)
{
    if (root == NULL)
    {   
        TreeNode* new_node = createTreeNode(contact);
        if (new_node == NULL)
            return NULL;

        new_node->contact = contact;
        return new_node;
    }

    if (contact->id < root->contact->id)
    {
        root->left = insertTreeNode(root->left, contact);
    }
    else if (contact->id > root->contact->id)
    {
        root->right = insertTreeNode(root->right, contact);
    }

    root->height = 1 + max(height(root->left), height(root->right));
    return balanceTree(root);
}

TreeNode *editTreeNode(TreeNode *root, Contact *contact, unsigned int id)
{
    if (root == NULL)
        return NULL;

    if (id < root->contact->id)
    {
        editTreeNode(root->left, contact, id);
    }
    else if (id > root->contact->id)
    {
        editTreeNode(root->right, contact, id);
    }
    else
    {
        strcpy(root->contact->name, contact->name);
        strcpy(root->contact->surname, contact->surname);
        strcpy(root->contact->phone_number, contact->phone_number);
    }

    return root;
}

TreeNode *removeTreeNode(TreeNode *root, unsigned int id)
{
    if (root == NULL)
        return root;

    // Шаг 1: Обычное удаление из бинарного дерева
    if (id < root->contact->id)
    {
        root->left = removeTreeNode(root->left, id);
    }
    else if (id > root->contact->id)
    {
        root->right = removeTreeNode(root->right, id);
    }
    else
    {
        // Найден узел, который нужно удалить
        if (root->left == NULL || root->right == NULL)
        {
            // Узел с одним или без детей
            TreeNode *temp = root->left ? root->left : root->right;

            // Если детей нет, temp = NULL
            if (temp == NULL)
            {
                // Нет детей
                temp = root;
                root = NULL;
            }
            else
            {
                root->contact->id = temp->contact->id;
                strcpy(root->contact->name, temp->contact->name);
                strcpy(root->contact->surname, temp->contact->surname);
                strcpy(root->contact->phone_number, temp->contact->phone_number);
                root->height = temp->height;
                root->left = temp->left;
                root->right = temp->right;
            }
            // Удаляем старый узел
            removeContact(temp->contact);
            free(temp);
        }
        else
        {
            // Узел с двумя детьми: берем inorder-successor (минимальный в правом поддереве)
            TreeNode *temp = findMinNode(root->right);

            // Копируем данные successor-а
            // (можно скопировать поля напрямую, без removeContact/createContact)
            root->contact->id = temp->contact->id;
            strcpy(root->contact->name, temp->contact->name);
            strcpy(root->contact->surname, temp->contact->surname);
            strcpy(root->contact->phone_number, temp->contact->phone_number);

            // Удаляем successor
            root->right = removeTreeNode(root->right, temp->contact->id);
        }
    }

    if (root == NULL)
        return root;

    root->height = 1 + max(height(root->left), height(root->right));
    return balanceTree(root);
}

int height(TreeNode *node)
{
    return node ? node->height : 0;
}

int getBalance(TreeNode *node)
{
    return node ? height(node->left) - height(node->right) : 0;
}

TreeNode *rightRotate(TreeNode *y)
{
    TreeNode *x = y->left;
    TreeNode *T2 = x->right;

    // Выполняем вращение
    x->right = y;
    y->left = T2;

    // Обновляем высоты
    y->height = 1 + max(height(y->left), height(y->right));
    x->height = 1 + max(height(x->left), height(x->right));

    return x;
}

TreeNode *leftRotate(TreeNode *x)
{
    TreeNode *y = x->right;
    TreeNode *T2 = y->left;

    // Выполняем вращение
    y->left = x;
    x->right = T2;

    // Обновляем высоты
    x->height = 1 + max(height(x->left), height(x->right));
    y->height = 1 + max(height(y->left), height(y->right));

    return y;
}

TreeNode *balanceTree(TreeNode *node)
{
    // Проверяем баланс текущего узла
    int balance = getBalance(node);

    // Левый левый случай
    if (balance > 1 && getBalance(node->left) >= 0)
    {
        return rightRotate(node);
    }

    // Правый правый случай
    if (balance < -1 && getBalance(node->right) <= 0)
    {
        return leftRotate(node);
    }

    // Левый правый случай
    if (balance > 1 && getBalance(node->left) < 0)
    {
        node->left = leftRotate(node->left);
        return rightRotate(node);
    }

    // Правый левый случай
    if (balance < -1 && getBalance(node->right) > 0)
    {
        node->right = rightRotate(node->right);
        return leftRotate(node);
    }

    return node;
}

TreeNode *findMinNode(TreeNode *root)
{
    TreeNode *current = root;
    while (current && current->left != NULL)
    {
        current = current->left;
    }
    return current;
}

int max(int a, int b) {
    return (a > b) ? a : b;
}
