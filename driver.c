#include "stats_functions.h"

/* -----------------------------------
            Handler Functions
   -----------------------------------*/

/*
The general signal handler function for Ctrl+C
*/
void handler(int signo){
  char i = 0;
  if(i != 0) getchar();
  while(i != 48 && i != 49)
  {
    write(1, "\nDo you wish to terminate the program? 1 - yes | 0 - no\n", 56);
    i = getchar();
    while(getchar() != '\n');
  }
  if(i == 49) kill(0, SIGUSR1);
  else kill(0, SIGCONT);
} 

/*
A child signal handler function for Ctrl+C
*/
void child_handler(int signo){
  raise(SIGSTOP);
}

/*
A decoy exit signal handler function for exit
*/
void decoy_exit(int signo){
  exit(0);
}

/* -----------------------------------
          Print Shortcut Function
   -----------------------------------*/

/*
For printing a divider between each section
*/
void print_divider(){
  printf("---------------------------------------\n");
}

/*
Refresh the screen
*/
void refresh(){
  printf("\e[1;1H\e[2J");
}

/* -----------------------------------
            General Sys Function
   -----------------------------------*/

/*
Displays the general system information
*/
int display_general_sysinfo(){
  struct utsname general_info;
  for(int i = 0; i < RETRY_COUNT + 1; i++){
    if(i > 0) fprintf(stderr, "Retry #%d: Fetching System info\n", i);
    if(uname(&general_info) == -1) perror("Error fetching system info");
    else break;
    if(i == RETRY_COUNT){
      fprintf(stderr, "CRITICAL: Unable to fetch System Info after %d attempts! Terminating!\n", RETRY_COUNT);
      kill(0, SIGUSR1);
    }
  }
  printf("### System Information ###\n");
  printf("System Name = %s\n", general_info.sysname);
  printf("Machine Name = %s\n", general_info.nodename);
  printf("Version = %s\n", general_info.version);
  printf("Release = %s\n", general_info.release);
  printf("Architecture = %s\n", general_info.machine);
  print_divider();
  return 0;
}

/*
Prints the program memory use
*/
int print_curr_mem_use(long mem_proc_use, long cpu_proc_use, long user_proc_use){
  struct rusage use;
  for(int i = 0; i < RETRY_COUNT + 1; i++){
    if(i > 0) fprintf(stderr, "Retry #%d: Fetching program memory usage\n", i);
    if(getrusage(RUSAGE_SELF, &use) == -1) perror("Fail to get program memory usage");
    else break;
    if(i == RETRY_COUNT){
      fprintf(stderr, "CRITICAL: Unable to fetch program memory usage after %d attempts! Terminating!\n", RETRY_COUNT);
      kill(0, SIGUSR1);
    }
  }
  long main_usage = use.ru_maxrss + mem_proc_use + cpu_proc_use + user_proc_use;
  printf(" Memory usage: %ld kilobytes\n", main_usage);
  return 0;
}

/*
Displays the number of samples and tdelay
*/
void print_sample_delay(int samples, int tdelay){
  printf("Nbr of samples: %d -- every %d secs\n", samples, tdelay);
}

/*
Displays the memory information
*/
void display_memory(char list_mem[][MAX_STR], int index, int samples, int sequential){
  printf("### Memory ### (Phys.Used/Tot -- Virtual Used/Tot)\n");
  if(!sequential) for(int i = 0; i < samples; i++) printf("%s\n", list_mem[i]);
  else {
    for(int i = 0; i < samples; i++) {
      if(i != index) printf("\n");
      else printf("%s\n", list_mem[i]);
    }
  }
  print_divider();
}

/*
Displays the CPU graphics
*/
void display_cpu_graphics(char list_cpu[][MAX_STR], int index, int samples, int sequential){
  if(!sequential) for(int i = 0; i < samples; i++) printf("%s\n", list_cpu[i]);
  else {
    for(int i = 0; i < samples; i++) {
      if(i != index) printf("\n");
      else printf("%s\n", list_cpu[i]);
    }
  }
}

/*
Display the header for the system info
*/
void display_header_content(int samples, int tdelay, long mem_proc_use, long cpu_proc_use, long user_proc_use){
  print_sample_delay(samples, tdelay);
  print_curr_mem_use(mem_proc_use, cpu_proc_use, user_proc_use);
  print_divider();
}

/* -----------------------------------
          Signal Action Function
   -----------------------------------*/

