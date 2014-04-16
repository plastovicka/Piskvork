/*
	(C) 2000-2011  Petr Lastovicka

	This program is free software; you can redistribute it and/or
	modify it under the terms of the GNU General Public License.
	*/
//-----------------------------------------------------------------
#include "hdr.h"
#pragma hdrstop
#include <shlobj.h>
#include <time.h>
#include "piskvork.h"

#pragma comment(lib,"comctl32.lib")
#pragma comment(lib,"version.lib")
#pragma comment(lib,"wsock32.lib")
#pragma comment(lib,"winmm.lib")

//-----------------------------------------------------------------
/*
USERC("Piskvork.rc");
USEUNIT("nettur.cpp");
USEUNIT("protocol.cpp");
USEUNIT("game.cpp");
USEUNIT("lang.cpp");
USEUNIT("netgame.cpp");
*/
//-----------------------------------------------------------------

//toolbar buttons
const int Ntool=18;
const TBBUTTON tbb[]={
	{6, 101, TBSTATE_ENABLED, TBSTYLE_BUTTON, {0}, 0},
	{0, 106, TBSTATE_ENABLED, TBSTYLE_BUTTON, {0}, 0},
	{1, 107, TBSTATE_ENABLED, TBSTYLE_BUTTON, {0}, 0},
	{0, 0, 0, TBSTYLE_SEP, {0}, 0},
	{10, 104, TBSTATE_ENABLED, TBSTYLE_BUTTON, {0}, 0},
	{12, 406, TBSTATE_ENABLED, TBSTYLE_BUTTON, {0}, 0},
	{14, 219, TBSTATE_ENABLED, TBSTYLE_BUTTON, {0}, 0},
	{13, 221, TBSTATE_ENABLED, TBSTYLE_BUTTON, {0}, 0},
	{0, 0, 0, TBSTYLE_SEP, {0}, 0},
	{7, 205, TBSTATE_ENABLED, TBSTYLE_BUTTON, {0}, 0},
	{3, 210, TBSTATE_ENABLED, TBSTYLE_BUTTON, {0}, 0},
	{5, 201, TBSTATE_ENABLED, TBSTYLE_BUTTON, {0}, 0},
	{4, 202, TBSTATE_ENABLED, TBSTYLE_BUTTON, {0}, 0},
	{2, 211, TBSTATE_ENABLED, TBSTYLE_BUTTON, {0}, 0},
	{11, 208, TBSTATE_ENABLED, TBSTYLE_BUTTON, {0}, 0},
	{0, 0, 0, TBSTYLE_SEP, {0}, 0},
	{8, 301, TBSTATE_ENABLED, TBSTYLE_BUTTON, {0}, 0},
	{9, 302, TBSTATE_ENABLED, TBSTYLE_BUTTON, {0}, 0},
	{21, 303, TBSTATE_ENABLED, TBSTYLE_BUTTON, {0}, 0},
	{18, 304, TBSTATE_ENABLED, TBSTYLE_BUTTON, {0}, 0},
	{15, 204, TBSTATE_ENABLED, TBSTYLE_BUTTON, {0}, 0},
	{22, 209, TBSTATE_ENABLED, TBSTYLE_BUTTON, {0}, 0},
	{16, 108, TBSTATE_ENABLED, TBSTYLE_BUTTON, {0}, 0},
	{20, 206, TBSTATE_ENABLED, TBSTYLE_BUTTON, {0}, 0},
	{17, 223, TBSTATE_ENABLED, TBSTYLE_BUTTON, {0}, 0},
	{19, 222, TBSTATE_ENABLED, TBSTYLE_BUTTON, {0}, 0},
};
char *toolNames[]={
	"New game", "Open position", "Save position", "",
	"Options", "Players settings", "Tournament", "Log window", "",
	"Undo all", "Undo 2x", "Undo", "Redo", "Redo 2x",
	"Redo all", "", "Open skin", "Next skin", "Previous skin", "Refresh",
	"Reset score", "Continue", "Pause", "Swap players",
	"Connect", "Disconnect",
};


char *priorTab[]={"idle", "below normal", "normal", "above normal", "high"};

Psquare
board=0,
 boardb, boardk,//pointer to the beginning and end of board
 lastMove;      //pointer to last move

Tplayer players[2];     //information about players
TturPlayer *turTable=0; //result of a tournament
TturCell *turCells=0;

int
width=20,  //number of squares horizontally (without borders)
 height=20, //number of squares vertically (without borders)
 height2,   //height+2
 player=1,  //who has turn
 moves,     //moves counter
 startMoves,//number of automatic opening moves
 widthc,    //width of the client area of the window
 widthb,    //right side of the board
 heightb,   //bottom side of the board
 coordW,    //width of coorninates at the left side
 coordH,    //height of coorninates at the top side
 mtop,      //toolBarH or 0
 toolBarVisible=1,//1=show toolbar
 maxMemory=80,   //maximum memory for AI (MB)
 tolerance=1000, //how longer can AI think after timeout
 hardTimeOut=0,  //should we check timeout for AI
 humanTimeOut=0, //should we check timeout for human
 exactFive=0,    //0= five or more stones win, 1=exactly 5 win
 continuous=0,   //0= until someone wins, 1=until board is full
 priority=2,     //AI process priority
 autoBegin=0,    //automatic openings
 opening,        //index of the current opening
 turOpening,     //the current tournament opening
 turRule=0,      //tournament mode
 turRepeat=5,    //tournament repeat count
 turCurRepeat,   //tournament repeat counter
 turMatchRepeat=2,//number of games per one repeat cycle
 turNplayers=0,  //number of tournament players
 turRecord=0,    //save games
 turOnlyLosses=0,//save only games which the first player lost
 turFormat=1,    //1=psq,2=rec
 turNet=0,       //0=local computer, 1=network
 turTieRepeat=1, //maximal repeat count when it's a tie
 turTieCounter,  //current number of sequential ties
 whoConnect=0,   //0=tournament, 1=network game
 logMessage=1,   //write message commands to the log window
 logDebug=0,     //write debug commands to the log window
 logMoves=1,     //write moves to the log window
 infoEval=0,     //send INFO evaluate x,y when mouse moves
 turLogMsg=1,    //write messages to messages.txt
 suspendAI=1,    //1=suspend pbrains when it isn't their turn
 debugAI=0,      //debug pbrains (when suspendAI==1)
 statusY,        //status bar height
 fontH,          //character height
 invert=0,       //swap symbols
 bmW,            //size of a square in pixels
 sameTime=0,     //both players have the same timeGame and timeMove 
 coordVisible=0, //show coordinates
 mx, my,          //mouse coordinates
 coordStart=0,   //coordinates origin, 0 or 1
 moveStart=0,    //move number start, 0 or 1
 moveDiv2=0,     //move number divide by two
 flipHoriz, flipVert, flipDiag, //board inversion
 hiliteDelay=600,//last move hilite duration 
 clearLog=1,     //clear log window at the beginning of a game
 turDelay=1500,  //pause between games in tournament
 turGamesTotal,  //total number of games in tournament
 turGamesCounter,//currently finished games in tournament
 gotoMove,       //number in goto move dialog box
 terminate,      //stopping AI
 saveLock,
 Naccel,
 lastTime[2],    //displayed game times (used to avoid flicker)
 lastTurnTime[2],//displayed turn times
 ignoreErrors=1, //don't show error messages from AI
 openingRandomShift1=1, //randomly rotate and transpose openings; single game
 openingRandomShiftT=0, //randomly rotate and transpose openings; tournament
 affinity=-1, //(change to version 8.4 by Tomas Kubes). -1 means that no affinity is set by user
 Nopening,
 includeUndoMoves=0, //if undo moves are included to the file
 soundNotification=1, //sound notification after computer move
 cmdlineGameOutFileFormat=1; //same as turFormat, but for commandLineGame


DWORD openingCRC; //(change to version 8.5 by Tomas Kubes). CRC of opening file in the tournament.

const int toolBarH=28; //toolbar height

DWORD lastTick;  //last move time (getTickCount())

char *title;     //window caption
char turPlayerStr[50000]; //tournament players EXE file names

Psquare
hilited=0;      //hilited square

bool
isWin9X,      //1=Win95/98/ME, 0=WinNT/2K/XP
 disableScore, //to avoid adding score more than once
 levelChanged, //to not show timeout when time limit is decreased
 turTimerAvail=false, //waiting between tournament games
 cmdLineGame,  //command line argument -p
 paused=true,  //game is paused
 finished=true,//game has finished
 delreg=false, //registry values were deleted
 autoBeginForce=false, //automatic tournament opening even if autoBegin = 0
 tmpPsq=false; //generate tmp_....psq file in net tournament on the client (from version 8.4 by Tomas Kubes)


TfileName fnrec, fnskin, fnbrain, fnstate, TahDat, PlochaDat, TimeOutsDat, MsgDat, InfoDat, cmdlineGameOutFile, cmdlineLogMsgOutFile, cmdlineLogPipeOutFile;
TdirName fntur, tempDir, dataDir;
Txywh mainPos={50, 15, 0, 0}, logPos={500, 40, 0, 0}, msgPos={500, -1, 0, 0}, resultPos;

HBITMAP bm;
HDC dc, bmpdc;
HWND hWin, logWnd, logDlg, resultDlg, msgWnd, msgDlg, toolbar;
HINSTANCE inst;
HACCEL haccel;
ACCEL accel[Maccel];
ACCEL dlgKey;
HANDLE thread, pipeThread;
HMODULE psapi;
COLORREF colors[3]; //background, text, winning line
CRITICAL_SECTION timerLock, drawLock;
SIZE szScore, szCoord, szMove, szName[2], szTimeGame[2], szTimeMove[2];

TmemInfo getMemInfo;

OPENFILENAME psqOfn={
	sizeof(OPENFILENAME), 0, 0, 0, 0, 0, 1,
	fnrec, sizeof(fnrec),
	0, 0, 0, 0, 0, 0, 0, "PSQ", 0, 0, 0
};
OPENFILENAME skinOfn={
	sizeof(OPENFILENAME), 0, 0, 0, 0, 0, 1,
	fnskin, sizeof(fnskin),
	0, 0, 0, 0, 0, 0, 0, "BMP", 0, 0, 0
};
OPENFILENAME brainOfn={
	sizeof(OPENFILENAME), 0, 0, 0, 0, 0, 1,
	fnbrain, sizeof(turPlayerStr),
	0, 0, 0, 0, 0, 0, 0, "EXE", 0, 0, 0
};
OPENFILENAME stateOfn={
	sizeof(OPENFILENAME), 0, 0, 0, 0, 0, 1,
	fnstate, sizeof(fnstate),
	0, 0, 0, 0, 0, 0, 0, "TUR", 0, 0, 0
};

const char
*subkey="Software\\Petr Lastovicka\\piskvorky";
struct Treg { char *s; int *i; } regVal[]={
	{"height", &height}, {"sirka", &width},
	{"hiliteDelay", &hiliteDelay},
	{"souradnice", &coordVisible},
	{"coordinatesOrigin", &coordStart},
	{"moveOrigin", &moveStart},
	{"X", &mainPos.x}, {"Y", &mainPos.y},
	{"hraci0", &players[0].isComp},
	{"timeMove0", &players[0].timeMove},
	{"timeMove1", &players[1].timeMove},
	{"timeGame0", &players[0].timeGame},
	{"timeGame1", &players[1].timeGame},
	{"sameTime", &sameTime},
	{"prohodit", &invert},
	{"maxMemory", &maxMemory},
	{"tolerance", &tolerance},
	{"checkTimeOut", &hardTimeOut},
	{"moveDiv2", &moveDiv2},
	{"humanTimeOut", &humanTimeOut},
	{"tournamentRule", &turRule},
	{"tournamentRepeat", &turRepeat},
	{"tournamentDelay", &turDelay},
	{"tournamentRec", &turRecord},
	{"tournamentRecFormat", &turFormat},
	{"saveMsg", &turLogMsg},
	{"onlyLosses", &turOnlyLosses},
	{"flipHoriz", &flipHoriz},
	{"flipVert", &flipVert},
	{"flipDiag", &flipDiag},
	{"saveFormat", (int*)&psqOfn.nFilterIndex},
	{"logMessage", &logMessage},
	{"logDebug", &logDebug},
	{"logMoves", &logMoves},
	{"Xlog", &logPos.x},
	{"Ylog", &logPos.y},
	{"Wlog", &logPos.w},
	{"Hlog", &logPos.h},
	{"Xresult", &resultPos.x},
	{"Yresult", &resultPos.y},
	{"Wresult", &resultPos.w},
	{"Hresult", &resultPos.h},
	{"Xmsg", &msgPos.x},
	{"Ymsg", &msgPos.y},
	{"Wmsg", &msgPos.w},
	{"Hmsg", &msgPos.h},
	{"debugAI", &suspendAI},
	{"debugAI2", &debugAI},
	{"autoBegin", &autoBegin},
	{"opening", &opening},
	{"toolbar", &toolBarVisible},
	{"port", &port},
	{"net", &turNet},
	{"whoConnect", &whoConnect},
	{"priority", &priority},
	{"tieRepeat", &turTieRepeat},
	{"matchRepeat", &turMatchRepeat},
	{"infoEvaluate", &infoEval},
	{"logPipe", &debugPipe},
	{"exactFive", &exactFive},
	{"continuous", &continuous},
	{"clearLog", &clearLog},
	{"ignoreErrors", &ignoreErrors},
	{"openingRandomShift1", &openingRandomShift1},
	{"openingRandomShiftT", &openingRandomShiftT},
	{"includeUndoMoves", &includeUndoMoves},
	{"soundNotification", &soundNotification},
};

