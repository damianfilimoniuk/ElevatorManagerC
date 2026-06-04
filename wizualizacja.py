import sys
import json
import threading
import queue
import time
import pygame

SLOW_MO_DELAY = 0

BG_COLOR = (30, 30, 46)
SHAFT_COLOR = (49, 50, 68)
FLOOR_COLOR = (88, 91, 112)
TEXT_COLOR = (205, 214, 244)
COLOR_UP = (166, 227, 161)
COLOR_DOWN = (243, 139, 168)
COLOR_STOP = (137, 180, 250)

data_queue = queue.Queue()


def print_classic_log(state):
    af = state["active_floor"]
    elevators_str = []
    for el in state["elevators"]:
        d = "^" if el["dir"] == 1 else ("v" if el["dir"] == -1 else "S")
        elevators_str.append(
            f"W{el['id']}: [{el['passengers']}/{el['capacity']}] {d}"
        )
    print(f"P{af} | {' | '.join(elevators_str)}  <-- {state['msg']}")


def read_stdin():
    for line in sys.stdin:
        try:
            state = json.loads(line.strip())
            data_queue.put(state)
            print_classic_log(state)
            time.sleep(SLOW_MO_DELAY)
        except json.JSONDecodeError:
            pass


threading.Thread(target=read_stdin, daemon=True).start()

pygame.init()
WIDTH, HEIGHT = 1000, 700
screen = pygame.display.set_mode((WIDTH, HEIGHT))
pygame.display.set_caption("Akademik Pico Bello - Live View")
clock = pygame.time.Clock()

font_title = pygame.font.SysFont("Segoe UI", 32, bold=True)
font_bold = pygame.font.SysFont("Segoe UI", 16, bold=True)
font_small = pygame.font.SysFont("Segoe UI", 14, bold=True)


class VisualElevator:
    def __init__(self, e_id):
        self.id = e_id
        self.y = HEIGHT - 100
        self.target_y = HEIGHT - 100
        self.passengers = 0
        self.capacity = 1
        self.dir = 0


current_state = None
vis_elevators = {}
last_msg = "Oczekiwanie na start systemu..."

running = True
while running:
    for event in pygame.event.get():
        if event.type == pygame.QUIT:
            running = False

    while not data_queue.empty():
        current_state = data_queue.get()
        last_msg = current_state.get("msg", last_msg)

    screen.fill(BG_COLOR)

    screen.blit(
        font_title.render("Akademik Pico Bello", True, TEXT_COLOR), (30, 20)
    )
    status_color = (
        COLOR_UP
        if "wsiada" in last_msg
        else (COLOR_DOWN if "wysiada" in last_msg else TEXT_COLOR)
    )
    screen.blit(
        font_bold.render(f"Ostatnia akcja: {last_msg}", True, status_color),
        (30, 60),
    )

    if current_state:
        num_floors = len(current_state["waiting"])
        num_elevators = len(current_state["elevators"])

        floor_height = (HEIGHT - 150) // max(1, num_floors)
        start_y = HEIGHT - 50

        for e_data in current_state["elevators"]:
            eid = e_data["id"]
            if eid not in vis_elevators:
                vis_elevators[eid] = VisualElevator(eid)

            ve = vis_elevators[eid]
            ve.target_y = start_y - (e_data["floor"] * floor_height) - 50
            ve.passengers = e_data["passengers"]
            ve.capacity = e_data["capacity"]
            ve.dir = e_data["dir"]

        for i in range(num_floors):
            y = start_y - (i * floor_height)
            pygame.draw.line(screen, FLOOR_COLOR, (40, y), (WIDTH - 40, y), 3)

            screen.blit(
                font_bold.render(f"Piętro {i}", True, TEXT_COLOR), (50, y - 35)
            )

            waiting = current_state["waiting"][i]
            if waiting > 0:
                badge = pygame.Rect(120, y - 35, 30, 20)
                pygame.draw.rect(screen, COLOR_DOWN, badge, border_radius=10)
                q_text = font_small.render(str(waiting), True, BG_COLOR)
                screen.blit(
                    q_text, (120 + 15 - q_text.get_width() // 2, y - 34)
                )

        for idx, (eid, ve) in enumerate(sorted(vis_elevators.items())):
            el_x = 250 + (idx * 140)

            pygame.draw.rect(
                screen,
                SHAFT_COLOR,
                (el_x - 15, 100, 90, HEIGHT - 130),
                border_radius=10,
            )

            ve.y += (ve.target_y - ve.y) * 0.15

            color = (
                COLOR_UP
                if ve.dir == 1
                else (COLOR_DOWN if ve.dir == -1 else COLOR_STOP)
            )

            pygame.draw.rect(
                screen, color, (el_x, ve.y, 60, 50), border_radius=8
            )

            pygame.draw.rect(screen, BG_COLOR, (el_x + 28, ve.y + 5, 4, 40))
            pass_text = font_bold.render(
                f"{ve.passengers}/{ve.capacity}", True, color
            )
            screen.blit(
                pass_text, (el_x + 30 - pass_text.get_width() // 2, ve.y - 25)
            )

            name_text = font_bold.render(f"Winda {eid}", True, TEXT_COLOR)
            screen.blit(
                name_text,
                (el_x + 30 - name_text.get_width() // 2, start_y + 15),
            )

    pygame.display.flip()
    clock.tick(60)

pygame.quit()

