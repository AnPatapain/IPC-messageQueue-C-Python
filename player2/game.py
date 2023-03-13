import pygame as pg
import sysv_ipc
import sys
import threading
import os
import re

# Constants
MAX = 100
KEY = 192001
COOR_MESSAGE_TYPE = 1

FROM_PY_TO_C = 2
FROM_C_TO_PY = 3

class ReadWrite:

    def __init__(self):
        self.message_queue = sysv_ipc.MessageQueue(KEY, sysv_ipc.IPC_CREAT)
        self.message = None
        print(self.message_queue)

    def send_message(self, message):
        self.message_queue.send(message.encode(), type=FROM_PY_TO_C)


    def read_message(self):
        self.message = self.message_queue.receive(type=FROM_C_TO_PY)
        self.message = self.decode_and_clean_message()
        

    def decode_and_clean_message(self):
        temp_list = list(self.message)
        temp_list[0] = self.message[0].decode(sys.getdefaultencoding(), errors='ignore')
        temp_list[0] = temp_list[0].split('\n')[0]
        temp_list[0] = temp_list[0].rstrip('\0')
        # print(temp_list[0])
        temp_list[1] = self.message[1]
        return tuple(temp_list)
        
    def get_coordinates(self):
        numbers = []
        pattern = r'\d+'
        for word in self.message[0].split():
            matches = re.findall(pattern, word)
            # if word.isdigit():
            if matches:
                print(word)
                numbers.append(int(float(word)))
        return numbers
    
    def get_message_text(self): return self.message[0]


class Character:

    def __init__(self, speed, color):
        self.image = pg.Surface((10, 10))
        self.image.set_colorkey((12,34,56))
        self.image.fill((12,34,56))
        pg.draw.circle(self.image, color, (50, 50), 5)
        self.rect = self.image.get_rect()
        self.color = color
        self.target_access = False

        self.pos = pg.Vector2(50, 50)
        self.set_target((50, 50))
        self.speed = speed

    def set_target(self, pos):
        self.target = pg.Vector2(pos)

    def get_target_access(self): return self.target_access

    def set_target_access(self, isAccess: bool): self.target_access = isAccess

    def draw(self,screen):
        screen.fill((0, 0, 0))
        pg.draw.circle(screen, self.color , (self.pos[0], self.pos[1]), 5)
        pg.display.update()

    def update(self):
        move = self.target - self.pos
        move_length = move.length()

        if move_length < self.speed:
            self.pos = self.target
            self.set_target_access(True)

        elif move_length != 0:
            move.normalize_ip()
            move = move * self.speed
            self.pos += move

        self.rect.topleft = list(int(v) for v in self.pos)

def run_game(character: Character, read_write: ReadWrite, screen, clock):
    while True:
        for event in pg.event.get():
            if event.type == pg.QUIT:
                return
            
            if event.type == pg.MOUSEBUTTONDOWN:
                character.set_target(pg.mouse.get_pos())
                tar = f"{character.target[0]} {character.target[1]}\n"
                print(tar)
                read_write.send_message(tar)

            if event.type==pg.KEYDOWN:
                if event.key == pg.K_ESCAPE:
                    pg.quit()
        character.update()
        screen.fill((20, 20, 20))
        character.draw(screen)
        pg.display.flip()
        clock.tick(60)

def run_read_write(read_write: ReadWrite, character: Character):
    quit = False
    
    while not quit:
        read_write.read_message()
        msg_text = read_write.get_message_text()
        if msg_text:
            print(msg_text)
        target = read_write.get_coordinates()
        print(target)
        if target:  
            print(target)
            character.set_target((target[0], target[1]))

def main():
    # Pygame configuration
    pg.init()
    screen = pg.display.set_mode((500, 500))
    clock = pg.time.Clock()

    # Create the agents for each thread
    # The character is for running the game and the reader is for running the reader
    character = Character(1.5, pg.Color("white"))
    read_write = ReadWrite()

    thread1 = threading.Thread(target=run_read_write, args=(read_write, character))
    thread2 = threading.Thread(target=run_game, args=(character, read_write, screen, clock))

    thread1.start()
    thread2.start()

    thread1.join()
    thread2.join()


if __name__ == '__main__':
    main()