struct Tregs { char *s; char *i; DWORD n; } regValS[]={
	{"soubor", fnrec, sizeof(fnrec)},
	{"skin", fnskin, sizeof(fnskin)},
	{"language", lang, sizeof(lang)},
	{"AI0", players[0].brain, sizeof(players[0].brain)},
	{"AI1", players[1].brain, sizeof(players[1].brain)},
	{"tournamentPlayers", turPlayerStr, sizeof(turPlayerStr)},
	{"tournamentFolder", fntur, sizeof(fntur)},
	{"servername", servername, sizeof(servername)},
	{"AIfolder", AIfolder, sizeof(AIfolder)},
	{"cmdGame", cmdGameEnd, sizeof(cmdGameEnd)},
	{"cmdTur", cmdTurEnd, sizeof(cmdTurEnd)},
	{"AIdataFolder", dataDir, sizeof(dataDir)},
};
//------------------------------------------------------------------
#define endA(a) (a+(sizeof(a)/sizeof(*a)))

void line(int x1, int y1, int x2, int y2)
{
	MoveToEx(dc, x1, y1, NULL);
	LineTo(dc, x2, y2);
}

void vprint(int x, int y, SIZE *sz, char *format, va_list va)
{
	char buf[128];
	RECT rc;
	int n;

	n = vsprintf(buf, format, va);
	
	EnterCriticalSection(&drawLock);
	SetTextAlign(dc, TA_CENTER);
	if(sz){
		rc.left= x-(sz->cx>>1);
		rc.right= x+(sz->cx>>1);
		rc.top= y;
		rc.bottom= y+sz->cy;
		if(RectVisible(dc, &rc) || !sz->cx){
			GetTextExtentPoint32(dc, buf, n, sz);
		}
	}
	ExtTextOut(dc, x, y, sz ? ETO_OPAQUE : 0, &rc, buf, n, 0);
	LeaveCriticalSection(&drawLock);
}

void print(int x, SIZE *sz, char *format, ...)
{
	va_list va;
	va_start(va, format);
	vprint(widthc*x/1000, heightb + (statusY/2-fontH)/2, sz, format, va);
	va_end(va);
}

void print2(int x, SIZE *sz, char *format, ...)
{
	va_list va;
	va_start(va, format);
	vprint(widthc*x/1000, heightb + (statusY*3/2-fontH)/2, sz, format, va);
	va_end(va);
}

void printMoves()
{
	print2(500, &szMove, lng(550, "Move %d"), moveStart + (moveDiv2 ? (moves-finished)/2 : moves));
}

void printScore()
{
	print(500, &szScore, lng(553, "Score %d:%d"),
		players[0].score, players[1].score);
}

void printMouseCoord()
{
	if(mx>=0 && my>=0 && mx<width && my<height && coordVisible){
		print2(650, &szCoord, " [%2d,%2d] ", mx+coordStart, my+coordStart);
	}
	else{
		print2(650, &szCoord, "");
	}
}

static int brainX[2]={220, 771};

void printLevel1()
{
	for(int i=0; i<2; i++){
		char buf[17];
		players[i].getName(buf, sizeof(buf));
		print(brainX[i], &szName[i], buf);
	}
}

void checkMenus()
{
	HMENU h=GetMenu(hWin);
	CheckMenuRadioItem(h, 401, 403, 401 + players[0].isComp + players[1].isComp, MF_BYCOMMAND);
	CheckMenuItem(h, 309, toolBarVisible ? MF_BYCOMMAND|MF_CHECKED : MF_BYCOMMAND|MF_UNCHECKED);
}

void printLevel()
{
	printLevel1();
	checkMenus();
}

int whoStarted()
{
	return player^(moves&1);
}

int vmsg(char *caption, char *text, int btn, va_list v)
{
	char buf[1024];
	if(!text) return IDCANCEL;
	_vsnprintf(buf, sizeof(buf), text, v);
	buf[sizeof(buf)-1]=0;
	return MessageBox(hWin, buf, caption, btn);
}

void msglng(int id, char *text, ...)
{
	va_list ap;
	va_start(ap, text);
	vmsg(title, lng(id, text), MB_OK|MB_ICONERROR, ap);
	va_end(ap);
}

int msg3(int btn, char *caption, char *text, ...)
{
	va_list ap;
	va_start(ap, text);
	int result = vmsg(caption, text, btn, ap);
	va_end(ap);
	return result;
}

void msg2(char *caption, char *text, ...)
{
	va_list ap;
	va_start(ap, text);
	vmsg(caption, text, MB_OK|MB_ICONERROR, ap);
	va_end(ap);
}

int msg1(int btn, char *text, ...)
{
	va_list ap;
	va_start(ap, text);
	int result = vmsg(title, text, btn, ap);
	va_end(ap);
	return result;
}

void msg(char *text, ...)
{
	va_list ap;
	va_start(ap, text);
	vmsg(title, text, MB_OK|MB_ICONERROR, ap);
	va_end(ap);
}

void vwrLog(char *text, va_list ap)
{
	char *buf;
	int len, n;

	buf=0;
	if(text){
		for(len=64;; len*=2){
			delete[] buf;
			buf=new char[len+3];
			n=_vsnprintf(buf, len, text, ap);
			if(n>=0) break;
		}
		buf[n]='\r';
		buf[n+1]='\n';
		buf[n+2]=0;
	}
	PostMessage(hWin, WM_COMMAND, 998, (LPARAM)buf);
}

void wrLog(char *text, ...)
{
	va_list ap;
	va_start(ap, text);
	vwrLog(text, ap);
	va_end(ap);
}

int getRadioButton(HWND hWnd, int item1, int item2)
{
	for(int i=item1; i<=item2; i++){
		if(IsDlgButtonChecked(hWnd, i)){
			return i-item1;
		}
	}
	return 0;
}

void browseFolder(char *buf, HWND wnd, int item, char *txt)
{
	BROWSEINFO bi;
	bi.hwndOwner= wnd;
	bi.pidlRoot=0;
	bi.pszDisplayName= buf;
	bi.lpszTitle= txt;
	bi.ulFlags=BIF_RETURNONLYFSDIRS;
	bi.lpfn=0;
	bi.iImage=0;
	ITEMIDLIST *iil;
	iil=SHBrowseForFolder(&bi);
	if(iil){
		if(SHGetPathFromIDList(iil, buf)){
			SetDlgItemText(wnd, item, buf);
		}
		IMalloc *ma;
		SHGetMalloc(&ma);
		ma->Free(iil);
		ma->Release();
	}
}

void delDir(char *path, bool d)
{
	HANDLE h;
	WIN32_FIND_DATA fd;
	char oldDir[256];

	GetCurrentDirectory(sizeof(oldDir), oldDir);
	if(!SetCurrentDirectory(path)) return;
	h = FindFirstFile("*", &fd);
	if(h!=INVALID_HANDLE_VALUE){
		do{
			if(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY){
				if(fd.cFileName[0]!='.' || (fd.cFileName[1] && fd.cFileName[1]!='.')){
					delDir(fd.cFileName, true);
				}
				continue;
			}
			DeleteFile(fd.cFileName);
		} while(FindNextFile(h, &fd));
		FindClose(h);
	}
	SetCurrentDirectory(oldDir);
	if(d) RemoveDirectory(path);
}

void cleanTemp()
{
	HANDLE h, p;
	DWORD i;
	TfileName buf;
	WIN32_FIND_DATA fd;

	//delete all temporary folders
	GetTempPath(sizeof(buf)-8, buf);
	if(!SetCurrentDirectory(buf)) return;
	h = FindFirstFile("psqp*", &fd);
	if(h!=INVALID_HANDLE_VALUE){
		do{
			if(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY){
				if(sscanf(fd.cFileName+4, "%d", &i)==1){
					p=OpenProcess(STANDARD_RIGHTS_REQUIRED, FALSE, i);
					if(p){
						CloseHandle(p);
					}
					else{
						delDir(fd.cFileName, true);
					}
				}
			}
		} while(FindNextFile(h, &fd));
		FindClose(h);
	}
}

int checkDir(HWND hDlg, int item)
{
	TdirName dir, buf;
	char *p;

	GetDlgItemText(hDlg, item, buf, sizeof(buf));
	if(!buf[0]) GetCurrentDirectory(sizeof(dir), dir);
	else GetFullPathName(buf, sizeof(dir), dir, &p);
	SetDlgItemText(hDlg, item, dir);
	DWORD attr= GetFileAttributes(dir);
	if(attr!=0xFFFFFFFF && (attr&FILE_ATTRIBUTE_DIRECTORY)) return 2;
	if(CreateDirectory(dir, 0)) return 1;
	msglng(816, "Cannot create folder %s", dir);
	SetFocus(GetDlgItem(hDlg, item));
	return 0;
}

int close(FILE *f, char *fn)
{
	if(!fclose(f)) return 0;
	if(msg1(MB_ICONEXCLAMATION|MB_RETRYCANCEL,
		lng(734, "Write error, file %s"), fn)
		==IDRETRY) return 2;
	return 1;
}

int getDlgItemMs(HWND hDlg, int item)
{
	char buf[32], *s, c1, c2;
	GetDlgItemText(hDlg, item, buf, sizeof(buf));

	c1=',', c2='.';
	if(strtod("1,5", 0)>1.4) c1='.', c2=',';
	for(s=buf; *s; s++){
		if(*s==c1) *s=c2;
	}
	return int(strtod(buf, 0)*1000);
}

void setDlgItemMs(HWND hDlg, int item, int ms)
{
	char buf[32];
	sprintf(buf, "%g", double(ms)/1000.0);
	SetDlgItemText(hDlg, item, buf);
}

enum { R_left=1, R_right=2, R_top=4, R_bottom=8 };

struct Tresize {
	int id;
	int flags;
};

void resize(Txywh &pos, const Tresize *data, int len, HWND hWnd, UINT mesg, LPARAM lP)
{
	int i, dw, dh;
	RECT rc;

	switch(mesg){
		case WM_INITDIALOG:
			SendMessage(hWnd, WM_SETICON, ICON_SMALL, (LPARAM)LoadIcon(inst, MAKEINTRESOURCE(1)));
			GetClientRect(hWnd, &rc);
			pos.oldW= rc.right-rc.left;
			pos.oldH= rc.bottom-rc.top;
			aminmax(pos.x, -20, GetSystemMetrics(SM_CXSCREEN)-100);
			aminmax(pos.y, -5, GetSystemMetrics(SM_CYSCREEN)-80);
			break;
		case WM_MOVE:
		case WM_EXITSIZEMOVE:
			if(!IsZoomed(hWnd) && !IsIconic(hWnd)){
				GetWindowRect(hWnd, &rc);
				pos.x= rc.left;
				pos.y= rc.top;
				pos.w= rc.right-rc.left;
				pos.h= rc.bottom-rc.top;
			}
			break;
		case WM_SIZE:
			if(!lP) break;
			if(pos.oldW){
				HDWP p=BeginDeferWindowPos(len);
				for(i=0; i<len; i++){
					const Tresize *d= data+i;
					HWND w= GetDlgItem(hWnd, d->id);
					GetWindowRect(w, &rc);
					MapWindowPoints(0, hWnd, (POINT*)&rc, 2);
					dw=LOWORD(lP)-pos.oldW;
					dh=HIWORD(lP)-pos.oldH;
					if(d->flags&R_left) rc.left+=dw;
					if(d->flags&R_right) rc.right+=dw;
					if(d->flags&R_top) rc.top+=dh;
					if(d->flags&R_bottom) rc.bottom+=dh;
					DeferWindowPos(p, w, 0, rc.left, rc.top,
						rc.right-rc.left, rc.bottom-rc.top, SWP_NOZORDER);
				}
				EndDeferWindowPos(p);
			}
			pos.oldW=LOWORD(lP);
			pos.oldH=HIWORD(lP);
			break;
		case WM_GETMINMAXINFO:
			((MINMAXINFO FAR*) lP)->ptMinTrackSize.x = 170;
			((MINMAXINFO FAR*) lP)->ptMinTrackSize.y = 150;
			break;
	}
}
//-----------------------------------------------------------------

Psquare Square(int x, int y)
{
	return boardb + x*height2+(y+1);
}

