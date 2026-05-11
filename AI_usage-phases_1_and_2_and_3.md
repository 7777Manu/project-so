# Documentare Analitică privind Utilizarea Inteligenței Artificiale - Faze Integrate (1, 2 și 3)

[cite_start]Prezentul document detaliază interacțiunea cu asistentul virtual Gemini, subliniind procesul de formulare a cerințelor, analiza critică a codului generat și adaptarea acestuia pentru a satisface constrângerile riguroase ale unui mediu de tip UNIX[cite: 3, 144].

---

## FAZA 1: Managementul Sistemului de Fișiere, Permisiuni și Filtrare Avansată

### 1. Instrumentul Utilizat
[cite_start]Pentru facilitarea implementării logicii de parsare și evaluare a condițiilor din cadrul comenzii `filter`[cite: 48, 54], am apelat la asistentul virtual **Gemini**.

### 2. Metodologia de Prompting
[cite_start]Am adoptat o abordare modulară[cite: 62], definind problema prin prompt-uri precise, axate pe decuplarea logicii de parsare de cea de evaluare:

* [cite_start]**Prompt 1 (Parsare structurală):** "Elaborează o funcție în limbajul C având semnătura `int parse_condition(const char *input, char *field, char *op, char *value);`[cite: 63]. Scopul acesteia este tokenizarea unui șir de caractere de formatul `câmp:operator:valoare` în componentele sale lexicale, asigurând o gestionare sigură a memoriei."
* [cite_start]**Prompt 2 (Evaluare logică):** "Având la dispoziție structura de date `Report` predefinită [cite: 17, 18, 19, 20, 21, 22, 23][cite_start], generează funcția `int match_condition(Report *r, const char *field, const char *op, const char *value);`[cite: 66]. Aceasta trebuie să evalueze predicatul logic și să asigure conversia implicită și sigură a tipurilor de date la runtime (ex. conversia argumentelor din format string în valori numerice întregi)."

### 3. Analiza Soluției Generate
* **Modulul de parsare:** AI-ul a propus o implementare bazată pe funcția standard `strtok` din biblioteca `<string.h>` pentru izolarea delimitatorilor, sugerând popularea parametrilor de ieșire prin intermediul funcției `strcpy`.
* **Modulul de potrivire:** Soluția a constat într-o arhitectură ramificată de decizii (`if-else` chaining) cuplată cu `strcmp` pentru determinarea câmpului vizat. Conversiile de tip necesare evaluării operatorilor relaționali au fost abordate prin funcții standard, precum `atoi()`.

### 4. Evaluare Critică și Refactorizare Definitivă
[cite_start]În urma unei analize riguroase a codului propus, am identificat și remediat o serie de vulnerabilități potențiale și incompatibilități cu standardele de programare robustă în C[cite: 97]:

1. **Integritatea memoriei (Problema funcțiilor distructive):** Modelul inițial apela `strtok` direct pe parametrul `input`. Deoarece `strtok` mutilează string-ul original prin inserarea caracterelor `\0`, iar datele proveneau direct din pointerii de argumente (`argv`), am introdus o etapă de sanitizare. Am instanțiat un buffer temporar local în care am duplicat datele utilizând `strncpy`, protejând astfel integritatea datelor din linia de comandă.
2. **Atenuarea riscului de Buffer Overflow:** Am eliminat complet utilizarea funcției nesigure `strcpy` recomandată de AI, înlocuind-o cu `strncpy`. Această decizie a impus definirea clară a limitelor de scriere (ex. 49 de caractere pentru câmpuri de nume) și terminarea manuală a string-urilor cu caracterul nul, garantând astfel stabilitatea programului la input-uri malitioase sau corupte.
3. [cite_start]**Conversii de tip pe arhitecturi moderne:** Pentru evaluarea câmpului `timestamp`[cite: 22], am observat că AI-ul nu a tratat corespunzător dimensiunea tipului `time_t`. Am rafinat algoritmul implementând conversia explicită `(time_t)atol(value)`, prevenind astfel posibile trunchieri de date pe arhitecturi pe 64 de biți.

### 5. Concluzii și Achiziții Cognitive
Această etapă a subliniat importanța tratării precaute a funcțiilor din `<string.h>` care prezintă efecte secundare (side-effects) asupra memoriei alocate. Mai mult, a reconfirmat necesitatea validării stricte a conversiilor de tip în contextul procesării intrărilor nesigure provenite de la utilizator.

---

## FAZA 2: Orchestrarea Proceselor (fork/exec) și Comunicarea Inter-Procese (IPC prin Semnale)

### 1. Metodologia de Prompting
[cite_start]Pentru a naviga complexitatea administrării proceselor și a semnalelor asincrone[cite: 99, 100], am direcționat cerințele către aspecte specifice de System Programming:
* [cite_start]**Prompt 1 (Managementul proceselor):** "Explică paradigma utilizării primitivei `fork()` în tandem cu familia de funcții `exec` pentru delegarea execuției unei comenzi de sistem (`rm -rf`)[cite: 103], asigurând totodată sincronizarea procesului părinte prin blocarea execuției până la terminarea copilului."
* [cite_start]**Prompt 2 (Handlere de semnal POSIX):** "Proiectează arhitectura unui daemon de fundal care își externalizează PID-ul[cite: 106]. [cite_start]Demonstrează înlocuirea funcției perimate `signal()` cu apelul de sistem robust `sigaction` [cite: 114, 115] [cite_start]pentru interceptarea semnalelor `SIGUSR1` și `SIGINT`[cite: 109, 110]."
* **Prompt 3 (Sincronizare IPC):** "Cum se poate orchestra trimiterea unui semnal `SIGUSR1` către un proces complet decuplat, având ca singură legătură un fișier text ce conține un PID?"

