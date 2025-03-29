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