int X(Psquare p)
{
	int x;

	if(flipDiag){
		x=p->y;
		if(flipHoriz) x=height+1-x;
	}
	else{
		x=p->x;
		if(flipHoriz) x=width+1-x;
	}
	x= (x-1)*bmW;
	if(coordVisible) x+=coordW;
	return x;
}

int Y(Psquare p)
{
	int y;

	if(flipDiag){
		y=p->x;
		if(flipVert) y=width+1-y;
	}
	else{
		y=p->y;
		if(flipVert) y=height+1-y;
	}
	y= (y-1)*bmW;
	if(coordVisible) y+=coordH;
	if(toolBarVisible) y+=toolBarH;
	return y;
}

void getMousePos(WPARAM lP, int &x0, int &y0)
{
	int w, x, y;
	x=LOWORD(lP);
	y=HIWORD(lP);
	if(coordVisible) x-=coordW, y-=coordH;
	if(toolBarVisible) y-=toolBarH;
	x=x/bmW; y=y/bmW;
	if(flipHoriz) x=(flipDiag ? height : width)-1-x;
	if(flipVert) y=(flipDiag ? width : height)-1-y;
	if(flipDiag) w=x, x=y, y=w;
	x0=x; y0=y;
}

bool onAIName(int i, LPARAM lP)
{
	return HIWORD(lP)>heightb &&
		abs(LOWORD(lP)-widthc*brainX[i]/1000)<30;
}

//draw square at position p
//z: 0=empty, 1=first player, 2=second player, 3=outside
void paintSquare(int z, Psquare p)
{
	if(z>2) return;
	if(z){
		if(invert) z^=3;
		if(p==hilited) z+=2;
	}
	BitBlt(dc, X(p), Y(p), bmW, bmW, bmpdc, z*bmW, 0, SRCCOPY);
}

void paintSquare(Psquare p)
{
	paintSquare(p->z, p);
}
//-----------------------------------------------------------------
void cancelHilite()
{
	Psquare p=hilited;
	if(p){
		hilited=0;
		paintSquare(p);
		KillTimer(hWin, 10);
	}
}

//hilite last move
void hiliteLast()
{
	cancelHilite();
	if(moves && hiliteDelay && !lastMove->winLineDir){
		SetTimer(hWin, 10, hiliteDelay, NULL);
		paintSquare(hilited=lastMove);
	}
}

void showLast()
{
	int d=hiliteDelay;
	if(d<100) hiliteDelay=700;
	hiliteLast();
	hiliteDelay=d;
}
//-----------------------------------------------------------------
DWORD getTickCount()
{
	static LARGE_INTEGER freq;
	if(!freq.QuadPart){
		QueryPerformanceFrequency(&freq);
		if(!freq.QuadPart) return GetTickCount();
	}
	LARGE_INTEGER c;
	QueryPerformanceCounter(&c);
	return (DWORD)(c.QuadPart*1000/freq.QuadPart);
}

void printTime(int pl)
{
	int t, c, y, x0, x;
	HGDIOBJ oldP;
	HPEN pen0, pen1;
	int W;
	static const int X1[2]={60, 940}, X2[2]={45, 955};

	EnterCriticalSection(&drawLock);
	//prepare pens
	pen0= CreatePen(PS_SOLID, 0, colors[0]);
	pen1= CreatePen(PS_SOLID, 0, colors[1]);
	oldP= GetCurrentObject(dc, OBJ_PEN);
	W=widthc/9;
	x0=0;
	if(pl) x0=widthc-W;
	t=players[pl].time;
	//turn time
	if(pl==player && !paused && (moves>0 || players[pl].isComp)){
		c=getTickCount()-lastTick;
		t+=c;
		lastTurnTime[pl]=c;
		players[pl].checkTimeOut(c);
	}
	print2(X1[pl], &szTimeMove[pl], "%.1f",
		double(lastTurnTime[pl])/1000.0);
	if(players[pl].timeMove>400 && !isNetGame){
		//progress
		y= heightb+(statusY*3/2+fontH)/2+1;
		x= W-W*lastTurnTime[pl]/players[pl].timeMove;
		if(x>0){
			amax(x, W);
			SelectObject(dc, pen1);
			line(x0, y, x0+x, y);
		}
		else{
			x=0;
		}
		SelectObject(dc, pen0);
		if(x<W) line(x0+x, y, x0+W, y);
	}
	//game time
	t/=1000;
	if(t!=lastTime[pl]){
		lastTime[pl]=t;
		print(X2[pl], &szTimeGame[pl], "%d:%02d", t/60, t%60);
		if(players[pl].timeGame && !isNetGame){
			//progress
			y= heightb+(statusY/2+fontH)/2+1;
			x= W-W*t/players[pl].timeGame;
			if(x>0){
				amax(x, W);
				SelectObject(dc, pen1);
				line(x0, y, x0+x, y);
			}
			else{
				x=0;
			}
			SelectObject(dc, pen0);
			if(x<W) line(x0+x, y, x0+W, y);
		}
	}
	//delete pens
	SelectObject(dc, oldP);
	DeleteObject(pen1);
	DeleteObject(pen0);
	LeaveCriticalSection(&drawLock);
}

bool getWinLine(Psquare p, Psquare &p2)
{
	int s;
	Psquare p1;

	s=p->winLineDir;
	if(!s) return false;
	p1=p->winLineStart;
	do{
		nxtP(p, 1);
	} while(p->winLineStart==p1);
	prvP(p, 1);
	p2=p;
	return true;
}

void printWinLine(Psquare p)
{
	Psquare p2;
	RECT rc;
	LONG w;

	if(!getWinLine(p, p2)) return;
	rc.left=X(p->winLineStart);
	rc.top=Y(p->winLineStart);
	rc.right=X(p2);
	rc.bottom=Y(p2);
	if(rc.left>rc.right) w=rc.left, rc.left=rc.right, rc.right=w;
	if(rc.top>rc.bottom) w=rc.top, rc.top=rc.bottom, rc.bottom=w;
	rc.right+=bmW;
	rc.bottom+=bmW;
	InvalidateRect(hWin, &rc, FALSE);
}

void status()
{
	printMoves();
	printScore();
	printLevel();
	printTime(0);
	printTime(1);
	printMouseCoord();
	BitBlt(dc, widthc*brainX[0]/1000-bmW/2, heightb+statusY/2, bmW-1, bmW-1,
		bmpdc, (invert+1)*bmW+1, 1, SRCCOPY);
	BitBlt(dc, widthc*brainX[1]/1000-bmW/2, heightb+statusY/2, bmW-1, bmW-1,
		bmpdc, (2-invert)*bmW+1, 1, SRCCOPY);
	if(!turNplayers) print2(brainX[whoStarted()]+45, 0, "*");
}

//paint entire window
void repaint(RECT *clip)
{
	HGDIOBJ penOld;
	HBRUSH brush;
	Psquare p, p2=0;
	RECT rc;
	int i, d, j;
	char buf[4];

	brush= CreateSolidBrush(colors[0]);
	if(coordVisible){
		//left
		rc.left=0;
		rc.right=coordW;
		rc.top=mtop;
		rc.bottom= heightb;
		FillRect(dc, &rc, brush);
		//top
		rc.left= coordW;
		rc.right=widthb;
		rc.bottom= rc.top+coordH;
		FillRect(dc, &rc, brush);
	}
	//right border
	rc.left= widthb;
	rc.right=widthc;
	if(rc.left<rc.right){
		rc.top=mtop;
		rc.bottom= heightb;
		FillRect(dc, &rc, brush);
	}
	//status bar
	if(clip->bottom>heightb){
		rc.top=heightb;
		rc.bottom=rc.top+statusY;
		rc.left=0;
		FillRect(dc, &rc, brush);
		lastTime[0]=lastTime[1]=-1;
		status();
	}
	DeleteObject(brush);
	//coordinates
	if(coordVisible){
		//top
		if(clip->top < mtop+coordH){
			SetTextAlign(dc, TA_CENTER|TA_TOP);
			d= (flipDiag ? height : width)-1;
			for(i=d; i>=0; i--){
				j=i;
				if(flipHoriz) j=d-j;
				_itoa(j+coordStart, buf, 10);
				TextOut(dc, i*bmW+(bmW/2)+coordW, mtop, buf, (int)strlen(buf));
			}
		}
		//left
		if(clip->left < coordW){
			SetTextAlign(dc, TA_RIGHT|TA_BASELINE);
			d= (flipDiag ? width : height)-1;
			for(i=d; i>=0; i--){
				j=i;
				if(flipVert) j=d-j;
				_itoa(j+coordStart, buf, 10);
				TextOut(dc, coordW-4, (i+1)*bmW-5+coordH+mtop, buf, (int)strlen(buf));
			}
		}
	}
	//board
	for(p=boardb; p<boardk; p++)  paintSquare(p);
	//border
	d=0;
	if(coordVisible) d=coordH;
	if(toolBarVisible) d+=toolBarH;
	for(i=(flipDiag ? width : height)-1; i>=0; i--){
		BitBlt(dc, widthb-1, i*bmW+d, 1, bmW, bmpdc, 0, 0, SRCCOPY);
	}
	d= coordVisible ? coordW : 0;
	for(i=(flipDiag ? height : width)-1; i>=0; i--){
		BitBlt(dc, i*bmW+d, heightb-1, bmW, 1, bmpdc, 0, 0, SRCCOPY);
	}
	SetPixel(dc, widthb-1, heightb-1, GetPixel(bmpdc, 0, 0));
	//winning lines through five or more squares
	for(p=boardb; p<boardk; p++){
		if(p==p->winLineStart){
			getWinLine(p, p2);
			penOld= SelectObject(dc, CreatePen(PS_SOLID, 3, colors[2]));
			line(X(p) + bmW/2, Y(p) + bmW/2, X(p2) + bmW/2, Y(p2) + bmW/2);
			DeleteObject(SelectObject(dc, penOld));
		}
	}
}

void invalidate()
{
	int i, w, h;
	SIZE sz;
	RECT rc, rcw;

	fontH= HIWORD(GetDialogBaseUnits());
	statusY= max(bmW*2, fontH*2+4);
	heightb = height*bmW+1;
	widthb = width*bmW+1;
	coordH=fontH;
	coordW=fontH+8;
	if(flipDiag){
		w=heightb; heightb=widthb; widthb=w;
	}
	if(coordVisible) heightb+=coordH, widthb+=coordW;
	heightb+=mtop;
	widthc = max(widthb, 400);
	if(SendMessage(toolbar, TB_GETMAXSIZE, 0, (LPARAM)&sz)){
		amin(widthc, sz.cx);
	}
	if(!IsIconic(hWin)){
		for(i=0; i<2; i++){
			GetClientRect(hWin, &rc);
			GetWindowRect(hWin, &rcw);
			w= widthc + rcw.right-rcw.left-rc.right;
			h= heightb + statusY + rcw.bottom-rcw.top-rc.bottom;
			if(w!= rcw.right-rcw.left || h!= rcw.bottom-rcw.top){
				SetWindowPos(hWin, 0, 0, 0, w, h, SWP_NOMOVE|SWP_NOZORDER);
				SendMessage(toolbar, TB_AUTOSIZE, 0, 0);
			}
		}
		GetClientRect(hWin, &rc);
		rc.top=mtop;
		InvalidateRect(hWin, &rc, FALSE);
	}
}

//-----------------------------------------------------------------
void copyToClipboard(HWND wnd)
{
	HGLOBAL hmem;
	char *ptr;
	int i= GetWindowTextLength(wnd)+1;
	if(OpenClipboard(0)){
		if(EmptyClipboard()){
			if((hmem=GlobalAlloc(GMEM_DDESHARE, isWin9X ? i : 2*i))!=0){
				if((ptr=(char*)GlobalLock(hmem))!=0){
					if(isWin9X) GetWindowText(wnd, ptr, i);
					else ((int(WINAPI*)(HWND, char*, int))GetProcAddress(GetModuleHandle("user32.dll"), "GetWindowTextW"))(wnd, ptr, i);
					GlobalUnlock(hmem);
					SetClipboardData(isWin9X ? CF_TEXT : CF_UNICODETEXT, hmem);
				}
				else{
					GlobalFree(hmem);
				}
			}
		}
		CloseClipboard();
	}
}

void openPsq()
{
	for(;;){
		psqOfn.hwndOwner= hWin;
		psqOfn.Flags= OFN_FILEMUSTEXIST|OFN_HIDEREADONLY|OFN_READONLY|OFN_NOCHANGEDIR;
		if(GetOpenFileName(&psqOfn)){
			openPsq(psqOfn.lpstrFile);
			break; //ok
		}
		if(CommDlgExtendedError()!=FNERR_INVALIDFILENAME
			|| !psqOfn.lpstrFile[0]) break; //cancel
		psqOfn.lpstrFile[0]=0;
	}
}