/*
Sets the general signal handling
*/
void settleSignals(){
  struct sigaction sa_int = {0}, sa_tstp = {0}, sa_decoy = {0};
  sa_int.sa_handler = handler;
  sa_int.sa_flags = SA_RESTART;
  sigemptyset(&sa_int.sa_mask);
  sigaddset(&sa_int.sa_mask, SIGINT);
  sigaddset(&sa_int.sa_mask, SIGTSTP);
  sa_tstp.sa_handler = SIG_IGN;
  sa_tstp.sa_flags = 0;
  sigemptyset(&sa_tstp.sa_mask);
  sa_decoy.sa_handler = decoy_exit;
  sa_decoy.sa_flags = 0;
  if(sigaction(SIGINT, &sa_int, NULL) == -1){
    perror("Error occurred setting handler for SIGINT");
    exit(EXIT_FAILURE);
  }
  if(sigaction(SIGTSTP, &sa_tstp, NULL) == -1){
    perror("Error occurred setting handler for SIGTSTP");
    exit(EXIT_FAILURE);
  }
  if(sigaction(SIGUSR1, &sa_decoy, NULL) == -1){
    perror("Error occurred setting handler for SIGUSR1");
    exit(EXIT_FAILURE);
  }
}

/*
Set the children signal handlers
*/
void settleChildSignals(){
  struct sigaction sa_child = {0};
  sa_child.sa_handler = child_handler;
  sa_child.sa_flags = SA_RESTART;
  sigemptyset(&sa_child.sa_mask);
  sigaddset(&sa_child.sa_mask, SIGINT);
  sigaddset(&sa_child.sa_mask, SIGTSTP);
  if(sigaction(SIGINT, &sa_child, NULL) == -1){
    perror("Error occurred setting child handler for SIGINT");
    exit(EXIT_FAILURE);
  }
}

/* -----------------------------------
            Display Function
   -----------------------------------*/

/*
The display system for displaying the system informations
It reads from pipe and prints but it initially creates child-process
for this
*/
void displaySystem(int system, int user, int graphics, int sequential, int samples, int tdelay){
  int userProc, memProc, cpuProc;
  int cpupipe[2], mempipe[2], userpipe[2];
  int processes = 0;
  if(system){
    processes++;
    if(pipe(cpupipe) == -1){
      perror("Fail to create a pipe for CPU info");
      kill(0, SIGUSR1);
    }
    cpuProc = fork();
    if(cpuProc == -1){
      perror("Fail to create a CPU info process");
      kill(0, SIGUSR1);
    }
    else if(cpuProc > 0){
      close(cpupipe[1]);
    }
    else if(cpuProc == 0){
      settleChildSignals();
      reqCPU(cpupipe, tdelay, samples, graphics, sequential);
      exit(0);
    }
    processes++;
    if(pipe(mempipe) == -1){
      perror("Fail to create a pipe for memory info");
      kill(0, SIGUSR1);
    }
    memProc = fork();
    if(memProc == -1){
      perror("Fail to create a memory info process");
      kill(0, SIGUSR1);
    }
    else if(memProc > 0){
      close(mempipe[1]);
    }
    else if(memProc == 0){
      settleChildSignals();
      reqMem(mempipe, tdelay, samples, graphics);
      exit(0);
    }
  }
  if(user){
    processes++;
    if(pipe(userpipe) == -1){
      perror("Fail to create a pipe for user-info");
      kill(0, SIGUSR1);
    }
    userProc = fork();
    if(userProc == -1){
      perror("Fail to create a user info process");
      kill(0, SIGUSR1);
    }
    else if(userProc > 0){
      close(userpipe[1]);
    }
    else if(userProc == 0){
      settleChildSignals();
      reqUsers(userpipe, tdelay, samples);
      exit(0);
    }
  }
  char mem_info[samples][MAX_STR];
  char cpu_graphics[samples][MAX_STR];
  for(int i = 0; i < samples; i++) {
      mem_info[i][0] = '\0';
      cpu_graphics[i][0] = '\0';
  }
  long cpu_proc_use = 0, mem_proc_use = 0, user_proc_use = 0;
  for(int i = 0; i <= samples; i++){
    int sizeRead;
    char userout[MAX_STR];
    char coreout[MAX_STR];
    if(sequential && i == samples) break;
    if(system) read(cpupipe[0], &cpu_proc_use, sizeof(cpu_proc_use));
    if(system && sequential && graphics){
      read(cpupipe[0], &sizeRead, sizeof(sizeRead));
      read(cpupipe[0], cpu_graphics[i], sizeRead);
    }
    if(system && i < samples){
      read(mempipe[0], &mem_proc_use, sizeof(mem_proc_use));
      read(mempipe[0], &sizeRead, sizeof(sizeRead));
      read(mempipe[0], mem_info[i], sizeRead);
    }
    if(user && i < samples){
      read(userpipe[0], &user_proc_use, sizeof(user_proc_use));
      read(userpipe[0], &sizeRead, sizeof(sizeRead));
      read(userpipe[0], userout, sizeRead);
    }
    if(system){
      read(cpupipe[0], &sizeRead, sizeof(sizeRead));
      read(cpupipe[0], coreout, sizeRead);
    }
    if(!sequential) refresh();
    if(sequential) printf("\n>>> Iteration %d\n", i + 1);
    display_header_content(samples, tdelay, mem_proc_use, cpu_proc_use, user_proc_use);
    if(system){
      display_memory(mem_info, i, samples, sequential);
    }
    if(user){
      printf("### Sessions/users ###\n");
      printf("%s", userout);
      print_divider();
    }
    if(system){
      printf("### CPU info ###\n");
      printf("%s", coreout);
      if(graphics) display_cpu_graphics(cpu_graphics, i, samples, sequential);
      print_divider();
      display_general_sysinfo();
      if(!sequential && i < samples && graphics){
        read(cpupipe[0], &sizeRead, sizeof(sizeRead));
        read(cpupipe[0], cpu_graphics[i], sizeRead);
      }
    }
  }
  for(int i = 0; i < processes; i++) wait(NULL);
}

