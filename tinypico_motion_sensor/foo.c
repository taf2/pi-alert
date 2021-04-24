#include <stdio.h>
#include <string.h>


/**
 * @fn int strend(const char *s, const char *t)
 * @brief Searches the end of string s for string t
 * @param s the string to be searched
 * @param t the substring to locate at the end of string s
 * @return one if the string t occurs at the end of the string s, and zero otherwise
 * ses: https://codereview.stackexchange.com/questions/54722/determine-if-one-string-occurs-at-the-end-of-another/54724
 */
int strend(const char *s, const char *t) {
  const size_t ls = strlen(s); // find length of s
  const size_t lt = strlen(t); // find length of t
  if (ls >= lt) {  // check if t can fit in s
    // point s to where t should start and compare the strings from there
    return (0 == memcmp(t, s + (ls - lt), lt));
  }
  return 0; // t was longer than s
}

int main() {
  const char *input2 = "23828328128.28282:foo:bar:now";
  const char *input1 = "1828282.1823828:update";
  int out = 0;

  if ( (out = strend(input1, ":update"))) {
    printf("yes matched: %d\n", out);
  } else {
    printf("no matched: %d\n", out);
  }
  if ( (out = strend(input2, ":update"))) {
    printf("yes matched: %d\n", out);
  } else {
    printf("no matched: %d\n", out);
  }
}