void savePsq()
{
	if(!lastMove) return;
	saveLock++;
	for(;;){
		psqOfn.hwndOwner= hWin;
		psqOfn.Flags= OFN_HIDEREADONLY|OFN_PATHMUSTEXIST;
		if(GetSaveFileName(&psqOfn)){
			savePsq(psqOfn.lpstrFile, psqOfn.nFilterIndex, -1, includeUndoMoves!=0); //-1 = errCode unavailable
			break; //ok
		}
		if(CommDlgExtendedError()!=FNERR_INVALIDFILENAME
			|| !psqOfn.lpstrFile[0]) break; //cancel
		psqOfn.lpstrFile[0]=0;
	}
	saveLock--;
}

//-----------------------------------------------------------------
char *loadSkin(HBITMAP fbmp)
{
	int w, h;
	BITMAP bmp;
	char *errs;

	if(!fbmp){
		errs= lng(700, "Cannot create bitmap for skin");
	}
	else{
		GetObject(fbmp, sizeof(BITMAP), &bmp);
		w= bmp.bmWidth;
		h= bmp.bmHeight;
		if(w != 5*h + 1){
			errs=lng(701, "Bitmap has wrong size");
		}
		else{
			bmW=h;
			DeleteObject(SelectObject(bmpdc, fbmp));
			bm=fbmp;
			for(int i=0; i<3; i++){
				colors[i]= GetPixel(bmpdc, bmW*5, i);
			}
			SetBkColor(dc, colors[0]);
			SetTextColor(dc, colors[1]);
			errs=0;
		}
	}
	return errs;
}
//-----------------------------------------------------------------
char* loadSkin()
{
	BITMAPFILEHEADER hdr;
	HANDLE h;
	DWORD r, s;
	char *errs= lng(702, "Can't read BMP file");

	h= CreateFile(fnskin, GENERIC_READ, FILE_SHARE_READ,
		0, OPEN_EXISTING, 0, 0);
	if(h!=INVALID_HANDLE_VALUE){
		ReadFile(h, &hdr, sizeof(BITMAPFILEHEADER), &r, 0);
		if(r==sizeof(BITMAPFILEHEADER) && ((char*)&hdr.bfType)[0]=='B' && ((char*)&hdr.bfType)[1]=='M'){
			s = GetFileSize(h, 0) - sizeof(BITMAPFILEHEADER);
			char *b= new char[s];
			if(b){
				ReadFile(h, b, s, &r, 0);
				if(r==s){
					HBITMAP bm1 = CreateDIBitmap(dc, (BITMAPINFOHEADER*)b,
						CBM_INIT, b+hdr.bfOffBits-sizeof(BITMAPFILEHEADER),
						(BITMAPINFO*)b, DIB_RGB_COLORS);
					errs= loadSkin(bm1);
					invalidate();
				}
			}
			delete[] b;
		}
		CloseHandle(h);
	}
	return errs;
}
//-----------------------------------------------------------------
void openSkin()
{
	skinOfn.hwndOwner= hWin;
	skinOfn.Flags= OFN_FILEMUSTEXIST|OFN_HIDEREADONLY|OFN_READONLY|OFN_NOCHANGEDIR;
	if(GetOpenFileName(&skinOfn)){
		char *err= loadSkin();
		msg(err);
	}
	else{
		if(CommDlgExtendedError()==FNERR_INVALIDFILENAME && *fnskin){
			*fnskin=0;
			openSkin();
		}
	}
}
//-----------------------------------------------------------------
char *onlyExt(char *dest)
{
	char *s;

	if(!*fnskin) strcpy(fnskin, "skins\\*.bmp");
	strcpy(dest, fnskin);
	for(s=strchr(dest, 0); s>=dest && *s!='\\'; s--);
	s++;
	strcpy(s, "*.bmp");
	return fnskin + (s-dest);
}
//-----------------------------------------------------------------
void prevSkin()
{
	WIN32_FIND_DATA fd;
	TfileName buf, prev;
	HANDLE h;
	char *t;
	int pass=0;

	t= onlyExt(buf);
	h= FindFirstFile(buf, &fd);
	if(h==INVALID_HANDLE_VALUE){
		openSkin();
	}
	else{
		*prev=0;
		do{
			if(!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)){
				if(!_stricmp(t, fd.cFileName) || pass>10){
					strcpy(t, prev);
					if(!loadSkin()) break;
				}
				else{
					strcpy(prev, fd.cFileName);
				}
			}
			if(!FindNextFile(h, &fd)){
				FindClose(h);
				h = FindFirstFile(buf, &fd);
				pass++;
			}
		} while(pass<20);
		FindClose(h);
	}
}
//-----------------------------------------------------------------
void nextSkin()
{
	WIN32_FIND_DATA fd;
	TfileName buf;
	HANDLE h;
	char *t;
	int pass=0;

	t= onlyExt(buf);
	h= FindFirstFile(buf, &fd);
	if(h==INVALID_HANDLE_VALUE){
		openSkin();
	}
	else{
		do{
			if(!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)){
				if(pass){
					strcpy(t, fd.cFileName);
					if(!loadSkin()) break;
				}
				else if(!_stricmp(t, fd.cFileName)) pass++;
			}
			if(!FindNextFile(h, &fd)){
				FindClose(h);
				h = FindFirstFile(buf, &fd);
				pass++;
			}
		} while(pass<3);
		FindClose(h);
	}
}
//-----------------------------------------------------------------
void deleteini()
{
	HKEY key;
	DWORD i;

	delreg=true;
	if(RegDeleteKey(HKEY_CURRENT_USER, subkey)==ERROR_SUCCESS){
		if(RegOpenKey(HKEY_CURRENT_USER,
			"Software\\Petr Lastovicka", &key)==ERROR_SUCCESS){
			i=1;
			RegQueryInfoKey(key, 0, 0, 0, &i, 0, 0, 0, 0, 0, 0, 0);
			RegCloseKey(key);
			if(!i)
				RegDeleteKey(HKEY_CURRENT_USER, "Software\\Petr Lastovicka");
		}
	}
}

void writeini(bool saveToolbar)
{
	HKEY key;
	if(RegCreateKey(HKEY_CURRENT_USER, subkey, &key)!=ERROR_SUCCESS)
		msglng(735, "Cannot write to Windows registry");
	else{
		for(Treg *u=regVal; u<endA(regVal); u++){
			RegSetValueEx(key, u->s, 0, REG_DWORD,
				(BYTE *)u->i, sizeof(int));
		}
		for(Tregs *v=regValS; v<endA(regValS); v++){
			RegSetValueEx(key, v->s, 0, REG_SZ,
				(BYTE *)v->i, strlen(v->i)+1);
		}
		RegSetValueExA(key, "keys", 0, REG_BINARY, (BYTE *)accel, Naccel*sizeof(ACCEL));
		if(saveToolbar){
			TBSAVEPARAMS sp;
			sp.hkr=HKEY_CURRENT_USER;
			sp.pszSubKey=subkey;
			sp.pszValueName="toolbar";
			SendMessage(toolbar, TB_SAVERESTORE, TRUE, (LPARAM)&sp);
		}
		RegCloseKey(key);
	}
}

