unit example;
{$INCLUDE header.pas}

implementation
uses pisqpipe,windows;

const MAX_BOARD=100;
var board:array[0..MAX_BOARD-1,0..MAX_BOARD-1] of integer;
    seed:cardinal;

procedure brain_init();
begin
  if (width<5)or(width>MAX_BOARD)or(height<5)or(height>MAX_BOARD) then begin
    pipeOut('ERROR size of the board');
    exit;
  end;
  seed:=start_time;
  pipeOut('OK');
end;

procedure brain_restart();
var x,y:integer;
begin
  for x:=0 to width-1 do begin
    for y:=0 to height-1 do begin
      board[x][y]:=0;
    end;
  end;
  pipeOut('OK');
end;

function isFree(x,y:integer):boolean;
begin
  result:=(x>=0)and(y>=0)and(x<width)and(y<height)and(board[x][y]=0);
end;

procedure brain_my(x,y:integer);
begin
  if isFree(x,y) then begin
    board[x][y]:=1;
  end else begin
    pipeOut('ERROR my move [%d,%d]',[x,y]);
  end;
end;

procedure brain_opponents(x,y:integer);
begin
  if isFree(x,y) then begin
    board[x][y]:=2;
  end else begin
    pipeOut('ERROR opponent''s move [%d,%d]',[x,y]);
  end;
end;

procedure brain_block(x,y:integer);
begin
  if isFree(x,y) then begin
    board[x][y]:=3;
  end else begin
    pipeOut('ERROR winning move [%d,%d]',[x,y]);
  end;
end;

function brain_takeback(x,y:integer):integer;
begin
  if (x>=0)and(y>=0)and(x<width)and(y<height)and(board[x][y]<>0) then begin
    board[x][y]:=0;
    result:=0;
  end else result:=2;
end;

function rnd(n:cardinal):cardinal;
begin
{$Q-}
  seed:=word(314159*seed+2718281);
  result:=(seed * n) shr 16;
end;

procedure brain_turn();
var x,y,i:integer;
begin
  i:=-1;
  repeat
    x:=rnd(width);
    y:=rnd(height);
    i:=i+1;
    if terminate>0 then exit;
  until isFree(x,y);

  if i>1 then pipeOut('DEBUG %d coordinates didn''t hit an empty field',[i]);
  do_mymove(x,y);
end;

procedure brain_end();
begin
end;

{$IFDEF DEBUG_EVAL}
procedure brain_eval(x,y:integer);
var dc:HDC;
    wnd:HWND;
    rc:TRECT;
    c:string;
begin
  wnd:=GetForegroundWindow();
  dc:= GetDC(wnd);
  GetClientRect(wnd,rc);
  str(board[x][y],c);
  TextOut(dc, rc.right-15, 3, PChar(c), 1);
  ReleaseDC(wnd,dc);
end;
{$ENDIF}

initialization
  infotext:='name="Random", author="Petr Lastovicka", version="3.1", country="Czech Republic", www="http://petr.lastovicka.sweb.cz"'
end.