### 2. Analiza Soluției Generate
* [cite_start]Codul generat a exemplificat corect crearea unui nou spațiu de memorie prin `fork()` și înlocuirea imaginii procesului cu utilitarul `rm` via `execlp`[cite: 103].
* Structurile `sigaction` au fost instanțiate corect, incluzând curățarea măștilor de semnal cu `sigemptyset`.
* [cite_start]Pentru partea de transmitere, AI-ul a propus o secvență clasică de I/O pe fișiere (deschidere, `fscanf`, urmată direct de apelul sistem `kill()`)[cite: 111, 114].

### 3. Evaluare Critică, Refactorizare și Învățare
1. **Conformitatea cu Standardele POSIX:** Pe parcursul implementării în IDE-ul local, m-am confruntat cu absența definițiilor pentru structurile `sigaction`. Studiind sugestiile AI-ului, am asimilat conceptul de macro-definiții de feature test și am rezolvat blocajul adăugând directiva `#define _XOPEN_SOURCE 700`, forțând vizibilitatea extensiilor POSIX.
2. **Reentranța și Async-Signal-Safety:** O lecție fundamentală de programare la nivel de sistem a apărut la proiectarea handler-elor de semnal. Deși inițial asistentul sugerase folosirea `printf`, am corectat implementarea trecând la apelul `write(STDOUT_FILENO, ...)`. Am învățat astfel că funcțiile din familia `stdio` nu sunt reentrante (async-signal-safe) și pot duce la deadlock-uri dacă întrerup firul de execuție principal într-un moment inoportun.
3. **Validarea Entităților de Sistem:** Am constatat o lacună în soluția propusă pentru funcția `kill()`. Dacă fișierul PID ar fi fost corupt (ex. PID 0 sau -1), programul ar fi putut emite semnale în mod eronat către grupul de procese sau sistemul întreg. Prin urmare, am introdus o santinelă de cod pentru a valida strict PID-ul (`pid > 0`) anterior emiterii oricărui semnal.

---

## FAZA 3: Arhitecturi Avansate IPC prin Conducte (Pipes) și Redirectări (dup2)

### 1. Metodologia de Prompting
[cite_start]În etapa finală, focusul a fost pe orchestrarea fluxurilor de date I/O standard între procese distincte[cite: 123]:
* [cite_start]**Prompt 1 (Modulul Scorer):** "Proiectează arhitectura unui executabil independent (`scorer`) capabil să acceseze fișiere binare și să efectueze o agregare de date (workload score) pe baza unui parametru transmis din exterior[cite: 132, 133]."
* [cite_start]**Prompt 2 (Mecanisme de Prevenție Coliziuni):** "Cum pot modifica un proces de fundal existent pentru a detecta la lansare prezența unui fișier de tip "lock" (conținând un PID)[cite: 127], a raporta coliziunea la nivel de `stdout` și a înceta execuția, permițând interceptarea facilă a acestui eveniment de către un proces supraveghetor?"
* [cite_start]**Prompt 3 (City Hub și Multiplexarea I/O):** "Stabilește fluxul de control pentru un program interactiv de tip REPL (`city_hub`)[cite: 124]. [cite_start]Detaliază mecanismul prin care procesul părinte instanțiază un pipe [cite: 126][cite_start], creează un sub-proces, deturnează descriptorul de fișier asociat `STDOUT_FILENO` folosind `dup2` [cite: 134, 138] către capătul de scriere al pipe-ului și ulterior înlocuiește imaginea procesului prin `execl`."

### 2. Analiza Soluției Generate și Evaluare Critică
* [cite_start]**Arhitectura manipulării descriptorilor de fișiere (`dup2`):** Asistentul AI a validat conceptul teoretic conform căruia deturnarea output-ului necesită o secvență strictă de pași în procesul copil: închiderea capătului de citire al pipe-ului (pentru a evita leak-urile de descriptori), clonarea descriptorului cu `dup2(pipefd[1], STDOUT_FILENO)`[cite: 134, 138], închiderea vechiului capăt de scriere, și executarea programului extern. Această rutină a fost integrată cu succes atât pentru `monitor_reports` cât și pentru agregatorul de scoruri.
* **Problematica fenomenului de "Block Buffering":** O descoperire tehnică notabilă a fost comportamentul bibliotecii C standard în lucrul cu pipe-uri. Când `monitor_reports` își detecta dublura, mesajul său de eroare nu ajungea la procesul părinte din cauza mecanismului de *block buffering* (activat automat când `stdout` nu mai este conectat la un terminal interactiv). Soluția documentată și implementată a fost forțarea scrierii datelor din buffer prin utilizarea apelului `fflush(stdout)`.
* **Arhitectura "Double-Fork" pentru procese daemonizate:** La implementarea funcției `start_monitor`, asistentul a generat o structură extrem de elegantă de tip dublu-fork. [cite_start]Procesul `city_hub` generează un prim copil (`hub_mon`)[cite: 125], care la rândul său generează procesul pentru monitor. Această separare a permis ca programul principal să nu rămână blocat așteptând ieșirea din pipe a monitorului, delegând bucla de citire a conductei primului copil, care rulează silențios în background. Aceasta a asigurat menținerea latenței zero în bucla interactivă REPL a utilizatorului.