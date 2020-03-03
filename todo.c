#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <unistd.h>
#include "todo.h"


// readHeader reads header into the head and sets cursor of the file
// to the end of header
int readHeader(header *head, FILE *fd) {
    if (head == NULL) {
        return 0; // valid error
    }

    // rewind to the beginning of the file
    // NOTE: fseek calls fflush if needed
    if (fseek(fd, 0L, SEEK_SET) != 0) { 
        return 0;
    }

    memset(head, 0, sizeof(header));

    if (fread(head, sizeof(header), 1, fd) != 1) {
        return 0;
    }

    /* long int cursor = ftell(fd); */
    /* printf("cusor at %ld, size is: %d", cursor, sizeof(header)); */
    return 1;
}

int writeHeader(header *head, FILE *fd) {
    // TODO: should i zero header space in the file?

    // get current position of cursor
    long int cursor = ftell(fd);
    if (cursor == -1) {
        return 0;
    }
    // rewind to the beginning of the file
    // NOTE: fseek calls fflush if needed
    if (fseek(fd, 0L, SEEK_SET) != 0) { 
        return 0;
    }

    printf("witing header\n");
    printf("version: %d\n", head->version);
    printf("_id_seq: %d\n", head->_id_seq);
    printf("count: %d\n", head->count);
    // wirte header to the file
    if (fwrite(head, sizeof(header), 1, fd) != 1) {
        return 0;
    }
    // return to the previous position
    return fseek(fd, cursor, SEEK_SET) == 0;
}

int isNumber(char *str) {
    int len = strlen(str);
    for (int i = 0; i != len; i++) {
        if (!isdigit(str[i])) return 0;
    }
    return 1;
}

int doneCommand(int arch, char **argv, char *file_path) {

    return 0;
}

int editCommand(int argc, char **argv, char *file_path) {
    fprintf(stderr, "edit: command not implemented yet\n");
    return 0;

    char *editor = getenv("EDITOR");
    if (editor == NULL) {
        fprintf(stderr, "edit: EDITOR environment variable does not set\n");
        return 0;
    }

    char cmd[4096];

    sprintf(cmd, "%s %s", editor, file_path);
    if (system(cmd) == -1) {
        fprintf(stderr, "edit: could not execute command \"%s\": %s\n", cmd, strerror(errno));
        return 0;
    }

    return 1;
}


// list of known priority tags in order highest priority
char *colors[] = {"\033[32m","\033[34m","\033[33m","\033[31m"};

int listCommand(int argc, char **argv, char *file_path) {
    if(access(file_path, F_OK) != 0) {
        return 1;
    }
    FILE *stream;

    if (!(stream = fopen(file_path, "r"))) {
        fprintf(stderr, "list: could not open todo file %s: %s\n",
                file_path, strerror(errno));
        return 0;
    }

    header head;
    if (!readHeader(&head, stream)) {
        fprintf(stderr, "list: could not read file %s header: %s",
                file_path, strerror(errno));
        return 0;
    }

    task t;
    memset(&t, 0, sizeof(task));

    while (fread(&t, sizeof(task), 1, stream) == 1) {
        int color = 0;
        for (int i = 63; i <= 257; i+= 64)  {
            if (t.priority < i) {
                break;
            }
            color++;
        }
        /* printf("id: %d\n", t.id); */
        /* printf("priority: %d\n", t.priority); */
        /* printf("project: %s\n", t.project); */
        /* printf("description: %s\n", t.description); */
        /* printf("created_at: %d\n", t.created_at); */
        /* printf("done_at: %d\n", t.done_at); */

        printf("[%s] %s%s\033[0m\n", t.project, colors[color], t.description);
    }

    if (errno != 0) {
        fprintf(stderr, "list: could not read task from file: %s\n", strerror(errno));
        fclose(stream);

        return 0;
    }

    fclose(stream);
    return 1;
}


int addCommand(int argc, char **argv, char *file_path) {
    task t;
    header h;

    memset(&h, 0, sizeof(header));
    memset(&t, 0, sizeof(task));

    // TODO(nk2ge5k): open file and read header
    //  if file does not eixsts than initialize empty header


    int narg  = 0;
    for (; narg != argc; narg += 2) {
        if ((narg+1) > argc || argv[narg][0] != '-') {
            break;
        }

        if (strcmp("-P", argv[narg]) == 0 ||
            strcmp("--project", argv[narg]) == 0) {

            if (strlen(argv[narg + 1]) + 1 > MAX_PROJECT) {
                fprintf(stderr, "add: project name \"%s\" is to long\n", argv[narg + 1]);
                return 0;
            }

            strcpy(t.project, argv[narg + 1]);
            continue;
        }

        if (strcmp("-p", argv[narg]) == 0 || 
            strcmp("--priority", argv[narg]) == 0) {

            if (!isNumber(argv[narg+1])) {
                fprintf(stderr, "add: invalid value for \"%s\" option, expected number\n", 
                    argv[narg]);
                return 0;
            }
            int p = atoi(argv[narg+1]);
            if (p < 0 || 255 < p) {
                fprintf(stderr, "add: invalid value of \"%s\" option, "
                        "expected number between 0 and 65535\n", argv[narg]);
                return 0;
            }

            t.priority = (priority)p;
            continue;
        }

        fprintf(stderr, "add: unknown option \"%s\"\n", argv[narg]);
        return 0;
    }

    if (narg == argc) {
        fprintf(stderr, "add: task description is required\n");
        return 0;
    }


    for (; narg != argc; narg++) {
        if (strlen(t.description) + strlen(argv[narg]) + 2 > MAX_DESCRIPTION) {
            fprintf(stderr, "add: message too long\n");
            return 0;
        }
        strcat(t.description, argv[narg]);
        strcat(t.description, " ");
    }

    // TODO: may be print file header with meta information about count of tasks
    // int the file and current id and version of the file to be able to stop
    // reading it if its too old of a version

    FILE *stream;
    if (access(file_path, R_OK | W_OK) == 0) {
        if (!(stream = fopen(file_path, "r+"))) {
            fprintf(stderr, "add: could not open todo file %s: %s\n", 
                    file_path, strerror(errno));
            fclose(stream);
            return 0;
        }
        // TODO: read header

        if (!readHeader(&h, stream)) {
            fprintf(stderr, "add: could not read file header: %d %s\n",
                    strerror(errno));
            fclose(stream);
            return 0;
        }
    } else {
        // TODO: initFile(FILE *fd)
        if (!(stream = fopen(file_path, "w+"))) {
            fprintf(stderr, "add: could not open todo file %s: %s\n", 
                    file_path, strerror(errno));
            fclose(stream);
            return 0;
        }

        if (!writeHeader(&h, stream)) {
            fprintf(stderr, "add: could not write header to the file: %s\n",
                    strerror(errno));
            fclose(stream);
            return 0;
        }
    }

    if (fseek(stream, 0, SEEK_END) != 0) {
        fprintf(stderr, "add: failed to seek file offset: %s", strerror(errno));
        fclose(stream);

        return 0;
    }

    t.created_at = time(NULL);
    t.id = h._id_seq++;
    h.count++;

    size_t written = fwrite(&t, sizeof(task), 1, stream);
    if (written != 1) {
        fprintf(stderr, "add: could not write task to the file: %s\n", strerror(errno));
        fclose(stream);
        return 0;
    }

    if (!writeHeader(&h, stream)){
        fprintf(stderr, "add: could not write header to the file: %s\n", strerror(errno));
        fclose(stream);
        return 0;
    }

    return 1;
}
