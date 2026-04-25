# Documentare Utilizare AI - Faza 1

## 1. Tool-ul folosit
[cite_start]Pentru implementarea funcțiilor ajutătoare necesare comenzii `filter`, am folosit asistentul virtual **Gemini**[cite: 63].

## 2. Prompt-urile folosite
[cite_start]Am urmat instrucțiunile din specificațiile proiectului și am împărțit problema în două prompt-uri distincte[cite: 64, 67]:

**Prompt 1 (pentru parsare):**
> "Am de implementat o comandă de filtrare în C. Scrie o funcție cu semnătura `int parse_condition(const char *input, char *field, char *op, char *value);` care primește un string de forma `field:operator:value` și îl desparte în cele trei componente. Returnează 1 dacă a reușit și 0 în caz de eroare."

**Prompt 2 (pentru potrivire):**
> "Am următoarea structură de date în C: 
> `typedef struct { int id; char inspector_name[50]; double lat; double lon; char category[30]; int severity; time_t timestamp; char description[96]; } Report;`
> Generează o funcție `int match_condition(Report *r, const char *field, const char *op, const char *value);` care returnează 1 dacă o înregistrare respectă condiția dată și 0 altfel. Tipurile de date trebuie convertite corespunzător (ex: value e string, dar severity e int)."

## 3. Ce a generat AI-ul
* **Pentru prima funcție (`parse_condition`):** AI-ul a generat o soluție bazată pe funcția `strtok` din `<string.h>` pentru a sparge textul folosind delimitatorul `:`. De asemenea, a propus copierea token-urilor în pointerii de ieșire folosind `strcpy`.
* **Pentru a doua funcție (`match_condition`):** AI-ul a creat o serie de blocuri `if-else` cu `strcmp` pentru a identifica câmpul (`severity`, `category`, etc.). [cite_start]A generat corect conversia din string în integer cu `atoi()` pentru `severity` și a implementat verificările pentru operatorii relaționali (`==`, `!=`, `<`, `>`, etc.)[cite: 61, 96]. 

## 4. Ce am modificat și de ce (Evaluare critică)
[cite_start]Deși codul de bază a fost util, am fost nevoit să fac mai multe ajustări de siguranță și compatibilitate pentru a-l integra în proiectul meu[cite: 72, 98, 99]:

1. **Protejarea string-ului original (`parse_condition`):** AI-ul folosea `strtok` direct pe parametrul `input`. Deoarece `strtok` modifică string-ul original adăugând caractere `\0`, iar `input` poate proveni direct din `argv` (care nu ar trebui corupt), am adăugat un buffer temporar `temp` în care copiez `input`-ul cu `strncpy` înainte de tokenizare.
2. **Prevenirea Buffer Overflow:** Am înlocuit `strcpy`-urile generate de AI pentru copierea rezultatelor cu `strncpy`, limitând dimensiunea la cât suportă variabilele din `main` (ex: 49 caractere pentru `field`, 9 pentru `op`), plus adăugarea manuală a terminatorului `\0`.
3. **Conversia de timp (`match_condition`):** Pentru câmpul `timestamp`, valoarea primită ca string a trebuit convertită în tipul `time_t`. [cite_start]AI-ul nu a fost complet precis aici, așa că am adaptat codul folosind `(time_t)atol(value)` pentru a asigura o conversie sigură pe sisteme pe 64 de biți[cite: 96, 97].

## 5. Ce am învățat
Acest exercițiu m-a ajutat să înțeleg limitările funcțiilor distructive din C (precum `strtok`), forțându-mă să folosesc un string temporar pentru a nu altera datele originale din argumentele programului. De asemenea, am consolidat conceptele de pointeri la caractere și importanța conversiei tipurilor de date la runtime (string $\rightarrow$ int/time_t) pentru operații logice de filtrare.