#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "onigmo.h"

static int
check(const char* pattern, const char* text, unsigned long limit,
      int backward, int match, OnigPosition expected)
{
  regex_t* reg;
  OnigRegion* region;
  OnigErrorInfo error;
  const UChar* str = (const UChar*)text;
  const UChar* end = str + strlen(text);
  OnigPosition result;
  int failed;

  if (onig_new(&reg, (const UChar*)pattern,
               (const UChar*)pattern + strlen(pattern), ONIG_OPTION_DEFAULT,
               ONIG_ENCODING_UTF8, ONIG_SYNTAX_RUBY, &error) != ONIG_NORMAL) {
    return 1;
  }
  region = onig_region_new();
  onig_set_match_operation_limit(limit);
  if (match) {
    result = onig_match(reg, str, end, str, region, ONIG_OPTION_NONE);
  }
  else {
    result = onig_search(reg, str, end, backward ? end : str,
                         backward ? str : end, region, ONIG_OPTION_NONE);
  }
  failed = result != expected;
  if (failed) {
    fprintf(stderr, "%s: limit=%lu backward=%d match=%d expected=%ld got=%ld\n",
            pattern, limit, backward, match, (long)expected, (long)result);
  }
  onig_region_free(region, 1);
  onig_free(reg);
  return failed;
}

int
main(void)
{
  char text[1002];
  UChar message[ONIG_MAX_ERROR_MESSAGE_LEN];
  int failed = 0;

  onig_init();
  failed |= onig_get_match_operation_limit() != 0;
  memset(text, 'a', sizeof(text) - 2);
  text[sizeof(text) - 2] = '!';
  text[sizeof(text) - 1] = '\0';
  failed |= check("^(a+)+$", text, 100, 0, 0, ONIGERR_MATCH_OPERATION_LIMIT_OVER);
  failed |= check("^(a*)*$", text, 100, 0, 1, ONIGERR_MATCH_OPERATION_LIMIT_OVER);
  /* Each candidate needs few instructions; the total search must be bounded. */
  failed |= check("[ab]$", text, 100, 0, 0, ONIGERR_MATCH_OPERATION_LIMIT_OVER);
  failed |= check("[ab]$", text, 100, 1, 0, ONIGERR_MATCH_OPERATION_LIMIT_OVER);
  failed |= check("[ab]$", text, 0, 0, 0, ONIG_MISMATCH);
  failed |= check("(a+)", "aaaa", 100, 0, 0, 0);
  failed |= check("(a+)", "aaaa", 100, 0, 1, 4);
  failed |= check("a", "b", 100, 0, 0, ONIG_MISMATCH);
  /* A new call gets a fresh budget after exhaustion. */
  failed |= check("a", "a", 100, 0, 0, 0);
  onig_error_code_to_str(message, ONIGERR_MATCH_OPERATION_LIMIT_OVER);
  failed |= strcmp((const char*)message, "match operation limit over") != 0;
  onig_set_match_operation_limit(0);
  onig_end();
  return failed;
}
