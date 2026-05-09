#define _XOPEN_SOURCE 700

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <sys/types.h>


// Variabilă globală pentru a ține programul în viață
volatile sig_atomic_t running = 1;

// Handler pentru CTRL+C
void handle_sigint(int sig) {
    const char *msg = "\n[Monitor] Received SIGINT. Shutting down...\n";
    write(STDOUT_FILENO, msg, strlen(msg));
    
    unlink(".monitor_pid");
    running = 0;
}

// Handler pentru semnalul că s-a adăugat un raport nou 
void handle_sigusr1(int sig) {
    const char *msg = "[Monitor] Alert: A new report has been added!\n";
    write(STDOUT_FILENO, msg, strlen(msg));
}

int main() {
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

    struct sigaction sa_int, sa_usr1;

    // Configurăm acțiunea pentru SIGINT cu verificare de erori
    sa_int.sa_handler = handle_sigint;
    sigemptyset(&sa_int.sa_mask);
    sa_int.sa_flags = 0;
    if (sigaction(SIGINT, &sa_int, NULL) == -1) {
        perror("Eroare la setarea handler-ului pentru SIGINT");
        return 1;
    }

    // Configurăm acțiunea pentru SIGUSR1 cu verificare de erori
    sa_usr1.sa_handler = handle_sigusr1;
    sigemptyset(&sa_usr1.sa_mask);
    sa_usr1.sa_flags = 0;
    if (sigaction(SIGUSR1, &sa_usr1, NULL) == -1) {
        perror("Eroare la setarea handler-ului pentru SIGUSR1");
        return 1;
    }

    while (running) {
        pause(); 
    }

    return 0;
}