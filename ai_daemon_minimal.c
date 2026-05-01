/*
 * AI Daemon - Ultra minimal version
 * Just prints and sleeps to test basic process functionality
 */

#include "types.h"
#include "stat.h"
#include "user.h"

int
main(void)
{
  printf(1, "AI Daemon: Starting minimal test\n");

  int i = 0;
  while (1) {
    printf(1, "AI Daemon: iteration %d\n", i);
    i++;
    sleep(1);
  }

  exit();
}