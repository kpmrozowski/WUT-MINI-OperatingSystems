#include "interface.h"

void method1(void);
void method2(int arg);

struct library Library = {
    .method1 = method1,
    .method2 = method2,
    .some_value = 36
};

void method1(void)
{
   //...
}
void method2(int arg)
{
   Library.some_value = arg;
}
