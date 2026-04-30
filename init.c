// init: The initial user-level program

#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"
#include "patch_persist.h"

char *argv[] = { "sh", 0 };

int
main(void)
{
  int pid, wpid;

  if(open("console", O_RDWR) < 0){
    mknod("console", 1, 1);
    open("console", O_RDWR);
  }
  dup(0);  // stdout
  dup(0);  // stderr

  // Load previously applied patches from disk (kernel self-evolution persistence)
  printf(1, "init: loading persisted patches\n");
  patch_persist_load_all();

  // Start the AI daemon for kernel self-evolution
  pid = fork();
  if(pid < 0){
    printf(1, "init: fork failed (AI daemon)\n");
  } else if(pid == 0){
    // Child process: exec ai_daemon
    exec("ai_daemon", argv);
    printf(1, "init: exec ai_daemon failed\n");
    exit();
  } else {
    printf(1, "init: started AI daemon (PID %d)\n", pid);
  }

  for(;;){
    printf(1, "init: starting sh\n");
    pid = fork();
    if(pid < 0){
      printf(1, "init: fork failed\n");
      exit();
    }
    if(pid == 0){
      exec("sh", argv);
      printf(1, "init: exec sh failed\n");
      exit();
    }
    while((wpid=wait()) >= 0 && wpid != pid)
      printf(1, "zombie!\n");
  }
}
