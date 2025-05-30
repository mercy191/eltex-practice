#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "contact_tree.h"

#define BAD_ID      -2
#define NULL_PTR    -1

typedef struct ContactBook {
    ContactTree* contact_tree;
    unsigned int id_increment;
} ContactBook;


ContactBook* createContactBook();

int removeContactBook(ContactBook* contact_book);

int addInContactBook(ContactBook* contact_book, Contact* contact);

int editInContactBook(ContactBook* contact_book, Contact *contact, unsigned int id);

int removeInContactBook(ContactBook* contact_book, unsigned int id);
