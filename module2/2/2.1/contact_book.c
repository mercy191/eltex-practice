#include "contact_book.h"

ContactBook* createContactBook()
{
    ContactBook* contact_book = (ContactBook*)malloc(sizeof(ContactBook));
    contact_book->contacts = NULL;
    contact_book->count = 0;
    contact_book->id_increment = 1;
    return contact_book;
}

Contact* createContact(char* name, char* surname, char* phone_number)
{   
    Contact* contact = (Contact*)malloc(sizeof(Contact));
    contact->id = 0;
    strcpy(contact->name, name);
    strcpy(contact->surname, surname);
    strcpy(contact->phone_number, phone_number);
    return contact;
}

int addContact(ContactBook* contact_book, Contact* contact) {
    int new_count = contact_book->count + 1;
    Contact** new_contacts = (Contact**)malloc(new_count * sizeof(Contact*));
    if (new_contacts == NULL) {
        return -1; 
    }   

    for (int i = 0; i < contact_book->count; i++) {
        *(new_contacts + i) = *(contact_book->contacts + i);
    }
    contact->id = contact_book->id_increment;
    *(new_contacts + new_count - 1) = contact;
 
    free(contact_book->contacts); 
    contact_book->contacts = new_contacts;
    contact_book->count = new_count;
    contact_book->id_increment++;

    return 0; 
}

int editContact(ContactBook* contact_book, Contact *contact, unsigned int index) {
    if (contact == NULL || contact_book == NULL) return -1;

    if (index >= contact_book->count) {
        return -2;
    }

    strncpy((**(contact_book->contacts + index)).name, contact->name, sizeof(contact->name)/sizeof(contact->name[0]));
    strncpy((**(contact_book->contacts + index)).surname, contact->surname, sizeof(contact->surname)/sizeof(contact->surname[0]));
    strncpy((**(contact_book->contacts + index)).phone_number, contact->phone_number, sizeof(contact->phone_number)/sizeof(contact->phone_number[0]));
  
    return 0;
}

int findContact(ContactBook* contact_book, unsigned int id) {

    for (int i = 0; i < contact_book->count; i++) {
        if ((**(contact_book->contacts + i)).id == id)
            return i;
    }

    return -1;
}

int removeContact(ContactBook* contact_book, unsigned int index) {
    if (index >= contact_book->count) {
        return -2;
    }
    
    int new_count = contact_book->count - 1;
    Contact** new_contacts = (Contact**)malloc(new_count * sizeof(Contact*));
    if (new_contacts == NULL) {
        return -1;
    }
    
    for (int i = 0; i < index; i++) {
        *(new_contacts + i) = *(contact_book->contacts + i);
    }
    for (int i = index + 1; i < contact_book->count; i++) {
        *(new_contacts + i - 1) = *(contact_book->contacts + i);
    }

    free(*(contact_book->contacts + index));
    free(contact_book->contacts);
    contact_book->contacts = new_contacts;
    contact_book->count = new_count;

    return 0;
}

int removeContactBook(ContactBook* contact_book){
    for (int i = 0; i < contact_book->count; i++) {
        free(*(contact_book->contacts + i));
    }
    free(contact_book->contacts);
    free(contact_book);
    return 0;
}
