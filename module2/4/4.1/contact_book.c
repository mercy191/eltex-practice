#include "contact_book.h"

ContactBook* createContactBook()
{
    ContactBook* contact_book = (ContactBook*)malloc(sizeof(ContactBook));
    if (contact_book == NULL) 
        return NULL;

    contact_book->contact_list = createContactList();
    if (contact_book->contact_list == NULL) 
        return NULL;

    contact_book->id_increment = 1;
    return contact_book;
}

int removeContactBook(ContactBook* contact_book)
{    
    if (contact_book == NULL) 
        return NULL_PTR;

    removeContactList(contact_book->contact_list);
    free(contact_book);
    return 0;
}

int addInContactBook(ContactBook *contact_book, Contact *contact)
{
    if (contact_book == NULL || contact == NULL) 
        return NULL_PTR;

    ListNode* current = insertListNode(contact_book->contact_list, contact);
    if (current == NULL)
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

    ListNode* current = editListNode(contact_book->contact_list, contact, id);
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

    ListNode* current = removeListNode(contact_book->contact_list, id);
    if (current == NULL)
        return NULL_PTR; 

    return 0;
}