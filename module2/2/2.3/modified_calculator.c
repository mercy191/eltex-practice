#include <stdio.h>
#include <stdarg.h>

int add(double* ans, int count, ...) {
    *ans = 0;
    va_list args;
    va_start(args, count);
    
    for (int i = 0; i < count; i++) {
        *ans += va_arg(args, double);
    }

    va_end(args);
    return 0;
}

int substract(double* ans, int count, ...) {
    va_list args;
    va_start(args, count);

    *ans = va_arg(args, double); 
    for (int i = 1; i < count; i++) {
        *ans -= va_arg(args, double);  
    }

    va_end(args);
    return 0;
}

int multiply(double* ans, int count, ...) {
    *ans = 1;
    va_list args;
    va_start(args, count);
    
    for (int i = 0; i < count; i++) {
        *ans *= va_arg(args, double);
    }

    va_end(args);
    return 0;
}

int divide(double* ans, int count, ...) { 
    va_list args;
    va_start(args, count);

    *ans = va_arg(args, double); 
    for (int i = 1; i < count; i++) {
        double divisor = va_arg(args, double);
        if (divisor == 0) {
            va_end(args);
            return -1;
        }
        *ans /= divisor;
    }

    va_end(args);
    return 0;
}

typedef int (*operation_func)(double*, int, ...);

typedef struct operation {
    char operator;
    operation_func func;
} operation;

operation operations[] = {
    {'+', add},
    {'-', substract},
    {'*', multiply},
    {'/', divide}
};

int main() {
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

    return 0;
}
