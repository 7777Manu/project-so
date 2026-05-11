#define _XOPEN_SOURCE 700

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <sys/types.h>

volatile sig_atomic_t running = 1;

// Handler pentru CTRL+C (SIGINT)
void handle_sigint(int sig) {
    const char *msg = "[INFO] Received SIGINT. Shutting down...\n";
    write(STDOUT_FILENO, msg, strlen(msg));
    unlink(".monitor_pid");
    running = 0;
}

// Handler pentru raport nou (SIGUSR1)
void handle_sigusr1(int sig) {
    const char *msg = "[ALERT] A new report has been added!\n";
    write(STDOUT_FILENO, msg, strlen(msg));
}

int main() {
    // --- FAZA 3: Verificare dacă mai rulează un monitor ---
    FILE *check_f = fopen(".monitor_pid", "r");
    if (check_f) {
        pid_t existing_pid;
        if (fscanf(check_f, "%d", &existing_pid) == 1) {
            // Trimitem mesajul de eroare (care va fi prins de pipe)
            printf("[ERROR] Monitor already running with PID: %d\n", existing_pid);
            fflush(stdout); // Foarte important la pipes ca textul sa plece imediat!
            fclose(check_f);
            return 1; // Ieșim
        }
        fclose(check_f);
    }
    // -------------------------------------------------------

    // 1. Dacă am ajuns aici, suntem singurul monitor. Salvăm PID-ul.
    pid_t pid = getpid();
    FILE *f = fopen(".monitor_pid", "w");
    if (!f) {
        perror("[ERROR] Eroare la crearea fișierului .monitor_pid");
        return 1;
    }
    fprintf(f, "%d\n", pid);
    fclose(f);
    
    printf("[INFO] Started successfully with PID: %d\n", pid);
    fflush(stdout);

    // 2. Setăm handlerele folosind sigaction
    struct sigaction sa_int, sa_usr1;

    sa_int.sa_handler = handle_sigint;
    sigemptyset(&sa_int.sa_mask);
    sa_int.sa_flags = 0;
    sigaction(SIGINT, &sa_int, NULL);

    sa_usr1.sa_handler = handle_sigusr1;
    sigemptyset(&sa_usr1.sa_mask);
    sa_usr1.sa_flags = 0;
    sigaction(SIGUSR1, &sa_usr1, NULL);

    // 3. Buclă infinită
    while (running) {
        pause(); 
    }

    return 0;
}