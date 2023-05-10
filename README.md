# =*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*= Linux System Stats =*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=
# An assignment for CSCB09
Program Usage
===================================
1) If there is no compiled file (no .exe or .out file) then please open terminal and navigate to the directory where you found this README file and execute in the terminal
> make
2) To run the program navigate to the directory where you found this README file and execute
> ./main \<OPTIONAL FLAGS\>
>
>  \<OPTIONAL FLAGS\>:
>- --system        prints system only data
>- --user          prints connected user only data
>- --graphics      prints the data with graphics (graphics only appear when displaying system data)
>- --sequential    prints the data in sequential order
>- --samples=N     prints the N samples of the data (N must be strictly a natural number and positive) (samples is defaulted to 10 if unspecified)
>- --tdelay=N      prints the samples of data with N seconds inbetween (N must be strictly a natural number and positive) (tdelay is defaulted to 1 if unspecified)
>> Any numerical arguments are also allows and will be interpreted in the position of samples tdelay.  
>> If repeated **VALID** positional arguments and/or samples tdelay flags are provided the ones at closest to end of line will be used. However if any of the flags provided are invalid then the program will not execute (such as providing a negative value then a positive value for --samples=N)
>> - Ex. "./main -4 10 5 10" will not function since an invalid flag (-4) is provided. Any invalid flags will deem the program execution invalid.

Function Overview
===================================
-----------------------------------
-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
In stats_functions.c file
-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
-----------------------------------

**long get_process_mem();**
- Uses \<sys/resource.h\> for its rusage struct to obtain the maximum resident set size of the process
- Returns:
  - "-1" = error occured
  - any value greater than 0 = maximum resident set size is returned

-----------------------------------
## CPU Functions
-----------------------------------
**void transfer_cpu_info(long long\* oldInfo, long long\* dest);**
- Copies the CPU information stored in the array oldInfo into the destination array dest
- Parameter:
  - oldInfo = the old CPU info to copy from
  - dest = the destination for CPU info to copy to
- Used to copy old CPU info into the destination array for keep track of current and previous CPU info

**int fetch_cpu_basics(char\* output, float curr_cpu, int\* size);**
- Reads the "/proc/cpuinfo" file to gather CPU core info by counting the number of processors in the system and also formats the text with the CPU usage given curr_cpu. All of which are saved in output
- If it fails to open the file for reading then it will try RETRY_COUNT number of times to read. If it has already tried all attempts then program terminates
- Parameters:
  - output = the output string to save the information in
  - curr_cpu = the current CPU usage
  - size = the length of the output string
- Returns:
  - "0" = success
- Error occurs if we cannot read the file
  - Error is reported in STDERR

**int fetch_cpu_current(long long* currInfo);**
- Reads the "/proc/stat" file to calculate the current CPU status and stores it into currInfo array
- If it fails to open the file for reading then it will try RETRY_COUNT number of times to read. If it has already tried all attempts then program terminates
- Parameter:
  - currInfo = the array for storing the parsed CPU info from this function
- Parses the "/proc/stat" file by reading the "cpu" section of the file and storing the 10 values in variable cpu for calculation
- The CPU information array contains the following after parsing:
  - cpu[0] = user
  - cpu[1] = nice
  - cpu[2] = system
  - cpu[3] = idle
  - cpu[4] = iowait
  - cpu[5] = irq
  - cpu[6] = softirq
  - cpu[7] = steal
- cpu idle value is calculated by: idle --> result is stored into currInfo[0]
- cpu nonidle value is calculated by: user + nice + system + irq + softirq + steal --> result is stored into currInfo[1]
- Returns:
  - "0" = success
- Error occurs if we cannot read the file
  - Error is reported in STDERR

**float calc_cpu_usage(long long\* initInfo, long long\* currInfo);**
- initInfo is the previous CPU info and currInfo is the current CPU info (the two CPU data is gathered in tdelay secs inbetween)
- Parameter:
  - initInfo = the previous CPU data
  - currInfo = the current CPU data
- the CPU usage is calculate by:
  - total = (current idle + nonidle) - (previous idle + nonidle)
  - idle = current idle - previous idle
  - CPU usage = ((total - idle) / total) * 100
  - Same as the formula provided by Marcelo
- Returns:
  - the cpu usage percentage

