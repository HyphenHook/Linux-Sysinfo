#include "stats_functions.h"

/*
Current Process usage obtaining
*/
long get_process_mem(){
  struct rusage use;
  if(getrusage(RUSAGE_SELF, &use) == -1) return -1;
  return use.ru_maxrss;
}

/* -----------------------------------
                CPU Function
   -----------------------------------*/
  
/*
Transfers old CPU info to the next CPU info array.
*/
void transfer_cpu_info(long long* oldInfo, long long* dest){
    dest[0] = oldInfo[0];
    dest[1] = oldInfo[1];
}

/* 
Fetches the basic CPU info (core and usage) and stores it in 'output' 
if failed to retrieve CPU core information and sends error message to
STDERR else return 0 if successful
*/
int fetch_cpu_basics(char* output, float curr_cpu, int* size){
  char info[127];
  FILE* cpuinfo;
  for(int i = 0; i < RETRY_COUNT + 1; i++){
    if(i > 0) fprintf(stderr, "Retry #%d: Fetching CPU core info\n", i);
    cpuinfo = fopen("/proc/cpuinfo", "r");
    if(cpuinfo == NULL) perror("Error fetching CPU core info");
    else break;
    if(i == RETRY_COUNT){
      fprintf(stderr, "CRITICAL: Unable to fetch CPU core info after %d attempts! Terminating!\n", RETRY_COUNT);
      kill(0, SIGUSR1);
    }
  }
  int core = 0;
  while(fgets(info, 127, cpuinfo) != NULL){
    if(strncmp(info, "processor", 9) == 0){
      char* p = info;
      while(*p != '\0'){
        if(*p < 48 || *p > 57) p++;
        else break;
      }
      if(strtol(p, NULL, 10) >= 0) core++;
    }
  }
  fclose(cpuinfo);
  int offset = snprintf(output, MAX_STR, "Number of cores: %d\n", core);
  offset += snprintf(output + offset, MAX_STR - offset, " total cpu use = %.2f%%\n", curr_cpu);
  *(size) = offset + 1;
  return 0;
}

/*
Fetches the current CPU and store it into currInfo
The CPU info is stored as idle and nonidle in array
if failed to retrieve CPU info and sends error message
to STDERR else return 0 if successful 
*/
int fetch_cpu_current(long long* currInfo){
  char info[MAX_STR];
  FILE* stat;
  for(int i = 0; i < RETRY_COUNT + 1; i++){
    if(i > 0) fprintf(stderr, "Retry #%d: Fetching CPU information\n", i);
    stat = fopen("/proc/stat", "r");
    if(stat == NULL) perror("Error fetching CPU info");
    else break;
    if(i == RETRY_COUNT){
      fprintf(stderr, "CRITICAL: Unable to fetch CPU information after %d attempts! Terminating!\n", RETRY_COUNT);
      kill(0, SIGUSR1);
    }
  }
  long long cpu[7];
  int count = 0;
  if(fgets(info, MAX_STR, stat) != NULL){
    char* p = info;
    p += 3;
    while(count < 7){
      cpu[count] = strtol(p, &p, 10);
      count++;
    }
  } 
  fclose(stat);
  long long idle = cpu[3];
  long long nonidle = cpu[0] + cpu[1] + cpu[2] + cpu[4] + cpu[5] + cpu[6];
  currInfo[0] = idle;
  currInfo[1] = nonidle;
  return 0;
}

/*
Calculate the current CPU usage given the previous CPU info and
the current CPU info then return such usage
*/
float calc_cpu_usage(long long* initInfo, long long* currInfo){
  float total = (currInfo[0] + currInfo[1]) - (initInfo[0] + initInfo[1]);
  float usage;
  if(total != 0){
    float idle = currInfo[0] - initInfo[0];
    usage = ((total - idle) / total) * 100;
  }
  else usage = 0;
  return usage;
}

/*
Creates the graphics bar for the CPU and saves it in
output
*/
void get_cpu_bar(char* output, float result, int* size){
  int offset = snprintf(output, MAX_STR, "         ||");
  int output_amt = result / 1;
  if(result - (float)((int) result) > 0) output_amt++;
  for(int i = 0; i < output_amt; i++) offset += snprintf(output + offset, MAX_STR - offset, "|");
  offset += snprintf(output + offset, MAX_STR - offset, " %.2f", result);
  *(size) = offset + 1;
}

/*
A general CPU function that calls other CPU functions for usage
*/
float get_cpu_data(char* output, int tdelay, int graphics, long long* initialCPU, long long* currentCPU, int* size){
  sleep(tdelay);
  fetch_cpu_current(currentCPU);
  float curr_cpu = calc_cpu_usage(initialCPU, currentCPU);
  if(graphics) get_cpu_bar(output, curr_cpu, size);
  return curr_cpu;
}

