#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define print_error(...) fprintf(stderr, __VA_ARGS__)
#define len(a) sizeof(a)/sizeof(a[0])
#define MAX_TAGS_COUNT 16
#define MAX_MSG_LENGTH 256

typedef void proc(int argc, char **argv);
typedef struct {
    char *name;
    char *desc;
    proc *proc;
} command;

void addCommand(int argc, char **argv);

command command_table[] = {
    {"add", "Add new task to list", addCommand},
};

// usage prints help information for command
void usage(char *command) { 
    fprintf(stderr, "USAGE:\n");
    fprintf(stderr, "   %s [OPTIONS] [SUBCOMMAND]\n", command);
    fprintf(stderr, "\n");
    fprintf(stderr, "OPTIONS:\n");
    fprintf(stderr, "   -V, --verion        Print version info and exit\n");
    fprintf(stderr, "   -h, --help          Show this help message\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "SUBCOMMANDS:\n");

    for (int i = 0; i != sizeof(command_table)/sizeof(command_table[0]); i++) {
        fprintf(stderr, "   %s     %s\n", command_table[i].name, command_table[i].desc);
    }

    exit(1);
}

// version information about command version
void version(char *command) {
    fprintf(stderr, "%s: 0.0.0-dev\n", command);
    exit(1);
}

void writeLine(FILE *fd, char **tags, int ntags, char *msg) {
    for (int i = 0; i != ntags; i++) {
        fputc('[', fd);
        fputs(tags[i], fd);
        fputs("] ", fd);
    }
    fputs(msg, fd);
    fputc('\n', fd);
}

void addCommand(int argc, char **argv) {
    char *tags[MAX_TAGS_COUNT];
    char msg[MAX_MSG_LENGTH];

    int ntags = 0;
    int narg  = 0;
    for (; narg < argc; narg += 2) {
        if ((narg+1) > argc || argv[narg][0] != '-') {
            break;
        }

        if (strcmp("-t", argv[narg]) == 0 || strcmp("--tag", argv[narg]) == 0) {
            if (strlen(argv[narg+1]) > 0) {
                tags[ntags++] = argv[narg+1];
            }
        }
    }

    if (narg == argc) {
        print_error("task message is required");
        /* addCommandUsage(); */
        return;
    }

    for (; narg != argc; narg++) {
        if (msg[0] != 0) {
            strcat(msg, " ");
        }
        strcat(msg, argv[narg]);
    }

    FILE *fd = fopen(".todo", "a");
    writeLine(fd, tags, ntags, msg);
    fclose(fd);

    return;
}


int main(int argc, char **argv) {
    if (argc == 1) {
        print_error("%s: missing subcommand argument\n\n", argv[0]);
        usage(argv[0]);
    }

    // show help message
    if (strcmp(argv[1], "--help") == 0 || 
        strcmp(argv[1], "-h") == 0) usage(argv[0]);

    // show version info
    if (strcmp(argv[1], "--version") == 0 ||
        strcmp(argv[1], "-V") == 0) version(argv[0]);



    int subargc = argc - 2;
    char **subargv;

    if (subargc > 0) {
        subargv = malloc(subargc*sizeof(char*));
        for (int i = 2; i != argc; i++) {
            subargv[i-2] = argv[i];
        }
    }


    for (int i = 0; i != sizeof(command_table)/sizeof(command_table[0]); i++) {
        if (strcasecmp(command_table[i].name, argv[1]) == 0) {
            command_table[i].proc(subargc, subargv);
            return 0;
        }
    }

    print_error("%s: unknown subcommand \"%s\"\n\n", argv[0], argv[1]);
    usage(argv[0]);

    return 1;
}
