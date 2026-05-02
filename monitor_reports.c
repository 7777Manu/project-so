#define _XOPEN_SOURCE 700

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <sys/types.h>


// Variabilă globală pentru a ține programul în viață
volatile sig_atomic_t running = 1;

// Handler pentru CTRL+C (SIGINT)
void handle_sigint(int sig) {
    // Folosim write în loc de printf în handlere pentru siguranță (System Programming best practice)
    const char *msg = "\n[Monitor] Received SIGINT. Shutting down...\n";
    write(STDOUT_FILENO, msg, strlen(msg));
    
    // Ștergem fișierul cu PID-ul la închidere
    unlink(".monitor_pid");
    running = 0;
}

// Handler pentru semnalul că s-a adăugat un raport nou (SIGUSR1)
void handle_sigusr1(int sig) {
    const char *msg = "[Monitor] Alert: A new report has been added!\n";
    write(STDOUT_FILENO, msg, strlen(msg));
}

int main() {
    // 1. Aflăm PID-ul și îl scriem în fișierul ascuns .monitor_pid
    pid_t pid = getpid();
    FILE *f = fopen(".monitor_pid", "w");
    if (!f) {
        perror("Eroare la crearea fișierului .monitor_pid");
        return 1;
    }
    fprintf(f, "%d\n", pid);
    fclose(f);
    
    printf("[Monitor] Started successfully with PID: %d\n", pid);
    printf("[Monitor] Waiting for signals...\n");

    // 2. Setăm handlerele folosind sigaction (Regulă strictă: fără signal())
    struct sigaction sa_int, sa_usr1;

    // Configurăm acțiunea pentru SIGINT
    sa_int.sa_handler = handle_sigint;
    sigemptyset(&sa_int.sa_mask);
    sa_int.sa_flags = 0;
    sigaction(SIGINT, &sa_int, NULL);

    // Configurăm acțiunea pentru SIGUSR1
    sa_usr1.sa_handler = handle_sigusr1;
    sigemptyset(&sa_usr1.sa_mask);
    sa_usr1.sa_flags = 0;
    sigaction(SIGUSR1, &sa_usr1, NULL);

    // 3. Buclă infinită: programul doarme (pause) până primește un semnal
    while (running) {
        pause(); 
    }

    return 0;
}