/* -----------------------------------
            Logged User Function
   -----------------------------------*/

/*
Fetches the current logged users and store it
in output
*/
int fetch_users(char* output, int* size){
  struct utmp *users_log;
  setutent();
  for(int i = 0; i < RETRY_COUNT + 1; i++){
    if(i > 0) fprintf(stderr, "Retry #%d: Fetching logged users info\n", i);
    users_log = getutent();
    if(users_log == NULL) perror("Error fetching logged users");
    else break;
    if(i == RETRY_COUNT){
      fprintf(stderr, "CRITICAL: Unable to fetch logged users info after %d attempts! Terminating!\n", RETRY_COUNT);
      kill(0, SIGUSR1);
    }
  }
  int offset = 0;
  while(users_log != NULL){
    if(users_log -> ut_type == USER_PROCESS) offset += snprintf(output + offset, MAX_STR - offset, "%s       %s (%s)\n", users_log -> ut_user, users_log -> ut_line, users_log -> ut_host);
    users_log = getutent();
  }
  *size = offset + 1;
  return 0;
}

/* -----------------------------------
              Memory Function
   -----------------------------------*/

/*
Get the graphics memory bar and store it in txt
*/
void get_memory_bar(char* txt, int* tail_local, float prev_mem, float current_mem){
  float difference;
  if(prev_mem == -1) difference = 0;
  else difference = current_mem - prev_mem;
  int progression = difference / 0.01;
  char progression_symb = '#', progression_end = 'o';
  if(difference >= 0.01) progression_end = '*';
  else if(difference < 0){
    progression *= -1;
    progression_symb = ':';
    progression_end = '@';
  }
  *(tail_local) += snprintf(txt + *(tail_local), MAX_STR - *(tail_local), "   |");
  for(int i = 0; i < progression; i++) *(tail_local) += snprintf(txt + *(tail_local), MAX_STR - *(tail_local), "%c", progression_symb);
  *(tail_local) += snprintf(txt + *(tail_local), MAX_STR - *(tail_local), "%c", progression_end);
  *(tail_local) += snprintf(txt + *(tail_local), MAX_STR - *(tail_local), " %.2f (%.2f)", difference, current_mem);
}

/*
Fetch the current memory usage information
*/
int fetch_meminfo(float* element){
  struct sysinfo sys;
  for(int i = 0; i < RETRY_COUNT + 1; i++){
    if(i > 0) fprintf(stderr, "Retry #%d: Fetching memory info\n", i);
    if(sysinfo(&sys) == -1) perror("Error fetching memory info");
    else break;
    if(i == RETRY_COUNT){
      fprintf(stderr, "CRITICAL: Unable to fetch memory info after %d attempts! Terminating!\n", RETRY_COUNT);
      kill(0, SIGUSR1);
    }
  }
  const double gbscale = 1073741824;
  long long total_vram = sys.totalram, total_ram = sys.totalram;
  total_vram += sys.totalswap;
  total_vram *= sys.mem_unit;
  total_ram *= sys.mem_unit;

  long long used_vram = sys.totalram - sys.freeram;
  used_vram += sys.totalswap - sys.freeswap;
  used_vram *= sys.mem_unit;

  long long used_ram = sys.totalram - sys.freeram;
  used_ram *= sys.mem_unit;

  float total_vram_gb = total_vram / gbscale, total_ram_gb = total_ram / gbscale;
  float used_vram_gb = used_vram / gbscale, used_ram_gb = used_ram / gbscale;
  element[0] = used_vram_gb;
  element[1] = total_vram_gb;
  element[2] = used_ram_gb;
  element[3] = total_ram_gb;
  return 0;
}

/*
A general memory function for calling all other memory functions
*/
int fetch_mem(char* output, float* prev_mem, int graphics, int* size){
  float ram_gather[4];
  fetch_meminfo(ram_gather);
  int offset = snprintf(output, MAX_STR, "%.2f GB / %.2f GB  --  %.2f GB / %.2f GB", ram_gather[2], ram_gather[3], ram_gather[0], ram_gather[1]);
  if(graphics) get_memory_bar(output, &offset, *prev_mem, ram_gather[0]);
  *(size) = offset + 1;
  *prev_mem = ram_gather[0];
  return 0;
}

/* -----------------------------------
          Child-Process Function
   -----------------------------------*/

