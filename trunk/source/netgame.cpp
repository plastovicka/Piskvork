/*
	(C) 2000-2015  Petr Lastovicka
	(C) 2015  Tianyi Hao

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
	*/
#include "hdr.h"
#pragma hdrstop
#include "piskvork.h"

#define NETGAME_VERSION 2

enum{
	C_INIT1=147, C_INIT2=132, C_INIT_DENY=226, C_BUSY=235,
	C_INFO=193, C_INFO_OK=177, C_MSG=238, C_END=249,
	C_UNDO_REQUEST=230, C_UNDO_YES=243, C_UNDO_NO=246,
	C_NEW_REQUEST=198, C_NEW_YES=225, C_NEW_NO=218,
};

bool isListening, isNetGame;
int netGameVersion, undoRequest;
HANDLE netGameThread;
sockaddr_in netGameIP;

struct TnetGameSettings {
	char width, height, begin, cont, rule5;
};
//---------------------------------------------------------------
void newNetGame(LPARAM lP)
{
	TnetGameSettings *b= (TnetGameSettings*)lP;
	killBrains();
	players[0].isComp= players[1].isComp= 0;
	width=b->width;
	height=b->height;
	newGame(b->begin ? 1-NET_PLAYER : NET_PLAYER, false);
	if(ruleFive!=b->rule5){
		ruleFive=b->rule5;
		wrLog(b->rule5==2 ? lng(660, "rule renju") : (b->rule5 ? lng(655, "Exactly five in a row win") : lng(654, "Five or more in a row win")));
	}
	if(continuous!=b->cont){
		continuous=b->cont;
		wrLog(b->cont ? lng(657, "Continuous game") : lng(656, "Single game"));
	}
	wrLog(b->begin ? lng(872, "You begin") : lng(873, "The other player begins"));
	resetScore();
}

void netGameDone()
{
	if(!netGameThread) return;
	closesocket(sock);
	sock=INVALID_SOCKET;
	WSACleanup();
	wrLog(lng(869, "Disconnected"));
	CloseHandle(netGameThread);
	netGameThread=0;
	isNetGame=false;
	printLevel1();
}

void postUndo()
{
	PostMessage(hWin, WM_COMMAND, 990, 0);
	while(undoRequest) Sleep(50);
}

void postNewGame()
{
	PostMessage(hWin, WM_COMMAND, 989, 0);
	while(undoRequest) Sleep(50);
}

