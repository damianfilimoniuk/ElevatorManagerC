# Symulator Wind - Akademik "Pico Bello" 🏢

Projekt zaliczeniowy z przedmiotu Systemy Operacyjne demonstrujący rozwiązanie klasycznego problemu synchronizacji wielu wątków współdzielących ograniczony zasób (windy). System zarządza ruchem pasażerskim w wielopiętrowym akademiku, zapobiegając zakleszczeniom (deadlocks) i zagłodzeniu wątków (starvation).

## Cechy projektu

* **Dwa warianty synchronizacji**: Projekt zawiera niezależne implementacje oparte na zmiennych warunkowych (`pthread_cond_t`) oraz semaforach POSIX (`sem_t`).
* **Inteligentny algorytm planowania (SSTF/LOOK)**: Windy nie poruszają się w sposób naiwny. Puste kabiny skanują budynek w poszukiwaniu najbliższego zgłoszenia, minimalizując czas oczekiwania, a w przypadku braku wezwań przechodzą w stan uśpienia.
* **Graficzna wizualizacja**: Dołączony skrypt w języku Python czyta logi generowane przez program w C (w formacie JSON) przez standardowe wejście (pipe) i renderuje płynną animację w czasie rzeczywistym, pozwalając na analizę "Slow Mo".

## Wymagania systemowe

* System operacyjny z rodziny Linux / macOS
* Kompilator `gcc` oraz program `make`
* Python 3.x
* Biblioteka Pygame (do instalacji: `pip install pygame` lub `sudo apt install python3-pygame`)

## Struktura plików

* `winda_utils.h` – Plik nagłówkowy zawierający definicje struktur (winda, system, kierunek) oraz deklaracje globalne.
* `winda_utils.c` – Moduł generujący i wysyłający logi stanu systemu w formacie JSON.
* `winda_cond.c` – Kod źródłowy wariantu 1 (zmienne warunkowe i muteksy).
* `winda_sem.c` – Kod źródłowy wariantu 2 (semafory nienazwane i muteksy).
* `wizualizacja.py` – Aplikacja graficzna GUI oparta na bibliotece Pygame.
* `Makefile` – Skrypt automatyzujący proces budowania projektu.

## Kompilacja

Aby skompilować oba warianty programu, otwórz terminal w folderze z projektem i wpisz:

    make

Polecenie to wygeneruje dwa pliki wykonywalne: `winda_cond` oraz `winda_sem`.
Aby wyczyścić pliki binarne, użyj polecenia `make clean`.

## Uruchamianie i Parametry

Program przyjmuje dokładnie 4 parametry wejściowe:
1. **Liczba wind** w budynku
2. **Liczba pięter** (wliczając parter jako piętro 0)
3. **Liczba studentów** (wątków) biorących udział w symulacji
4. **Pojemność windy** (maksymalna liczba pasażerów w jednej kabinie)

### Uruchomienie samej symulacji (tryb tekstowy surowy)

    ./winda_cond 2 5 15 3

### Uruchomienie z wizualizacją graficzną i logami (Zalecane)
Wykorzystujemy mechanizm potoku (pipe), aby przekazać dane z C do Pythona:

    ./winda_cond 2 5 15 3 | python3 wizualizacja.py

*(Zamiast `winda_cond` możesz w ten sam sposób uruchomić program `winda_sem`)*.

W oknie terminala pojawią się czytelne, klasyczne logi systemowe, natomiast w oddzielnym oknie uruchomi się animacja graficzna z opóźnieniem "Slow Mo" (domyślnie 1 sekunda na zdarzenie). Szybkość symulacji można modyfikować, zmieniając stałą `SLOW_MO_DELAY` w pliku `wizualizacja.py`.