/*
A child-process function that continuously fetch logged
users and sends it through a pipe to parent
*/
void reqUsers(int* pipe, int tdelay, int samples){
  close(pipe[0]);
  for(int i = 0; i < samples; i++){
    char output[MAX_STR];
    int sizeRead = 0;
    long currentMemUse = get_process_mem();
    if(currentMemUse < 0) {
      for(int j = 0; j < RETRY_COUNT; j++){
        perror("User fetching process fail to get process memory usage");
        fprintf(stderr, "Retry #%d: Fetching memory info for user fetching process\n", i+1);
        currentMemUse = get_process_mem();
        if(currentMemUse >= 0) break;
        if(j == RETRY_COUNT - 1){
          fprintf(stderr, "CRITICAL: Unable to fetch memory usage from user data fetch process after %d attempts! Terminating!\n", RETRY_COUNT);
          kill(0, SIGUSR1);
        }
      }
    }
    write(pipe[1], &currentMemUse, sizeof(currentMemUse));
    fetch_users(output, &sizeRead);
    write(pipe[1], &sizeRead, sizeof(sizeRead));
    write(pipe[1], output, sizeRead);
    sleep(tdelay);
  }
  close(pipe[1]);
}

/*
A child-process function that continuously fetch memory info
and sends it through a pipe to parent
*/
void reqMem(int* pipe, int tdelay, int samples, int graphics){
  close(pipe[0]);
  float prev_mem = -1;
  for(int i = 0; i < samples; i++){
    char output[MAX_STR];
    int sizeRead = 0;
    long currentMemUse = get_process_mem();
    if(currentMemUse < 0) {
      for(int j = 0; j < RETRY_COUNT; j++){
        perror("Memory fetching process fail to get process memory usage");
        fprintf(stderr, "Retry #%d: Fetching memory info for memory fetching process\n", i+1);
        currentMemUse = get_process_mem();
        if(currentMemUse >= 0) break;
        if(j == RETRY_COUNT - 1){
          fprintf(stderr, "CRITICAL: Unable to fetch memory usage from memory data fetch process after %d attempts! Terminating!\n", RETRY_COUNT);
          kill(0, SIGUSR1);
        }
      }
    }
    write(pipe[1], &currentMemUse, sizeof(currentMemUse));
    fetch_mem(output, &prev_mem, graphics, &sizeRead);
    write(pipe[1], &sizeRead, sizeof(sizeRead));
    write(pipe[1], output, sizeRead);
    sleep(tdelay);
  }
  close(pipe[1]);
}

/*
A child-process function that continuously fetch CPU info
and sends it through a pipe to parent
*/
void reqCPU(int* pipe, int tdelay, int samples, int graphics, int sequential){
  close(pipe[0]);
  float curr_cpu = 0;
  long long initialCPU[2];
  long long currentCPU[2];
  fetch_cpu_current(initialCPU);
  for(int i = 0; i <= samples; i++){
    if(sequential && i == samples) break;
    char output[MAX_STR];
    int sizeRead = 0;
    long currentMemUse = get_process_mem();
    if(currentMemUse < 0) {
      for(int j = 0; j < RETRY_COUNT; j++){
        perror("CPU fetching process fail to get process memory usage");
        fprintf(stderr, "Retry #%d: Fetching memory usage for CPU fetching process\n", i+1);
        currentMemUse = get_process_mem();
        if(currentMemUse >= 0) break;
        if(j == RETRY_COUNT - 1){
          fprintf(stderr, "CRITICAL: Unable to fetch memory usage from CPU data fetch process after %d attempts! Terminating!\n", RETRY_COUNT);
          kill(0, SIGUSR1);
        }
      }
    }
    write(pipe[1], &currentMemUse, sizeof(currentMemUse));
    if(sequential) {
      curr_cpu = get_cpu_data(output, tdelay, graphics, initialCPU, currentCPU, &sizeRead);
      if(graphics){
        write(pipe[1], &sizeRead, sizeof(sizeRead));
        write(pipe[1], output, sizeRead);
      }
    }
    fetch_cpu_basics(output, curr_cpu, &sizeRead);
    write(pipe[1], &sizeRead, sizeof(sizeRead));
    write(pipe[1], output, sizeRead);
    if(sequential) transfer_cpu_info(currentCPU, initialCPU);
    if(!sequential && i < samples){
      curr_cpu = get_cpu_data(output, tdelay, graphics, initialCPU, currentCPU, &sizeRead);
      transfer_cpu_info(currentCPU, initialCPU);
      if(graphics){
        write(pipe[1], &sizeRead, sizeof(sizeRead));
        write(pipe[1], output, sizeRead);
      }
    }
  }
  close(pipe[1]);
}