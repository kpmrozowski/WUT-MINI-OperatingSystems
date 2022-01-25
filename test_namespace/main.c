#include "interface.h"
#include <stdio.h>

int main(void)
{
    Library.method1();
    // Library.method2(6);
    printf("%d\n", Library.some_value);
    return 0;
}
