#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct Contact
{
    unsigned int id;    
    char name[30];
    char surname[30];
    char phone_number[15];
} Contact;


Contact *createContact(unsigned int id, char *name, char *surname, char *phone);

void removeContact(Contact *contact);
