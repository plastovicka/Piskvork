How to compile this example
---------------------------
Just open file skelet.dpr in Delphi. The program uses some Windows functions so it cannot be compiled in Borland Pascal for DOS. After compilation, you have to rename the output file to pbrain-random.exe. The exe file will not work without a pbrain- prefix.

How to create your own AI
-------------------------
Replace file example.pas with your own algorithm. File pisqpipe.pas contains communication between your AI and the game manager and it is not recommended to modify that file, because it might be changed in future protocol versions.
More information about the protocol and tournament rules can be found at http://gomocup.wz.cz
