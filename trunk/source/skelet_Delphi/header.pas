interface
var infotext:string; { AI identification (copyright, version) }

{ you have to implement these functions }
procedure brain_init(); { create the board and call pipeOut('OK'); }
procedure brain_restart(); { delete old board, create new board, call pipeOut('OK'); }
procedure brain_turn(); { choose your move and call do_mymove(x,y);
                      0<=x<width, 0<=y<height }
procedure brain_my(x,y:integer); { put your move to the board }
procedure brain_opponents(x,y:integer); { put opponent's move to the board }
procedure brain_block(x,y:integer); { [x,y] belongs to a winning line (when info_continuous is 1) }
function brain_takeback(x,y:integer):integer; { clear a square; return value: 0:success, 1:not supported, 2:error }
procedure brain_end();  { delete temporary files, free resources }

{$IFDEF DEBUG_EVAL}
procedure brain_eval(x,y:integer); { display evaluation of square [x,y] }
{$ENDIF}


