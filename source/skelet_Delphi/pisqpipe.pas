{ functions that communicate with manager through pipes }
{ modifications}
{  8.12.2008 - accessing undefined memory - marked ###F20081208 - Fontan}
unit pisqpipe;
interface
uses example; {change this to the name of your unit}

var
{ information about a game - you should use these variables }
width:integer=0;{ the board width }
height:integer; { the board height }
info_timeout_turn:integer=30000;  { time for one turn in milliseconds }
info_timeout_match:integer=1000000000; { total time for a game }
info_time_left:integer=1000000000;  { left time for a game }
info_max_memory:integer=0; { maximum memory in bytes, zero if unlimited }
info_game_type:integer=1;  { 0:human opponent, 1:AI opponent, 2:tournament, 3:network tournament }
info_exact5:integer=0; { 0:five or more stones win, 1:exactly five stones win }
info_renju:integer=0; { 0:Gomoku, 1:renju }
info_continuous:integer=0; { 0:single game, 1:continuous }
terminate:integer; { you must return from brain_turn when terminate>0 }
start_time:longword; { tick count at the beginning of turn }
dataFolder:string; { folder for persistent files }

{ these functions are implemented in this file }
procedure pipeOut(s:string);overload;
procedure pipeOut(fmt:string; args:array of const);overload;
procedure do_mymove(x,y:integer);
procedure suggest(x,y:integer);

implementation
uses windows,sysutils;
var cmd:string;
    tid:DWORD;
    event1,event2:THANDLE;

{* write a line to the pipe }
procedure pipeOut(s:string);overload;
begin
  writeln(s);
  Flush(output);
end;

procedure pipeOut(fmt:string; args:array of const);overload;
begin
  pipeOut(Format(fmt,args));
end;

{* parse two integers }
function parse_xy(param:string; var x,y:integer): boolean;
var i,ex,ey:integer;
begin
  i:=pos(',',param);
  val(copy(param,1,i-1),x,ex);
  val(copy(param,i+1,255),y,ey);
  if (i=0)or(ex<>0)or(ey<>0)or(x<0)or(y<0) then result:=false
  else result:=true;
end;

{* parse coordinates x,y }
function parse_coord(param:string; var x,y:integer): boolean;
begin
  result:=parse_xy(param,x,y);
  if (x>=width)or(y>=height) then result:=false
end;

{* parse coordinates x,y and player number z }
procedure parse_3int_chk(param:string; var x,y,z:integer);
var i,ex:integer;
    eyz:boolean;
begin
  i:=pos(',',param);
  val(copy(param,1,i-1),x,ex);
  eyz:=parse_xy(copy(param,i+1,255),y,z);
  if (i=0)or(ex<>0)or not eyz or(x<0)or(y<0)or(x>=width)or(y>=height) then z:=0;
end;

{* get word after cmd if input starts with cmd, otherwise return false }
function get_cmd_param(cmd,input:string; var param:string):boolean;
var n1,n2:integer;
begin
  n1:=length(cmd);
  n2:=length(input);
  if (n1>n2)or(StrLIComp(PChar(cmd),PChar(input),n1)<>0) then begin
    { it is not cmd }
    result:=false;
    exit;
  end;
  result:=true;
  repeat
    inc(n1);
    if n1>length(input) then begin {###F20081208}
      param:='';
      exit;
    end;
  until input[n1]<>' ';
  param:=copy(input,n1,255)
end;

{* send suggest }
procedure suggest(x,y:integer);
begin
  pipeOut('SUGGEST %d,%d',[x,y]);
end;

{* write move to the pipe and update internal data structures }
procedure do_mymove(x,y:integer);
begin
  brain_my(x,y);
  pipeOut('%d,%d',[x,y]);
end;

{* main function for the working thread }
function threadLoop(param:Pointer):DWORD;stdcall;
begin
  while true do begin
    WaitForSingleObject(event1,INFINITE);
    brain_turn();
    SetEvent(event2);
  end;
end;

{* start thinking }
procedure turn();
begin
  terminate:=0;
  ResetEvent(event2);
  SetEvent(event1);
end;

{* stop thinking }
procedure stop();
begin
  terminate:=1;
  WaitForSingleObject(event2,INFINITE);
end;

procedure start();
begin
  start_time:=GetTickCount();
  stop();
  if width=0 then begin
    width:=20;
    height:=20;
    brain_init();
  end;
end;

{* do command cmd }
procedure do_command();
var
  param,info,t:string;
  x,y,who,e:integer;
begin
  if get_cmd_param('info',cmd,param) then begin
    if get_cmd_param('max_memory',param,info) then info_max_memory:=StrToInt(info);
    if get_cmd_param('timeout_match',param,info) then info_timeout_match:=StrToInt(info);
    if get_cmd_param('timeout_turn',param,info) then info_timeout_turn:=StrToInt(info);
    if get_cmd_param('time_left',param,info) then info_time_left:=StrToInt(info);
    if get_cmd_param('game_type',param,info) then info_game_type:=StrToInt(info);
    if get_cmd_param('rule',param,info) then begin e:=StrToInt(info); info_exact5:=e and 1; info_continuous:=(e shr 1)and 1; info_renju:=(e shr 2)and 1; end;
    if get_cmd_param('folder',param,info) then dataFolder:=info;
{$IFDEF DEBUG_EVAL}
    if get_cmd_param('evaluate',param,info) then begin
      if parse_coord(info,x,y) then brain_eval(x,y);
    end;
{$ENDIF}
    { unknown info is ignored }
  end
  else if get_cmd_param('start',cmd,param) then begin
    width:= StrToInt(param);
    height:=width;
    start();
    brain_init();
  end
  else if get_cmd_param('rectstart',cmd,param) then begin
    if not parse_xy(param,width,height) then begin
      pipeOut('ERROR bad RECTSTART parameters');
    end else begin
      start();
      brain_init();
    end;
  end
  else if get_cmd_param('restart',cmd,param) then begin
    start();
    brain_restart();
  end
  else if get_cmd_param('turn',cmd,param) then begin
    start();
    if not parse_coord(param,x,y) then begin
      pipeOut('ERROR bad coordinates');
    end else begin
      brain_opponents(x,y);
      turn();
    end;
  end
  else if get_cmd_param('play',cmd,param) then begin
    start();
    if not parse_coord(param,x,y) then begin
      pipeOut('ERROR bad coordinates');
    end else begin
      do_mymove(x,y);
    end;
  end
  else if get_cmd_param('begin',cmd,param) then begin
    start();
    turn();
  end
  else if get_cmd_param('about',cmd,param) then begin
    pipeOut('%s',[infotext]);
  end
  else if get_cmd_param('end',cmd,param) then begin
    stop();
    brain_end();
    halt;
  end
  else if get_cmd_param('board',cmd,param) then begin
    start();
    while true do begin { fill the whole board }
      readln(cmd);
      parse_3int_chk(cmd,x,y,who);
      if who=1 then brain_my(x,y)
      else if who=2 then brain_opponents(x,y)
      else if who=3 then brain_block(x,y)
      else begin
        if StrIComp(PChar(cmd),'done')<>0 then pipeOut('ERROR x,y,who or DONE expected after BOARD');
        break;
      end;
    end;
    turn();
  end
  else if get_cmd_param('takeback',cmd,param) then begin
    start();
    t:='ERROR bad coordinates';
    if parse_coord(param,x,y) then begin
      e:= brain_takeback(x,y);
      if e=0 then t:='OK'
      else if e=1 then t:='UNKNOWN';
    end;
    pipeOut(t);
  end
  else begin
    pipeOut('UNKNOWN command');
  end;
end;


{* main function for AI console application  }
initialization
begin
  event1:=CreateEvent(nil,FALSE,FALSE,nil);
  CreateThread(nil,0,@threadLoop,nil,0,tid);
  event2:=CreateEvent(nil,TRUE,TRUE,nil);

  while true do begin
    readln(cmd);
    do_command();
  end;
end;
end.