void readini()
{
	HKEY key;
	DWORD d;

	if(RegOpenKey(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders", &key)==ERROR_SUCCESS){
		d=sizeof(dataDir);
		RegQueryValueEx(key, "Common AppData", 0, 0, (BYTE *)dataDir, &d);
		RegCloseKey(key);
	}
	if(RegOpenKey(HKEY_CURRENT_USER, subkey, &key)==ERROR_SUCCESS){
		for(Treg *u=regVal; u<endA(regVal); u++){
			d=sizeof(int);
			RegQueryValueEx(key, u->s, 0, 0, (BYTE *)u->i, &d);
		}
		for(Tregs *v=regValS; v<endA(regValS); v++){
			d=v->n;
			RegQueryValueEx(key, v->s, 0, 0, (BYTE *)v->i, &d);
		}
		d=sizeof(accel);
		if(RegQueryValueExA(key, "keys", 0, 0, (BYTE *)accel, &d)==ERROR_SUCCESS){
			Naccel=d/sizeof(ACCEL);
		}
		RegCloseKey(key);
	}
}
//-----------------------------------------------------------------
//tournament result window procedure
BOOL CALLBACK ResultProc(HWND hWnd, UINT msg, WPARAM wP, LPARAM lP)
{
	static const Tresize D[]={{101, R_right|R_bottom},
	{IDOK, R_top|R_bottom}, {606, R_top|R_bottom}, {566, R_top|R_bottom}};

	resize(resultPos, D, sizeA(D), hWnd, msg, lP);

	switch(msg){
		case WM_INITDIALOG:
			setDlgTexts(hWnd, 16);
			return TRUE;
		case WM_COMMAND:
			wP=LOWORD(wP);
			if(wP==566){
				copyToClipboard(GetDlgItem(hWnd, 101));
				SetFocus(GetDlgItem(hWnd, IDOK));
			}
			if(wP==606){
				TfileName fn;
				strcpy(getTurDir(fn), fnHtmlTable);
				ShellExecute(hWin, "open", fn, 0, 0, SW_SHOWNORMAL);
			}
			if(wP==IDOK || wP==IDCANCEL) ShowWindow(hWnd, SW_HIDE);
			break;
	}
	return FALSE;
}
//-----------------------------------------------------------------
//log window procedure
BOOL CALLBACK LogProc(HWND hWnd, UINT msg, WPARAM wP, LPARAM lP)
{
	static const Tresize D[]={{101, R_right|R_bottom},
	{4, R_top|R_bottom}, {8, R_top|R_bottom}, {566, R_top|R_bottom}};

	resize(logPos, D, sizeA(D), hWnd, msg, lP);

	switch(msg){
		case WM_INITDIALOG:
			setDlgTexts(hWnd, 18);
			logWnd=GetDlgItem(hWnd, 101);
			SendMessage(logWnd, EM_LIMITTEXT, 0, 0);
			return TRUE;
		case WM_COMMAND:
			wP=LOWORD(wP);
			if(wP==566){
				copyToClipboard(logWnd);
				SetFocus(GetDlgItem(hWnd, IDOK));
			}
			if(wP==8){
				SetWindowText(logWnd, "");
				SetFocus(GetDlgItem(hWnd, IDOK));
			}
			if(wP==4 || wP==IDCANCEL) ShowWindow(hWnd, SW_HIDE);
			if(wP==4 && turTimerAvail){
				SetTimer(hWin, 997, 10, 0);
			}
			break;
	}
	return FALSE;
}
//-----------------------------------------------------------------
//options dialog procedure
BOOL CALLBACK OptionsProc(HWND hWnd, UINT msg, WPARAM wP, LPARAM)
{
	int i, id;
	static int width1, height1, tolerance1, maxMemory1, exactFive1, continuous1;
	static struct{ int *prom, id; } D[]={
		{&width, 103},
		{&height, 104},
		{&hiliteDelay, 108},
		{&tolerance, 109},
		{&maxMemory, 111},
		{&coordVisible, 510},
		{&hardTimeOut, 515},
		{&moveDiv2, 516},
		{&moveStart, 546},
		{&humanTimeOut, 517},
		{&coordStart, 518},
		{&logMessage, 540},
		{&logDebug, 541},
		{&logMoves, 542},
		{&suspendAI, 544},
		{&debugAI, 658},
		{&autoBegin, 545},
		{&infoEval, 548},
		{&debugPipe, 549},
		{&clearLog, 509},
		{&openingRandomShift1, 568},
		{&ignoreErrors, 659},
		{&includeUndoMoves, 670},
		{&soundNotification, 671},
	};

	switch(msg){
		case WM_INITDIALOG:
			setDlgTexts(hWnd, 12);
			width1=width;
			height1=height;
			tolerance1=tolerance;
			maxMemory1=maxMemory;
			exactFive1=exactFive;
			continuous1=continuous;
			for(i=0; i<sizeof(D)/sizeof(*D); i++){
				id=D[i].id;
				if(id>=100)
				 if(id<300){
					 SetDlgItemInt(hWnd, id, *D[i].prom, FALSE);
				 }
				 else{
					 CheckDlgButton(hWnd, id, *D[i].prom ? BST_CHECKED : BST_UNCHECKED);
				 }
			}
			CheckRadioButton(hWnd, 555, 556, 555+exactFive);
			CheckRadioButton(hWnd, 559, 560, 559+continuous);
			for(i=0; i<sizeA(priorTab); i++){
				SendDlgItemMessage(hWnd, 120, CB_ADDSTRING, 0, (LPARAM)lng(690+i, priorTab[i]));
			}
			SendDlgItemMessage(hWnd, 120, CB_SETCURSEL, priority, 0);
			SetDlgItemText(hWnd, 122, dataDir);
			PostMessage(hWnd, WM_COMMAND, 544, 0);
			return TRUE;

		case WM_COMMAND:
			wP=LOWORD(wP);
			switch(wP){
				case 124:
					browseFolder(dataDir, hWnd, 122, lng(558, "Folder for AI data files"));
					break;
				case 544: //suspend pbrain
					EnableWindow(GetDlgItem(hWnd, 658), IsDlgButtonChecked(hWnd, 544));
					break;
				case IDOK:
					for(i=0; i<sizeof(D)/sizeof(*D); i++){
						id=D[i].id;
						if(id>=100)
						 if(id<200){
							 *D[i].prom= GetDlgItemInt(hWnd, id, NULL, FALSE);
						 }
						 else{
							 *D[i].prom= IsDlgButtonChecked(hWnd, id);
						 }
					}
					exactFive= getRadioButton(hWnd, 555, 556);
					continuous= getRadioButton(hWnd, 559, 560);
					priority= (int)SendMessage(GetDlgItem(hWnd, 120), CB_GETCURSEL, 0, 0);
					if(isWin9X){
						if(priority==1) priority=0;
						if(priority==3) priority=4;
					}
					setPriority(players[0].process);
					setPriority(players[1].process);
					GetDlgItemText(hWnd, 122, dataDir, sizeof(dataDir));
					if(turNplayers || isNetGame){
						width=width1, height=height1, tolerance=tolerance1;
						exactFive=exactFive1, continuous=continuous1;
					}
					if(width1!=width || height1!=height){
						wP=119;
					}
					else{
						if(maxMemory1!=maxMemory){
							players[0].sendInfo(INFO_MEMORY);
							players[1].sendInfo(INFO_MEMORY);
						}
						if(exactFive1!=exactFive || continuous1!=continuous){
							players[0].sendInfo(INFO_RULE);
							players[1].sendInfo(INFO_RULE);
						}
						invalidate();
					}
				case IDCANCEL:
					EndDialog(hWnd, wP);
			}
			break;
	}
	return FALSE;
}
//-----------------------------------------------------------------
//players dialog procedure
BOOL CALLBACK PlayersProc(HWND hWnd, UINT msg, WPARAM wP, LPARAM)
{
	Tplayer *p;
	BOOL e;
	int k;
	static int oldComp[2];
	static char oldBrain[2][_MAX_PATH];
	static int oldTime[2];
	static int oldTimeGame[2];

	switch(msg){

		case WM_INITDIALOG:
			setDlgTexts(hWnd, 14);
			for(k=0; k<2; k++){
				p= &players[k];
				oldComp[k]= p->isComp;
				oldTime[k]= p->timeMove;
				oldTimeGame[k]= p->timeGame;
				strcpy(oldBrain[k], p->brain);
				SetDlgItemText(hWnd, 101+k, p->brain);
				setDlgItemMs(hWnd, 103+k, p->timeMove);
				SetDlgItemInt(hWnd, 105+k, p->timeGame, FALSE);
			}
			CheckRadioButton(hWnd, 522, 523, 523-players[0].isComp);
			CheckRadioButton(hWnd, 524, 525, 525-players[1].isComp);
			CheckDlgButton(hWnd, 530, sameTime ? BST_CHECKED : BST_UNCHECKED);
			PostMessage(hWnd, WM_COMMAND, 530, 0);
			PostMessage(hWnd, WM_COMMAND, 522, 0);
			PostMessage(hWnd, WM_COMMAND, 524, 0);
			return TRUE;

		case WM_COMMAND:
			wP=LOWORD(wP);
			switch(wP){
				case 530: //same times for both players
					e= !IsDlgButtonChecked(hWnd, 530);
					EnableWindow(GetDlgItem(hWnd, 104), e);
					EnableWindow(GetDlgItem(hWnd, 106), e);
					break;
				case 522: case 523: //computer/human
					EnableWindow(GetDlgItem(hWnd, 101), IsDlgButtonChecked(hWnd, 522));
					break;
				case 524: case 525: //computer/human
					EnableWindow(GetDlgItem(hWnd, 102), IsDlgButtonChecked(hWnd, 524));
					break;
				case 200: case 201: //find AI on disk
					softPause();
					brainOfn.hwndOwner= hWnd;
					brainOfn.Flags= OFN_FILEMUSTEXIST|OFN_HIDEREADONLY|OFN_READONLY;
					if(GetOpenFileName(&brainOfn)){
						SetDlgItemText(hWnd, wP-99, brainOfn.lpstrFile);
					}
					break;
				case IDOK:
					sameTime= IsDlgButtonChecked(hWnd, 530);
					for(k=0; k<2; k++){
						p= &players[k];
						p->timeMove= getDlgItemMs(hWnd, 103+k);
						p->timeGame= GetDlgItemInt(hWnd, 105+k, NULL, FALSE);
						if(sameTime && k==1){
							p->timeMove=players[0].timeMove;
							p->timeGame=players[0].timeGame;
						}
						p->isComp= 1-getRadioButton(hWnd, 522+2*k, 523+2*k);
						if(p->isComp!=oldComp[k]){
							killBrains();
							resetScore();
						}
						GetDlgItemText(hWnd, 101+k, p->brain, sizeof(p->brain));
						if(_stricmp(p->brain, oldBrain[k])){
							killBrains();
							p->brainChanged();
							resetScore();
						}
						if(p->timeMove!=oldTime[k]){
							p->sendInfo(INFO_TIMEMOVE);
							levelChanged=true;
							printLevel();
						}
						if(p->timeGame!=oldTimeGame[k]){
							p->sendInfo(INFO_TIMEGAME|INFO_TIMELEFT);
							levelChanged=true;
						}
					}
				case IDCANCEL:
					EndDialog(hWnd, wP);
					break;
			}
			break;
	}
	return FALSE;
}
//-----------------------------------------------------------------
BOOL CALLBACK moveProc(HWND hWnd, UINT msg, WPARAM wP, LPARAM)
{
	int i;

	switch(msg){
		case WM_INITDIALOG:
			setDlgTexts(hWnd, 17);
			SetDlgItemInt(hWnd, 101, gotoMove, FALSE);
			return TRUE;
		case WM_COMMAND:
			wP=LOWORD(wP);
			switch(wP){
				case IDOK:
					gotoMove= GetDlgItemInt(hWnd, 101, NULL, FALSE);
					i= gotoMove-moveStart;
					if(moveDiv2) i*=2;
					while(moves<i && redo());
					while(moves>i && undo());
				case IDCANCEL:
					EndDialog(hWnd, wP);
					break;
			}
			break;
	}
	return FALSE;
}
//-----------------------------------------------------------------
BOOL CALLBACK msgProc(HWND hWnd, UINT msg, WPARAM wP, LPARAM lP)
{
	static const Tresize D[]={{101, R_right|R_bottom},
	{9, R_top|R_bottom}, {IDCANCEL, R_top|R_bottom}};

	resize(msgPos, D, sizeA(D), hWnd, msg, lP);

	switch(msg){
		case WM_INITDIALOG:
			setDlgTexts(hWnd, 24);
			msgWnd=GetDlgItem(hWnd, 101);
			return TRUE;
		case WM_COMMAND:
			wP=LOWORD(wP);
			switch(wP){
				case 9:
					if(isNetGame) netGameMsg();
					break;
				case IDCANCEL:
					ShowWindow(hWnd, SW_HIDE);
					break;
			}
			break;
	}
	return FALSE;
}
//---------------------------------------------------------------
BOOL CALLBACK clientProc(HWND hWnd, UINT msg, WPARAM wP, LPARAM)
{
	BOOL e;

	switch(msg){
		case WM_INITDIALOG:
			setDlgTexts(hWnd, 19);
			SetDlgItemText(hWnd, 101, servername);
			SetDlgItemInt(hWnd, 110, port, FALSE);
			SetDlgItemText(hWnd, 103, AIfolder);
			CheckRadioButton(hWnd, 597, 598, 597+whoConnect);
			PostMessage(hWnd, WM_COMMAND, 597, 0);
			return 1;

		case WM_COMMAND:
			wP=LOWORD(wP);
			switch(wP){
				case 107: //folder
					browseFolder(AIfolder, hWnd, 103, lng(590, "Folder for AI:"));
					break;
				case 597: case 598:
					e= IsDlgButtonChecked(hWnd, 597);
					EnableWindow(GetDlgItem(hWnd, 103), e);
					break;
				case IDOK:
					if(!checkDir(hWnd, 103)) break;
					GetDlgItemText(hWnd, 101, servername, sizeof(servername));
					port= GetDlgItemInt(hWnd, 110, NULL, FALSE);
					GetDlgItemText(hWnd, 103, AIfolder, sizeof(AIfolder));
					whoConnect= getRadioButton(hWnd, 597, 598);
					if(whoConnect ? netGameStart() : clientStart()){
						SetFocus(GetDlgItem(hWnd, 101));
						break;
					}
				case IDCANCEL:
					EndDialog(hWnd, wP);
					break;
			}
			break;
	}
	return 0;
}
//-----------------------------------------------------------------
BOOL CALLBACK disconnectProc(HWND hWnd, UINT msg, WPARAM wP, LPARAM)
{

	switch(msg){

		case WM_INITDIALOG:
			setDlgTexts(hWnd, 23);
			CheckRadioButton(hWnd, 595, 596, 595);
			return TRUE;

		case WM_COMMAND:
			wP=LOWORD(wP);
			switch(wP){
				case 596:
					SetFocus(GetDlgItem(hWnd, 101));
					break;
				case IDOK:
					if(getRadioButton(hWnd, 595, 596)){
						killClient(GetDlgItemInt(hWnd, 101, NULL, FALSE));
					}
					else{
						serverEnd();
					}
				case IDCANCEL:
					EndDialog(hWnd, wP);
					break;
			}
			break;
	}
	return FALSE;
}
//-----------------------------------------------------------------
void turAddAI(bool noPath)
{
	char *s, *d, *path;
	int i, j;

	d=strchr(turPlayerStr, 0);
	if(d>turPlayerStr && d[-1]!='\n'){ *d++='\r'; *d++='\n'; }
	path=brainOfn.lpstrFile;
	i=(int)strlen(path);
	s= path+i+1;
	if(!*s){
		//one file
		if(noPath) path=cutPath(path);
		strncpy(d, path, sizeof(turPlayerStr)-(d-turPlayerStr)-1);
	}
	else{
		//multiple files
		for(; *s; s++){
			j=(int)strlen(s);
			if((d-turPlayerStr)+i+j+4>sizeof(turPlayerStr)) break;
			if(!noPath){
				strcpy(d, path);
				d+=i;
				*d++='\\';
			}
			strcpy(d, s);
			d+=j;
			*d++='\r';
			*d++='\n';
			s+=j;
		}
		*d=0;
	}
}

BOOL CALLBACK TurProc(HWND hWnd, UINT msg, WPARAM wP, LPARAM)
{
	BOOL e;
	int i, id;
	FILE *f;
	static struct{ int *prom, id; } D[]={
		{&turRepeat, 102},
		{&turMatchRepeat, 115},
		{&players[0].timeMove, 153},
		{&players[0].timeGame, 105},
		{&turDelay, 158},
		{&tolerance, 109},
		{&maxMemory, 111},
		{&turTieRepeat, 112},
		{&turRecord, 565},
		{&turOnlyLosses, 573},
		{&port, 110},
		{&autoBegin, 545},
		{&turLogMsg, 567},
		{&openingRandomShiftT, 568},
	};

	switch(msg){

		case WM_INITDIALOG:
			setDlgTexts(hWnd, 15);
			SetDlgItemText(hWnd, 101, turPlayerStr);
			SetDlgItemText(hWnd, 106, fntur);
			SetDlgItemText(hWnd, 113, cmdGameEnd);
			SetDlgItemText(hWnd, 114, cmdTurEnd);
			CheckRadioButton(hWnd, 570, 571, 570+turRule);
			CheckRadioButton(hWnd, 575, 576, 575+turNet);
			CheckRadioButton(hWnd, 555, 556, 555+exactFive);
			CheckDlgButton(hWnd, 572, turFormat==2 ? BST_CHECKED : BST_UNCHECKED);
			for(i=0; i<sizeof(D)/sizeof(*D); i++){
				id=D[i].id;
				if(id>=100)
				 if(id<300){
					 if(id<150){
						 SetDlgItemInt(hWnd, id, *D[i].prom, FALSE);
					 }
					 else{
						 setDlgItemMs(hWnd, id, *D[i].prom);
					 }
				 }
				 else{
					 CheckDlgButton(hWnd, id, *D[i].prom ? BST_CHECKED : BST_UNCHECKED);
				 }
			}
			PostMessage(hWnd, WM_COMMAND, 565, 0);
			PostMessage(hWnd, WM_COMMAND, 576, 0);
			return 1;

		case WM_COMMAND:
			wP=LOWORD(wP);
			switch(wP){
				case 564: //find AI on disk
					brainOfn.hwndOwner= hWnd;
					brainOfn.Flags= OFN_FILEMUSTEXIST|OFN_HIDEREADONLY|OFN_READONLY|OFN_ALLOWMULTISELECT|OFN_EXPLORER;
					*brainOfn.lpstrFile=0;
					if(GetOpenFileName(&brainOfn)){
						GetDlgItemText(hWnd, 101, turPlayerStr, sizeof(turPlayerStr)-2);
						turAddAI(false);
						SetDlgItemText(hWnd, 101, turPlayerStr);
					}
					break;
				case 565: //save games to
					e= (BOOL)IsDlgButtonChecked(hWnd, 565);
					EnableWindow(GetDlgItem(hWnd, 572), e);
					EnableWindow(GetDlgItem(hWnd, 573), e);
					break;
				case 575: //local
				case 576: //network
					EnableWindow(GetDlgItem(hWnd, 110), IsDlgButtonChecked(hWnd, 576));
					break;
				case 107: //folder
					browseFolder(fntur, hWnd, 106, lng(565, "Save games to folder:"));
					break;
				case 569: //open
					if(!checkDir(hWnd, 106)) break;
					GetDlgItemText(hWnd, 106, fntur, sizeof(fntur));
					strcpy(getTurDir(fnstate), fnState);
					f=fopen(fnstate, "rb");
					if(f){
						fclose(f);
					}
					else{
						stateOfn.hwndOwner= hWnd;
						stateOfn.Flags= OFN_FILEMUSTEXIST|OFN_HIDEREADONLY|OFN_READONLY|OFN_EXPLORER;
						stateOfn.lpstrInitialDir=fntur;
						*stateOfn.lpstrFile=0;
						if(!GetOpenFileName(&stateOfn)) break;
					}
					//!
				case IDOK:
					if(!checkDir(hWnd, 106)) break;
					GetDlgItemText(hWnd, 101, turPlayerStr, sizeof(turPlayerStr));
					GetDlgItemText(hWnd, 106, fntur, sizeof(fntur));
					GetDlgItemText(hWnd, 113, cmdGameEnd, sizeof(cmdGameEnd));
					GetDlgItemText(hWnd, 114, cmdTurEnd, sizeof(cmdTurEnd));
					turRule= getRadioButton(hWnd, 570, 571); //radio is not visible when starting tournament
					turNet= getRadioButton(hWnd, 575, 576);
					exactFive= getRadioButton(hWnd, 555, 556);
					turFormat= 1+IsDlgButtonChecked(hWnd, 572);
					for(i=0; i<sizeof(D)/sizeof(*D); i++){
						id=D[i].id;
						if(id>=100)
						 if(id<300){
							 *D[i].prom= (id<150) ? GetDlgItemInt(hWnd, id, NULL, FALSE) : getDlgItemMs(hWnd, id);
						 }
						 else{
							 *D[i].prom= IsDlgButtonChecked(hWnd, id);
						 }
					}
				case IDCANCEL:
					EndDialog(hWnd, wP);
					break;
			}
			break;
	}
	return FALSE;
}
//-----------------------------------------------------------------
void reloadMenu()
{
	static int subId[]={456, 455, 454, 453, 452, 450, 451};
	loadMenu(hWin, "MAINMENU", subId);
	checkMenus();
	DrawMenuBar(hWin);
}

void accelChanged()
{
	DestroyAcceleratorTable(haccel);
	if(Naccel>0){
		haccel= CreateAcceleratorTable(accel, Naccel);
	}
	else{
		haccel= LoadAccelerators(inst, MAKEINTRESOURCE(3));
		Naccel= CopyAcceleratorTable(haccel, accel, sizeA(accel));
	}
}

void getCmdName(char *buf, int n, int cmd)
{
	char *s, *d;

	s=lng(cmd, 0);
	if(s){
		lstrcpyn(buf, s, n);
	}
	else{
		MENUITEMINFO mii;
		mii.cbSize=sizeof(MENUITEMINFO);
		mii.fMask=MIIM_TYPE;
		mii.dwTypeData=buf;
		mii.cch= n;
		GetMenuItemInfo(GetMenu(hWin), cmd, FALSE, &mii);
	}
	for(s=d=buf; *s; s++){
		if(*s=='\t') *s=0;
		if(*s!='&') *d++=*s;
	}
	*d=0;
}

void changeKey(HWND hDlg, UINT vkey)
{
	char buf[64];
	BYTE flg;
	ACCEL *a, *e;

	dlgKey.key=(USHORT)vkey;
	//read shift states
	flg=FVIRTKEY|FNOINVERT;
	if(GetKeyState(VK_CONTROL)<0) flg|=FCONTROL;
	if(GetKeyState(VK_SHIFT)<0) flg|=FSHIFT;
	if(GetKeyState(VK_MENU)<0) flg|=FALT;
	dlgKey.fVirt=flg;
	//set hotkey edit control
	printKey(buf, &dlgKey);
	SetWindowText(GetDlgItem(hDlg, 131), buf);
	//modify accelerator table
	e=accel+Naccel;
	if(!dlgKey.cmd){
		for(a=accel; a<e; a++){
			if(a->key==vkey && a->fVirt==flg){
				PostMessage(hDlg, WM_COMMAND, a->cmd, 0);
			}
		}
	}
	else{
		for(a=accel;; a++){
			if(a==e){
				*a=dlgKey;
				Naccel++;
				break;
			}
			if(a->cmd==dlgKey.cmd){
				a->fVirt=dlgKey.fVirt;
				a->key=dlgKey.key;
				if(vkey==0){
					memcpy(a, a+1, (e-a-1)*sizeof(ACCEL));
					Naccel--;
				}
				break;
			}
		}
		accelChanged();
		//change the main menu
		reloadMenu();
	}
}

static WNDPROC editWndProc;

//window procedure for hotkey edit control
LRESULT CALLBACK hotKeyClassProc(HWND hWnd, UINT mesg, WPARAM wP, LPARAM lP)
{
	switch(mesg){
		case WM_KEYDOWN:
		case WM_SYSKEYDOWN:
			if(wP!=VK_CONTROL && wP!=VK_SHIFT && wP!=VK_MENU && wP!=VK_RWIN && wP!=VK_LWIN){
				if((wP==VK_BACK || wP==VK_DELETE) && dlgKey.key) wP=0;
				changeKey(GetParent(hWnd), (UINT)wP);
				return 0;
			}
			break;
		case WM_CHAR:
			return 0; //don't write character to the edit control
	}
	return CallWindowProc((WNDPROC)editWndProc, hWnd, mesg, wP, lP);
}

LRESULT CALLBACK KeysDlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM)
{
	static const int Mbuf=64;
	char buf[Mbuf];
	int i, n;

	switch(message){
		case WM_INITDIALOG:
			editWndProc = (WNDPROC)SetWindowLong(GetDlgItem(hWnd, 131),
				GWL_WNDPROC, (LONG)hotKeyClassProc);
			setDlgTexts(hWnd, 25);
			dlgKey.cmd=0;
			return TRUE;

		case WM_COMMAND:
			wParam=LOWORD(wParam);
			switch(wParam){
				default:
					//popup menu item
					if(wParam<500 && wParam>=100 && wParam!=131 && wParam!=172){
						//set command text
						dlgKey.cmd=(USHORT)wParam;
						getCmdName(buf, Mbuf, wParam);
						SetDlgItemText(hWnd, 172, buf);
						HWND hHotKey=GetDlgItem(hWnd, 131);
						if(!*buf) dlgKey.cmd=0;
						else SetFocus(hHotKey);
						//change accelerator table
						dlgKey.key=0;
						*buf=0;
						for(ACCEL *a=accel+Naccel-1; a>=accel; a--){
							if(a->cmd==dlgKey.cmd){
								dlgKey.fVirt=a->fVirt;
								dlgKey.key=a->key;
								printKey(buf, &dlgKey);
								break;
							}
						}
						//set hotkey edit control
						SetWindowText(hHotKey, buf);
					}
					break;
				case 130:
				{
					RECT rc;
					GetWindowRect(GetDlgItem(hWnd, 130), &rc);
					//create popup menu from the main menu
					HMENU hMenu= CreatePopupMenu();
					MENUITEMINFO mii;
					n=GetMenuItemCount(GetMenu(hWin));
					for(i=0; i<n; i++){
						mii.cbSize=sizeof(mii);
						mii.fMask=MIIM_SUBMENU|MIIM_TYPE;
						mii.cch=Mbuf;
						mii.dwTypeData=buf;
						GetMenuItemInfo(GetMenu(hWin), i, TRUE, &mii);
						InsertMenuItem(hMenu, 0xffffffff, TRUE, &mii);
					}
					//show popup menu
					TrackPopupMenuEx(hMenu,
						TPM_RIGHTBUTTON, rc.left, rc.bottom, hWnd, NULL);
					//delete popup menu
					for(i=0; i<n; i++){
						RemoveMenu(hMenu, 0, MF_BYPOSITION);
					}
					DestroyMenu(hMenu);
					break;
				}
				case IDOK:
				case IDCANCEL:
					EndDialog(hWnd, wParam);
					return TRUE;
			}
	}
	return FALSE;
}
//-----------------------------------------------------------------
DWORD getVer()
{
	HRSRC r;
	HGLOBAL h;
	void *s;
	VS_FIXEDFILEINFO *v;
	UINT i;

	r=FindResource(0, (char*)VS_VERSION_INFO, RT_VERSION);
	h=LoadResource(0, r);
	s=LockResource(h);
	if(!s || !VerQueryValue(s, "\\", (void**)&v, &i)) return 0;
	return v->dwFileVersionMS;
}

