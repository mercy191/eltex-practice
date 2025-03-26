#include "contact_book.h"

ContactBook* createContactBook()
{
    ContactBook* contact_book = (ContactBook*)malloc(sizeof(ContactBook));
    if (contact_book == NULL) 
        return NULL;

    contact_book->contact_tree = createContactTree();
    if (contact_book->contact_tree == NULL) 
        return NULL;

    contact_book->id_increment = 1;
    return contact_book;
}

int removeContactBook(ContactBook* contact_book)
{    
    if (contact_book == NULL) 
        return NULL_PTR;

    removeContactTree(contact_book->contact_tree->root);
    free(contact_book);
    return 0;
}

int addInContactBook(ContactBook *contact_book, Contact *contact)
{
    if (contact_book == NULL || contact == NULL) 
        return NULL_PTR;

    contact_book->contact_tree->root = insertTreeNode(contact_book->contact_tree->root, contact);
    if (contact_book->contact_tree->root == NULL)
        return NULL_PTR;  

    contact_book->id_increment++;
    return 0;
}

int editInContactBook(ContactBook *contact_book, Contact *contact, unsigned int id)
{
    if (contact_book == NULL || contact == NULL) 
        return NULL_PTR;

    if (id < 1) 
        return BAD_ID;

    TreeNode* current = editTreeNode(contact_book->contact_tree->root, contact, id);
    if (current == NULL)
        return NULL_PTR;  

    return 0;
}

int removeInContactBook(ContactBook *contact_book, unsigned int id)
{
    if (contact_book == NULL) 
        return NULL_PTR;

    if (id < 1) 
        return BAD_ID;

    contact_book->contact_tree->root = removeTreeNode(contact_book->contact_tree->root, id);
    if (contact_book->contact_tree->root == NULL)
        return NULL_PTR; 

    return 0;
}
