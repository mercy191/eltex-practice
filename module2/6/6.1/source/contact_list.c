#include "../include/contact_list.h"

ContactList *createContactList()
{
    ContactList *contact_list = (ContactList *)malloc(sizeof(ContactList));
    if (contact_list == NULL)
        return NULL;

    contact_list->head = NULL;
    contact_list->tail = NULL;
    contact_list->size = 0;
    return contact_list;
}

ContactList *removeContactList(ContactList *list)
{
    if (list == NULL)
        return NULL;

    ListNode *current = list->head;
    ListNode *next_node;

    while (current != NULL)
    {
        next_node = current->next;
        removeContact(current->contact);
        free(current);
        current = next_node;
    }

    free(list);
    return NULL;
}

ListNode *createListNode()
{
    ListNode *new_node = (ListNode *)malloc(sizeof(ListNode));
    if (new_node == NULL)
        return NULL;

    new_node->contact = NULL;
    new_node->next = NULL;
    new_node->prev = NULL;
    return new_node;
}

ListNode *findListNode(ContactList *list, unsigned int id)
{
    if (list == NULL || id < 1) 
        return NULL;

    ListNode *current = list->head;
    while (current != NULL)
    {
        if (current->contact->id == id)
        {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

ListNode *insertListNode(ContactList *list, Contact *contact)
{
    if (list == NULL || contact == NULL) 
        return NULL;

    ListNode *new_node = createListNode();
    if (new_node == NULL)
        return NULL;

    new_node->contact = contact;

    ListNode* current = list->head;
    while (current != NULL && current->contact->id < new_node->contact->id) {
        current = current->next;
    }

    if (current == NULL) {
        if (list->tail != NULL) {
            list->tail->next = new_node;
            new_node->prev = list->tail;
            list->tail = new_node;
        }
        else {
            list->head = list->tail = new_node;  
        }
    }
    else {
        new_node->next = current;
        new_node->prev = current->prev;
        if (current->prev != NULL) {
            current->prev->next = new_node;
        }
        else {
            list->head = new_node;
        }
        current->prev = new_node;
    }
    list->size ++;

    return new_node;
}

ListNode* editListNode(ContactList *list, Contact *contact, unsigned int id) 
{
    if (list == NULL || contact == NULL || id < 1) 
        return NULL;

    ListNode* current = findListNode(list, id);
    if (current == NULL) 
        return NULL;

    strncpy(current->contact->name, contact->name, sizeof(contact->name)/sizeof(contact->name[0]));
    strncpy(current->contact->surname, contact->surname, sizeof(contact->surname)/sizeof(contact->surname[0]));
    strncpy(current->contact->phone_number, contact->phone_number, sizeof(contact->phone_number)/sizeof(contact->phone_number[0]));

    return current;
}

ListNode *removeListNode(ContactList *list, unsigned int id)
{
    if (list == NULL || id < 1) 
        return NULL;

    ListNode *current = findListNode(list, id);    
    if (current != NULL)
    {
        ListNode *prev = current->prev;
        if (current->prev == NULL)
        {
            list->head = current->next;
            if (list->head != NULL) { 
                list->head->prev = NULL;
            }
        }
        else
        {
            prev->next = current->next;
            if (current->next != NULL) {
                current->next->prev = current->prev;
            }
        }
        removeContact(current->contact);
        free(current);
        list->size--;
        return prev;
    }
    return NULL;
}
