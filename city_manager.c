#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>

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

// --- FUNCTII AI (PARSE & MATCH) ---

int parse_condition(const char *input, char *field, char *op, char *value) {
    char temp[256];
    strncpy(temp, input, 256);
    char *f = strtok(temp, ":");
    char *o = strtok(NULL, ":");
    char *v = strtok(NULL, ":");
    if (!f || !o || !v) return 0;
    strcpy(field, f); strcpy(op, o); strcpy(value, v);
    return 1;
}

int match_condition(Report *r, const char *field, const char *op, const char *value) {
    long r_val = 0, c_val = atol(value);
    char *r_str = NULL;
    if (strcmp(field, "severity") == 0) r_val = r->severity;
    else if (strcmp(field, "timestamp") == 0) r_val = r->timestamp;
    else if (strcmp(field, "inspector") == 0) r_str = r->inspector_name;
    else if (strcmp(field, "category") == 0) r_str = r->category;
    else return 0;

    if (r_str) {
        if (strcmp(op, "==") == 0) return strcmp(r_str, value) == 0;
        if (strcmp(op, "!=") == 0) return strcmp(r_str, value) != 0;
    } else {
        if (strcmp(op, "==") == 0) return r_val == c_val;
        if (strcmp(op, "!=") == 0) return r_val != c_val;
        if (strcmp(op, "<") == 0)  return r_val < c_val;
        if (strcmp(op, "<=") == 0) return r_val <= c_val;
        if (strcmp(op, ">") == 0)  return r_val > c_val;
        if (strcmp(op, ">=") == 0) return r_val >= c_val;
    }
    return 0;
}

// --- FUNCTII AJUTATOARE ---

void mode_to_string(mode_t mode, char *str) {
    strcpy(str, "---------");
    if (mode & S_IRUSR) str[0] = 'r'; if (mode & S_IWUSR) str[1] = 'w'; if (mode & S_IXUSR) str[2] = 'x';
    if (mode & S_IRGRP) str[3] = 'r'; if (mode & S_IWGRP) str[4] = 'w'; if (mode & S_IXGRP) str[5] = 'x';
    if (mode & S_IROTH) str[6] = 'r'; if (mode & S_IWOTH) str[7] = 'w'; if (mode & S_IXOTH) str[8] = 'x';
    str[9] = '\0';
}

void log_action(const char *dist, const char *role, const char *user, const char *msg) {
    char path[256]; sprintf(path, "%s/logged_district", dist);
    int fd = open(path, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd != -1) {
        char buf[512]; sprintf(buf, "[%ld] %s (%s): %s\n", time(NULL), user, role, msg);
        write(fd, buf, strlen(buf)); close(fd);
    }
}

// --- OPERATIILE PRINCIPALE ---

void add_report(const char *dist, const char *user, const char *role) {
    char path[256]; sprintf(path, "%s/reports.dat", dist);
    int fd = open(path, O_WRONLY | O_CREAT | O_APPEND, 0664);
    if (fd == -1) return;
    Report r; r.id = (int)time(NULL) % 10000;
    strncpy(r.inspector_name, user, 50); r.timestamp = time(NULL);
    printf("X: "); scanf("%lf", &r.lat); printf("Y: "); scanf("%lf", &r.lon);
    printf("Category: "); scanf("%s", r.category); printf("Severity: "); scanf("%d", &r.severity);
    getchar(); printf("Description: "); fgets(r.description, 96, stdin);
    r.description[strcspn(r.description, "\n")] = 0;
    write(fd, &r, sizeof(Report)); chmod(path, 0664); close(fd);
    char link[256]; sprintf(link, "active_reports-%s", dist);
    unlink(link); symlink(path, link);
    log_action(dist, role, user, "ADD");
}

void list_reports(const char *dist) {
    char path[256]; sprintf(path, "%s/reports.dat", dist);
    struct stat st; if (stat(path, &st) == -1) return;
    char m_str[11]; mode_to_string(st.st_mode, m_str);
    printf("Perms: %s | Size: %ld | Mod: %s", m_str, st.st_size, ctime(&st.st_mtime));
    int fd = open(path, O_RDONLY); Report r;
    while (read(fd, &r, sizeof(Report)) > 0)
        printf("ID: %d | %s | Sev: %d\n", r.id, r.category, r.severity);
    close(fd);
}

void view_report(const char *dist, int id) {
    char path[256]; sprintf(path, "%s/reports.dat", dist);
    int fd = open(path, O_RDONLY); Report r;
    while (read(fd, &r, sizeof(Report)) > 0) {
        if (r.id == id) {
            printf("Full Report %d:\nInsp: %s\nPos: %.2f, %.2f\nDesc: %s\n", r.id, r.inspector_name, r.lat, r.lon, r.description);
            close(fd); return;
        }
    }
    printf("Report not found.\n"); close(fd);
}

void remove_report(const char *dist, int id, const char *role, const char *user) {
    if (strcmp(role, "manager") != 0) { printf("Managers only!\n"); return; }
    char path[256]; sprintf(path, "%s/reports.dat", dist);
    int fd = open(path, O_RDWR); if (fd == -1) return;
    Report r; off_t current_pos; int found = 0;
    while (read(fd, &r, sizeof(Report)) > 0) {
        if (r.id == id) {
            found = 1; current_pos = lseek(fd, 0, SEEK_CUR) - sizeof(Report);
            Report next;
            while (read(fd, &next, sizeof(Report)) > 0) {
                off_t next_pos = lseek(fd, 0, SEEK_CUR);
                lseek(fd, current_pos, SEEK_SET);
                write(fd, &next, sizeof(Report));
                current_pos += sizeof(Report);
                lseek(fd, next_pos, SEEK_SET);
            }
            struct stat st; fstat(fd, &st);
            ftruncate(fd, st.st_size - sizeof(Report));
            break;
        }
    }
    close(fd); if (found) log_action(dist, role, user, "REMOVE");
}

void filter_reports(const char *dist, int argc, char *argv[]) {
    char path[256]; sprintf(path, "%s/reports.dat", dist);
    int fd = open(path, O_RDONLY); if (fd == -1) return;
    Report r;
    while (read(fd, &r, sizeof(Report)) > 0) {
        int all_match = 1;
        for (int i = 7; i < argc; i++) {
            char f[50], o[10], v[100];
            if (parse_condition(argv[i], f, o, v)) {
                if (!match_condition(&r, f, o, v)) { all_match = 0; break; }
            }
        }
        if (all_match) printf("Match ID: %d | %s\n", r.id, r.description);
    }
    close(fd);
}

int main(int argc, char *argv[]) {
    if (argc < 7) return 1;
    char *role = argv[2], *user = argv[4], *cmd = argv[5], *dist = argv[6];
    mkdir(dist, 0750); chmod(dist, 0750);
    if (strcmp(cmd, "--add") == 0) add_report(dist, user, role);
    else if (strcmp(cmd, "--list") == 0) list_reports(dist);
    else if (strcmp(cmd, "--view") == 0 && argc > 7) view_report(dist, atoi(argv[7]));
    else if (strcmp(cmd, "--remove_report") == 0 && argc > 7) remove_report(dist, atoi(argv[7]), role, user);
    else if (strcmp(cmd, "--filter") == 0) filter_reports(dist, argc, argv);
    return 0;
}