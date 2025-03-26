#include "contact.h"

Contact *createContact(unsigned int id, char *name, char *surname, char *phone_number) {
    Contact *contact = (Contact *)malloc(sizeof(Contact));
    if (contact == NULL)
        return NULL;
    
    contact->id = id;
    strcpy(contact->name, name);
    strcpy(contact->surname, surname);
    strcpy(contact->phone_number, phone_number);
    return contact;
}

void removeContact(Contact *contact)
{
    if (contact == NULL)
        return;

    free(contact);
}
