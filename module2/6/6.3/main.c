#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>

typedef int (*operation_func)(double*, int, ...);

typedef struct operation {
    char operator;
    operation_func func;
} operation;


int main()
{
    void* handle = dlopen("./libcalculator.so", RTLD_LAZY);
    if (!handle) {
        return -1;
    }

    operation_func add, substract, multiply, divide;

    add = (operation_func)dlsym(handle, "add");
    substract = (operation_func)dlsym(handle, "substract");
    multiply = (operation_func)dlsym(handle, "multiply");
    divide = (operation_func)dlsym(handle, "divide");

    if (!add || !substract || !multiply || !divide) {
        dlclose(handle);
        return -1;
    }

    operation operations[] = {
        {'+', add},
        {'-', substract},
        {'*', multiply},
        {'/', divide}
    };

    char oper, ch;
    int count;
    double ans = 0.0;
    int error = 0;
    
    int operations_count = sizeof(operations) / sizeof(operations[0]);

    do {
        
        double num1, num2, num3, num4;
        printf("Введите 4 числа:\n");
        scanf("%lf %lf %lf %lf%c", &num1, &num2, &num3, &num4, &ch);

        printf("Введите операцию (+, -, *, /): ");
        scanf("%c", &oper);

        int found = 0;
        for (int i = 0; i < operations_count; i++) {
            if (operations[i].operator == oper) {
                error = operations[i].func(&ans, 4, num1, num2, num3, num4);  
                found = 1;
                break;
            }
        }

        if (!found) {
            printf("Ошибка: нераспознанная операция.\n");
        } else {
            if (error == -1) {
                printf("Ошибка: деление на 0!\n");
            } else {
                printf("Ответ: %lf\n", ans);
            }
        }

        printf("Продолжить (Введите 'y' или 'n')?: ");
        scanf(" %c", &ch);  

    } while (ch != 'n');

    dlclose(handle);
    return 0;

}