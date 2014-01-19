How to compile this example
---------------------------
You have to create a project. The project type is Win32 Console Application. Add files pisqpipe.cpp and example.cpp. Then select Multithreaded DLL or Multithreaded run-time library in the project's options. Set output file name to pbrain-random.exe in the project's options. The exe file will not work without a pbrain- prefix.

How to create your own AI
-------------------------
Replace file example.cpp with your own algorithm. Please don't change files pisqpipe.cpp and pisqpipe.h, because they contain communication between your AI and the game manager and they might be changed in future protocol versions. 
More information about the protocol and tournament rules can be found at http://gomocup.wz.cz
