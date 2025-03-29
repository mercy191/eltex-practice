#include <stdio.h>
#include <stdlib.h>
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


int main() {
    char oper, ch;
    double num1, num2, ans = 0;
    int error = 0;

    do {
        printf("Введите первый операнд, операцию, второй операнд: ");
        scanf("%lf %c %lf%c", &num1, &oper, &num2, &ch);

        switch(oper) 
        {
            case '+': 
                error = add(&ans, 2, num1, num2);
                printf("Ответ: %lf\n", ans); 
                break;

            case '-': 
                error = substract(&ans, 2, num1, num2); 
                printf("Ответ: %lf\n", ans);
                break;

            case '*': 
                error = multiply(&ans, 2, num1, num2); 
                printf("Ответ: %lf\n", ans);
                break;

            case '/': 
                error = divide(&ans, 2, num1, num2);
                if (error != 0){
                    printf("Деление на ноль\n");
                    break;
                }
                printf("Ответ: %lf\n", ans);
                break;

            default:
                ans = 0; 
                break;
        }       
        printf("Продолжать (Введите 'y' или 'n')?: ");
        scanf("%c", &ch);

    } while (ch != 'n');

    return 0;
}