#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

#define MAX_INPUT 256
#define MAX_ARGS 20

// Functia pentru a porni monitorul si a-i citi output-ul prin PIPE
void cmd_start_monitor() {
    int pipefd[2];
    if (pipe(pipefd) == -1) { 
        perror("Pipe failed"); 
        return; 
    }

    pid_t hub_mon_pid = fork();
    if (hub_mon_pid < 0) { 
        perror("Fork failed"); 
        return; 
    }

    if (hub_mon_pid == 0) { // Suntem in procesul copil (hub_mon)
        pid_t monitor_pid = fork();
        
        if (monitor_pid == 0) { // Suntem in nepot (noul monitor_reports)
            close(pipefd[0]); // Inchidem capatul de citire
            
            // Redirectam output-ul standard (stdout) catre capatul de scriere al pipe-ului
            dup2(pipefd[1], STDOUT_FILENO); 
            close(pipefd[1]);
            
            // Preluam controlul cu executabilul monitor_reports
            execl("./monitor_reports", "monitor_reports", NULL);
            perror("Exec failed");
            exit(1);
        } else { // Suntem din nou in copil (hub_mon) care asculta la pipe
            close(pipefd[1]); // Inchidem capatul de scriere
            char buffer[512];
            ssize_t bytes_read;
            
            printf(">> [Hub] Starting monitor in background...\n");
            
            // Citim constant din pipe ce printeaza monitorul
            while ((bytes_read = read(pipefd[0], buffer, sizeof(buffer) - 1)) > 0) {
                buffer[bytes_read] = '\0';
                
                // Afisam mesajul interceptat
                printf(">> [Hub intercepted]: %s", buffer);
                
                // Daca gasim un mesaj de eroare, stim ca s-a oprit
                if (strstr(buffer, "[ERROR]") != NULL) {
                    printf(">> [Hub] Monitor failed to start (already running).\n");
                    break;
                }
            }
            close(pipefd[0]);
            wait(NULL); // Asteptam ca monitorul sa termine
            exit(0); // hub_mon isi termina treaba in fundal
        }
    } else {
        // Suntem in procesul parinte (city_hub). Nu dam wait() ca sa nu blocam meniul!
        // hub_mon ramane in background.
        return;
    }
}

// Functia pentru a calcula scorurile cu executabilul extern
void cmd_calculate_scores(int count, char *districts[]) {
    printf("\n=== Combined Workload Report ===\n");
    for (int i = 0; i < count; i++) {
        int pipefd[2];
        if (pipe(pipefd) == -1) continue;

        pid_t pid = fork();
        if (pid == 0) { // Copil (scorer)
            close(pipefd[0]);
            
            // Redirectam output-ul catre pipe folosind dup2
            dup2(pipefd[1], STDOUT_FILENO); 
            close(pipefd[1]);
            
            execl("./scorer", "scorer", districts[i], NULL);
            perror("Exec scorer failed");
            exit(1);
        } else { // Parinte (city_hub)
            close(pipefd[1]);
            char buffer[1024];
            ssize_t bytes;
            
            // Citim tot ce a printat scorer si afisam
            while ((bytes = read(pipefd[0], buffer, sizeof(buffer) - 1)) > 0) {
                buffer[bytes] = '\0';
                printf("%s", buffer);
            }
            close(pipefd[0]);
            wait(NULL); // Asteptam sa termine scorer-ul curent inainte sa trecem la urmatorul
        }
    }
    printf("================================\n\n");
}

int main() {
    char input[MAX_INPUT];
    char *args[MAX_ARGS];

    printf("Welcome to City Hub!\n");
    printf("Commands: start_monitor, calculate_scores <dist1> <dist2> ..., exit\n");

    while (1) {
        printf("city_hub> ");
        if (!fgets(input, MAX_INPUT, stdin)) break;

        input[strcspn(input, "\n")] = 0; // Stergem newline-ul pus de fgets
        if (strlen(input) == 0) continue;

        // Parsam comanda
        int arg_count = 0;
        char *token = strtok(input, " ");
        while (token != NULL && arg_count < MAX_ARGS) {
            args[arg_count++] = token;
            token = strtok(NULL, " ");
        }

        if (strcmp(args[0], "exit") == 0) {
            printf("Exiting City Hub...\n");
            break;
        } else if (strcmp(args[0], "start_monitor") == 0) {
            cmd_start_monitor();
        } else if (strcmp(args[0], "calculate_scores") == 0) {
            if (arg_count < 2) {
                printf("Usage: calculate_scores <district1> [district2...]\n");
            } else {
                cmd_calculate_scores(arg_count - 1, &args[1]);
            }
        } else {
            printf("Unknown command.\n");
        }
    }
    return 0;
}