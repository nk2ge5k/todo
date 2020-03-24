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

int lookupTask(FILE *stream, int id, task *t) {
    if (t == NULL) {
        // TODO: create temporaray task
    }

    // TODO: should i use temp task?
    header head;
    if (!readHeader(&head, stream)) {
        fprintf(stderr, "could not read file header: %s", strerror(errno));
        return 0;
    }

    int low  = 0;
    int high = head.count - 1;
    int mid;

    int offset;

    while (low <= high) {
        mid = ((unsigned int)low + (unsigned int)high) >> 1;
        offset = sizeof(header)+(sizeof(task)*mid); // TODO: bounds check

        if(fseek(stream, offset, SEEK_SET) == -1) {
            fprintf(stderr, "could not seek file offset %d: %s", offset, strerror(errno));
            return 0;
        }

        if (fread(t, sizeof(task), 1, stream) != 1) {
            fprintf(stderr, "could not read task: %s", strerror(errno));
            return 0;
        }

        if (t->id < id)  {
            low  = mid + 1;
        } else if (t->id > id) {
            high = mid - 1;
        } else {
            if(fseek(stream, offset, SEEK_SET) == -1) {
                fprintf(stderr, "could not seek file offset %d: %s", offset, strerror(errno));
                return 0;
            }
            return 1;
        }
    }

    return 0;
}

int doneCommand(int argc, char **argv, char *file_path) {
    if (argc < 1) {
        fprintf(stderr, "done: missing task id argument\n");
        return 0;
    }

    int target_id = atoi(argv[0]);

    if(access(file_path, F_OK) != 0) {
        return 1;
    }

    FILE *stream;
    if (!(stream = fopen(file_path, "r+"))) {
        fprintf(stderr, "done: could not open todo file %s: %s\n",
                file_path, strerror(errno));
        return 0;
    }

    task t;
    memset(&t, sizeof(task), 0);

    if (!lookupTask(stream, target_id, &t)) {
        fprintf(stderr, "done: could not find task %d\n", target_id);
        fclose(stream);

        return 0;
    }

    t.done_at = time(NULL);

    if (fwrite(&t, sizeof(task), 1, stream) != 1) {
        fprintf(stderr, "done: could not write task to the file: %s\n", strerror(errno));
        fclose(stream);

        return 0;
    }

    fclose(stream);
    return 1;
}

static int cmptasks(const void *p1, const void *p2) {
    const task *t1 = (task *)p1;
    const task *t2 = (task *)p2;

    const int t1_done = (t1->done_at > 0);
    const int t2_done = (t2->done_at > 0);

    if (t1_done != t2_done) 
        return t1_done > t2_done ? 1 : -1;

    if (t1->priority == t2->priority) {
        if (t1->id == t1->id) return 0;

        return t1->id < t2->id ? -1 : 1;
    }

    return t1->priority > t2->priority ? -1 : 1;
}

// list of known priority tags in order highest priority
char *colors[] = {"\033[32m","\033[34m","\033[33m","\033[31m"};

int listCommand(int argc, char **argv, char *file_path) {

    int narg = 0;
    int show_done = 0;

    char project[MAX_PROJECT];
    memset(&project, 0, sizeof(project));

    for (; narg != argc; narg += 2) {
        if ((narg+1) > argc || argv[narg][0] != '-') {
            break;
        }

        if (strcmp("-p", argv[narg]) == 0 ||
            strcmp("--project", argv[narg]) == 0) {

            if (strlen(argv[narg + 1]) + 1 > MAX_PROJECT) {
                fprintf(stderr, "list: project name \"%s\" is to long\n", argv[narg + 1]);
                continue;
            }

            strcpy(project, argv[narg + 1]);
            continue;
        }

        if (strcmp("-d", argv[narg]) == 0 ||
            strcmp("--done", argv[narg]) == 0) {
            show_done = 1;

            continue;
        }
    }

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

    task *tasks = malloc(sizeof(task) * head.count);
    task *t = tasks;
    while (fread(t, sizeof(task), 1, stream) == 1) t++;

    qsort(tasks, head.count, sizeof(task), cmptasks);

    for (int i = 0; i < head.count; i++) {
        if (strlen(project) > 0 && strcasecmp(project, tasks[i].project) != 0) {
            continue;
        }

        int is_done = (tasks[i].done_at > 0);

        if (!show_done && is_done) continue;

        int color = 0;
        for (int j = 63; j <= 257; j+= 64)  {
            if (tasks[i].priority < j) {
                break;
            }
            color++;
        }

        if (is_done) printf("\033[9m");
        printf("%3d [%s]\t%s%s\033[0m", 
                tasks[i].id, tasks[i].project, colors[color], tasks[i].description);
        if (is_done) printf("\033[0m");
        printf("\n");
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

        if (strcmp("-p", argv[narg]) == 0 ||
            strcmp("--project", argv[narg]) == 0) {

            if (strlen(argv[narg + 1]) + 1 > MAX_PROJECT) {
                fprintf(stderr, "add: project name \"%s\" is to long\n", argv[narg + 1]);
                return 0;
            }

            strcpy(t.project, argv[narg + 1]);
            continue;
        }

        if (strcmp("-P", argv[narg]) == 0 || 
            strcmp("--priority", argv[narg]) == 0) {

            if (!isNumber(argv[narg+1])) {
                fprintf(stderr, "add: invalid value for \"%s\" option, expected number\n", 
                    argv[narg]);
                return 0;
            }
            int p = atoi(argv[narg+1]);
            if (p < 0 || 255 <= p) {
                fprintf(stderr, "add: invalid value of \"%s\" option, "
                        "expected number between 0 and 255\n", argv[narg]);
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
    t.id = ++h._id_seq;
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
