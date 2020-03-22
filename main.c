#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <unistd.h>
#include "todo.h"

#define PATH_MAX 4096

// version information about command version
void version(char *cmd) {
    fprintf(stderr, "%s: 2.0.0\n", cmd);
    exit(1);
}

// addCommandUsage prints help message for the addCommand
void addCommandUsage(char *cmd) {
    fprintf(stderr, "USAGE: \n");
    fprintf(stderr, "\t%s add [OPTIONS] [MESSAGE]\n", cmd);
    fprintf(stderr, "\n");
    fprintf(stderr, "OPTIONS:\n");
    fprintf(stderr, "\t-p, --project\tProject name\n");
    fprintf(stderr, "\t-P, --priority\tTask priority\n");
    fprintf(stderr, "\t-h, --help\tShow this help message\n");

    exit(1);
}


// listCommandUsage prints help message for the listCommand
void listCommandUsage(char *cmd) {
    fprintf(stderr, "USAGE: \n");
    fprintf(stderr, "\t%s list\n", cmd);
    fprintf(stderr, "\n");
    fprintf(stderr, "OPTIONS:\n");
    fprintf(stderr, "\t-h, --help\t\tShow this help message\n");

    exit(1);
}

// doneCommandUsage prints help message for the doneCommand
void doneCommandUsage(char *cmd) {
    fprintf(stderr, "USAGE: \n");
    fprintf(stderr, "\t%s done [ID]\n", cmd);
    fprintf(stderr, "\n");
    fprintf(stderr, "OPTIONS:\n");
    fprintf(stderr, "\t-h, --help\t\tShow this help message\n");

    exit(1);
}


char* defaultFilePath(char *file_path, int size) {
    const char *FILE_NAME = ".todo";

    char *homedir = getenv("HOME");
    int hlen = strlen(homedir);

    if (hlen != 0 && hlen < size) {
        strcpy(file_path, homedir);
    } else {
        char cwd[PATH_MAX];
        getcwd(cwd, size);
        strcpy(file_path, cwd);
    }

    if (size > (strlen(file_path) + strlen(FILE_NAME) + 1)) {
        strcat(file_path, "/");
        strcat(file_path, FILE_NAME);
    }

    return file_path;
}

typedef struct command {
    char *name;         // subcommand name
    char *desc;         // subcommand description for the help message
    todoCommand *exec;         // function for this command
    printUsage *usage;  // function that prints usage for the current subcommand
} command;

command command_table[] = {
    {"add", "Add new task to list", addCommand, addCommandUsage},
    {"list", "Prints the list of existing tasks", listCommand, listCommandUsage},
    {"done", "Changes task status to done", doneCommand, doneCommandUsage},
};

// usage prints help information for command
void usage(char *cmd) { 
    fprintf(stderr, "USAGE:\n");
    fprintf(stderr, "\t%s [OPTIONS] [SUBCOMMAND]\n", cmd);
    fprintf(stderr, "\n");
    fprintf(stderr, "OPTIONS:\n");
    fprintf(stderr, "\t-V, --verion\tPrint version info and exit\n");
    fprintf(stderr, "\t-h, --help\tShow this help message\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "SUBCOMMANDS:\n");

    for (int i = 0; i != sizeof(command_table)/sizeof(command_table[0]); i++) {
        fprintf(stderr, "\t%5.5s\t\t%s\n", command_table[i].name, command_table[i].desc);
    }

    exit(1);
}

int main(int argc, char **argv) {
    if (argc == 1) {
        fprintf(stderr, "%s: missing subcommand argument\n\n", argv[0]);
        usage(argv[0]);
    }

    char *file_path = defaultFilePath(malloc(sizeof(char) * PATH_MAX), PATH_MAX);

    int subargc;        // number of subcommand aruments
    int cmd_argn = 1;   // position in argv array where subcommand starts
    char **subargv = NULL;

    // show help message
    if (strcmp(argv[1], "--help") == 0 || 
            strcmp(argv[1], "-h") == 0) usage(argv[0]);

    // show version info
    if (strcmp(argv[1], "--version") == 0 ||
            strcmp(argv[1], "-V") == 0) version(argv[0]);

    // set todo file path
    if (strcmp(argv[1], "--file") == 0 ||
            strcmp(argv[1], "-f") == 0) {
        if (argc < 3) {
            fprintf(stderr, "%s: missing value for \"%s\" option\n", argv[0], argv[1]);
            usage(argv[0]);
        }
        file_path = argv[2];
        cmd_argn += 2;
    }

    if (argc == cmd_argn) {
        fprintf(stderr, "%s: missing subcommand\n", argv[0]);
        usage(argv[0]);
    }

    subargc = argc-cmd_argn-1;

    for (int j = 0; j != sizeof(command_table)/sizeof(command); j++) {
        if (strcasecmp(command_table[j].name, argv[cmd_argn]) != 0) continue;

        command cmd = command_table[j];
        if (subargc > 0) {
            // show help message
            if (strcmp(argv[cmd_argn+1], "--help") == 0 || 
                    strcmp(argv[cmd_argn+1], "-h") == 0) cmd.usage(argv[cmd_argn]);

            subargv = malloc(sizeof(char*) * subargc);
            for (int i = 0; i != subargc; i++) {
                subargv[i] = argv[cmd_argn+i+1];
            }
        }

        int ok = cmd.exec(subargc, subargv, file_path);
        free(subargv);
        if (!ok) {
            cmd.usage(argv[0]);
        }

        return 0;
    }

    fprintf(stderr, "%s: unknown subcommand \"%s\"\n\n", argv[0], argv[1]);
    usage(argv[0]);

    return 1;
}
