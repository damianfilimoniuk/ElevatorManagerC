import sys
import json
import threading
import queue
import pygame
import time

# --- KONFIGURACJA SLOW MO ---
SLOW_MO_DELAY = 1.0 

data_queue = queue.Queue()

def print_classic_log(state):
    """Odbudowuje klasyczny log tekstowy i pluje nim w terminal"""
    active_floor = state["active_floor"]
    if active_floor < len(state["waiting"]):
        waiting = state["waiting"][active_floor]
    else:
        waiting = 0
        
    elevators_str = []
    for el in state["elevators"]:
        if el["dir"] == 1: d = '^'
        elif el["dir"] == -1: d = 'v'
        else: d = 'S'
        elevators_str.append(f"W{el['id']}: [{el['passengers']}/{el['capacity']}] {d}")
        
    el_joined = " | ".join(elevators_str)
    print(f"P{active_floor} [S:{waiting}] | {el_joined}  <-- {state['msg']}")

def read_stdin():
    """Wątek działający w tle: czyta logi z C i robi Slow Mo"""
    for line in sys.stdin:
        try:
            state = json.loads(line.strip())
            data_queue.put(state)
            print_classic_log(state)
            time.sleep(SLOW_MO_DELAY)
        except json.JSONDecodeError:
            pass

# Odpalenie czytacza
threading.Thread(target=read_stdin, daemon=True).start()

# --- INICJALIZACJA PYGAME ---
pygame.init()
WIDTH, HEIGHT = 800, 600
screen = pygame.display.set_mode((WIDTH, HEIGHT))
pygame.display.set_caption("Symulator Wind - Akademik Pico Bello")
clock = pygame.time.Clock()

# Nowoczesne, gładkie czcionki
title_font = pygame.font.SysFont("Segoe UI", 28, bold=True)
font = pygame.font.SysFont("Segoe UI", 16, bold=True)
small_font = pygame.font.SysFont("Segoe UI", 14, bold=True)

# --- PALETA KOLORÓW (Modern Dark Theme) ---
BG_COLOR = (30, 30, 46)          # Tło aplikacji (głęboki granat/szary)
SHAFT_COLOR = (49, 50, 68)       # Tło szybów windowych
FLOOR_COLOR = (88, 91, 112)      # Linie pięter
TEXT_COLOR = (205, 214, 244)     # Zwykły tekst
COLOR_UP = (166, 227, 161)       # Winda jedzie do góry (Pastelowa Zieleń)
COLOR_DOWN = (243, 139, 168)     # Winda jedzie w dół (Pastelowa Czerwień)
COLOR_STOP = (137, 180, 250)     # Winda stoi (Pastelowy Niebieski)

current_state = None

running = True
while running:
    # 1. Obsługa zdarzeń
    for event in pygame.event.get():
        if event.type == pygame.QUIT:
            running = False

    # 2. Pobierz najnowszy stan (zgarnij tylko najświeższą klatkę)
    while not data_queue.empty():
        current_state = data_queue.get()

    # 3. RYSOWANIE
    screen.fill(BG_COLOR)

    # Elegancki tytuł na samej górze
    title_surface = title_font.render("Akademik Pico Bello", True, TEXT_COLOR)
    screen.blit(title_surface, (WIDTH // 2 - title_surface.get_width() // 2, 15))

    if current_state:
        num_floors = len(current_state["waiting"])
        elevators = current_state["elevators"]
        num_elevators = len(elevators)

        # Dynamiczne obliczanie wymiarów
        floor_height = (HEIGHT - 100) // max(1, num_floors)
        start_y = HEIGHT - 30

        # Najpierw rysujemy szyby windowe (będą za windami)
        for idx in range(num_elevators):
            el_x = 200 + (idx * 130)
            shaft_rect = (el_x - 10, 70, 80, HEIGHT - 100)
            pygame.draw.rect(screen, SHAFT_COLOR, shaft_rect, border_radius=5)
        
        # Rysowanie pięter
        for i in range(num_floors):
            y = start_y - (i * floor_height)
            
            # Gruba, ładna linia stropu
            pygame.draw.line(screen, FLOOR_COLOR, (40, y), (WIDTH - 40, y), 4)
            
            waiting = current_state["waiting"][i]
            
            # Wypisywanie numeru piętra po lewej
            floor_text = font.render(f"Piętro {i}", True, TEXT_COLOR)
            screen.blit(floor_text, (50, y - floor_height // 2 - 10))
            
            # Nowoczesny "dymek" z liczbą oczekujących studentów (jeśli ktoś czeka)
            if waiting > 0:
                badge_rect = pygame.Rect(125, y - floor_height // 2 - 10, 30, 20)
                pygame.draw.rect(screen, COLOR_DOWN, badge_rect, border_radius=10)
                q_text = small_font.render(str(waiting), True, (30, 30, 46))
                screen.blit(q_text, (125 + 15 - q_text.get_width()//2, y - floor_height // 2 - 9))

        # Rysowanie samych wind (kabin)
        for idx, el in enumerate(elevators):
            el_x = 200 + (idx * 130)
            el_y = start_y - (el["floor"] * floor_height) - 45 # Wysokość samej kabiny
            
            if el["dir"] == 1: color = COLOR_UP
            elif el["dir"] == -1: color = COLOR_DOWN
            else: color = COLOR_STOP
            
            # Główna kabina
            pygame.draw.rect(screen, color, (el_x, el_y, 60, 45), border_radius=8)
            
            # Subtelny pionowy pasek udający zamknięte "drzwi" windy
            pygame.draw.rect(screen, BG_COLOR, (el_x + 28, el_y + 5, 4, 35))
            
            # Liczba pasażerów unosząca się delikatnie nad windą
            pass_text = font.render(f"{el['passengers']}/{el['capacity']}", True, color)
            screen.blit(pass_text, (el_x + 30 - pass_text.get_width() // 2, el_y - 25))
            
            # Oznaczenie "W1", "W2" na samym dole szybu
            name_text = font.render(f"W{el['id']}", True, TEXT_COLOR)
            screen.blit(name_text, (el_x + 30 - name_text.get_width() // 2, start_y + 10))

    pygame.display.flip()
    clock.tick(30)

pygame.quit()
