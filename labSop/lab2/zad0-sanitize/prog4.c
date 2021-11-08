// https://sop.mini.pw.edu.pl/pl/node/155
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char** argv) {
  int x=5;
  char name[10];
  scanf("%s", name); // Reading stirng of unknown (probably exceeding 9) length
  printf("You typed: %s\nThe number is %d\n", name, x);
  return 0;
}