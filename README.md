# Symulator Wind – System Synchronizacji Wątków w Akademiku

Zaawansowany projekt z zakresu systemów operacyjnych, prezentujący wielowątkową symulację ruchu wind w wirtualnym akademiku. System demonstruje praktyczne zastosowanie mechanizmów synchronizacji wątków standardu POSIX oraz komunikację międzyprocesową z zewnętrznym modułem wizualizacyjnym napisanym w języku Python.

## 🎯 Cel projektu
Celem projektu jest zaprojektowanie i implementacja wielowątkowego systemu symulacji ruchu wind, z wykorzystaniem zaawansowanych mechanizmów synchronizacji procesów. 

Projekt stawia przed sobą następujące cele techniczne i edukacyjne:
* **Synchronizacja wielowątkowa:** Praktyczne opanowanie mechanizmów `mutex` do zapewnienia wzajemnego wykluczenia oraz zmiennych warunkowych i semaforów POSIX do obsługi powiadomień pomiędzy wątkami studentów i wind.
* **Optymalizacja algorytmiczna:** Implementacja uproszczonego algorytmu sterowania windami , który pozwala na inteligentne podejmowanie decyzji o kierunku ruchu, minimalizując czas oczekiwania.
* **Rozwiązywanie problemów współbieżności:** Zapobieganie zjawiskom takim jak *race condition*, *deadlock* czy *ticket stealing*.
* **Wizualizacja czasu rzeczywistego:** Wykorzystanie potokó do przesyłania ustrukturyzowanego stanu systemu z warstwy logiki do warstwy prezentacji.

## 🏗️ Struktura Projektu

Projekt został podzielony na dwie niezależne warstwy: obliczeniową oraz prezentacyjną.

### Pliki źródłowe:
* `winda_zmienne.c` - Główny silnik symulacji wykorzystujący **zmienne warunkowe** (`pthread_cond_t`).
* `winda_semafory.c` - Alternatywny silnik symulacji wykorzystujący **semafory POSIX** (`sem_t`).
* `winda_utils.h` / `winda_utils.c` - Definicja struktur danych (stan budynku, parametry wind) oraz funkcja formatująca logi systemowe i strumień JSON.
* `wizualizacja.py` - Graficzny interfejs czasu rzeczywistego (Smooth Scrolling, Dark Mode).
* `Makefile` - Skrypt automatyzujący proces budowania i uruchamiania systemu.

## 🚀 Wymagania systemowe
* Kompilator GCC (ze wsparciem dla biblioteki `pthread`)
* Środowisko Linux / WSL / macOS
* Narzędzie `make`
* Python 3.x z zainstalowaną biblioteką Pygame (`pip install pygame`)

## 🛠️ Kompilacja i Uruchamianie

W projekcie wykorzystano plik `Makefile`, który maksymalnie upraszcza proces testowania. 
W terminalu, w głównym folderze projektu, dostępne są następujące komendy:

### 1. Budowanie projektu
```bash
make
```
Kompiluje jednocześnie obie wersje systemu (`winda_zmienne` oraz `winda_semafory`).

### 2. Szybkie uruchomienie z wizualizacją
Aby automatycznie skompilować system i od razu podpiąć go pod graficzną nakładkę w Pythonie (uruchomienie domyślne: 3 windy, 8 pięter, 4 osoby pojemności):

Wersja oparta na zmiennych warunkowych:
```bash
make run-zmienne
```

Wersja oparta na semaforach:
```bash
make run-semafory
```

### 3. Uruchamianie manualne (Tryb tekstowy / Terminal)
Jeżeli chcesz uruchomić program bez interfejsu graficznego, obserwując jedynie klasyczne logi tekstowe i surowy JSON, użyj polecenia:
```bash
./winda_zmienne <liczba_wind> <liczba_pieter> <pojemnosc_windy>
# Przykład: ./winda_zmienne 4 10 5
```

### 4. Sprzątanie projektu
Aby usunąć pliki binarne oraz pliki obiektowe `.o`:
```bash
make clean
```

## ⚙️ Działanie Algorytmu
Winda w stanie bezczynności (`DIR_STOP`) cyklicznie skanuje tablicę oczekujących studentów. Gdy wykryje zapotrzebowanie, oblicza odległość do każdego ze zgłoszeń, wybiera najbliższe piętro i rusza w jego stronę. Pasażer wsiadając nadaje windzie nowy wektor ruchu zależny od swojego celu. 

Dzięki zastosowaniu wywołań blokujących (`pthread_cond_wait` oraz `sem_wait`), system nie zużywa zasobów procesora (eliminacja zjawiska *busy-waiting*) podczas oczekiwania na zdarzenia.
