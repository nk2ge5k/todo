#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#define print_error(...) fprintf(stderr, __VA_ARGS__)
#define len(a) sizeof(a)/sizeof(a[0])

const int PATH_MAX       = 4096;
const int MAX_MSG_LENGTH = 256;
const int MAX_TAGS_COUNT = 16;

typedef int proc(int argc, char **argv, char *file_path);
typedef void printUsage(char *cmd);
typedef struct {
    char *name;
    char *desc;
    proc *proc;
    printUsage *usage;
} command;

int addCommand(int argc, char **argv, char *file_path);
void addCommandUsage(char *cmd);
int listCommand(int argc, char **argv, char *file_path);
void listCommandUsage(char *cmd);

command command_table[] = {
    {"add", "Add new task to list", addCommand, addCommandUsage},
    {"list", "Prints the list of existing tasks", listCommand, listCommandUsage},
};

// usage prints help information for command
void usage(char *cmd) { 
    fprintf(stderr, "USAGE:\n");
    fprintf(stderr, "   %s [OPTIONS] [SUBCOMMAND]\n", cmd);
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
void version(char *cmd) {
    fprintf(stderr, "%s: 0.0.0-dev\n", cmd);
    exit(1);
}

void listCommandUsage(char *cmd) {
    fprintf(stderr, "USAGE: \n");
    fprintf(stderr, "   %s list [OPTIONS]\n", cmd);
    fprintf(stderr, "\n");
    fprintf(stderr, "OPTIONS:\n");
    fprintf(stderr, "   -t, --tag           Filter tasks by tag\n");
    fprintf(stderr, "   -h, --help          Show this help message\n");

    exit(1);
}

// list of known priority tags in order highest priority
char *known_tags[] = { "urgent", "high", "norm", "low" };
char *colors[] = { "\033[31m", "\033[93m", "\033[34m", "\033[32m" };

int listCommand(int argc, char **argv, char *file_path) {

    FILE *stream;
    if (!(stream = fopen(file_path, "r"))) {
        print_error("list: could not open file %s: %s\n", file_path, strerror(errno));
        return 0;
    }

    char    *tags[MAX_TAGS_COUNT];
    char    *buf    = malloc(4 << 10);
    size_t  len     = 0;
    ssize_t nread   = 0;

    // TODO(nk2ge5k): кажется это можно внести в отдельную функцию
    int ntags = 0;
    int narg  = 0;
    for (; narg != argc; narg += 2) {
        if ((narg+1) > argc || argv[narg][0] != '-') {
            break;
        }

        if (strcmp("-t", argv[narg]) == 0 || strcmp("--tag", argv[narg]) == 0) {
            if (strlen(argv[narg+1]) > 0) {
                tags[ntags++] = argv[narg+1];
            }
        }
    }

    while((nread = getline(&buf, &len, stream)) != -1) {
        if (ntags > 0) {
            // если бы передан список тэгов в команду, то пытамся отфильтровать
            // те таски, которые не содержат преданных тэгов, в случае если
            // список тэгов пуст, то оставляем все таски
            int hasTag = 0;

            for (int i = 0; i < ntags; i++) {
                if (hasTag = (strstr(buf, tags[i]) != NULL)) {
                    break;
                }
            }
            if (!hasTag) {
                continue;
            }
        }

        char *color = "\033[39m"; // default color

        for (int i = 0; i < (sizeof(known_tags)/sizeof(known_tags[0])); i++) {

            if (strstr(buf, known_tags[i]) != NULL) {
                color = colors[i];
                break;
            }
        }

        // TOOD: scan for tag
        // Кажется разуммным будет следующий алгоритм:
        // найти первый тэг понять для него цвет видимо из какого-то конфига
        // и напечатать с этим цветом, кажется на текущий момент будет достаточно
        // если конфиг цветов будет прямо в бинаре. Потом можно попробовать
        // притащить json или yaml
        printf("%s%s\033[0m", color, buf);
    }

    free(buf);
    fclose(stream);

    return 1;
}


void addCommandUsage(char *cmd) {
    fprintf(stderr, "USAGE: \n");
    fprintf(stderr, "   %s add [OPTIONS] [MESSAGE]\n", cmd);
    fprintf(stderr, "\n");
    fprintf(stderr, "OPTIONS:\n");
    fprintf(stderr, "   -t, --tag           Add tag for a new task\n");
    fprintf(stderr, "   -h, --help          Show this help message\n");

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

int addCommand(int argc, char **argv, char *file_path) {

    char *tags[MAX_TAGS_COUNT];
    char msg[MAX_MSG_LENGTH];


    int ntags = 0;
    int narg  = 0;
    for (; narg != argc; narg += 2) {
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
        print_error("add: task message is required\n\n");
        return 0;
    }

    for (; narg != argc; narg++) {
        if (strlen(msg) != 0) {
            strcat(msg, " ");
        }
        strcat(msg, argv[narg]);
    }

    FILE *fd;
    if ((fd = fopen(file_path, "a"))) {
        writeLine(fd, tags, ntags, msg);
        fclose(fd);
    } else {
        print_error("add: could not open todo file %s: %s\n", file_path, strerror(errno));
        return 0;
    }

    return 1;
}

char* defaultFilePath(char *file_path, int size) {
    const char *FILE_NAME = "todo.txt";

    char *homedir = getenv("HOME");
    int hlen = strlen(homedir);

    if (hlen != 0 && hlen < size) {
        strcpy(file_path, homedir);
    } else {
        char cwd[PATH_MAX];
        // TODO: get cwd
        getcwd(cwd, size);
        strcpy(file_path, cwd);
    }

    if (size > (strlen(file_path) + strlen(FILE_NAME) + 1)) {
        strcat(file_path, "/");
        strcat(file_path, FILE_NAME);
    }

    return file_path;
}


int main(int argc, char **argv) {
    if (argc == 1) {
        print_error("%s: missing subcommand argument\n\n", argv[0]);
        usage(argv[0]);
    }

    char *file_path = defaultFilePath(malloc(sizeof(char) * PATH_MAX), PATH_MAX);

    int subargc;            // number of subcommand aruments
    int comm_argc = 2;      // position in argv array where subcommand arguments starts
    char **subargv = NULL;

    // show help message
    if (strcmp(argv[1], "--help") == 0 || 
        strcmp(argv[1], "-h") == 0) usage(argv[0]);

    // show version info
    if (strcmp(argv[1], "--version") == 0 ||
        strcmp(argv[1], "-V") == 0) version(argv[0]);


    if (strcmp(argv[1], "--file") == 0 ||
        strcmp(argv[1], "-f") == 0) {
        if (argc > comm_argc) {
            free(file_path);
            file_path = argv[comm_argc++];
        }
    }

    subargc = argc-comm_argc;
    command cmd;
    for (int j = 0; j != sizeof(command_table)/sizeof(command); j++) {
        if (strcasecmp(command_table[j].name, argv[1]) == 0) {
            cmd = command_table[j];

            if (subargc > 0) {
                // show help message
                if (strcmp(argv[argc-subargc], "--help") == 0 || 
                    strcmp(argv[argc-subargc], "-h") == 0) cmd.usage(argv[0]);

                subargv = malloc(sizeof(char*) * subargc);
                for (int i = 0; i != subargc; i++) {
                    subargv[i] = argv[comm_argc+i];
                }
            }

            int ok = cmd.proc(subargc, subargv, file_path);
            free(subargv);

            if (!ok) {
                cmd.usage(argv[0]);
            }

            return 0;
        }
    }

    print_error("%s: unknown subcommand \"%s\"\n\n", argv[0], argv[1]);
    usage(argv[0]);

    return 1;
}
