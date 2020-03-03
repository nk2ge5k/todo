#include <time.h>
#include <stdio.h>

#define MAX_PROJECT 255
#define MAX_DESCRIPTION 1024

// priority represents task priority
typedef unsigned char priority;

// task represents single task
typedef struct task {
    int id;
    priority priority;                  // integer representation of task priority
    time_t created_at;                  // time of task creation
    time_t done_at;

    char project[MAX_PROJECT];          // optional task project
    char description[MAX_DESCRIPTION];  // task description
} task;


// header represents todo file header
typedef struct header {
    unsigned short version;
    unsigned int _id_seq;
    unsigned int count;
    // TODO: think of the way for extending header
} header;

// proc represents command procedure its accepts three arguments:
// argc      - number of elements in argv array
// argv      - array of command arguments
// file_path - path to file that contains tasks
// retuns 1 in case of success and 0 in case of failure
typedef int todoCommand(int argc, char **argv, char *file_path);

// printUsage prints usage of the command to stderr
typedef void printUsage(char *cmd);

// addCommand funtion that adds task to the TODO list
int addCommand(int argc, char **argv, char *file_path);
// listCommand prints list of existing task into standard output
int listCommand(int argc, char **argv, char *file_path);
// editCommand opens editor to allow editing of the tasks
int editCommand(int argc, char **argv, char *file_path);