/* -----------------------------------
      Argument Parsing Functions
   -----------------------------------*/

/*
Integer checking for a given argument
*/
int check_int(char* txt, int* data){
  for(int i = 0; txt[i] != '\0'; i++){
    if(txt[i] < '0' || txt[i] > '9') return 0;
  }
  *data = atoi(txt);
  return 1;
}

/*
Self-made argument parser
*/
int arg_parser(int argc, char** argv, int* index, int* data, int* positional_input){
  if(argc < 1) {printf("Not enough arguments provided!"); return '#';}
  if(argc == 1 || *index >= argc) return -1;
  char* currArg = argv[*index];
  *index = *index + 1;
  int covInt = check_int(currArg, data);
  if(strcmp(currArg, "--system") == 0) return 's';
  else if(strcmp(currArg, "--user") == 0) return 'u';
  else if(strcmp(currArg, "--graphics")== 0) return 'g';
  else if(strcmp(currArg, "--sequential")== 0) return 'p';
  else if(strncmp(currArg, "--tdelay=", 9)== 0){
    *data = strtol(currArg + 9, NULL, 10);
    if(*data <= 0) {printf("Invalid tdelay amount: %d\n", *data); return 'k';}
    return 't';
  }
  else if(strncmp(currArg, "--samples=", 10) == 0){
    *data = strtol(currArg + 10, NULL, 10);
    if(*data <= 0) {printf("Invalid samples amount: %d\n", *data); return 'k';}
    return 'a';
  }
  else if(covInt) {
    if(*data <= 0 && *positional_input == 0) {printf("Invalid samples amount: %d\n", *data); *positional_input += 1; return 'k';}
    else if(*data <= 0 && *positional_input == 1){printf("Invalid tdelay amount: %d\n", *data); *positional_input -= 1; return 'k';}
    if(*positional_input == 0)*positional_input += 1;
    else *positional_input -= 1;
    return 'i';
  }
  else printf("Invalid argument: %s\n", currArg);
  return '?';
}

/* -----------------------------------
              Main Function
   -----------------------------------*/

/*
Main function for handling and recognizing arguments
*/
int main(int argc, char** argv){
  settleSignals();
  int system = 1, user = 1, graphics = 0, sequential = 0;
  int sysuser = 0;
  int samples = 10;
  int tdelay = 1;
  int positional_input = 0;
  int arg_index = 1, data = 0;
  int invalid = 0;
  int opt;
  while((opt = arg_parser(argc, argv, &arg_index, &data, &positional_input)) != -1){
    switch(opt){
      case 's':{
        user = 0;
        sysuser++;
        if(sysuser == 2) {sysuser = 0; system = 1; user = 1;}
        positional_input = 0;
        break;
      }
      case 'u':{
        system = 0;
        sysuser++;
        if(sysuser == 2) {sysuser = 0; system = 1; user = 1;}
        positional_input = 0;
        break;
      }
      case 'g':{
        graphics++;
        positional_input = 0;
        break;
      }
      case 'p':{
        sequential++;
        positional_input = 0;
        break;
      }
      case 'i':{
        if(positional_input){samples = data;}
        else {tdelay = data;}
        break;
      }
      case 'k':{
        invalid = 1;
        break;
      }
      case 'a':{
        samples = data;
        positional_input = 0;
        break;
      }
      case 't':{
        tdelay = data;
        positional_input = 0;
        break;
      }
      case '?':{
        invalid = 1;
        break;
      }
      case '#':{
        invalid = 1;
        break;
      }
    }
  }
  if(invalid == 0) displaySystem(system, user, graphics, sequential, samples, tdelay);
}