**void get_cpu_bar(char\* txt, float result, int* size);**
- Given a pointer for a specific text and the cpu usage result store the graphical interpretation of the cpu result in txt and save the length of txt into the 'size'
- Parameter:
  - txt = the pointer to store the CPU graphics bar into
  - result = the CPU usage result
  - size = the pointer to store the length of 'txt'
- The graphics bar is represented by |
  - result is rounded up and divided by 1 to obtain how many bars to add
  - 0.00 is || which is the baseline

-----------------------------------
## Logged User Functions
-----------------------------------
**int fetch_users(char* output, int* size);**
- Saves the connected sessions/users utilizing \<utmp.h\>'s utmp struct into 'output' and also save the length of the output into the 'size'
- If it fails to execute getutent() then it will try RETRY_COUNT number of times to execute. If it has already tried all attempts then program terminates
- Parameters:
  - output = the pointer to store the users fetched
  - size = the pointer to store the length of 'output'
- Error occurs if getutent(); fails
  - Error is reported in STDERR
- Returns:
  - "0" = success

-----------------------------------
## Memory Functions
-----------------------------------
**void get_memory_bar(char\* txt, int* tail_local, float\* prev_mem, float current_mem);**
- Given a pointer for a specific text append the graphical interpretation of the memory usage in txt
- Parameter:
  - txt = the pointer to append the memory usage graphics bar 
  - tail_local = the number of positions to move the head pointer txt to for it to reach the eol 
    - used for snprintf to append the graphics
  - prev_mem = the stored result of the previous memory usage. Used for determining the difference between current memory usage and previous
  - current_mem = the current memory usage data
- The graphic bars signifies different things
  - ::::@ = decrease in memory usage from previous
  - ####* = increase in memory usage from previous
  - o = positive zero
  - @ = negative zero
- The graphic bar amount is calculated via (current_mem - prev_mem) / 0.01
  - each 0.01 of difference represent an additional bar of difference

**void fetch_meminfo(float\* element);**
- Uses the \<sys/sysinfo.h\> for its sysinfo struct to obtains the RAM information on the system
- Given an float array called element store calculated memory information into element
- If it fails to execute sysinfo() then it will try RETRY_COUNT number of times to execute. If it has already tried all attempts then program terminates
- Parameter:
  - element = a float array used for holding memory info
- The data is calculated as the following:
  - Total Virtual Memory = (totalram + totalswap) * mem_unit
  - Total Physical Memory = totalram * mem_unit
  - Used Virtual Memory = ((totalram - freeram) + (totalswap - freeswap)) * mem_unit
  - Used Physical Memory = (totalram - freeram) * mem_unit
  - All of the above are divided by 1073741824 which is the byte to GB factor
- The element array holds the following after the function call
  - element[0] stores the used virtual ram in GB
  - element[1] stores the total virtual ram in GB
  - element[2] stores the used physical ram in GB
  - element[3] stores the total physical ram in GB
- Error occurs if sysinfo(); fails
  - Error is reported in STDERR

**float fetch_mem(char\* output, float\* prev_mem, int graphics, int* size);**
- Gets the memory info and stores the string representation of the memory data into output
- Parameter:
  - output = the pointer to store the memory usage string to
  - prev_mem = the previous memory usage
  - graphics = the interger representation of whether graphics is wanted
  - size = the pointer to store the length of the output string
- This function calls fetch_meminfo to obtain the current memory info
- If graphics is wanted, after the storing of memory usage string into output, the get_memory_bar function is called with prev_mem as argument to append the memory usage bar using the prev_mem 
- This function returns the float of the current memory usage
- Returns:
  - "0" = success

-----------------------------------
## Child-Process Functions
-----------------------------------
All of the following function closes the read side of the pipe since it only uses the write side of the pipe

**void reqUsers(int* pipe, int tdelay, int samples);**
- A function for a child process to continuous getting logged user information
- Parameters:
  - pipe = the pipe to write to
  - tdelay = the delay
  - samples = number of samples to collect
- This function basically calls fetch_users then write the output to pipe 'samples' number of times
- Errors occur if get_process_mem() fails
  - Error reported to STDERR

**void reqMem(int* pipe, int tdelay, int samples, int graphics);**
- A function for a child process to continuous getting memory information
- Parameters:
  - pipe = the pipe to write to
  - tdelay = the delay
  - samples = number of samples to collect
  - graphics = indication of graphics are wanted
