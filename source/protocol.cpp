/*
	(C) Petr Lastovicka, Tianyi Hao

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
#include <assert.h>
#include <tlhelp32.h>
#include "piskvork.h"

int errDelay=20000;
int debugPipe=0;
static HANDLE startEvent, start2Event;
CRITICAL_SECTION threadsLock, infoLock;

const DWORD priorCnst[]={IDLE_PRIORITY_CLASS, 16384,
NORMAL_PRIORITY_CLASS, 32768, HIGH_PRIORITY_CLASS};

//-----------------------------------------------------------------
void getMsgFn(char *fn)
{
	if(strlen(cmdlineLogMsgOutFile) > 0){
		strcpy(fn, cmdlineLogMsgOutFile);
		return;
	}
	else if(isClient){
		strcpy(fn, tempDir);
	}
	else{
		getTurDir(fn);
	}
	strcat(fn, fnMsg);
}

void vwrMessage(char *fmt, va_list va)
{
	FILE *f;
	TfileName fn;

	if(!isServer) vwrLog(fmt, va);
	if((!turNplayers || !turLogMsg) && strlen(cmdlineLogMsgOutFile) == 0) return;
	getMsgFn(fn);
	f=fopen(fn, "ab");
	if(f){
		vfprintf(f, fmt, va);
		fputc('\r', f);
		fputc('\n', f);
		fclose(f);
	}
}

void wrMessage(char *fmt, ...)
{
	va_list va;
	va_start(va, fmt);
	vwrMessage(fmt, va);
	va_end(va);
}

void Tplayer::wrMsg(char *s)
{
	if(players[0].isComp && players[1].isComp){
		wrMessage("%2d) %s: %s", moves, getFilePart(), s);
	}
	else{
		wrMessage("%s", s);
	}
}

void errMsg(char *text, ...)
{
	int i1, i2;
	va_list va;

	va_start(va, text);
	if(turNplayers){
		if(!disableScore){
			disableScore=true;
			turTable[i1=players[player].turPlayerId].errors++;
			turTable[i2=players[1-player].turPlayerId].winsE++;
			getCell(i1, i2)->error++;
			win7TaskbarProgress.SetProgressState(hWin, TBPF_ERROR);
			turAddTime();
		}
		vwrMessage(text, va);
		show(logDlg);
		turDelay+=errDelay;
		//Tomas Kubes: I didn' find error message code list, so i choose -2 to mean other code
		finishGame(-2);
		turDelay-=errDelay;
	}
	else if(cmdLineGame){
		fprintf(stderr, text);
		finishGame(-2);
	}
	else{
		softPause();
		vmsg(title, text, MB_OK|MB_ICONERROR, va);
	}
	va_end(va);
}

bool Tplayer::notRunning()
{
	DWORD e;
	return !process || !GetExitCodeProcess(process, &e) || e!=STILL_ACTIVE;
}

#define fort(l) for(ThreadItem *item=(l).nxt;\
	item!=&(l); item=item->nxt)

void ThreadItem::append(ThreadItem *item)
{
	prv->nxt= item;
	item->prv= prv;
	prv= item;
	item->nxt= this;
}

void Tplayer::openThreads()
{
	if(!process) return;
	static FARPROC pOpenThread;
	if(!pOpenThread){
		pOpenThread= GetProcAddress(GetModuleHandle("kernel32.dll"), "OpenThread");
		if(!pOpenThread) return;
	}
	THREADENTRY32 te;
	te.dwSize = sizeof(THREADENTRY32);
	HANDLE h = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
	if(h!=(HANDLE)-1){
		if(Thread32First(h, &te)){
			do{
				if(te.th32OwnerProcessID==processId){
					ThreadItem *t= new ThreadItem;
					t->h= ((HANDLE(WINAPI*)(DWORD, BOOL, DWORD))pOpenThread)(THREAD_SUSPEND_RESUME, 0, te.th32ThreadID);
					threads.append(t);
				}
			} while(Thread32Next(h, &te));
		}
		CloseHandle(h);
	}
}

void setPriority(HANDLE process)
{
	if(process){
		aminmax(priority, 0, sizeA(priorCnst));
		SetPriorityClass(process, priorCnst[priority]);
	}
}

int Tplayer::createProcess(bool pipe, STARTUPINFO &si)
{
	//add quotes
	char *exe=brain2;
	char buf[sizeof(TfileName)+2];
	if(exe[0]!='"'){
		buf[0]='"';
		size_t len = strlen(brain2);
		memcpy(buf+1, exe, len);
		buf[len+1]='"';
		buf[len+2]=0;
		exe=buf;
	}
	//start process
	si.cb=sizeof(STARTUPINFO);
	si.wShowWindow = SW_HIDE;
	si.dwFlags |= STARTF_USESHOWWINDOW | STARTF_FORCEOFFFEEDBACK;
	PROCESS_INFORMATION pi;
	UINT m= SetErrorMode((ignoreErrors || isClient) ? SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX : 0);
	if(!CreateProcess(0, exe, 0, 0, pipe ? TRUE : FALSE,
		0, 0, tempDir, &si, &pi)){
		DWORD err= GetLastError();
		if(err==ERROR_FILE_NOT_FOUND) errMsg(lng(739, "File not found: %s"), brain2);
		else errMsg(lng(806, "Can't create process (error %d)"), err);
		return 1;
	}
	SetErrorMode(m);
	CloseHandle(pi.hThread);
	process=pi.hProcess;
	processId=pi.dwProcessId;
	setPriority(process);
	//setting affinity (change to version 8.4 by Tomas Kubes)
	if(affinity != -1){
		BOOL res = SetProcessAffinityMask(process, (DWORD_PTR)affinity);
		if(res == 0){
			//Error setting affinity mask!!
			errMsg(lng(820, "Error setting affinity mask! (error %d)"), GetLastError());
			return 1;
		}
	}
	return 0;
}


//create pipes and start AI process
void Tplayer::startPipeAI()
{
	HANDLE h, hChildStdOut, hChildStdIn, hChildStdErr;

	// Set up the security attributes struct
	SECURITY_ATTRIBUTES sa;
	sa.nLength= sizeof(SECURITY_ATTRIBUTES);
	sa.lpSecurityDescriptor = NULL;
	sa.bInheritHandle = TRUE;

	// Create the AI output pipe
	if(!CreatePipe(&h, &hChildStdOut, &sa, 0)) errMsg("CreatePipe failed");

	if(!DuplicateHandle(GetCurrentProcess(), h,
		GetCurrentProcess(), &pipeFromAI,
		0, FALSE, DUPLICATE_SAME_ACCESS)) errMsg("DuplicateHandle failed");
	CloseHandle(h);

	// Create the AI input pipe
	if(!CreatePipe(&hChildStdIn, &h, &sa, width*height*10)) errMsg("CreatePipe failed");

	if(!DuplicateHandle(GetCurrentProcess(), h,
		GetCurrentProcess(), &pipeToAI,
		0, FALSE, DUPLICATE_SAME_ACCESS)) errMsg("DuplicateHandle failed");
	CloseHandle(h);

	if(!DuplicateHandle(GetCurrentProcess(), hChildStdOut,
		GetCurrentProcess(), &hChildStdErr,
		0, TRUE, DUPLICATE_SAME_ACCESS)) errMsg("DuplicateHandle failed");

	STARTUPINFO si;
	memset(&si, 0, sizeof(si));
	si.dwFlags = STARTF_USESTDHANDLES;
	si.hStdInput  = hChildStdIn;
	si.hStdOutput = hChildStdOut;
	si.hStdError  = hChildStdErr;
	createProcess(true, si);
	CloseHandle(hChildStdIn);
	CloseHandle(hChildStdOut);
	CloseHandle(hChildStdErr);
	wrDebugLog("______Process started______");
}

DWORD WINAPI pipeThreadLoop(void *param)
{
	DEBUG_EVENT e;
	ThreadItem *t;
	Tplayer *Player= (Tplayer*)param;
	DWORD cont;
	bool done=false;

	//create brain's process
	Player->startPipeAI();
	ResetEvent(start2Event);
	SetEvent(startEvent);
	//wait until brain sends OK
	//(DebugActiveProcess must not be called immediately after CreateProcess)
	if(WaitForInputIdle(Player->process, 5000)==WAIT_FAILED) Sleep(800); else Sleep(200);
	WaitForSingleObject(start2Event, INFINITE);
	if(Player->notRunning()) return 1;

	if(!DebugActiveProcess(Player->processId)) return 2;
	do{
		if(!WaitForDebugEvent(&e, INFINITE)) return 3;
		cont=DBG_CONTINUE;
		EnterCriticalSection(&threadsLock);
		switch(e.dwDebugEventCode){
			case EXCEPTION_DEBUG_EVENT:
				if(e.u.Exception.ExceptionRecord.ExceptionCode!=STATUS_BREAKPOINT){
					cont=DBG_EXCEPTION_NOT_HANDLED;
				}
				break;
			case CREATE_PROCESS_DEBUG_EVENT:
				CloseHandle(e.u.CreateProcessInfo.hFile);
				if(isWin9X) CloseHandle(e.u.CreateProcessInfo.hProcess);
				t=new ThreadItem;
				t->h= e.u.CreateProcessInfo.hThread;
				t->id= e.dwThreadId;
				Player->threads.append(t);
				break;
			case CREATE_THREAD_DEBUG_EVENT:
				t=new ThreadItem;
				t->h= e.u.CreateThread.hThread;
				t->id= e.dwThreadId;
				Player->threads.append(t);
				break;
			case EXIT_THREAD_DEBUG_EVENT:
			{
				fort(Player->threads){
					if(item->id==e.dwThreadId){
						if(isWin9X) CloseHandle(item->h);
						delete item;
						break;
					}
				}
				break;
			}
			case EXIT_PROCESS_DEBUG_EVENT:
				while(Player->threads.nxt!=&Player->threads){
					ThreadItem *item= Player->threads.nxt;
					if(isWin9X) CloseHandle(item->h);
					delete item;
				}
				done=true;
				break;
			case LOAD_DLL_DEBUG_EVENT:
				CloseHandle(e.u.LoadDll.hFile);
				break;
		}
		LeaveCriticalSection(&threadsLock);
		ContinueDebugEvent(e.dwProcessId, e.dwThreadId, cont);
	} while(!done);
	return 0;
}

bool cmpExt(char *s, char *e)
{
	int len=(int)strlen(s)-4;
	return len>0 && s[len]=='.' && _stricmp(s+len+1, e)==0;
}

struct TchooseExe {
	char *exe;
	int n;
	char *zip;
	int eval;
	void find(const char *path);
};

void TchooseExe::find(const char *path)
{
	HANDLE h;
	int e;
	char *f, *s;
	WIN32_FIND_DATA fd;
	char oldDir[256];
	char name[MAX_PATH];

	GetCurrentDirectory(sizeof(oldDir), oldDir);
	SetCurrentDirectory(path);
	h = FindFirstFile("*", &fd);
	if(h!=INVALID_HANDLE_VALUE){
		do{
			if(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY){
				if(fd.cFileName[0]!='.') find(fd.cFileName);
				continue;
			}
			e=0;
			if(cmpExt(fd.cFileName, "exe")) e=1000;
			else if(cmpExt(fd.cFileName, "com")) e=900;
			strcpy(name, fd.cFileName);
			_strupr(name);
			if(!strcmp(name, "RENJU.BOOK")){
				TfileName buf;
				sprintf(buf, "%srenju.book", tempDir);
				CopyFile(name, buf, FALSE);
			}
			//remove extension
			s=strchr(name, 0)-4;
			if(s > name && *s == '.') *s = 0;
			//prefer 64 in file name on 64-bit OS
			if(strstr(name,"64")){
				if(is64bit) e+=170;
				else e-=170;
			}
			//prefer exe which has same name as zip
			if(!strcmp(name, zip)) e+=200;
			else if(strstr(name, zip)) e+=50;
			if(e>eval){
				eval=e;
				GetFullPathName(fd.cFileName, n, exe, &f);
			}
		} while(FindNextFile(h, &fd));
	}
	FindClose(h);
	SetCurrentDirectory(oldDir);
}

DWORD searchAI(const char *src, int n, char *dest, char **filePart)
{
	DWORD d;
	char *fp;
	TfileName fn;

	d=SearchPath(0, src, ".exe", n, dest, &fp);
	if(!d) d=SearchPath(0, src, ".zip", n, dest, &fp);
	if(!d && cutPath(src)==src && strlen(src)<sizeof(fn)-7){
		strcpy(fn, "pbrain-");
		strcpy(fn+7, src);
		d=SearchPath(0, fn, ".exe", n, dest, &fp);
		if(!d) d=SearchPath(0, fn, ".zip", n, dest, &fp);
	}
	if(filePart) *filePart=fp;
	return d;
}

int Tplayer::brainChanged()
{
	char *filePart, *workDir;
	DWORD d;
	TchooseExe che;
	TfileName buf;
	const int L=14;

	if(!brain[0]) strcpy(brain, myAI);
	brain2[0]=0;
	d=searchAI(brain, sizeof(buf)-L-1, buf+L, &filePart);
	if(d>=sizeof(buf)-L-1) return 14;
	if(d==0) return 11;
	if(cmpExt(filePart, "zip")){
		wrLog(lng(865, "Extracting %s"), filePart);
		//create temporary folder
		workDir=temp;
		CreateDirectory(workDir, 0);
		delDir(workDir, false);
		//run external unzip
		memcpy(buf, "unzip -qq -o \"", L);
		strcat(filePart, "\"");
		STARTUPINFO si;
		memset(&si, 0, sizeof(si));
		si.cb= sizeof(si);
		si.dwFlags= STARTF_USESHOWWINDOW;
		si.wShowWindow= SW_HIDE;
		PROCESS_INFORMATION pi;
		if(CreateProcess(0, buf, 0, 0, FALSE, NORMAL_PRIORITY_CLASS, 0, workDir, &si, &pi)==0){
			return 12;
		}
		CloseHandle(pi.hThread);
		WaitForSingleObject(pi.hProcess, 50000);
		CloseHandle(pi.hProcess);
		strchr(filePart, 0)[-1]=0;
		//find exe file in destination folder
		che.eval=-1;
		che.exe=brain2;
		che.n=sizeof(brain2);
		che.zip=filePart;
		strchr(che.zip, 0)[-4]=0;
		_strupr(filePart);
		che.find(workDir);
		if(che.eval<0) return 13;
	}
	else{
		strcpy(brain2, buf+L);
	}
	return 0;
}

char *Tplayer::getFilePart()
{
	return cutPath(brain);
}

//kill AI process and close pipes
void Tplayer::stopAI()
{
	if(pipeToAI){
		sendCommand("END");
		CloseHandle(pipeToAI);
		pipeToAI=0;
	}
	if(process){
		if(WaitForSingleObject(process, pipeFromAI ? 700 : 20)!=WAIT_OBJECT_0){
			TerminateProcess(process, 0);
		}
		CloseHandle(process);
		process=0;
	}
	if(pipeFromAI){
		CloseHandle(pipeFromAI);
		pipeFromAI=0;
	}
	if(dbgThread){
		WaitForSingleObject(dbgThread, 3000);
		CloseHandle(dbgThread);
		dbgThread=0;
	}
	mustSendBoard=true;
	DlastBoard=0;
}

void Tplayer::wrDebugLog(char *s)
{
	TfileName fn;
	char *t;
	FILE *f;

	if(((debugPipe && !turNplayers) || strlen(cmdlineLogPipeOutFile) > 0)&& *s){
		if(strlen(cmdlineLogPipeOutFile) > 0){
			strcpy(fn, cmdlineLogPipeOutFile);
			strcat(fn, cutPath(brain2));
			strcat(fn, ".log");
		}
		else{
			strcpy(fn, brain2);
			t=strchr(fn, 0);
			while(t>fn && *t!='.') t--;
			strcpy(t, ".log");
		}
		f=fopen(fn, "at");
		if(f){
			fputs(s, f);
			fputc('\n', f);
			fclose(f);
		}
	}
}

//read one line from the pipe
int Tplayer::readCommand(char *buf, int n)
{
	char *p;

	do{
		for(p=buf;;){
			DWORD r;
			if(!ReadFile(pipeFromAI, p, 1, &r, 0) || !r){
				*p=0;
				return 1;
			}
			if(*p=='\r' || *p=='\n') break;
			if(p<buf+n-3) p++;
		}
		*p=0;
		wrDebugLog(buf);
		if(!_strnicmp(buf, "debug ", 6)){
			if(logDebug) wrLog("%s: %s", getFilePart(), buf+6);
			p=buf;
		}
		if(!_strnicmp(buf, "message ", 8)){
			if(logMessage || strlen(cmdlineLogMsgOutFile) > 0) wrMsg(buf+8);
			p=buf;
		}
	} while(p==buf);
	return 0;
}

//write one line to the pipe
void Tplayer::sendCommand(char *text, ...)
{
	if(!pipeToAI) return;
	char buf[256];
	va_list v;
	va_start(v, text);
	int i=vsprintf(buf, text, v);
	buf[i++]='\r';
	buf[i++]='\n';
	buf[i]=0;
	DWORD w;
	WriteFile(pipeToAI, buf, i, &w, 0);
	if(strncmp(buf, "INFO eval", 9)){
		buf[i-2]=0;
		wrDebugLog(buf);
	}
	va_end(v);
}

void Tplayer::sendInfoCmd(char *key, int value)
{
	if(infoDatFile){
		fprintf(infoDatFile, "%s %d\n", key, value);
	}
	else{
		sendCommand("INFO %s %d", key, value);
	}
}

void Tplayer::sendInfoCmd(char *key, char *value)
{
	if(infoDatFile){
		fprintf(infoDatFile, "%s %s\n", key, value);
	}
	else{
		sendCommand("INFO %s %s", key, value);
	}
}

int Tplayer::sendInfo(int mask)
{
	int result=0;

	EnterCriticalSection(&infoLock);
	infoDatFile=0;
	if(!pipeToAI){
		if(this!=&players[player]){ result=9; goto lend; }
		infoDatFile= fopen(InfoDat, "wt");
		if(!infoDatFile){ result=7; goto lend; }
		mask=INFO_ALL;
	}
	if(mask & INFO_MEMORY) sendInfoCmd("max_memory", maxMemory > 2047 ? 0x7FFFFFFF : maxMemory<<20);
	if(mask & INFO_TIMEGAME) sendInfoCmd("timeout_match", timeGame*1000);
	if(mask & INFO_TIMEMOVE){
		sendInfoCmd("timeout_turn", timeMove);
		mustSendInfo=false;
	}
	if(mask & INFO_GAMETYPE){
		int i= players[0].isComp && players[1].isComp;
		if(turNplayers) i= isClient ? 3 : 2;
		sendInfoCmd("game_type", i);
	}
	if((mask & INFO_RULE)){
		sendInfoCmd("rule", (ruleFive != 2 ? ruleFive : 4) | (continuous<<1));
	}
	if(mask & INFO_TIMELEFT) sendInfoCmd("time_left", timeLeft());
	if((mask & INFO_DIR) && dataDir[0] && !isClient){
		sendInfoCmd("folder", dataDir);
	}

	if(infoDatFile){
		if(fclose(infoDatFile)) result=8;
	}
lend:
	LeaveCriticalSection(&infoLock);
	return result;
}

//write the board and info to files (only for FILE protocol)
int Tplayer::storeBoard()
{
	FILE *f= fopen(PlochaDat, "wt");
	if(!f) return 1;
	for(int y=0; y<height; y++){
		for(int x=0; x<width; x++){
			Psquare p=Square(x, y);
			fputc(p->blocked() ? '#' : "-xo-"[p->z], f);
		}
		fputc('\n', f);
	}
	if(fclose(f)) return 2;

	f= fopen(TahDat, "wt");
	if(!f) return 3;
	fprintf(f, "%c\n", player ? 'o' : 'x');
	if(fclose(f)) return 4;

	timeInit=0;
	f= fopen(TimeOutsDat, "wt");
	if(!f) return 5;
	fprintf(f, "%d\n%d\n", timeMove/1000, timeLeft());
	if(fclose(f)) return 6;

	f= fopen(MsgDat, "w");
	fclose(f);

	return sendInfo();
}

//send the last move to AI, and wait for the AI move
//runs in a worker thread
int Tplayer::move()
{
	char buf[8000];
	int x, y, err, i;
	DWORD threadId;
	Psquare p;
	FILE *f;
	Txyp *t;
	char *s;
	bool restarted;

	timeInit=-1;
	if(!isComp) return 0;
	if(finished || terminateAI){
		softPause();
		return 1;
	}
	if(process && notRunning()){
		stopAI();
	}
	if(!brain2[0]){
		err=brainChanged();
		if(err==11) errMsg(lng(739, "File not found: %s"), brain);
		if(err==12) errMsg(lng(811, "Can't run unzip"));
		if(err==13) errMsg(lng(812, "Cannot extract files from %s"), brain);
		if(err==14) errMsg("File path is too long");
		if(err) return err;
	}
	if(!_strnicmp(cutPath(brain2), "pbrain-", 7)){
		if(mustSendBoard && moves<=DlastBoard+1 && moves>1){
			for(i=moves-2, p=lastMove->nxt; i>=0; i--, p=p->nxt){
				t=&lastBoard[i];
				if(t->x!=p->x || t->y!=p->y || t->p!=p->winLineStart) goto l1; //board has changed
			}
			for(i=DlastBoard-1; i>=moves-1; i--){
				if(lastBoard[i].p) goto l1; //TAKEBACK cannot remove winning lines
			}
			for(i=DlastBoard-1; i>=moves-1; i--){
				sendCommand("TAKEBACK %d,%d", lastBoard[i].x-1, lastBoard[i].y-1);
				readCommand(buf, sizeof(buf));
				if(terminateAI) return 9; //time out
				if(_stricmp(buf, "OK")) goto l1; //not supported
			}
			mustSendBoard=false;
		}
		if(mustSendBoard){
		l1:
			if(!process && !pipeToAI && !dbgThread){
				//create process and pipes
				if(!debugAI || !suspendAI){
					startPipeAI();
				}
				else{
					ResetEvent(startEvent);
					dbgThread=CreateThread(0, 0, pipeThreadLoop, this, 0, &threadId);
					WaitForSingleObject(startEvent, INFINITE);
				}
				if(!process){
					SetEvent(start2Event);
					stopAI();
					return 2;
				}
				if(width==height)
					sendCommand("START %d", width);
				else
					sendCommand("RECTSTART %d,%d", width, height);
				restarted=false;
			}
			else{
				sendCommand("RESTART");
				restarted=true;
			}
			readCommand(buf, sizeof(buf));
			SetEvent(start2Event);
			if(terminateAI) return 9;
			if(_stricmp(buf, "OK")){
				if(restarted){ stopAI(); goto l1; }
				if((width!=20 || height!=20) && !turNplayers &&
					(!_strnicmp(buf, "UNKNOWN", 7) && width!=height ||
					!_strnicmp(buf, "ERROR", 5) && strstr(buf, "20"))){
					softPause();
					PostMessage(hWin, WM_COMMAND, 993, 0);
				}
				else{
					errMsg(lng(808, "%s did not answer OK to the START command\r\n%s"),
						getFilePart(), buf);
					stopAI();
				}
				return 2;
			}
			mustSendBoard=false;
			timeInit= getTickCount()-lastTick;
			sendInfo();
			if(moves==0){
				sendCommand("BEGIN");
			}
			else if(moves==1){
				sendCommand("TURN %d,%d", lastMove->x-1, lastMove->y-1);
			}
			else{
				EnterCriticalSection(&infoLock);
				sendCommand("BOARD");
				for(p=lastMove; p->nxt; p=p->nxt);
				for(;;){
					assert(p->z==1 || p->z==2);
					sendCommand("%d,%d,%d", p->x-1, p->y-1, p->blocked() ? 3 :
						(this==&players[0] ? p->z : 3-p->z));
					if(p==lastMove) break;
					p=p->pre;
				}
				sendCommand("DONE");
				LeaveCriticalSection(&infoLock);
			}
		}
		else{
			assert(moves>0);
			timeInit=0;
			if(mustSendInfo) sendInfo(INFO_TIMEMOVE);
			sendInfo(INFO_TIMELEFT);
			sendCommand("TURN %d,%d", lastMove->x-1, lastMove->y-1);
		}
		//wait for AI turn
		for(;;){
			readCommand(buf, sizeof(buf));
			if(terminateAI) return 9;
			if(_strnicmp(buf, "suggest ", 8)){
				if(sscanf(buf, "%d,%d", &x, &y)!=2){
					if(!buf[0] && notRunning()){
						errMsg(lng(818, "%s has exited unexpectedly !"), getFilePart());
					}
					else{
						errMsg(lng(804, "%s has not returned coordinates x,y\r\n%s"),
							getFilePart(), buf);
					}
					stopAI();
					return 3;
				}
				break;
			}
			sendCommand("PLAY %s", buf+8);
		}
		getMem();
	}
	else{
		//write the board to file plocha.dat
		if(storeBoard()){
			errMsg(lng(733, "The board cannot be written to %s"), PlochaDat);
			return 8;
		}
		//start process and wait until it is finished
		STARTUPINFO si;
		memset(&si, 0, sizeof(si));
		if(createProcess(false, si)) return 4;
		WaitForSingleObject(process, INFINITE);
		getMem();
		if(process){
			CloseHandle(process);
			process=0;
		}
		if(terminateAI) return 9;

		//read messages
		f= fopen(MsgDat, "rt");
		if(f){
			while(fgets(buf, sizeof(buf), f)){
				s=strchr(buf, '\n');
				if(s) *s=0;
				wrMsg(buf);
			}
			fclose(f);
		}
		//read result from file tah.dat
		f= fopen(TahDat, "r");
		if(!f){
			errMsg(lng(805, "%s has not created file %s"), getFilePart(), TahDat);
			return 5;
		}
		i=fscanf(f, "%d,%d", &x, &y);
		fclose(f);
		if(i!=2){
			errMsg(lng(809, "%s has not returned coordinates x,y"),
				getFilePart());
			return 6;
		}
	}
	err=0;
	EnterCriticalSection(&timerLock);
	if(finished){
		err=15; //timeout
	}
	else if(memory > (DWORD(maxMemory)<<20) && turNplayers){
		err=16;
	}
	else{
		//draw a square, increment moves, switch players
		if(!doMove(Square(x, y))){
			err=7;
		}
	}
	//remember the board
	delete[] lastBoard;
	if(moves==0 || lastMove->blocked() || err){
		DlastBoard=0;
		lastBoard=0;
	}
	else{
		lastBoard= new Txyp[DlastBoard=moves];
		for(i=moves-1, p=lastMove; i>=0; i--, p=p->nxt){
			t=&lastBoard[i];
			t->x= p->x;
			t->y= p->y;
			t->p= p->winLineStart;
		}
	}
	LeaveCriticalSection(&timerLock);
	if(err==16) errMsg(lng(813, "%s allocated %u kB of memory"),
		getFilePart(), memory>>10);
	if(err==7) errMsg(lng(807, "%s returned invalid move [%d,%d]"),
		getFilePart(), x, y);
	return err;
}

//worker thread main function
DWORD WINAPI threadLoop(LPVOID)
{
	startEvent= CreateEvent(0, TRUE, FALSE, 0);
	start2Event= CreateEvent(0, TRUE, FALSE, 0);
	//infinite loop
	for(;;){
		if(paused || !players[player].isComp) SuspendThread(thread);
		//suspend opponent
		Tplayer *p= &players[1-player];
		int d = suspendAI;
		bool dbg = (p->dbgThread!=0);
		if(d){
			EnterCriticalSection(&threadsLock);
			if(!dbg) p->openThreads();
			fort(p->threads) SuspendThread(item->h);
			LeaveCriticalSection(&threadsLock);
		}
		//move
		int errCode = players[player].move();

		//save continuous results in tournament
		if(tmpPsq && turNplayers > 0) saveRecTmp(errCode);

		//resume opponent
		if(d){
			EnterCriticalSection(&threadsLock);
			fort(p->threads) ResumeThread(item->h);
			if(!dbg){
				while(!p->threads.isEmpty()){
					ThreadItem *t= p->threads.nxt;
					CloseHandle(t->h);
					delete t;
				}
			}
			LeaveCriticalSection(&threadsLock);
		}
	}
}
