#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <sys/wait.h>
#include <signal.h>

typedef struct {
    int id;
    char inspector_name[50];
    double lat;
    double lon;
    char category[30];
    int severity;
    time_t timestamp;
    char description[96];
} Report;


/* ====================== FUNCTII GENERATE CU AI (conform cerinței) ====================== */
int parse_condition(const char *input, char *field, char *op, char *value) {
    char temp[256];
    strncpy(temp, input, sizeof(temp)-1); temp[sizeof(temp)-1] = '\0';
    char *f = strtok(temp, ":");
    char *o = strtok(NULL, ":");
    char *v = strtok(NULL, ":");
    if (!f || !o || !v) return 0;
    strncpy(field, f, 49); field[49] = '\0';
    strncpy(op, o, 9);     op[9] = '\0';
    strncpy(value, v, 99); value[99] = '\0';
    return 1;
}

int match_condition(Report *r, const char *field, const char *op, const char *value) {
    if (strcmp(field, "severity") == 0) {
        int rval = r->severity, cval = atoi(value);
        if (strcmp(op,"==")==0) return rval == cval;
        if (strcmp(op,"!=")==0) return rval != cval;
        if (strcmp(op,"<")==0)  return rval < cval;
        if (strcmp(op,"<=")==0) return rval <= cval;
        if (strcmp(op,">")==0)  return rval > cval;
        if (strcmp(op,">=")==0) return rval >= cval;
    } else if (strcmp(field, "category") == 0) {
        if (strcmp(op,"==")==0) return strcmp(r->category, value)==0;
        if (strcmp(op,"!=")==0) return strcmp(r->category, value)!=0;
    } else if (strcmp(field, "inspector") == 0) {
        if (strcmp(op,"==")==0) return strcmp(r->inspector_name, value)==0;
        if (strcmp(op,"!=")==0) return strcmp(r->inspector_name, value)!=0;
    }
    return 0;
}

/* ====================== FUNCTII AJUTATOARE ====================== */

// Converteste permisiunile din st_mode in format rwxrwxrwx (cerinta obligatorie)
void mode_to_string(mode_t mode, char *str) {
    strcpy(str, "---------");
    if (mode & S_IRUSR) str[0] = 'r';
    if (mode & S_IWUSR) str[1] = 'w';
    if (mode & S_IXUSR) str[2] = 'x';
    if (mode & S_IRGRP) str[3] = 'r';
    if (mode & S_IWGRP) str[4] = 'w';
    if (mode & S_IXGRP) str[5] = 'x';
    if (mode & S_IROTH) str[6] = 'r';
    if (mode & S_IWOTH) str[7] = 'w';
    if (mode & S_IXOTH) str[8] = 'x';
    str[9] = '\0';
}

// Verifica daca rolul curent are acces la fisier (folosind stat() si biții reali)
int has_permission(const char *path, const char *role, int need_write) {
    struct stat st;
    if (stat(path, &st) == -1) return 0;

    if (strcmp(role, "manager") == 0) {
        return need_write ? (st.st_mode & S_IWUSR) : (st.st_mode & S_IRUSR);
    } else if (strcmp(role, "inspector") == 0) {
        return need_write ? (st.st_mode & S_IWGRP) : (st.st_mode & S_IRGRP);
    }
    return 0;
}

// Creaza structura directorului cu permisiunile exacte cerute (750, 664, 640, 644)
void create_district_structure(const char *dist) {
    mkdir(dist, 0750);
    chmod(dist, 0750);

    char p[256];

    sprintf(p, "%s/reports.dat", dist);
    int fd = open(p, O_WRONLY | O_CREAT | O_APPEND, 0664);
    if (fd != -1) { close(fd); chmod(p, 0664); }

    sprintf(p, "%s/district.cfg", dist);
    fd = open(p, O_WRONLY | O_CREAT | O_TRUNC, 0640);
    if (fd != -1) { 
        write(fd, "2\n", 2); 
        close(fd); 
        chmod(p, 0640); 
    }

    sprintf(p, "%s/logged_district", dist);
    fd = open(p, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd != -1) { close(fd); chmod(p, 0644); }
}

// Actualizeaza symlink-ul active_reports-<district>
void update_symlink(const char *dist) {
    char target[256], link[256];
    sprintf(target, "%s/reports.dat", dist);
    sprintf(link, "active_reports-%s", dist);
    unlink(link);
    symlink(target, link);
}