BOOL CALLBACK AboutProc(HWND hWnd, UINT msg, WPARAM wP, LPARAM)
{
	char buf[48];
	DWORD d;

	switch(msg){
		case WM_INITDIALOG:
			setDlgTexts(hWnd, 11);
			d=getVer();
			sprintf(buf, "%d.%d", HIWORD(d), LOWORD(d));
			SetDlgItemText(hWnd, 101, buf);
			return TRUE;

		case WM_COMMAND:
			wP=LOWORD(wP);
			switch(wP){
				case 502:
					GetDlgItemTextA(hWnd, wP, buf, sizeA(buf));
					ShellExecuteA(0, 0, buf, 0, 0, SW_SHOWNORMAL);
					break;
				case IDOK:
				case IDCANCEL:
					EndDialog(hWnd, wP);
			}
			break;
	}
	return FALSE;
}
//-----------------------------------------------------------------
void drawTitle()
{
	title= lng(20, "Piskvork");
	if(isClient) title= lng(21, "Piskvork - client");
	if(isServer) title= lng(22, "Piskvork - server");
	SetWindowText(hWin, title);
}

void moveMsgWnd()
{
	int d;
	RECT rc, rcs;

	SystemParametersInfo(SPI_GETWORKAREA, 0, &rcs, 0);
	GetWindowRect(msgDlg, &rc);
	d=rc.bottom-rcs.bottom;
	amax(d, logPos.y);
	if(d>0){
		SetWindowPos(logDlg, 0, logPos.x, logPos.y-=d, 0, 0, SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE);
		SetWindowPos(msgDlg, 0, rc.left, rc.top-=d, 0, 0, SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE);
	}
}

void langChanged()
{
	reloadMenu();
	status();
	drawTitle();
	psqOfn.lpstrFilter=lng(801, "Saved positions (*.psq)\0*.psq\0Gomotur record (*.rec)\0*.rec\0All files\0*.*\0");
	skinOfn.lpstrFilter=lng(500, "Skins (*.bmp)\0*.bmp\0");
	brainOfn.lpstrFilter=lng(802, "Artifical inteligence (*.exe,*.zip)\0*.exe;*.com;*.bat;*.zip\0");
	stateOfn.lpstrFilter=lng(814, "Tournament state (*.tur)\0*.tur\0");
	//create dialog windows
	changeDialog(logDlg, logPos, "LOG", (DLGPROC)LogProc);
	changeDialog(resultDlg, resultPos, "RESULT", (DLGPROC)ResultProc);
	if(msgPos.y<0){
		RECT rc;
		GetWindowRect(logDlg, &rc);
		msgPos.x=rc.left;
		msgPos.y=rc.bottom;
	}
	changeDialog(msgDlg, msgPos, "MSG", (DLGPROC)msgProc, logDlg);
}