- This function basically calls fetch_men() then write the output to pipe 'samples' number of times
- Errors occur if get_process_mem() fails
  - Error reported to STDERR

**void reqCPU(int* pipe, int tdelay, int samples, int graphics, int sequential);**
- A function for a child process to continuous getting CPU information
- Parameters:
  - pipe = the pipe to write to
  - tdelay = the delay
  - samples = number of samples to collect
  - graphics = indication of graphics are wanted
  - sequential = indication if this was sequential
- This function basically calls a series of CPU functions then write the output to pipe 'samples' number of times
- Errors occur if get_process_mem() fails
  - Error reported to STDERR

-----------------------------------
-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
In driver.c file
-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
-----------------------------------

-----------------------------------
## Handler Functions
-----------------------------------
**void handler(int signo);**
- A signal handler used to handle the signal SIGINT
- The handler asks for termination and behaves as according to the input. It sends signals after proper input via kill()
  - "1" - yes = terminate program 
  - "0" - no = send SIGCONT to process group (childrens)
- Parameters:
  - signo = the signal number

**void child_handler(int signo);**
- A signal handler used by the child process to handle SIGINT
- It raises a signal to itself to SIGSTOP
- Parameters:
  - signo = the signal number

**void decoy_exit(int signo);**
- A signal handler used for exit when handling the user define signal SIGUSR1
- Simply exits()
- Parameters:
  - signo = the signal number

-----------------------------------
## Handler Functions
-----------------------------------
**void print_divider();**
- Prints a divider to divide different data into sections using "---------------------------------------"

**void refresh();**
- Prints the ANSI ESCape Code to clear screen using "\e[1;1H\e[2J"

-----------------------------------
## General Sys Function
-----------------------------------
**int display_general_sysinfo();**
- Prints the general system information utilizing \<sys/utsname.h\>'s utsname struct and print the system name, operating system, system version, system release, and the system architecture
- If uname() fails to execute (get info about system) then the program will retry RETRY_COUNT number of times until a sample was gathered or all attempts was tried. If all attempts was tried then the program will terminate due to unable to obtain info.
- Returns:
  - "0" = success
- Errors occur if uname() fails
  - Error reported to STDERR

**int print_curr_mem_use(long mem_proc_use, long cpu_proc_use, long user_proc_use);**
- Prints the current memory usage section in the format: " Memory usage: {ru_maxrss of parent + ru_maxrss of all its children} kilobytes"
- Uses \<sys/resource.h\> for its rusage struct to obtain the maximum resident set size of the parent
- If getrusage() fails to execute (get info about system) then the program will retry RETRY_COUNT number of times until a sample was gathered or all attempts was tried. If all attempts was tried then the program will terminate due to unable to obtain info.
- Returns:
  - "0" = success
- Parameters:
  - mem_proc_use = the memory usage of the memory fetching process
  - cpu_proc_use = the memory usage of the cpu fetching process
  - user_proc_use = the memory usage of the user fetching process
- Errors occur if getrusage() fails
  - Error reported to STDERR

**void print_sample_delay(int samples, int tdelay);**
- Prints the sample and delay values of the command run in the following format: "Nbr of samples: {samples} -- every {tdelay} secs"
- Parameters:
  - samples = the number of samples going to be collected
  - tdelay = the delay inbetween each sample collected

**void display_memory(char list_mem[][MAX_STR], int index, int samples, int sequential);**
- Prints all currently stored memory usage data that is in list_mem given the sample size
- Parameter:
  - list_mem[samples][MAX_STR] = the array of size MAX_STR strings that stores the memory usage data
  - index = the index to print the memory usage data and is only used when the sequential flag is used
  - samples = the sample size of the program call and is used to determine the size of the list_mem
  - sequential = the integer representation of whether sequential is wanted
- If sequential is required then the display_memory only prints what is there on that specific iteration {index} and blank for all other lines

**void display_cpu_graphics(char list_cpu[][MAX_STR], int index, int samples, int sequential);**
- Prints all currently stored CPU graphics that is in list_cpu given the sample size
- Parameter:
  - list_cpu[samples][MAX_STR] = the array of size MAX_STR strings that stores the graphic representation of each iterations' CPU usage
  - index = the index to print the graphic representation and is only used when the sequential flag is used
  - samples = the sample size of the program call and is used to determine the size of the list_cpu
  - sequential = the integer representation of whether sequential is wanted
