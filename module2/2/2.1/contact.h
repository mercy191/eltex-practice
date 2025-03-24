#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct Contact {
    unsigned int id;    
    char name[30];
    char surname[30];
    char phone_number[15];
} Contact;

typedef struct ContactBook {
    Contact **contacts;
    unsigned int count;
    unsigned int id_increment;
} ContactBook;

ContactBook* createContactBook();

Contact* createContact(char* name, char* surname, char* phone_number);

int addContact(ContactBook* contact_book, Contact* contact);

int editContact(ContactBook* contact_book, Contact *contact, unsigned int index);

int findContact(ContactBook* contact_book, unsigned int id);

int removeContact(ContactBook* contact_book, unsigned int index);

int removeContactBook(ContactBook* contact_book);
