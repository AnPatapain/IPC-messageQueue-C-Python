To test:
Go to directory player1: 
    c process: gcc peer.c -o peer
    python process: python3 game.py

Go to directory player2: 
    c process: gcc peer.c -o peer
    python process: python3 game.py



There are two process for each player: C process and Python process. 
In C process there are two threads:
 -> one is for reading message from peer and then send to python process
 -> one is for reading message from python process and then send to peer

In Python process there are two threads:
 -> one is for reading message from C process and set target for the dot
 -> one is for run the game, get the current coordination of dot and then send to C process
