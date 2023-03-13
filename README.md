<h1>To test:</h1>
<p>Go to directory player1:</p> 

    c process: gcc peer.c -o peer
    
    python process: python3 game.py


<p>Go to directory player2:</p>

    c process: gcc peer.c -o peer
    
    python process: python3 game.py



<h1>There are two process for each player: C process and Python process.</h1>
<br>
<h3>In C process there are two threads:</h3>
<br>
 -> one is for reading message from peer and then send to python process
 <br>
 -> one is for reading message from python process and then send to peer
<br>
<h3>In Python process there are two threads:</h3>
<br>
 -> one is for reading message from C process and set target for the dot
 <br>
 -> one is for run the game, get the current coordination of dot and then send to C process