//-----------------------------------------------------------------
LRESULT CALLBACK MainWndProc(HWND hWnd, UINT mesg, WPARAM wP, LPARAM lP)
{
	static PAINTSTRUCT ps;
	RECT rcw;
	int i, x, y;
	int hr;
	bool a;
	Psquare p;
	FILE *f;
	char buf[256];

	hr=player;
	if(!players[hr].isComp && players[1-hr].isComp) hr=1-hr;

	switch(mesg) {
		case WM_COMMAND:
			wP=LOWORD(wP);
			if(setLang(wP)) break;
			switch(wP){
				case 120: //readme
				case 250: //what's new
				{
					getExeDir(buf, wP==250 ? lng(26, "WhatsNew.txt") : lng(13, "piskvork.txt"));
					if(ShellExecute(0, "open", buf, 0, 0, SW_SHOWNORMAL)==(HINSTANCE)ERROR_FILE_NOT_FOUND){
						msglng(739, "File %s not found", buf);
					}
				}
					break;
				case 121: //about
					DialogBox(inst, "ABOUT", hWnd, (DLGPROC)AboutProc);
					break;
				case 122: //Download another AI	
					ShellExecute(NULL, "open", "http://gomocup.wz.cz/gomoku/download.php?source=piskvork", NULL, NULL, SW_SHOWNORMAL);
					break;
				case 310:
					SendMessage(toolbar, TB_CUSTOMIZE, 0, 0);
					invalidate();
					break;
				case 103: //exit
					SendMessage(hWin, WM_CLOSE, 0, 0);
					break;
				case 105: //delete settings
					if(MessageBox(hWnd, lng(736, "Do you want to delete your settings ?"),
					 title, MB_ICONQUESTION | MB_YESNO | MB_DEFBUTTON2) ==IDYES){
						deleteini();
					}
					break;
				case 107: //save position
					savePsq();
					break;
				case 108: //ESC
					hardPause();
					if(turNplayers && !isClient){
						a=turTimerAvail;
						turTimerAvail=false;
						if(MessageBox(hWnd, lng(810, "Do you want to abort the tournament ?"),
							title, MB_ICONQUESTION | MB_YESNO | MB_DEFBUTTON2) ==IDYES){
							if(a) KillTimer(hWnd, 997);
							turAbort();
						}
						else{
							resume();
							turTimerAvail=a;
						}
					}
					break;
				case 209: //continue
					if(stopThinking()==0) resume();
					break;
				case 203: //show last move
					showLast();
					break;
				case 204: //delete score
					resetScore();
					break;
				case 207: //swap symbols
					invert= !invert;
					invalidate();
					break;
				case 221: //log window
					show(logDlg);
					break;
				case 224: //tournament result
					if(!isClient) show(resultDlg);
					break;
				case 301: //open skin
					openSkin();
					break;
				case 302: //next skin
					nextSkin();
					break;
				case 303: //previous skin
					prevSkin();
					break;
				case 304: //refresh skin
					loadSkin();
					break;
				case 305: //flip horizontally
					flipHoriz=!flipHoriz;
					invalidate();
					break;
				case 306: //flip vertically
					flipVert=!flipVert;
					invalidate();
					break;
				case 307: //flip diagonally
					flipDiag=!flipDiag;
					invalidate();
					break;
				case 104: //options
					if(!isServer || !turNplayers){
						if(DialogBox(inst, "OPTIONS", hWnd, (DLGPROC)OptionsProc)
							== 119){
							killBrains();
							newGame(player, true);
						}
					}
					break;
				case 998: //write log
					if(lP){
						i=GetWindowTextLength(logWnd);
						SendMessage(logWnd, EM_SETSEL, i, i);
						SendMessage(logWnd, EM_REPLACESEL, FALSE, lP);
						delete[](char*)lP;
					}
					else if(clearLog){
						SetWindowText(logWnd, "");
					}
					break;
				case 996: //result
					SetDlgItemText(resultDlg, 101, lP ? (char*)lP : "");
					delete[](char*)lP;
					break;
				case 995: //tie
					restartGame();
					break;
				case 994: //show log or result
					ShowWindow((HWND)lP, SW_SHOW);
					SetForegroundWindow((HWND)lP);
					break;
				case 992: //start network game
					if(isNetGame) newNetGame(lP);
					break;
				case 991: //network player's turn
					if(isNetGame && player==NET_PLAYER){
						doMove1((Psquare)lP, 1);
						hiliteLast();
					}
					break;
				case 990: //network undo
					if(isNetGame){
						while(moves>undoRequest && undo());
						undoRequest=0;
					}
					break;
				case 989: //network new game
					if(isNetGame){
						newGame(player, false);
						undoRequest=0;
					}
					break;
				case 219: //tournament button
					PostMessage(hWin, WM_COMMAND, turNplayers ? 224 : 220, 0);
					break;
				case 309: //show toolbar
					i = IsWindowVisible(toolbar);
					mtop=0;
					if(!i) mtop= toolBarH;
					ShowWindow(toolbar, i ? SW_HIDE : SW_SHOW);
					toolBarVisible = !i;
					invalidate();
					checkMenus();
					break;
				case 230:
					DialogBox(inst, "KEYS", hWin, (DLGPROC)KeysDlgProc);
					break;
				case 222: //disconnect
					clientEnd();
					netGameEnd();
					if(isServer){
						DialogBox(inst, "DISCONNECT", hWnd, (DLGPROC)disconnectProc);
					}
					break;
				case 227: //send text message to the other player 
					if(isNetGame){
						SetWindowText(msgWnd, "");
						moveMsgWnd();
						show(msgDlg);
						SetFocus(msgWnd);
					}
					else msglng(819, "Messages can be sent only between two players over the internet");
					break;
				case 101: //new game
					if(isNetGame){
						if(finished) newGame(player, false);
						else netGameNew();
					}
					break;
				case 201: //undo
				case 210:
					if(isNetGame) netGameUndo();
					break;
			}

			if(turNplayers || isNetGame) break;

			switch(wP){
				case 993: //resize to 20x20
					killBrains();
					width=height=20;
					newGame(whoStarted(), true);
					resume();
					break;
				case 225: //tournament from board
					//creates a temporary opening file from the board
					if(!lastMove) break;
					getExeDir(buf, tmpOpenings);
					f = fopen(buf, "wt");
					if(!f){
						msglng(733, "Cannot create file %s", buf);
						break;
					}
					for(p=lastMove; p->nxt; p=p->nxt);
					//p is the first move
					for(;;){
						fprintf(f, "%d,%d", p->x - (width/2+1), p->y - (height/2+1));
						p=p->pre;
						if(!p) break;
						fprintf(f, ", ");
					}
					fclose(f);
					//!
				case 220: //tournament
					i=DialogBox(inst, "TOURNAMENT", hWnd, (DLGPROC)TurProc);
					if(i==IDOK){
						writeini();
						if(wP==220){
							autoBeginForce = false;
							turStart();
						}
						else{
							autoBeginForce = true;
							turStart(tmpOpenings);
						}
					}
					if(i==569){
						turRestart();
					}
					break;
				case 101: //new game
					newGame((moves<=startMoves+1 && (moves&1)==0 || moves==width*height) ? 1-player : player, true);
					resume();
					break;
				case 106: //open position
					killBrains();
					openPsq();
					break;
				case 404: //increment level
					setLevel(players[hr].level()+1, hr);
					break;
				case 405: //decrement level
					setLevel(players[hr].level()-1, hr);
					break;
				case 401: //human x human
					switchPlayer(0, 0);
					break;
				case 402: //computer x human
					switchPlayer(1, 0);
					break;
				case 403: //computer x computer
					switchPlayer(1, 1);
					break;
				case 406: //players settings
					DialogBox(inst, "PLAYERS", hWnd, (DLGPROC)PlayersProc);
					printLevel();
					break;
				case 206: //swap players
				{
					killBrains();
					Tplayer w= players[0];
					players[0]=players[1];
					players[1]=w;
					int t=players[0].time;
					players[0].time=players[1].time;
					players[1].time=t;
					players[0].threads.init();
					players[1].threads.init();
					printScore();
					printLevel();
					resume();
				}
					break;
				case 210: //undo 2x
					if(moves>2){
						if(notSuspended()){
							hardPause();
						}
						else{
							hiliteLast();
							UpdateWindow(hWnd);
							Sleep(min(hiliteDelay, 40));
							undo();
						}
						hiliteLast();
						UpdateWindow(hWnd);
						Sleep(min(hiliteDelay, 150));
						undo();
					}
					break;
				case 201: //undo
					hardPause();
					hiliteLast();
					UpdateWindow(hWnd);
					Sleep(min(hiliteDelay, 70));
					undo();
					break;
			}

			if(notSuspended()) break;

			switch(wP){
				case 223:
					if(!isServer) DialogBox(inst, "CLIENT", hWnd, (DLGPROC)clientProc);
					break;
				case 202: //redo
					redo();
					hiliteLast();
					break;
				case 205: //undo all
					while(undo());
					break;
				case 208: //redo all
					while(redo());
					break;
				case 211: //redo 2x
					redo();
					hiliteLast();
					UpdateWindow(hWnd);
					Sleep(min(hiliteDelay, 250));
					redo();
					hiliteLast();
					break;
				case 212: //go to move
					DialogBox(inst, "GOTO", hWnd, (DLGPROC)moveProc);
					break;
			}
			break;

		case WM_RBUTTONDOWN:
			if(onAIName(1, lP)) switchPlayer(players[0].isComp, 1-players[1].isComp);
			else if(onAIName(0, lP)) switchPlayer(1-players[0].isComp, players[1].isComp);
			else if(finished || moves==0){
				PostMessage(hWnd, WM_COMMAND, 101, 0);
			}
			else{
				showLast();
			}
			break;

		case WM_LBUTTONDBLCLK:
			if(turNplayers || isNetGame) break;
			for(i=0; i<2; i++){
				if(onAIName(i, lP)){
					softPause();
					brainOfn.hwndOwner= hWnd;
					brainOfn.Flags= OFN_FILEMUSTEXIST|OFN_HIDEREADONLY|OFN_READONLY;
					if(GetOpenFileName(&brainOfn)){
						Tplayer *p= &players[i];
						if(_stricmp(p->brain, brainOfn.lpstrFile) || !p->isComp){
							killBrains();
							strcpy(p->brain, brainOfn.lpstrFile);
							p->brainChanged();
							if(!p->isComp) p->isComp=1;
							printLevel();
							resume();
						}
					}
					break;
				}
			}
			break;

		case WM_LBUTTONDOWN:
			if(notSuspended()) break;
			if(HIWORD(lP)<=heightb){
				if((!players[player].isComp || (wP & MK_CONTROL)) &&
					(player!=NET_PLAYER || !isNetGame)){
					getMousePos(lP, x, y);
					if(players[player].isComp) boardChanged();
					if(moves==startMoves) lastTick=getTickCount();
					doMove1(Square(x, y));
				}
				resume();
			}
			break;

		case WM_MOUSEMOVE:
			getMousePos(lP, x, y);
			if(x!=mx || y!=my){
				mx=x; my=y;
				printMouseCoord();
				sendInfoEval();
			}
			break;

		case WM_TIMER:
			EnterCriticalSection(&timerLock);
			if(wP==10) cancelHilite();
			if(wP==102) printTime(player);
			LeaveCriticalSection(&timerLock);
			if(wP==997 && turNplayers && turTimerAvail){
				turTimerAvail=false;
				KillTimer(hWnd, wP);
				turLocalNext();
			}
			break;

		case WM_CHAR:
			if(isNetGame && wP>32){
				SendMessage(hWnd, WM_COMMAND, 227, 0);
				char c[2];
				c[0]=(char)wP;
				c[1]=0;
				SendMessage(msgWnd, EM_REPLACESEL, TRUE, (LPARAM)c);
			}
			else if(wP>='0' && wP<='9') setLevel(wP-'0', hr);
			break;

		case WM_KEYDOWN:
			if(wP==VK_ESCAPE) SendMessage(hWnd, WM_COMMAND, 108, 0);
			break;

		case WM_PAINT:
			EnterCriticalSection(&drawLock);
			BeginPaint(hWnd, &ps);
			repaint(&ps.rcPaint);
			EndPaint(hWnd, &ps);
			LeaveCriticalSection(&drawLock);
			break;

		case WM_NOTIFY:
			if(lP){
				NMTOOLBAR *nmtool;
				switch(((NMHDR*)lP)->code){
					case TBN_QUERYDELETE:
					case TBN_QUERYINSERT:
						return TRUE;
					case TBN_GETBUTTONINFO:
						nmtool= (NMTOOLBAR*)lP;
						i= nmtool->iItem;
						if(i<sizeA(tbb)){
							lstrcpyn(nmtool->pszText, lng(tbb[i].idCommand+1000, toolNames[i]), nmtool->cchText);
							nmtool->tbButton= tbb[i];
							return TRUE;
						}
						break;
					case TBN_RESET:
						while(SendMessage(toolbar, TB_DELETEBUTTON, 0, 0));
						SendMessage(toolbar, TB_ADDBUTTONS, Ntool, (LPARAM)tbb);
						break;
					case TTN_NEEDTEXT:
					{
						TOOLTIPTEXT *ttt= (LPTOOLTIPTEXT)lP;
						for(i=0; i<sizeA(tbb); i++){
							if(tbb[i].idCommand==(int)ttt->hdr.idFrom){
								ttt->hinst= NULL;
								ttt->lpszText= lng(ttt->hdr.idFrom+1000, toolNames[i]);
							}
						}
					}
						break;

#ifndef UNICODE
						//Linux
					case TTN_NEEDTEXTW:
					{
						TOOLTIPTEXTW *ttt = (LPTOOLTIPTEXTW)lP;
						for(i=0; i<sizeA(tbb); i++){
							if(tbb[i].idCommand==(int)ttt->hdr.idFrom){
								ttt->hinst= NULL;
								MultiByteToWideChar(CP_ACP, 0, lng(ttt->hdr.idFrom+1000, toolNames[i]), -1, ttt->szText, sizeA(ttt->szText));
								ttt->szText[sizeA(ttt->szText)-1]=0;
							}
						}
					}
						break;
#endif
				}
			}
			break;

		case WM_MOVE:
			if(!IsZoomed(hWnd) && !IsIconic(hWnd)){
				GetWindowRect(hWnd, &rcw);
				mainPos.y= rcw.top;
				mainPos.x= rcw.left;
			}
			break;

		case WM_APP+123:
			p=Square(LOWORD(lP), HIWORD(lP));
			if(player && (wP==1 || wP==2)) wP^=3;
			if(p>=boardb && p<boardk) paintSquare(wP, p);
			break;

		case WM_QUERYENDSESSION:
			if(!delreg) writeini();
			return TRUE;

		case WM_CLOSE:
			if(!delreg) writeini();
			killBrains();
			serverEnd();
			clientEnd();
			netGameEnd();
			netGameEnd();
			DestroyWindow(logDlg);
			DestroyWindow(resultDlg);
			DestroyWindow(msgDlg);
			DestroyWindow(hWin);
			hWin=0;
			break;
		case WM_DESTROY:
			PostQuitMessage(0);
			break;

		default:
			return DefWindowProc(hWnd, mesg, wP, lP);
	}
	return 0;
}
//-----------------------------------------------------------------
char* parseCommandLine(bool &openingEnabled)
{
	//reading commandline args (change to version 8.4 and 8.5 by Tomas Kubes)
	char *psqArg = "";
	cmdlineGameOutFile[0] = '\0'; //default: no output filename
	cmdlineLogMsgOutFile[0] = '\0'; //default: no output filename
	cmdlineLogPipeOutFile[0] = '\0'; //default: no output filename

	for(int i=1; i < __argc; i++){
		char *arg = __argv[i];
		if(!strcmp(arg, "-p") && __argc > i+2){
			cmdLineGame=true;
			strncpy(players[0].brain, __argv[++i], sizeof(players[0].brain)-1);
			strncpy(players[1].brain, __argv[++i], sizeof(players[0].brain)-1);
		}
		else if(!strcmp(arg, "-a") && __argc > i+1){ //processor affinity
			if(!sscanf(__argv[++i], "%d", &affinity))
			{
				msg("Unknown affinity %s", __argv[i]);
			}
		}
		else if(!strcmp(arg, "-tmppsq")){ //continous generating of psq files on client in network tournament
			tmpPsq = true;
		}
		else if(!strcmp(arg, "-h") || !strcmp(arg, "-help")){//help
			msg("Please see piskvork.txt for command line options.");
		}
		else if(!strcmp(arg, "-rule") && __argc > i+1){ //rule
			int exactFiveTmp;
			int exactFiveParsed = sscanf(__argv[++i], "%d", &exactFiveTmp);
			if(!exactFiveParsed || exactFiveTmp > 1 || exactFiveTmp < 0)
			 msg("Unknown rule %s", __argv[i]);
			else
				exactFive = exactFiveTmp;
		}
		else if(!strcmp(arg, "-timematch") && __argc > i+1){ //time game
			int timeGameTmp;
			int timeGameParsed = sscanf(__argv[++i], "%d", &timeGameTmp);
			if(!timeGameParsed || timeGameTmp < 0)
			 msg("Unknown timematch %s", __argv[i]);
			else
			{
				players[0].timeGame = timeGameTmp;
				players[1].timeGame = timeGameTmp;
			}
		}
		else if(!strcmp(arg, "-timeturn") && __argc > i+1){ //time move
			int timeMoveTmp;
			int timeMoveParsed = sscanf(__argv[++i], "%d", &timeMoveTmp);
			if(!timeMoveParsed || timeMoveTmp < 0)
			 msg("Unknown timeturn %s", __argv[i]);
			else
			{
				players[0].timeMove = timeMoveTmp;
				players[1].timeMove = timeMoveTmp;
			}
		}
		else if(!strcmp(arg, "-memory") && __argc > i+1){ //time move
			int memoryTmp;
			int memoryParsed = sscanf(__argv[++i], "%d", &memoryTmp);
			if(!memoryParsed || memoryTmp < 0)
			 msg("Unknown memory %s", __argv[i]);
			else
			{
				maxMemory = memoryTmp;
				//players[0].memory = memoryTmp;   
				//players[1].memory = memoryTmp;   		 
			}
		}
		else if(!strcmp(arg, "-opening") && __argc > i+1){ //time move
			int openingTmp;
			int openingParsed = sscanf(__argv[++i], "%d", &openingTmp);
			if(!openingParsed || openingTmp < -1 || openingTmp > Nopening)
			 msg("Unknown opening %s", __argv[i]);
			else
			{
				if(openingTmp >= 0)
				 opening = openingTmp;
				openingEnabled = openingTmp >= 0;
			}
		}
		else if(!strcmp(arg, "-logmsg") && __argc > i+1){ //msg outfile -  file to save game
			lstrcpyn(cmdlineLogMsgOutFile, __argv[++i], sizeA(cmdlineLogMsgOutFile));
		}
		else if(!strcmp(arg, "-logpipe") && __argc > i+1){ //pipe outfile -  file to save game
			lstrcpyn(cmdlineLogPipeOutFile, __argv[++i], sizeA(cmdlineLogPipeOutFile));
		}
		else if(!strcmp(arg, "-outfile") && __argc > i+1){ //game outfile -  file to save game
			lstrcpyn(cmdlineGameOutFile, __argv[++i], sizeA(cmdlineGameOutFile));
		}
		else if(!strcmp(arg, "-outfileformat") && __argc > i+1){ //outfileformat - format of file to save game
			int cmdlineGameOutFileFormatTmp;
			int cmdlineGameOutFileFormatTmpParsed = sscanf(__argv[++i], "%d", &cmdlineGameOutFileFormatTmp);
			if(!cmdlineGameOutFileFormatTmpParsed || cmdlineGameOutFileFormatTmp < 1 || cmdlineGameOutFileFormatTmp > 2)
			 msg("Unknown outfileformat %s, please provide 1 for *.psq, 2 for *.rec", __argv[i]);
			else
			{
				cmdlineGameOutFileFormat = cmdlineGameOutFileFormatTmp;
			}
		}
		else if(arg[0]!='-' || strchr(arg, '.')){
			psqArg=arg; //*.psq association
		}
		else{
			msg("Unknown switch %s", arg);
		}
	}
	return psqArg;
}
//-----------------------------------------------------------------
int pascal WinMain(HINSTANCE hInstance, HINSTANCE hPrevInst, LPSTR, int cmdShow)
{
	int i;
	WNDCLASS wc;
	MSG mesg;
	OSVERSIONINFO v;

	v.dwOSVersionInfoSize= sizeof(OSVERSIONINFO);
	GetVersionEx(&v);
	isWin9X= v.dwPlatformId==VER_PLATFORM_WIN32_WINDOWS;
	if(GetProcAddress(GetModuleHandle("comctl32.dll"), "DllGetVersion")){
		INITCOMMONCONTROLSEX iccs;
		iccs.dwSize= sizeof(INITCOMMONCONTROLSEX);
		iccs.dwICC= ICC_BAR_CLASSES;
		InitCommonControlsEx(&iccs);
	}
	else{
		InitCommonControls();
	}
	inst=hInstance;
	//temporary file names for old AI interface
	GetTempPath(sizeof(tempDir), tempDir);
	sprintf(strchr(tempDir, 0), "psqp%d\\", GetCurrentProcessId());
	sprintf(players[0].temp, "%sunz0\\", tempDir);
	sprintf(players[1].temp, "%sunz1\\", tempDir);
	sprintf(TahDat, "%sTah.dat", tempDir);
	sprintf(PlochaDat, "%sPlocha.dat", tempDir);
	sprintf(TimeOutsDat, "%sTimeOuts.dat", tempDir);
	sprintf(InfoDat, "%sInfo.dat", tempDir);
	sprintf(MsgDat, "%smsg.dat", tempDir);

	//read settings
	players[0].isComp=1;
	players[0].timeGame= 180;
	players[0].timeMove= 5000;
	players[1].timeGame= 600;
	players[1].timeMove= 60000;
	readini();
	initLang();
	//register window class
	wc.style= CS_OWNDC|CS_DBLCLKS;
	wc.lpfnWndProc= MainWndProc;
	wc.cbClsExtra= 0;
	wc.cbWndExtra= 0;
	wc.hInstance= hInstance;
	wc.hIcon= LoadIcon(hInstance, MAKEINTRESOURCE(1));
	wc.hCursor= LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground= NULL;
	wc.lpszMenuName= 0;
	wc.lpszClassName= "Piskvork";
	if(!hPrevInst && !RegisterClass(&wc)) return 11;
	//create main window
	int sw, sh;
	sw=GetSystemMetrics(SM_CXSCREEN);
	sh=GetSystemMetrics(SM_CYSCREEN);
	aminmax(mainPos.x, -5, sw-300);
	aminmax(mainPos.y, -5, sh-200);
	hWin = CreateWindow("Piskvork", "",
		WS_OVERLAPPEDWINDOW-WS_THICKFRAME-WS_MAXIMIZEBOX, mainPos.x, mainPos.y,
		0, 0, NULL, NULL, hInstance, NULL);
	if(!hWin) return 12;
	InitializeCriticalSection(&drawLock);
	dc= GetDC(hWin);
	//create toolbar
	int n=sizeA(tbb);
	for(i=0; i<sizeA(tbb); i++){
		if(tbb[i].fsStyle==TBSTYLE_SEP) n--;
	}
	toolbar = CreateToolbarEx(hWin,
		WS_CHILD|TBSTYLE_TOOLTIPS|TBSTYLE_FLAT, 1000, n,
		inst, 11, tbb, Ntool,
		16, 16, 16, 16, sizeof(TBBUTTON));
	TBSAVEPARAMS sp;
	sp.hkr=HKEY_CURRENT_USER;
	sp.pszSubKey=subkey;
	sp.pszValueName="toolbar";
	SendMessage(toolbar, TB_SAVERESTORE, FALSE, (LPARAM)&sp);
	if(toolBarVisible){
		ShowWindow(toolbar, SW_SHOW);
		UpdateWindow(toolbar);
		mtop=toolBarH;
	}
	//load skin
	bmpdc= CreateCompatibleDC(dc);
	loadSkin(LoadBitmap(hInstance, MAKEINTRESOURCE(10)));
	if(!*fnskin){
		getExeDir(fnskin, "skins\\");
	}
	else{
		loadSkin();
	}
	CreateDirectory(tempDir, 0);
	accelChanged();
	langChanged();
	InitializeCriticalSection(&infoLock);
	InitializeCriticalSection(&timerLock);
	InitializeCriticalSection(&threadsLock);
	InitializeCriticalSection(&netLock);
	DWORD threadId;
	thread= CreateThread(0, 0, threadLoop, 0, CREATE_SUSPENDED, &threadId);
	players[0].brainChanged();
	players[1].brainChanged();
	for(i=0; i<Mplayer; i++){
		Tclient *c=&clients[i];
		c->socket=INVALID_SOCKET;
		c->thread=0;
		c->player[0]=-1;
	}
	ShowWindow(hWin, cmdShow);
	initOpeningTab();

	bool openingEnabled = true;
	char *psqArg = parseCommandLine(openingEnabled);

	if(cmdLineGame){
		players[0].isComp=players[1].isComp=1;
		players[0].brainChanged();
		players[1].brainChanged();
		newGame(0, openingEnabled);
		resume();
	}
	else{
		newGame(!players[1].isComp, false);
		openPsq(psqArg);
	}

	while(GetMessage(&mesg, NULL, 0, 0)==TRUE)
	 if(mesg.hwnd==msgWnd || !TranslateAccelerator(hWin, haccel, &mesg)){
		 if(!logDlg || !IsDialogMessage(logDlg, &mesg))
		 if(!msgDlg || !IsDialogMessage(msgDlg, &mesg))
		 if(!resultDlg || !IsDialogMessage(resultDlg, &mesg)){
			 TranslateMessage(&mesg);
			 DispatchMessage(&mesg);
		 }
	 }

	delDir(tempDir, true);
	FreeLibrary(psapi);
	DeleteDC(bmpdc);
	DeleteObject(bm);
	DestroyAcceleratorTable(haccel);
	shutOpeningTab();
	delete[] board;
	delete[] turTable;
	delete[] turCells;
	DeleteCriticalSection(&drawLock);
	DeleteCriticalSection(&netLock);
	DeleteCriticalSection(&threadsLock);
	DeleteCriticalSection(&timerLock);
	DeleteCriticalSection(&infoLock);
	cleanTemp();
	return 0;
}
//-----------------------------------------------------------------