DWORD WINAPI netGameLoop(void *param)
{
	int len, c, x, y;
	Psquare p;
	TnetGameSettings *b;
	char *m, *u, *a;
	hostent *h;
	char buf[256];

	if(!param){
		buf[0]=(BYTE)C_INIT1;
		buf[1]=NETGAME_VERSION;
		wr(buf, 2);
		c=rd1();
		if(c!=C_INIT2) goto le;
		c=rd1();
		if(c<1) goto le;
		netGameVersion=c;
		SetForegroundWindow(hWin);
		h= gethostbyaddr((char*)&netGameIP.sin_addr, 4, netGameIP.sin_family);
		a= inet_ntoa(netGameIP.sin_addr);
		if(msg1(MB_YESNO|MB_ICONQUESTION,
			lng(868, "Do you want to play with %s [%s] ?"),
			(h && h->h_name) ? h->h_name : "???", a ? a : "???")!=IDYES){
			wr1(C_INIT_DENY);
			goto le;
		}
		buf[0]=(BYTE)C_INFO;
		buf[1]=sizeof(TnetGameSettings);
		b= (TnetGameSettings*)(buf+2);
		b->width=(char)width;
		b->height=(char)height;
		b->begin= (player==1);
		b->rule5=(char)ruleFive;
		b->cont=(char)continuous;
		if(netGameVersion<2) b->rule5=b->cont=0;
		wr(buf, 2+sizeof(TnetGameSettings));
		if(rd1()!=C_INFO_OK) goto le;
		b->begin= !b->begin;
	}
	else{
		buf[0]=(BYTE)C_INIT2;
		buf[1]=NETGAME_VERSION;
		wr(buf, 2);
		c=rd1();
		if(c==C_BUSY) wrLog(lng(874, "The other player is already playing with someone else"));
		if(c!=C_INIT1) goto le;
		c=rd1();
		if(c<1) goto le;
		netGameVersion=c;
		wrLog(lng(871, "Connection established. Waiting for response..."));
		c=rd1();
		if(c==C_INIT_DENY) wrLog(lng(870, "The other player doesn't want to play with you !"));
		if(c!=C_INFO) goto le;
		len=rd1();
		b= (TnetGameSettings*)buf;
		memset(buf, 0, sizeof(TnetGameSettings));
		if(len<2 || rd(buf, len)<0) goto le;
		wr1(C_INFO_OK);
	}
	SendMessage(hWin, WM_COMMAND, 992, (LPARAM)b);
	undoRequest=0;

	for(;;){
		x=rd1();
		switch(x){
			case C_MSG: //message
				len=rd2();
				if(len<=0) goto le;
				u=new char[2*len];
				if(rd(u, 2*len)>=0){
					m=new char[len+1];
					WideCharToMultiByte(CP_ACP, 0, (WCHAR*)u, len, m, len, 0, 0);
					m[len]='\0';
					wrLog("--->  %s", m);
					delete[] m;
					SetWindowPos(logDlg, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE|SWP_ASYNCWINDOWPOS|SWP_NOACTIVATE|SWP_SHOWWINDOW);
				}
				delete[] u;
				break;
			case C_UNDO_REQUEST:
				y=rd2();
				if(y<=0 || y>moves) goto le;
				if(undoRequest){
					//both players pressed undo simultaneously
					if(undoRequest<0) undoRequest=y;
					amax(undoRequest, y);
					postUndo();
				}
				else{
					c=msg1(MB_YESNO|MB_ICONQUESTION,
						lng(876, "The other player wants to UNDO the last move.\r\nDo you agree ?"));
					if(c==IDYES){
						undoRequest=y;
						wr1(C_UNDO_YES);
						postUndo();
					}
					else{
						wr1(C_UNDO_NO);
					}
				}
				break;
			case C_NEW_REQUEST:
				if(undoRequest){
					if(undoRequest<0){
						//both players pressed NewGame simultaneously
						postNewGame();
					}
					else{
						postUndo();
					}
				}
				else{
					c=msg1(MB_YESNO|MB_ICONQUESTION,
						lng(877, "The other player wants to start a NEW game.\r\nDo you agree ?"));
					if(c==IDYES){
						undoRequest=-1;
						wr1(C_NEW_YES);
						postNewGame();
					}
					else{
						wr1(C_NEW_NO);
					}
				}
				break;
			case C_UNDO_YES:
				if(undoRequest<=0) goto le;
				postUndo();
				break;
			case C_UNDO_NO:
				if(undoRequest>0){
					msglng(878, "Sorry, the other player does not allow to UNDO your move.");
					undoRequest=0;
				}
				break;
			case C_NEW_YES:
				if(undoRequest>=0) goto le;
				postNewGame();
				break;
			case C_NEW_NO:
				if(undoRequest<0){
					msglng(879, "Sorry, the other player does not allow to start a NEW game.");
					undoRequest=0;
				}
				break;
			default: //move or error
				if(x<0 || x>=width){
					show(logDlg);
					goto le;
				}
				while(finished && (getTickCount()-lastTick<5000 || saveLock)){
					Sleep(200);
				}
				if(finished) SendMessage(hWin, WM_COMMAND, 101, 0);
				y=rd1();
				if(y<0 || y>=height) goto le;
				p=Square(x, y);
				p->time=rd4();
				PostMessage(hWin, WM_COMMAND, 991, (LPARAM)p);
		}
	}
le:
	EnterCriticalSection(&netLock);
	netGameDone();
	LeaveCriticalSection(&netLock);
	return 0;
}

