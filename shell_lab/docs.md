# Inner workings of TSH

## Evaluation of commands

1. check if command builtin
  - *yes*: Execute command immediatly
  - *no*:
    1. block SIGCHLD in current process
    2. fork the process
    3. _if_ in child
      a. unblock SIGCHLD signal
      b. set new process group
      c. execute the given programm
    4. _if_ in parent
      a. add the job to the jobs array
      b1. _if_ run in foreground: wait for completion
      b2. _if_ run in background: print information
      c. unblock SIGCHLD signal
2. FGBG built in commands
  - TODO: 
3. wait in foreground function:
  - waits until fg child signals then executes corresponding action

## Signal Handlers

### SIGCHLD
Child process has terminated or stopped. 
Reap all current zombie children.

1. a SIGCHLD is received
2. go through all child processes that are terminated or stopped
3. 

### SIGINT
Catch SIGINT and send it to fg job

### SIGSTP
Catch SIGSTP and send it to fg job

## Missing

starts jobs of not valid commands -- rtest04
fgbg
