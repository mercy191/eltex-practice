#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <dlfcn.h>
#include "../include/contact_book.h"

#define ADD                 100
#define EDIT                200
#define REMOVE              300
#define PRINT_PHONE_BOOK    400
#define QUIT                500

void printContactBook(ContactBook* contact_book);

void printMainHeader(void);
void printSubHeader(int* p_statement);
void setStatement(int *p_statement, char *key);
void getKey(char *key);


int main() {

    char key = '\n';
    unsigned int statement = PRINT_PHONE_BOOK;
    _Bool exit_status = 0;

    ContactBook* contact_book = createContactBook();    
    if (contact_book == NULL)
        return -1;

    while (!exit_status)
    {
        unsigned int id = 0;
        char name[30];
        char surname[30];
        char phone_number[15];
        
        system("clear");
        printMainHeader();
        printSubHeader(&statement);
        
        switch(statement) 
        {
            case ADD:
                
                printf("Введите имя, фамилию и номер телефона: \n");
                scanf("%s %s %s", name, surname, phone_number);
                addInContactBook(contact_book, createContact(contact_book->id_increment, name, surname, phone_number));
                statement = PRINT_PHONE_BOOK;
                break;

            case EDIT:
                
                printf("Введите id существующего контакта, новые имя, фамилию и номер телефона: \n");
                scanf("%d %s %s %s", &id, name, surname, phone_number);
                Contact* temp_contact = createContact(id, name, surname, phone_number);
                editInContactBook(contact_book, temp_contact, id);
                free(temp_contact);
                statement = PRINT_PHONE_BOOK;
                break; 

            case REMOVE: 

                printf("Введите id существующего контакта: \n");
                scanf("%d", &id);
                removeInContactBook(contact_book, id);
                statement = PRINT_PHONE_BOOK;
                break; 

            case PRINT_PHONE_BOOK:
                printContactBook(contact_book);
                break;

            case QUIT: 
                //saveInFile(filename, contact_book);
                exit_status = 1;
                break;    
        }

        getKey(&key);
        setStatement(&statement, &key);
    }

    return removeContactBook(contact_book);
}

/* Отображение GUI */
void printMainHeader(void) 
{
    printf("\t\t\t\t\tТелефонная книга\n");
    printf("(a) - Новыый контакт    ");
    printf("(e) - Изменить контакт    ");
    printf("(r) - Стереть контакт    ");
    printf("(p) - Все контакты    ");
    printf("(q) - Выход\n");
}

void printSubHeader(int* p_statement) 
{
    switch(*p_statement) 
    {
        case ADD:
            printf("\tМеню добавления\n");
            break;
        case EDIT:
            printf("\tВыберите контакт для изменения\n");
            break;
        case REMOVE:
            printf("\tВыберите контакт для удаления\n");
            break;
        case PRINT_PHONE_BOOK:
            printf("\tМеню отображения\n");
            break;
        default:
            printf("\n");
            break;
    }

}

void setStatement(int *p_statement, char *key) 
{
    if (*key == 'a') *p_statement = ADD;
    if (*key == 'e') *p_statement = EDIT;
    if (*key == 'r') *p_statement = REMOVE;
    if (*key == 'p') *p_statement = PRINT_PHONE_BOOK;
    if (*key == 'q') {
        *p_statement = QUIT;
    }
}

void getKey(char *key) 
{
    *key = (char)getchar();
}

void printContactBook(ContactBook* contact_book)
{
    if (contact_book == NULL) 
        return;

    printf("-----------------------------------\n");
    ListNode* current = contact_book->contact_list->head;
    for(int i = 0; i < contact_book->contact_list->size; i++) {
        printf("%d %s %s %s \n", current->contact->id, current->contact->name, current->contact->surname, current->contact->phone_number);
        current = current->next;
    }
    printf("-----------------------------------\n");
}