DWORD WINAPI listenNetGame(void *)
{
	sockaddr_in sa;
	SOCKET sock_c;
	int sl;
	bool b;

	wrLog(lng(866, "Waiting for the other player"));
	for(;;){
		sl= sizeof(sa);
		if((sock_c= accept(sock_l, (sockaddr *)&sa, &sl))==INVALID_SOCKET){
			wrLog(lng(867, "Finished listening for requests from the internet"));
			break;
		}
		EnterCriticalSection(&netLock);
		b=isNetGame;
		isNetGame=true;
		LeaveCriticalSection(&netLock);
		if(b){
			wr1(sock_c, C_BUSY);
			Sleep(1000);
			closesocket(sock_c);
		}
		else{
			sock=sock_c;
			netGameIP=sa;
			initWSA();
			DWORD id;
			ResumeThread(netGameThread=CreateThread(0, 0, netGameLoop, 0, CREATE_SUSPENDED, &id));
			stopListen();
			break;
		}
	}
	WSACleanup();
	isListening=false;
	return 0;
}

int netGameStart()
{
	int err=3;
	EnterCriticalSection(&netLock);
	if(isNetGame) err=4;
	else if(servername[0]) err= startConnection();
	if(!err){
		//connection succeeded
		isNetGame=true;
		DWORD id;
		ResumeThread(netGameThread=CreateThread(0, 0, netGameLoop, (void*)1, CREATE_SUSPENDED, &id));
	}
	LeaveCriticalSection(&netLock);
	if(err==3){
		//not connected, try listening
		err= startListen();
		if(!err){
			isListening=true;
			DWORD id;
			CloseHandle(CreateThread(0, 0, listenNetGame, 0, 0, &id));
		}
	}
	return err;
}

void netGameEnd()
{
	if(!isNetGame){
		if(isListening) stopListen();
		return;
	}
	wr1(C_END);
	undoRequest=0;
	for(int i=0; i<50; i++){
		Sleep(200);
		if(!isNetGame) return;
	}
	EnterCriticalSection(&netLock);
	if(netGameThread){
		wrLog("The other computer is not responding");
		netGameDone();
		TerminateThread(netGameThread, 1);
	}
	LeaveCriticalSection(&netLock);
}

void netGameMove(Psquare p)
{
	if(player!=NET_PLAYER){
		char buf[6];
		buf[0]=char(p->x-1);
		buf[1]=char(p->y-1);
		buf[2]=char(p->time>>24);
		buf[3]=char(p->time>>16);
		buf[4]=char(p->time>>8);
		buf[5]=char(p->time);
		wr(buf, 6);
	}
}

void oldVersion()
{
	msglng(880, "Sorry, the other player has older version of Piskvork.exe");
}

void netGameUndo()
{
	int i;
	char buf[3];

	i=moves-1;
	if(player!=NET_PLAYER) i--;
	if(!undoRequest && i>0){
		if(netGameVersion<2){
			oldVersion();
		}
		else{
			buf[0]=(BYTE)C_UNDO_REQUEST;
			undoRequest=i;
			buf[1]=char(i>>8);
			buf[2]=char(i);
			wr(buf, 3);
			wrLog(lng(881, "Sending undo request..."));
		}
	}
}

void netGameNew()
{
	if(!undoRequest && moves>0){
		if(netGameVersion<2){
			oldVersion();
		}
		else{
			undoRequest=-1;
			wr1(C_NEW_REQUEST);
			wrLog(lng(882, "Sending a new game request..."));
		}
	}
}

void netGameMsg()
{
	int len;
	char *buf, *u;

	len= GetWindowTextLength(msgWnd);
	if(len){
		buf= new char[len+1];
		GetWindowText(msgWnd, buf, len+1);
		u= new char[len*2+4];
		MultiByteToWideChar(CP_ACP, 0, buf, len, (WCHAR*)(u+4), len);
		u[1]=(BYTE)C_MSG;
		u[2]=char(len>>8);
		u[3]=char(len);
		wr(u+1, len*2+3);
		delete[] u;
		wrLog("<  %s", buf);
		delete[] buf;
		SetWindowText(msgWnd, "");
		SetFocus(msgWnd);
	}
}
