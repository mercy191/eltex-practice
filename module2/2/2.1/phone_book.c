#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "contact_book.h"

#define ADD                 100
#define EDIT                200
#define REMOVE              300
#define PRINT_PHONE_BOOK    400
#define QUIT                500

int readFromFile(const char* filename, ContactBook* contact_book);
int saveInFile(const char* filename, ContactBook* contact_book);
int addInFile(const char* filename, Contact* contact);
int editContactInFile(const char *filename, Contact *contact, unsigned int index);
void printContactBook(ContactBook* contact_book);

void printMainHeader(void);
void printSubHeader(int* p_statement);
void setStatement(int *p_statement, char *key);
void getKey(char *key);


int main() {
    const char* filename = "contacts.bin";
    char key = '\n';
    unsigned int statement = PRINT_PHONE_BOOK;
    _Bool exit_status = 0;

    ContactBook* contact_book = createContactBook();
    readFromFile(filename, contact_book);

    while (!exit_status)
    {
        unsigned int id = 0;
        int index = -1; 
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
                addContact(contact_book, createContact(name, surname, phone_number));
                statement = PRINT_PHONE_BOOK;
                break;

            case EDIT:
                
                printf("Введите id существующего контакта, новые имя, фамилию и номер телефона: \n");
                scanf("%d %s %s %s", &id, name, surname, phone_number);
                Contact* temp_contact = createContact(name, surname, phone_number);
                index = findContact(contact_book, id);
                if (index == -1)
                    printf("Контакт с id = %d не найден.", id);
                else    
                    editContact(contact_book, temp_contact, index);
                free(temp_contact);
                statement = PRINT_PHONE_BOOK;
                break; 

            case REMOVE: 

                printf("Введите id существующего контакта: \n");
                scanf("%d", &id);
                index = findContact(contact_book, id);
                if (index == -1)
                    printf("Контакт с id = %d не найден.", id);
                else    
                    removeContact(contact_book, index);
                statement = PRINT_PHONE_BOOK;
                break; 

            case PRINT_PHONE_BOOK:
                printContactBook(contact_book);
                break;

            case QUIT: 
                saveInFile(filename, contact_book);
                exit_status = 1;
                break;    
        }

        getKey(&key);
        setStatement(&statement, &key);
    }

    return removeContactBook(contact_book);
}

/* Отображение GUI */
void printMainHeader(void) {
    printf("\t\t\t\t\tТелефонная книга\n");
    printf("(a) - Новыый контакт    ");
    printf("(e) - Изменить контакт    ");
    printf("(r) - Стереть контакт    ");
    printf("(p) - Все контакты    ");
    printf("(q) - Выход\n");
}

void printSubHeader(int* p_statement) {
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

void setStatement(int *p_statement, char *key) {
    if (*key == 'a') *p_statement = ADD;
    if (*key == 'e') *p_statement = EDIT;
    if (*key == 'r') *p_statement = REMOVE;
    if (*key == 'p') *p_statement = PRINT_PHONE_BOOK;
    if (*key == 'q') {
        *p_statement = QUIT;
    }
}

void getKey(char *key) {
    *key = (char)getchar();
}

void printContactBook(ContactBook* contact_book) {
    printf("-----------------------------------\n");
    for (int i = 0; i < contact_book->count; i++) {
        unsigned int id = (**(contact_book->contacts + i)).id;
        char * name = (**(contact_book->contacts + i)).name;
        char* surname = (**(contact_book->contacts + i)).surname;
        char* phone_number = (**(contact_book->contacts + i)).phone_number;
        printf("%d %s %s %s \n", id, name, surname, phone_number);
    }
    printf("-----------------------------------\n");
}


/* Работа с файлами */
int readFromFile(const char* filename, ContactBook* contact_book) {  
    if (contact_book == NULL) return -1;

    FILE *fp = fopen(filename, "rb");
    if (!fp) return -1;
    
    int struct_count = 0;
    fread(&struct_count, sizeof(unsigned int), 1, fp);
  
    Contact** contacts = (Contact**)malloc(struct_count * sizeof(Contact*));
    if (contacts == NULL) {
        fclose(fp);
        return -1;
    }
    
    unsigned int max_id_increment = 0;
    for (unsigned int i = 0; i < struct_count; i++) {
        *(contacts + i) = (Contact*)malloc(sizeof(Contact));             
        if (!fread(*(contacts + i), sizeof(Contact), 1, fp)) {
            Contact** current_contacts = (Contact**)malloc(i * sizeof(Contact*));
            for (int j = 0; j < i; j++) {
                *(current_contacts + j) = *(contacts + j);                      
            }
            free(contacts);
            contact_book->count = i;
            contact_book->contacts = current_contacts;
            contact_book->id_increment = ++max_id_increment;
            fclose(fp);
            return -2;
        }
        
        if ((**(contacts + i)).id > max_id_increment) {
            max_id_increment = (**(contacts + i)).id;
        }  
    }

    contact_book->count = struct_count;
    contact_book->contacts = contacts;
    contact_book->id_increment = ++max_id_increment;

    fclose(fp);
    return 0;
}

int saveInFile(const char* filename, ContactBook* contact_book) {
    FILE *fp = fopen(filename, "wb");
    if (!fp) return -1;

    if (!fwrite(&contact_book->count, sizeof(unsigned int), 1, fp)){
        fclose(fp);
        return -1;
    }
  
    for (unsigned int i = 0; i < contact_book->count; i++) {
        if (!fwrite(*(contact_book->contacts + i), sizeof(Contact), 1, fp)) {
            fseek(fp, 0, SEEK_SET);
            fwrite(&i, sizeof(unsigned int), 1, fp);
            break;
        }
    }  

    fclose(fp);
    return 0;
}

int addInFile(const char* filename, Contact* contact) {
    FILE *fp = fopen(filename, "r+b");
    if (fp == NULL) {
        return -1;
    }

    unsigned int struct_count = 0;
    if (!fread(&struct_count, sizeof(unsigned int), 1, fp)) {
        fclose(fp);
        return -1;
    } 
    
    struct_count++;
    fseek(fp, 0, SEEK_SET);  
    if (!fwrite(&struct_count, sizeof(unsigned int), 1, fp)) {
        fclose(fp);
        return -1;
    }

    fseek(fp, 0, SEEK_END);
    if (!fwrite(contact, sizeof(Contact), 1, fp)) {
        fclose(fp);
        return -1;
    }

    fclose(fp);
    return 0;
}

int editContactInFile(const char *filename, Contact *contact, unsigned int index) {

    FILE *fp = fopen(filename, "r+b");
    if (fp == NULL) {
        return -1;
    }

    unsigned int struct_count = 0;
    if (!fread(&struct_count, sizeof(unsigned int), 1, fp)) {
        fclose(fp);
        return -1;
    }

    if (index < 0 || index >= struct_count) {
        fclose(fp);
        return -2;
    }

    fseek(fp, sizeof(unsigned int) + index * sizeof(Contact), SEEK_SET);

    if (!fwrite(contact, sizeof(Contact), 1, fp)) {
        fclose(fp);
        return -1;
    }

    fclose(fp);
    return 0;
}