// Verifica daca exista symlink-uri rupte (dangling) folosind lstat() - cerinta specifica
void check_dangling_symlink(const char *dist) {
    char linkname[256];
    sprintf(linkname, "active_reports-%s", dist);
    struct stat st;
    if (lstat(linkname, &st) == 0 && S_ISLNK(st.st_mode)) {
        if (stat(linkname, &st) == -1) {
            printf("Warning: Dangling symlink detected: %s\n", linkname);
        }
    }
}

void log_action(const char *dist, const char *role, const char *user, const char *action) {
    char p[256]; sprintf(p, "%s/logged_district", dist);
    int fd = open(p, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd != -1) {
        char buf[512]; sprintf(buf, "%ld %s %s %s\n", (long)time(NULL), user, role, action);
        write(fd, buf, strlen(buf)); close(fd);
    }
}

// ====================== OPERATII ======================

// Adauga un report nou (ambele roluri au voie)
void add_report(const char *dist, const char *user, const char *role) {
    char p[256]; 
    snprintf(p, sizeof(p), "%s/reports.dat", dist); // UPDATE 1: folosim snprintf pentru securitate
    
    if (!has_permission(p, role, 1)) {
        printf("Access denied for add.\n");
        return;
    }

    Report r = {0};
    r.id = (int)time(NULL) % 100000;
    strncpy(r.inspector_name, user, 49);
    r.timestamp = time(NULL);

    printf("X: "); scanf("%lf", &r.lat);
    printf("Y: "); scanf("%lf", &r.lon);
    printf("Category: "); scanf("%29s", r.category);
    
    // --- UPDATE 2: Validare stricta pentru Severity ---
    do {
        printf("Severity (1-3): "); 
        scanf("%d", &r.severity);
        if (r.severity < 1 || r.severity > 3) {
            printf("Eroare: Severitatea trebuie sa fie obligatoriu 1, 2 sau 3!\n");
        }
    } while (r.severity < 1 || r.severity > 3);
    // ------------------------------------------------

    getchar();
    printf("Description: "); fgets(r.description, 95, stdin);
    r.description[strcspn(r.description, "\n")] = '\0';

    int fd = open(p, O_WRONLY | O_APPEND);
    if (fd != -1) {
        write(fd, &r, sizeof(Report));
        close(fd);
        chmod(p, 0664);
    }
    update_symlink(dist);

    // --- FAZA 2: Notificarea monitorului ---
    int monitor_notified = 0;
    FILE *f_pid = fopen(".monitor_pid", "r");
    
    if (f_pid) {
        pid_t monitor_pid;
        // UPDATE 3: Verificare suplimentara pentru PID valid (> 0)
        if (fscanf(f_pid, "%d", &monitor_pid) == 1 && monitor_pid > 0) {
            // Trimitem semnalul SIGUSR1
            if (kill(monitor_pid, SIGUSR1) == 0) {
                monitor_notified = 1;
            } else {
                perror("Eroare la trimiterea semnalului catre monitor");
            }
        }
        fclose(f_pid);
    }

    // Scriem în log mesajul corespunzător în funcție de succesul notificării
    char action_msg[256];
    if (monitor_notified) {
        strcpy(action_msg, "add (monitor notified)");
    } else {
        strcpy(action_msg, "add (monitor could not be informed)");
    }

    log_action(dist, role, user, action_msg);
    // ----------------------------------------
    printf("Report added successfully (ID: %d)\n", r.id);
}

// Sterge un report folosind lseek + ftruncate (exact cum cere cerinta)
void remove_report(const char *dist, int id, const char *role, const char *user) {
    if (strcmp(role, "manager") != 0) {
        printf("Only managers can remove reports.\n");
        return;
    }

    char path[256]; sprintf(path, "%s/reports.dat", dist);
    if (!has_permission(path, role, 1)) {
        printf("Access denied.\n");
        return;
    }

    int fd = open(path, O_RDWR);
    if (fd == -1) return;

    Report r;
    off_t pos = 0;
    int found = 0;

    while (read(fd, &r, sizeof(Report)) == sizeof(Report)) {
        if (r.id == id) {
            found = 1;
            break;
        }
        pos += sizeof(Report);
    }

    if (!found) {
        printf("Report %d not found.\n", id);
        close(fd);
        return;
    }

    // Shiftam inregistrarile urmatoare (cerinta stricta)
    Report next;
    off_t read_pos = pos + sizeof(Report); 
    off_t write_pos = pos;                 

    while (1) {
        lseek(fd, read_pos, SEEK_SET);
        if (read(fd, &next, sizeof(Report)) <= 0) break;

        lseek(fd, write_pos, SEEK_SET);
        write(fd, &next, sizeof(Report));

        read_pos += sizeof(Report);
        write_pos += sizeof(Report);
    }

    ftruncate(fd, write_pos); 
    close(fd);

    update_symlink(dist);
    log_action(dist, role, user, "remove");
    printf("Report %d removed.\n", id);
}

