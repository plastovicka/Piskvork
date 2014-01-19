How to compile
--------------
Create two C++ projects. The first project is Win32 Application and it contains files from the folder "source". You have to add library modules wsock32.lib mswsock.lib version.lib comctl32.lib. The output file is piskvork.exe which is a user interface and a tournament manager. The second project is Win32 Console Application and it contains files from the folder "source\pbrain" and a file pisqpipe.cpp from the folder "source\skelet". Rename the output file to pbrain-pela.exe. You have to select Multithreaded DLL or Multithreaded run-time library in both projects' options. 

Algorithm
---------
The algorithm is based on a function evaluate. It calculates evaluation for all board squares. Computer regards only those squares which have high evaluation. It is calculated for both players and stored in members h[0] and h[1] in struct Tsquare. Evaluation is calculated for all directions (vertical, forward diagonal, horizontal, backward diagonal) and stored in members ps[0], ps[1], ps[2], ps[3] in struct Tprior. The sum of these four directions is stored in a member pv in struct Tprior. 
When a game begins, 1MB array K is created which contains evaluation for all possible patterns within a line. The evaluation depends on a count of own or opponent's symbols within a line. Each item in the array corresponds to a pattern of 9 squares. 
Squares which have high evaluation are placed in linked lists. The list goodMoves[1] contains squares with evaluation between 158 and 315. The list goodMoves[2] contains squares with evaluation between 316 and 2045. The list goodMoves[3] contains squares with evaluation higher then 2046. If all these lists are empty, then all the board is searched to find a suitable square.
At first, the computer searches moves that make four symbols in a line. Depth of this search is unlimited. The opponent has only one possiblitity to defend if the attacker has four symbols. That's why there are not too many combinations. 
Most of the thinking time is spent in a function alfabeta that does depth search and backtracking. The depth depends on a time for a turn. It begins at depth 6 and then repeats the search for larger depths until time runs out. The search can be finished sooner in some special situations - if someone wins in a few moves or if all lines are occupied by both players and nobody has chance to attack. Unfortunately, time of a search grows exponentially with the depth. In order to decrease number of variations, distances between two consecutive moves must be less then 7. Moreover, two successive attacker's moves must be on one line. 
Offensive and defensive moves are search independently. 
If the depth is not large enough to find winning moves, the computer moves on a square which has the highest evaluation or almost highest. There is a little randomness so that a game is not the same each time.

How to create your own AI
-------------------------
Use files pisqpipe.cpp and pisqpipe.h that contain all neccessary communication between your AI and the game manager. Then implement functions brain_init, brain_restart, brain_turn, brain_my, brain_opponents, brain_block, brain_takeback, brain_end. Your EXE file must have a prefix pbrain-.
The second possibility is to use obsolete file protocol which communicates through files Tah.dat, Plocha.dat, TimeOuts.dat, Info.dat, msg.dat.
Visit http://gomocup.wz.cz where you will find more information about the protocol and tournament rules.
