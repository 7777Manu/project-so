#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>

#define MAX_INSPECTORS 100

// Trebuie să avem aceeași structură ca în city_manager
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

// Structură ajutătoare pentru a ține minte scorul fiecărui inspector
typedef struct {
    char name[50];
    int score;
} InspectorScore;

int main(int argc, char *argv[]) {
    // Verificăm dacă am primit numele districtului ca argument
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <district_name>\n", argv[0]);
        return 1;
    }

    char filepath[256];
    snprintf(filepath, sizeof(filepath), "%s/reports.dat", argv[1]);

    int fd = open(filepath, O_RDONLY);
    if (fd == -1) {
        printf("District '%s': No reports found or district does not exist.\n", argv[1]);
        return 0;
    }

    InspectorScore scores[MAX_INSPECTORS];
    int num_inspectors = 0;
    Report r;

    // Citim fiecare raport din fișierul binar
    while (read(fd, &r, sizeof(Report)) == sizeof(Report)) {
        int found = 0;
        // Căutăm dacă inspectorul există deja în lista noastră
        for (int i = 0; i < num_inspectors; i++) {
            if (strcmp(scores[i].name, r.inspector_name) == 0) {
                scores[i].score += r.severity; // Adunăm severitatea 
                found = 1;
                break;
            }
        }
        // Dacă e un inspector nou, îl adăugăm în listă
        if (!found && num_inspectors < MAX_INSPECTORS) {
            strncpy(scores[num_inspectors].name, r.inspector_name, 49);
            scores[num_inspectors].name[49] = '\0';
            scores[num_inspectors].score = r.severity;
            num_inspectors++;
        }
    }
    close(fd);

    // Afișăm rezultatul (care mai târziu va fi capturat prin pipe )
    printf("--- Workload for District: %s ---\n", argv[1]);
    for (int i = 0; i < num_inspectors; i++) {
        printf("Inspector %s: %d\n", scores[i].name, scores[i].score);
    }

    return 0;
}