- If sequential is required then the display_cpu_graphics only prints what is there on that specific iteration {index} and blank for all other lines

**void display_header_content(int samples, int tdelay, long mem_proc_use, long cpu_proc_use, long user_proc_use);**
- A function used for bundling the calls to print_sample_delay(), print_curr_mem_use();, print_divider();
- Parameters:
  - samples = the number of samples going to be collected
  - tdelay = the delay inbetween each sample collected
  - mem_proc_use = the memory usage of the memory fetching process
  - cpu_proc_use = the memory usage of the cpu fetching process
  - user_proc_use = the memory usage of the user fetching process

-----------------------------------
## Signal Action Functions
-----------------------------------
**void settleSignals();**
- Sets the signal behavior for SIGINT, SIGTSTP, SIGUSR1
- SIGINT: sets the handler to the handler(int signo); function
- SIGTSTP: sets it to ignore this signal using SIG_IGN
- SIGUSR1: sets the handler to the decoy_exit(int signo); function
- Error occur if sigaction fails to set the handler
  - Error reported to STDERR and terminates the program

**void settleChildSignals();**
- Sets the signal behavior for SIGINT for child process
- SIGINT: sets the handler to the child_handler(int signo); function
- Error occur if sigaction fails to set the handler
  - Error reported to STDERR and terminates the program

-----------------------------------
## Display Data Function
-----------------------------------
**void displaySystem(int system, int user, int graphics, int sequential, int samples, int tdelay);**
- Displays the required system info given the arguments
- Parameter:
  - system = integer value representing whether to show system info or not
  - user = integer value representing whether to show users connected or not
  - graphics = integer value representing whether to show graphics or not
  - sequential = integer value representing whether to display as sequential or not
  - samples = integer value for how many samples to collect
  - tdelay = integer value for how many seconds to wait inbetween samples
- Function initially creates children processes to fetch memory, CPU, and logged users information
  - Depending on the flag, only the required child process will be created, if user wasn't requested then a child process of user won't be created (same with pipe)
- Function receives information from the pipe and then prints the required data as specified by the flags
- Error occur if fork fails or pipe fails
  - Error reported to STDERR and terminates the program

-----------------------------------
## Argument Parsing Functions
-----------------------------------
**int check_int(char\* txt, int\* data);**
- Checks the argument to determine if its a simple integer or not
- Parameter:
  - txt = the pointer of the string argument
  - data = the pointer to store the numerical value if the argument is an integer
- If its an integer argument return 1 else return 0

**int arg_parser(int argc, char\*\* argv, int\* index, int\* data, int\* positional_input);**
- Parses the given argument in argv at index
- Parameter:
  - argc = total number of arguments in argv
  - argv = the array of arguments
  - index = the index of the argument we are parsing
  - data = the pointer to store a numerical positional argument in
  - positional_input = the pointer to decide whether the current numerical argument is to be stored as samples or tdelay
- This function returns a char representing the argument we just read's flag
  - \# means not enough arguments provided
  - k means invalid numerical value is provided (ie all value less than or equal to 0)
  - s means system flag
  - u means user flag
  - g means graphics flag
  - p means sequential flag
  - i means a valid numerical positional argument is provided 
  - t means a valid tdelay is provided
  - a means a valid sample size is provided
  - -1 means we have finished reading all arguments
  - ? means an unknown argument is provided

-----------------------------------
## Main Function
-----------------------------------
**int main(int argc, char\*\* argv);**
- The main function for interpreting what to do after argument parsing and calling the display function is argument parsing was successful
- Parameter:
  - argc = the number of arguments the user has given
  - argv = the string representation of each of the arguments the user has given
- Function defaults tdelay = 1, samples = 10, system = 1, user = 1, graphics = 0, sequential = 0
- The main function continuously calls arg_parser until no more arguments are left to parse (-1 is returned from the parser)
  - each call to arg_parser returns a character that is used to update the flag variables
  - If any of the error flag is returned then the current arguments are deemed invalid and no display will be launched
  - If all the arguments provided is correct then the displaySystem function will be called displaying all the data
        