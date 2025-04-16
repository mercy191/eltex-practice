#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "contact.h"

typedef struct ListNode
{
    Contact *contact;
    struct ListNode *next;
    struct ListNode *prev;
} ListNode;

typedef struct ContactList
{
    struct ListNode *head;
    struct ListNode *tail;
    unsigned int size;
} ContactList;


ContactList *createContactList();

ContactList *removeContactList(ContactList *list);

ListNode *createListNode();

ListNode *findListNode(ContactList *list, unsigned int id);

ListNode *insertListNode(ContactList *list, Contact *contact);

ListNode* editListNode(ContactList *list, Contact *contact, unsigned int id);

ListNode *removeListNode(ContactList *list, unsigned int id);
