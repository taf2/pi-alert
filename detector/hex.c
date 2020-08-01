/*
 * easy conversion from decimal to hex for Transfer-Encoding: chunked testing
 */
#include <stdio.h>
#include <stdlib.h>

int main() {
  char hexbuf[1024];

  snprintf(hexbuf, 1024, "%X", 133);

  printf("your hex value is: %s\n", hexbuf);
  snprintf(hexbuf, 1024, "%X", 10);

  printf("your hex value is: %s\n", hexbuf);
}