// Actualizeaza threshold cu verificare stricta a permisiunilor 640
void update_threshold(const char *dist, int new_value, const char *role, const char *user) {
    if (strcmp(role, "manager") != 0) {
        printf("Only managers can update threshold.\n");
        return;
    }

    char path[256]; sprintf(path, "%s/district.cfg", dist);

    struct stat st;
    if (stat(path, &st) == -1 || (st.st_mode & 0777) != 0640) {
        printf("Access denied or permissions on district.cfg do not match 640.\n");
        return;
    }

    if (!has_permission(path, role, 1)) {
        printf("Access denied.\n");
        return;
    }

    int fd = open(path, O_WRONLY | O_TRUNC);
    if (fd != -1) {
        char buf[32]; sprintf(buf, "%d\n", new_value);
        write(fd, buf, strlen(buf));
        close(fd);
        chmod(path, 0640);
        log_action(dist, role, user, "update_threshold");
        printf("Severity threshold updated to %d\n", new_value);
    }
}

// Filtreaza rapoartele conform conditiilor
void filter_reports(const char *dist, int argc, char **argv) {
    char p[256]; sprintf(p, "%s/reports.dat", dist);
    int fd = open(p, O_RDONLY);
    if (fd == -1) return;

    Report r;
    int printed = 0;
    while (read(fd, &r, sizeof(Report)) == sizeof(Report)) {
        int matches = 1;
        for (int i = 0; i < argc; i++) {
            if (strchr(argv[i], ':')) {
                char field[50], op[10], value[100];
                if (parse_condition(argv[i], field, op, value)) {
                    if (!match_condition(&r, field, op, value)) {
                        matches = 0;
                        break;
                    }
                }
            }
        }
        if (matches) {
            printf("ID:%d | %s | Sev:%d | %s\n", r.id, r.category, r.severity, r.description);
            printed = 1;
        }
    }
    close(fd);
    if (!printed) printf("No reports match the filter condition.\n");
}

void remove_district(const char *dist, const char *role) {
    if (strcmp(role, "manager") != 0) {
        printf("Only managers can remove districts.\n");
        return;
    }

    char linkname[256];
    sprintf(linkname, "active_reports-%s", dist);
    unlink(linkname);

    pid_t pid = fork();

    if (pid < 0) {
        perror("Eroare la crearea procesului copil (fork)");
        return;
    } else if (pid == 0) {
        execlp("rm", "rm", "-rf", dist, NULL);
        
        perror("Eroare la executarea comenzii rm");
        exit(1);
    } else {
        int status;
        wait(&status);
        
        if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
            printf("District '%s' removed completely.\n", dist);
        } else {
            printf("A aparut o problema la stergerea districtului.\n");
        }
    }
}

// ====================== MAIN ======================
int main(int argc, char **argv) {
    if (argc < 6) {
        printf("Usage: ./city_manager --role <role> --user <user> <command> <district> [args...]\n");
        return 1;
    }

    char *role = NULL, *user = NULL, *command = NULL, *district = NULL;
    int arg_value = -1;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--role") == 0 && i+1 < argc) role = argv[++i];
        else if (strcmp(argv[i], "--user") == 0 && i+1 < argc) user = argv[++i];
        else if (!command && argv[i][0] != '-') command = argv[i];
        else if (!district && argv[i][0] != '-') district = argv[i];
        else if (atoi(argv[i]) != 0) arg_value = atoi(argv[i]);
    }

    if (!role || !user || !command || !district) {
        printf("Missing arguments\n");
        return 1;
    }

    if (strcmp(command, "remove_district") != 0) {
        check_dangling_symlink(district);
        create_district_structure(district);
    }

    if (strcmp(command, "add") == 0) {
        add_report(district, user, role);
    } else if (strcmp(command, "list") == 0) {
        list_reports(district);
    } else if (strcmp(command, "filter") == 0) {
        filter_reports(district, argc, argv);
    } else if (strcmp(command, "update_threshold") == 0 && arg_value != -1) {
        update_threshold(district, arg_value, role, user);
    } else if (strcmp(command, "remove_report") == 0 && arg_value != -1) {
        remove_report(district, arg_value, role, user);
    } else if (strcmp(command, "remove_district") == 0) {
        remove_district(district, role);
    } else {
        printf("Unknown command: %s\n", command);
    }

    return 0;
}