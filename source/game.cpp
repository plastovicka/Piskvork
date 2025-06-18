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
//-----------------------------------------------------------------
#include "hdr.h"
#pragma hdrstop
#include <ctype.h>
#include "piskvork.h"

const int mutexPsqTimeoutMiliseconds = 50;
const int mutexTableTimeoutMiliseconds = 500;

const int WIN_POINT=3, TIE_POINT=1;

int diroff[9];      //pointer difference to adjacent squares
SYSTEMTIME turDateTime;
char cmdGameEnd[512], cmdTurEnd[512];
signed char **openingTab;

int levelTab[]={100, 300, 600, 1000, 1500, 2500, 5000, 10000, 15000, 30000, 60000};

//-----------------------------------------------------------------
void initOpeningTab(char *filename)
{
	int i, n, err=0;
	long x;
	FILE *f;
	char buf1[1024], *s, *e;
	signed char buf2[256], *o;
	TfileName fn;

	char* filename0 = filename;
	if(!filename) filename=fnOpenings;
	getExeDir(fn, filename);
	f=fopen(fn, "rt");
	if(!f){
		if(filename0 && autoBegin) msglng(730, "Cannot open file %s", filename); //show error only if it's tournament
		return;
	}
	//find out number of lines
	for(n=0;; n++){
		if(!fgets(buf1, sizeof(buf1), f)) break;
	}
	rewind(f);
	//allocate the array
	shutOpeningTab();
	openingTab=new signed char*[n];
	//read the file
	for(n=1;; n++){
		if(!fgets(buf1, sizeof(buf1), f)) break;
		s=buf1;
		for(i=1;;){
			x=strtol(s, &e, 10);
			if(e==s || x >=50 || x < -50) break;
			s=e;
			buf2[i++]=(signed char)x;
			while(*s==' ' || *s=='\t') s++;
			if(*s!=',') break;
			s++;
		}
		while(*s==' ') s++;
		if(*s!='\n' && *s || (i&1)==0){
			if(!err) err=n;
		}
		else if(i>1){
			buf2[0]=char((i-1)/2);
			openingTab[Nopening++]= o= new signed char[i];
			memcpy(o, buf2, i);
		}
	}
	fclose(f);
	if(err) msglng(875, "Error in %s, line %d", fnOpenings, err);
}

void shutOpeningTab()
{
	for(int i=0; i<Nopening; i++){
		delete[] openingTab[i];
	}
	Nopening=0;
	delete[] openingTab;
	openingTab=0;
}

signed char* getOpening(int i)
{
	if(!Nopening) return (signed char*)"";
	return openingTab[unsigned(i)%Nopening];
}

char *getTurDir(char *buf)
{
	strcpy(buf, fntur);
	char *s=strchr(buf, 0);
	if(s>buf && s[-1]!='\\'){ *s++='\\'; *s=0; }
	return s;
}

char *getLngSec(bool b)
{
	return b ? lng(531, "s") : lng(535, "ms");
}

TturCell *getCell(int loser, int winner)
{
	return turCells + loser + winner*turNplayers;
}

void show(HWND wnd)
{
	PostMessage(hWin, WM_COMMAND, 994, (LPARAM)wnd);
}


DWORD seed= GetTickCount();

unsigned rnd(unsigned n)
{
	seed=seed*367413989+174680251;
	return (unsigned)(UInt32x32To64(n, seed)>>32);
}

//-----------------------------------------------------------------
void Tplayer::getName(char *buf, int n)
{
	if(isNetGame && this==&players[NET_PLAYER]){
		strcpy(buf, lng(554, "Internet"));
	}
	else if(isComp){
		char *s= brain;
		char *p= strchr(s, 0)-4;
		if(p<=s || *p!='.') p=0;
		if(p) *p=0;
		s=cutPath(s);
		if(!_strnicmp(s, "pbrain-", 7)) s+=7;
		char t[32];
		sprintf(t, "%.1f", double(timeMove)/1000.0);
		sprintf(buf, "%.*s %s", n-2-(int)strlen(t), s, t);
		if(p) *p='.';
	}
	else{
		strcpy(buf, lng(552, "Human"));
	}
}

void Tplayer::getMem()
{
	static bool err=false;

	if(!getMemInfo){
		if(err || isWin9X) return;
		psapi=LoadLibrary("psapi.dll");
		getMemInfo= (TmemInfo)GetProcAddress(psapi, "GetProcessMemoryInfo");
		if(!getMemInfo){ err=true; return; }
	}
	PROCESS_MEMORY_COUNTERS p;
	if(getMemInfo(process, &p, sizeof(PROCESS_MEMORY_COUNTERS))){
		aminU(memory,p.PeakWorkingSetSize);
	}
}

void Tplayer::rdBrain(FILE *f)
{
	char *s;
	TfileName buf;

	buf[0]=0;
	fgets(buf, sizeof(buf), f);
	s=strchr(buf, '\n');
	if(s) *s=0;
	if(GetKeyState(VK_CONTROL)>=0){
		killBrains();
		if(!_strnicmp(buf, "homo-", 5)){
			isComp=0;
		}
		else if(buf[0]){
			strcpy(brain, buf);
			isComp=1;
			brainChanged();
		}
		else return;
		printLevel();
	}
}

int Tplayer::timeLeft()
{
	return timeGame ? timeGame*1000-time-timeInit : 0x7fffffff;
}

void Tplayer::checkTimeOut(int currentTime)
{
	if(!turNplayers && !(isComp ? hardTimeOut : humanTimeOut) ||
		isTimeOut || levelChanged || isNetGame) return;
	if(!isComp) timeInit=0;
	currentTime -= tolerance + timeInit;
	if((currentTime <= timeMove || timeInit<0) &&
		currentTime <= timeLeft()) return;
	isTimeOut=true;
	if(!disableScore){
		disableScore=true;
		Tplayer *p= (this==&players[0]) ? &players[1] : &players[0];
		if(turNplayers){
			turTable[turPlayerId].timeouts++;
			turTable[p->turPlayerId].winsE++;
			getCell(turPlayerId, p->turPlayerId)->error++;
			win7TaskbarProgress.SetProgressState(hWin, TBPF_ERROR);
			turAddTime();
		}
		else{
			p->score++;
		}
		printScore();
		wrGameResult();
	}
	if(turNplayers || cmdLineGame){
		wrMessage(lng(817, "%s: Timeout"), getFilePart());
		finishGame(15); //timeout
	}
	else{
		softPause();
		msglng(803, "Time out !\r\n\n%s", isComp ? getFilePart() : "");
	}
}
//-----------------------------------------------------------------
void openPsq(char *fn)
{
	FILE *f;
	int i, j, o1, o2, h, t, n, format;
	Psquare p;
	char *s;

	if(!*fn) return;
	if(fn[0]=='\"'){
		s=fn+strlen(fn)-1;
		if(*s=='\"'){ *s=0; fn++; }
	}
	if((f=fopen(fn, "rt"))==0){
		msglng(730, "Cannot open file %s", fn);
	}
	else{
		format=0;
		if(fscanf(f, "Piskvorky %dx%d, %d:%d, %d\n",
			&i, &j, &o1, &o2, &h)==5 && (h==0 || h==1)){ //*.psq format
			format=1;
			if(GetKeyState(VK_CONTROL)>=0){
				players[0].isComp=o1>0;
				if(o1>0 && o1<=sizeA(levelTab)) players[0].timeMove=levelTab[o1-1];
				players[1].isComp=o2>0;
				if(o2>0 && o2<=sizeA(levelTab)) players[1].timeMove=levelTab[o2-1];
			}
		}
		else{ //gomotur (*.rec) format
			rewind(f);
			i=max(width, 20);
			j=max(height, 20);
			h=0;
			s=cutPath(fn);
			if(strlen(s)>10 && isdigit(s[0]) && isdigit(s[1]) && isdigit(s[2]) && isdigit(s[3]) && isdigit(s[8]) && isdigit(s[9]) &&
				s[10]=='.' && (s[9]&1)==0) h=1;
			players[0].rdBrain(f);
			players[1].rdBrain(f);
			format=2;
		}
		if(format){
			if(width!=i || height!=j){
				killBrains();
				width=i; height=j;
			}
			newGame(h, false);
			while((n=fscanf(f, "\n%d,%d,%d", &i, &j, &t))>=2){
				if(format==1) i--, j--;
				p= Square(i, j);
				p->time= n>2 ? t : 0;
				if(!doMove1(p, 2)){
					msglng(800, "Wrong coordinates");
					break;
				}
			}
			if(format!=2){
				players[0].rdBrain(f);
				players[1].rdBrain(f);
			}
			else if(moves==0){
				format=0;
			}
			UpdateWindow(hWin);
			printTime(0);
			printTime(1);
		}
		if(!format) msglng(731, "Unknown format of file %s", fn);
		fclose(f);
	}
}

//-----------------------------------------------------------------
void wrNames(FILE *f, int h)
{
	const char *s1, *s2, *w;
	s1= players[0].isComp ? cutPath(players[0].brain) : "homo-1";
	s2= players[1].isComp ? cutPath(players[1].brain) : "homo-2";
	if(h) w=s1, s1=s2, s2=w;
	fprintf(f, "%s\n%s\n", s1, s2);
}

void savePsq(char *fn, int format, int errCode, bool _includeUndoMoves)
{
	Psquare p;
	FILE *f;
	int h, d;

start:
	if(!lastMove) return;
	f=fopen(fn, "wt");
	if(!f){
		msglng(733, "Cannot create file %s", fn);
	}
	else{
		EnterCriticalSection(&timerLock);
		h=player^(moves&1);
		if(format==2){
			wrNames(f, h);
		}
		else{
			fprintf(f, "Piskvorky %dx%d, %d:%d, %d\n",
				width, height,
				(players[0].level()+1)*players[0].isComp,
				(players[1].level()+1)*players[1].isComp,
				h);
		}

		for(p=lastMove; p->nxt; p=p->nxt);

		d= format==1 ? 0 : 1;
		for(int i = 0; p; p=p->pre, i++)
		{
			if(!_includeUndoMoves && i >= moves)
				break;
			fprintf(f, "%d,%d,%d\n", p->x-d, p->y-d, p->time);
		}
		if(format!=2 && (turNplayers || cmdLineGame)) wrNames(f, 0);
		fprintf(f, "%d\n", errCode);
		LeaveCriticalSection(&timerLock);
		if(close(f, fn)==2) goto start;
	}
}


void MutexNameFromPath(char *path, char *buffer)
{
	//global mutex name is lowercase path, where backslash is replaced by colon
	strcpy(buffer, "Global\\");
	buffer += 7;

	//replace backslash by ":" and make it tolower
	char c;
	do{
		c = (char)tolower(*path++);
		if(c == '\\') c = ':';
		*buffer++ = c;
	} while(c);
}

void saveRecTmp(int errCode)
{
	// I am not sure, how long it can wait,
	// I think that only a little, because
	// waiting for mutex is part of AI time
	// and can cause timeout

	TfileName buf;

	if(isClient) strcat(strcpy(buf, AIfolder), "\\");
	else getTurDir(buf);
	strcat(buf, "tmp\\");
	CreateDirectory(buf, 0);

	sprintf(strchr(buf, 0), "tmp_%dx%d.%s", players[0].turPlayerId, players[1].turPlayerId, (turFormat==2) ? "rec" : "psq");

	// writing temporary psq file (in -tmppsq mode) must be synchronized
	// with upload application reading the same file

	// the same mutex name is in upload application  
	TfileName mutexNameGlobal;
	MutexNameFromPath(buf, mutexNameGlobal);

	HANDLE hIOMutex= CreateMutex(NULL, FALSE, mutexNameGlobal);
	if(hIOMutex == NULL)
		return;
	try
	{
		if(WaitForSingleObject(hIOMutex, mutexPsqTimeoutMiliseconds) != WAIT_OBJECT_0)
			goto end;//if the temporary file is not written, it doesn't matter
		savePsq(buf, turFormat, errCode, false);
	} catch(...)
	{
	}
	ReleaseMutex(hIOMutex);
end:
	CloseHandle(hIOMutex);
}

void saveRec(int errCode, bool _includeUndoMoves)
{
	TfileName buf;
	char *s;

	if(turOnlyLosses && (players[0].turPlayerId!=0 || (moves&1)) &&
		(players[1].turPlayerId!=0 || !(moves&1))) return;
	s= getTurDir(buf);
	for(int i=1; i<1000; i++){
		sprintf(s, "%dx%d-%d(%d).%s", players[0].turPlayerId,
			players[1].turPlayerId, moves, i,
			(turFormat==2) ? "rec" : "psq");
		if(GetFileAttributes(buf)==0xFFFFFFFF){
			wrMessage("-> %s", buf);
			savePsq(buf, turFormat, errCode, _includeUndoMoves);
			break;
		}
	}
}
//-----------------------------------------------------------------
void turGetBrain(int i, char *out, int n, bool noPath)
{
	char *s, *t, *s0;
	int len;

	s=turPlayerStr;
	while(*s=='\r' || *s=='\n' || *s==' ') s++;
	while(i>0){
		if(*s=='\r' || *s=='\n'){
			while(*s=='\r' || *s=='\n' || *s==' ') s++;
			i--;
		}
		else{
			s++;
		}
	}
	s0=s;
	while(*s!='\r' && *s!='\n' && *s) s++;
	if(s>s0 && s[-1]==' ') s--;
	if(noPath){
		t=s;
		while(t>=s0 && *t!='\\') t--;
		s0=t+1;
		if(!_strnicmp(s0, "pbrain-", 7)) s0+=7;
	}
	len= int(s-s0);
	if(noPath && len>4 && s[-4]=='.') len-=4;
	amax(len, n-1);
	memcpy(out, s0, len);
	out[len]=0;
}

int __cdecl turCmpRatio(const void *a, const void *b)
{
	TturPlayer *p1=&turTable[*(int*)a], *p2=&turTable[*(int*)b];
	float d= p2->ratio - p1->ratio;
	if(d>0) return 1;
	if(d<0) return -1;
	return getCell(*(int*)a, *(int*)b)->sum() - getCell(*(int*)b, *(int*)a)->sum();
}

int __cdecl turCmpPoints(const void *a, const void *b)
{
	TturPlayer *p1=&turTable[*(int*)a], *p2=&turTable[*(int*)b];
	int i= p2->points - p1->points;
	if(i!=0) return i;
	i=turCmpRatio(a, b);
	if(i!=0) return i;
	if(!p1->Nmoves || !p2->Nmoves) return 0;
	return p1->time/p1->Nmoves - p2->time/p2->Nmoves;
}

void removeChar(char *dest, char *src, char ch)
{
	char c;
	do{
		c= *src++;
		if(c!=ch) *dest++ = c;
	} while(c);
}

void turResultInner(TfileName& fn)
{
	char *result, *s, *s0;
	int i, j, s1, s2, e, m;
	int *A;
	FILE *f;
	TturCell *c1, *c2;
	TturPlayer *t;
	TfileName buf;
	char buf1[128];
	char ruleBuffer[128];
	const float k=1e-8f;
	SYSTEMTIME date;

	//calculate points and ratio
	for(i=0; i<turNplayers; i++){
		t=&turTable[i];
		s1=t->wins+t->winsE;
		s2=t->losses+t->timeouts+t->errors;
		t->ratio= (s1+k)/(s2+k);
		t->points=0;
		for(j=0; j<turNplayers; j++){
			s1=getCell(j, i)->sum();
			s2=getCell(i, j)->sum();
			if(s1>=s2 && s1+s2>1) t->points+= (s1>s2) ? WIN_POINT : TIE_POINT;
		}
	}
	//sort results
	A= new int[turNplayers];
	for(i=0; i<turNplayers; i++) A[i]=i;
	qsort(A, turNplayers, sizeof(int), turRule!=1 &&
		turCurRepeat<turRepeat ? turCmpPoints : turCmpRatio);

	//HTML table  
	f=fopen(fn, "wt");
	if(f){
		fprintf(f, "<HTML><HEAD><TITLE>Piskvork tournament result</TITLE><LINK href=\"piskvork.css\" type=text/css rel=stylesheet></HEAD><BODY><TABLE border=1><TBODY align=center>\n");
		for(i=-1; i<turNplayers; i++){
			fputs("<TR>", f);
			for(j=-1; j<turNplayers; j++){
				if(turRule==1 && j>=0 && A[j]) continue;
				if(i==j){
					fputs("<TD class=\"dash\">-</TD>", f);
				}
				else if(i<0 || j<0){
					fputs("<TH>", f);
					turGetBrain(i>=0 ? A[i] : A[j], buf, sizeof(buf), true);
					fputs(buf, f);
					fputs("</TH>", f);
				}
				else{
					c1=getCell(A[i], A[j]);
					c2=getCell(A[j], A[i]);
					fprintf(f, "<TD class=\"");
					if(c1->sum() > c2->sum())
					{
						fprintf(f, "win");
					}
					else if(c1->sum() < c2->sum())
					{
						fprintf(f, "loss");
					}
					else
					{
						fprintf(f, "draw");
					}
					fprintf(f, "\">%d : %d", c1->sum(), c2->sum());
					/*
					fprintf("<BR>%d+%d", c1->start,c1->notStart);
					if(c1->error) fprintf(f,"+%d",c1->error);
					fprintf(f,":%d+%d", c2->start, c2->notStart);
					if(c2->error) fprintf(f,"+%d",c2->error);
					*/
					fputs("</TD>", f);
				}
			}
			fputs("</TR>\n", f);
		}
		fprintf(f, "<TR><TH>%s</TH>", lng(607, "Total"));
		for(j=0; j<turNplayers; j++){
			if(turRule==1 && A[j]) continue;
			t=&turTable[A[j]];
			s1=t->wins+t->winsE;
			s2=t->losses+t->timeouts+t->errors;

			fprintf(f, "<TD class=\"");
			if(s1 > s2)
			{
				fprintf(f, "win");
			}
			else if(s1 < s2)
			{
				fprintf(f, "loss");
			}
			else
			{
				fprintf(f, "draw");
			}
			fprintf(f, "\">%d : %d</TD>", s1, s2);
		}
		fprintf(f, "</TR>\n<TR><TH>%s</TH>", lng(608, "Ratio"));
		for(j=0; j<turNplayers; j++){
			if(turRule==1 && A[j]) continue;
			fputs("<TD>", f);
			float r= turTable[A[j]].ratio;
			if(r<1E6f) fprintf(f, "%.3f", r);
			else fputc('-', f);
			fputs("</TD>", f);
		}
		if(turRule!=1){
			fprintf(f, "</TR>\n<TR><TH>%s</TH>", lng(609, "Points"));
			for(j=0; j<turNplayers; j++){
				fprintf(f, "<TD>%d</TD>", turTable[A[j]].points);
			}
		}
		fputs("</TR>\n</TBODY></TABLE>", f);
	}

	s= result= new char[turNplayers*(_MAX_PATH+180)+300];
	s+=sprintf(s, "-------------------------------------\r\n");
	s0=s;
	s+=GetDateFormat(LOCALE_USER_DEFAULT, 0, &turDateTime, 0, s, 30)-1;
	*s++=' ';
	*s++=' ';
	s+=GetTimeFormat(LOCALE_USER_DEFAULT, TIME_NOSECONDS, &turDateTime, 0, s, 20)-1;
	*s++=' ';
	*s++='-';
	*s++=' ';
	GetLocalTime(&date);
	if(date.wDay!=turDateTime.wDay || date.wMonth!=turDateTime.wMonth){
		s+=GetDateFormat(LOCALE_USER_DEFAULT, 0, &date, 0, s, 30)-1;
		*s++=' ';
		*s++=' ';
	}
	s+=GetTimeFormat(LOCALE_USER_DEFAULT, TIME_NOSECONDS, &date, 0, s, 20)-1;
	if(f) fprintf(f, "<BR>%s", s0);
	*s++='\r';
	*s++='\n';
	m=players[0].timeMove;
	if(!players[0].timeGame) strcpy(buf, lng(610, "unlimited"));
	else sprintf(buf, "%d %s", players[0].timeGame, getLngSec(true));
	if(autoBeginForce){
		strcpy(buf1, lng(613, "Yes"));
	}
	else if(autoBegin && Nopening){
		sprintf(buf1, lng(612, "start from %d, used %d, available %d"),
			unsigned(turOpening)%Nopening+1, (turMatchRepeat*turRepeat+1)/2, Nopening);
	}
	else{
		strcpy(buf1, lng(611, "No"));
	}
	removeChar(ruleBuffer, ruleFive==2 ? lng(557, "renju rule") :(ruleFive & 1 ? lng(556, "exactly five stones wins") : lng(555, "five or more stones wins")), '&');
	if(ruleFive & 8) {
		strcat(ruleBuffer, ", ");
		strcat(ruleBuffer, lng(661, "Caro rule"));
	}

	s+=sprintf(s, lng(600, "Time for turn: %d %s,   Time for match: %s\r\nTolerance: %d %s,   Memory: %d MB\r\nOpenings: %s\r\nRule: %s\r\nOpening CRC: %x\r\nGames played: %d"),//
		m%1000 ? m : m/1000, getLngSec(m%1000==0), buf,
		tolerance%1000 ? tolerance : tolerance/1000, getLngSec(tolerance%1000==0),
		maxMemory, buf1, ruleBuffer, openingCRC, turGamesCounter);
	if(turGamesCounter<turGamesTotal){
		s0=s;
		s+=sprintf(s, "  (%d%%)", turGamesCounter*100/turGamesTotal);
		if(f) fprintf(f, "%s", s0);
	}
	s+=sprintf(s, "\r\n-------------------------------------");
	for(i=0; i<turNplayers; i++){
		t= &turTable[A[i]];
		turGetBrain(t-turTable, buf, sizeof(buf), true);
		s+=sprintf(s, lng(601, "\r\n\r\n%s\r\nwins: %d=%d+%d"),
			buf, t->wins+t->winsE, t->wins1, t->wins-t->wins1);
		if(t->winsE) s+=sprintf(s, "+%d", t->winsE);
		e=t->timeouts+t->errors;
		s+=sprintf(s, lng(602, ",   losses: %d=%d+%d"),
			t->losses+e, t->losses1, t->losses-t->losses1);
		if(e) s+=sprintf(s, "+%d", e);
		if(t->errors) s+=sprintf(s, lng(603, ",  %d errors"), t->errors);
		if(t->Nmoves){
			m= t->time/t->Nmoves;
			s+=sprintf(s, lng(604, "\r\ntime/turn: %d %s (max: %.1f s),  time/game: %d s\r\nmoves/game: %d,  CRC: %x"),
				m>9000 ? m/1000 : m, getLngSec(m>9000),
				double(t->maxTurnTime)/1000,
				t->time/t->Ngames/1000, t->Nmoves/t->Ngames, t->crc);
			if(t->memory){
				s+=sprintf(s, lng(605, ",   memory: %.2f MB"), double(t->memory)/1048576);
			}
		}
	}
	s+=sprintf(s, "\r\n\r\n");
	delete[] A;
	if(f){
		fputs("</BODY></HTML>", f);
		fclose(f);
	}

	//store result.txt
	strcpy(getTurDir(buf), fnResult);
	f=fopen(buf, "wb");
	if(f){
		fputs(result, f);
		fclose(f);
	}
	//write string to the result dialog box
	PostMessage(hWin, WM_COMMAND, 996, (LPARAM)result);
}

void turResult()
{
	TfileName tablePath;
	strcat(getTurDir(tablePath), fnHtmlTable);
	TfileName mutexNameGlobal;
	MutexNameFromPath(tablePath, mutexNameGlobal);
	HANDLE hIOMutex= CreateMutex(NULL, FALSE, mutexNameGlobal);
	if(hIOMutex == NULL)
		return;
	try
	{
		if(WaitForSingleObject(hIOMutex, mutexTableTimeoutMiliseconds) != WAIT_OBJECT_0)
			goto end;
		turResultInner(tablePath);
	} catch(...)
	{
	}
	ReleaseMutex(hIOMutex);
end:
	CloseHandle(hIOMutex);
}

void wrGameResult()
{
	if(moves<9) return;
	for(int i=0; i<2; i++){
		char buf[64];
		players[i].getName(buf, sizeof(buf));
		int m=players[i].time/players[i].Nmoves;
		wrLog(lng(652, "%s:  %d %s/turn"), buf,
			m>10000 ? m/1000 : m, getLngSec(m>10000));
	}
}

void executeCmd(char *cmd)
{
	if(!cmd || !cmd[0]) return;
	STARTUPINFO si;
	memset(&si, 0, sizeof(si));
	si.cb= sizeof(si);
	si.dwFlags= STARTF_USESHOWWINDOW;
	si.wShowWindow= SW_MINIMIZE;
	PROCESS_INFORMATION pi;
	if(CreateProcess(0, cmd, 0, 0, FALSE, NORMAL_PRIORITY_CLASS, 0, fntur, &si, &pi)){
		CloseHandle(pi.hThread);
		WaitForSingleObject(pi.hProcess, 10000);
		CloseHandle(pi.hProcess);
	}
}

void turNext(int p1, int p2)
{
	if(p2>=turNplayers){
		turCurRepeat--;
		p1=0; p2=1;
		if(turCurRepeat<=0) p2=0;
	}
	if((turRepeat-turCurRepeat)&1){
		int w=p1; p1=p2; p2=w;
	}
	players[0].turPlayerId=p1;
	players[1].turPlayerId=p2;
}

void turNext()
{
	int p1, p2, w;
	p1=players[0].turPlayerId;
	p2=players[1].turPlayerId;
	if(p1>p2){
		w=p1; p1=p2; p2=w;
	}
	p2++;
	if(turRule==0){
		if(p2<turNplayers) turNext(p1, p2);
		else turNext(p1+1, p1+2);
	}
	else{
		turNext(0, p2);
	}
}

void turLocalNext()
{
	killBrains();
	if(!turNext(&clients[0])) return;
	for(int k=0; k<2; k++){
		Tplayer *p= &players[k];
		p->turPlayerId=clients[0].player[k];
		turGetBrain(p->turPlayerId, p->brain, sizeof(players[0].brain), false);
		p->brainChanged();
		p->score= turTable[p->turPlayerId].wins;
	}
	opening=clients[0].opening;
	invert= !invert;
	turTieCounter=0;
	newGame(0, true, autoBeginForce);
	resume();
}

int turStart1()
{
	if(turNet){
		if(!isServer && serverStart()) return 1;
	}
	else{
		serverEnd();
		Sleep(500);
	}
	killBrains();
	amax(turDelay, 10000);
	return 0;
}

//count number of players
int getNplayers()
{
	char *s;

	turNplayers=0;
	s=turPlayerStr;
	while(*s=='\r' || *s=='\n' || *s==' ') s++;
	if(*s) turNplayers++;
	while(*s){
		if(*s=='\n' || *s=='\r'){
			while(*s=='\r' || *s=='\n' || *s==' ') s++;
			if(!*s) break;
			turNplayers++;
		}
		else{
			s++;
		}
	}
	if(turNplayers<2){ turNplayers=0; return 1; }
	if(turRule==0){
		turGamesTotal=turNplayers*(turNplayers-1)*turRepeat*turMatchRepeat/2;
	}
	else{
		turGamesTotal=(turNplayers-1)*turRepeat*turMatchRepeat;
	}
	//create tables
	delete[] turTable;
	turTable= new TturPlayer[turNplayers];
	memset(turTable, 0, turNplayers*sizeof(TturPlayer));
	delete[] turCells;
	turCells= new TturCell[turNplayers*turNplayers];
	memset(turCells, 0, turNplayers*turNplayers*sizeof(TturCell));
	return 0;
}

//calculate CRC of all AI
int turCRC()
{
	int i;
	TfileName fn;

	for(i=0; i<turNplayers; i++){
		turGetBrain(i, fn, sizeof(fn), false);
		if(calcCRC(fn, turTable[i].crc)){
			msglng(730, "Cannot open %s", fn);
			turNplayers=0;
			return 2;
		}
	}
	return 0;
}

int turTestCRC()
{
	int i;
	TfileName fn;
	DWORD c;

	for(i=0; i<turNplayers; i++){
		turGetBrain(i, fn, sizeof(fn), false);
		if(calcCRC(fn, c)){
			msglng(730, "Cannot open %s", fn);
			turNplayers=0;
			return 2;
		}
		if(c!=turTable[i].crc){
			turTable[i].crc=c;
			wrLog("%s changed since last tournament !", cutPath(fn));
		}
	}
	return 0;
}

void turStart(char* openingFileName)
{
	if(!openingFileName) openingFileName=fnOpenings;
	initOpeningTab(openingFileName);
	calcCRC(openingFileName, openingCRC);
	if(turStart1()) return;
	EnterCriticalSection(&netLock);
	if(!getNplayers() && !turCRC()){
		turGamesCounter=0;
		turCurRepeat=turRepeat;
		turOpening=opening;
		if(Nopening && !(turGamesTotal%Nopening)) turOpening=0;
		players[0].turPlayerId=players[1].turPlayerId=0;
		GetLocalTime(&turDateTime);
		//clear result
		turResult();
		TfileName fn;
		getMsgFn(fn);
		DeleteFile(fn);

		win7TaskbarProgress.SetProgressState(hWin, TBPF_NORMAL);
		win7TaskbarProgress.SetProgressValue(hWin, 0, 100);
	}
	LeaveCriticalSection(&netLock);
	if(!isServer) turLocalNext();
}

void turRestart()
{
	if(turStart1()) return;
	EnterCriticalSection(&netLock);
	if(!rdState() && !turTestCRC()){
		turResult();
	}
	LeaveCriticalSection(&netLock);
	if(!isServer) turLocalNext();
}
//-----------------------------------------------------------------
void Tclient::gameFinished()
{
	turGamesCounter++;
	gameCount++;
	int w=player[0]; player[0]=player[1]; player[1]=w;
	if(gameCount==turMatchRepeat) player[0]=-1;
	turResult();
	wrState();
	executeCmd(cmdGameEnd);
}

void finishGame(int errCode)
{
	if(!finished){
		finished=true;
		softPause();
		//next game
		if(cmdLineGame){
			writeini(false); //don't save toolbar, because it could deadlock
			int c=3;
			if(players[0].score>players[1].score) c=1;
			if(players[0].score<players[1].score) c=2;

			if(strlen(cmdlineGameOutFile) != 0)
				savePsq(cmdlineGameOutFile, cmdlineGameOutFileFormat, 0, false);

			exit(c);
		}
		else if(isClient){
			clientFinished();
		}
		else if(turNplayers){
			EnterCriticalSection(&netLock);
			if(turRecord) saveRec(errCode, false);
			clients[0].gameFinished();
			turTimerAvail=true;
			SetTimer(hWin, 997, turDelay+10, 0);
			LeaveCriticalSection(&netLock);
		}
	}
}


void softPause()
{
	paused=true;
	KillTimer(hWin, 102);
}

void resume()
{
	if(isServer || finished) return;
	if(paused){
		paused=false;
		lastTick=getTickCount();
		SetTimer(hWin, 102, 100, 0);
	}
	ResumeThread(thread);
}

int stopThinking()
{
	if(paused || isServer || isClient || finished) return 0;
	Tplayer *p= &players[player];
	if(!p->isComp || !p->pipeToAI) return 0;
	p->infoDatFile=0;
	p->sendInfoCmd("timeout_turn", 0);
	p->mustSendInfo=true;
	return 1;
}

void boardChanged()
{
	players[0].mustSendBoard=true;
	players[1].mustSendBoard=true;
}

bool notSuspended()
{
	DWORD n= SuspendThread(thread);
	ResumeThread(thread);
	return n<=0;
}

void hardPause()
{
	softPause();
	if(notSuspended()){
		terminateAI++;
		//the working thread is waiting in ReadFile and 
		//  can be unblocked only by terminating the AI
		players[player].stopAI();
		//NOTE: the working thread can start AI again just before Suspend
		while(notSuspended()) Sleep(50);
		terminateAI--;
	}
}

void killBrains()
{
	hardPause();
	players[0].stopAI();
	players[1].stopAI();
}

void sendInfoEval()
{
	if(!infoEval || notSuspended()) return;
	if(mx>=0 && my>=0 && mx<width && my<height){
		Tplayer *p= &players[player];
		if(!p->isComp || !p->pipeToAI) p=&players[1-player];
		p->sendCommand("INFO evaluate %d,%d", mx, my);
	}
}

void turAddTime()
{
	if(!turNplayers) return;
	for(int k=0; k<2; k++){
		TturPlayer *p= &turTable[players[k].turPlayerId];
		p->time+= players[k].time;
		p->Nmoves+= players[k].Nmoves;
		p->Ngames++;
		aminU(p->memory, players[k].memory);
	}
}

//-----------------------------------------------------------------
void init()
{
	int x, y;
	Psquare p;

	//alocate board
	aminmax(height, 5, 80);
	aminmax(width, 5, 120);
	delete[] board;
	height2=height+2;
	board= new Tsquare[(width+10)*(height2)];
	boardb= board + 5*height2;
	boardk= boardb+ width*height2;
	diroff[0]=sizeof(Tsquare);
	diroff[4]=-diroff[0];
	diroff[1]=sizeof(Tsquare)*(1+height2);
	diroff[5]=-diroff[1];
	diroff[2]=sizeof(Tsquare)* height2;
	diroff[6]=-diroff[2];
	diroff[3]=sizeof(Tsquare)*(-1+height2);
	diroff[7]=-diroff[3];
	diroff[8]=0;

	//initialize board
	p=board;
	for(x=-4; x<=width+5; x++){
		for(y=0; y<=height+1; y++){
			p->z= (x<1 || y<1 || x>width || y>height) ? 3 : 0;
			p->x= x;
			p->y= y;
			p->winLineDir=0;
			p->winLineStart=0;
			p->foul=false;
			p++;
		}
	}
	moves=0;
	lastMove=0;
}

void resetScore()
{
	players[0].score=players[1].score=0;
	disableScore=false;
	printScore();
}

void newGame1()
{
	hardPause();
	boardChanged();
	players[0].time=players[1].time=0;
	players[0].Nmoves= players[1].Nmoves= 0;
	players[0].isTimeOut= players[1].isTimeOut= false;
	if(isWin9X) debugAI=1;
	hilited=0;
	finished=false;
	lastTick=getTickCount();
}

void wrMsgNewGame(int p1, int p2)
{
	char n[2][64];

	if(!turNplayers) return;
	wrMessage("______________________________");
	turGetBrain(p1, n[0], sizeof(n[0]), true);
	turGetBrain(p2, n[1], sizeof(n[1]), true);
	wrMessage("%s x %s", n[0], n[1]);
}

void playOpening()
{
	int j, x, y, x0, y0, w, r, xmin, xmax, ymin, ymax;
	signed char *o,*o0;

	o = o0 = getOpening(opening);
	x0 = width / 2;
	y0 = height / 2;
	r = 0;
	if (turNplayers ? openingRandomShiftT : openingRandomShift1) {
		//random rotation or mirroring
		r = rnd(8);
		//mininal and maximal coordinates
		xmin = ymin = 99;
		xmax = ymax = -99;
		for (j = *o++; j > 0; j--) {
			x = *o++;
			y = *o++;
			if (r & 4) w = x, x = y, y = w;
			amax(xmin, x);
			amin(xmax, x);
			amax(ymin, y);
			amin(ymax, y);
		}
		//random translation (if not near edge of the board)
		const int B = 4;
		if (-xmin < x0 - B && xmax < x0 - B) x0 += rnd(4) - 2;
		if (-ymin < y0 - B && ymax < y0 - B) y0 += rnd(4) - 2;
		if ((r & 1) && !(width & 1)) x0--;
		if ((r & 2) && !(height & 1)) y0--;
	}
	o = o0;
	for (j = *o++; j>0; j--) {
		x = *o++;
		y = *o++;
		if (r & 4) w = x, x = y, y = w;
		if (r & 1) x = -x;
		if (r & 2) y = -y;
		doMove1(Square(x0 + x, y0 + y));
	}
	opening++;
}

void newGame(int pl, bool openingEnabled, bool forceAutoBegin)
{
	if(!isClient){
		wrMsgNewGame(players[pl].turPlayerId, players[1-pl].turPlayerId);
	}
	newGame1();
	player=pl;
	init();
	players[0].memory=players[1].memory=0;
	lastTurnTime[0]=lastTurnTime[1]=0;
	disableScore=false;
	wrLog(0);

	if ((autoBegin || forceAutoBegin) && openingEnabled) playOpening();
	startMoves=moves;
	invalidate();
	UpdateWindow(hWin);
}

void restartGame()
{
	newGame1();
	while(moves>startMoves){
		lastMove->z= 0;
		lastMove=lastMove->nxt;
		player=1-player;
		moves--;
	}
	invalidate();
	UpdateWindow(hWin);
	resume();
}

//-----------------------------------------------------------------
//put symbol at square p, switch players
//action: 0=local turn, 1=network turn, 2=file opened, 3=redo
bool doMove1(Psquare p, int action)
{
	int i, poc, s, i1, i2, playerWin, playerLoss;
	TturPlayer *t1, *t2;
	TturCell *c;
	Psquare p1, p2, lastMove0;

	lastMove0=lastMove;
	if(finished || p<boardb || p>=boardk || p->z) return false;
	p->z= player+1;
	lastMove=p;
	//increment moves counter
	moves++;
	players[player].Nmoves++;
	printMoves();
	//append square to the list
	p->nxt=lastMove0;
	if(!lastMove0 || lastMove0->pre != p) p->pre=0;
	if(lastMove0) lastMove0->pre= p;
	//renju rule
	bool f = (ruleFive == 2) && checkforbid();
	//redraw square
	paintSquare(p);
	//increment time
	if(action!=3){
		DWORD t=getTickCount();
		if(action==0){
			p->time= t-lastTick;
			if(turNplayers) amin(turTable[players[player].turPlayerId].maxTurnTime, p->time);
		}
		if(action<2){
			lastTurnTime[player]= p->time;
		}
		lastTick=t;
		//write log
		if(logMoves){
			wrLog("%2d) [%d,%d],  %d ms", moves-1+moveStart,
				p->x-1+coordStart, p->y-1+coordStart, p->time);
		}
	}
	players[player].time += p->time;
	//send coordinates to network player
	if(isNetGame) netGameMove(p);

	//switch players
	player=1-player;
	printTime(1-player);
	levelChanged=false;
	//check winning lines
	for(i=0; i<4; i++){
		s=diroff[i];
		p1=p2=p;
		poc=-1;
		do{
			prvP(p1, 1);
			poc++;
		} while(p1->z==p->z && !p1->blocked());
		do{
			nxtP(p2, 1);
			poc++;
		} while(p2->z==p->z && !p2->blocked());

		//Caro rule
		if(poc==5 && (ruleFive&8) && p1->z==3-p->z && !p1->blocked() && p2->z==3-p->z && !p2->blocked()) 
			poc=0; 

		if((ruleFive%2 ? poc==5 : poc>=5) || f){
			nxtP(p1, 1);
			prvP(p2, 1);
			//win
			playerWin = 1-player;
			playerLoss = player;
			if(f){
				playerWin = player;
				playerLoss = 1-player;
			}
			if(!disableScore && action<2){
				if(!continuous || turNplayers) disableScore=true;
				if(turNplayers){
					t1=&turTable[i1=players[playerWin].turPlayerId];
					t2=&turTable[i2=players[playerLoss].turPlayerId];
					t1->wins++;
					t2->losses++;
					c= getCell(i2, i1);
					if((moves&1) ^ f){ //winner started
						t1->wins1++;
						c->start++;
					}
					else{ //loser started
						t2->losses1++;
						c->notStart++;
					}
					turAddTime();
				}
				else{
					players[playerWin].score++;
				}
				//log
				char buf[64];
				players[playerWin].getName(buf, sizeof(buf));
				wrLog(lng(653, "\r\n%s wins\r\n"), buf);
				wrGameResult();
			}
			cancelHilite();
			if(!f)
			{
				//draw a line
				for(;; prvP(p2, 1)){
					p2->winLineDir=s;
					p2->winLineStart=p1;
					if(p2==p1) break;
				}
				printWinLine(p);
			}
			else {
				p->foul=true;
				printFoul(p);
			}
			boardChanged();

			printScore();
			if(!continuous || turNplayers) finishGame(0);//standard finish
			break;
		}
	}
	if(moves>=width*height-25 && !finished){
		//board is almost full
		if(turNplayers && turTieCounter<turTieRepeat){
			turTieCounter++;
			finished=true;
			PostMessage(hWin, WM_COMMAND, 995, 0);
		}
		else{
			if(!disableScore && action<2){
				disableScore=true;
				turAddTime();
				//log
				if(!continuous || turNplayers) wrLog(lng(651, "\r\nIt's a draw!\r\n")); else wrLog("");
				wrGameResult();
			}
			finishGame(0); //standard finish
		}
	}
	return true;
}

//move to square p and hilite
bool doMove(Psquare p)
{
	bool b=doMove1(p);
	if(b){
		hiliteLast();
		//if sound notification is enabled and human is playing with computer and computer has finished the move
		if(soundNotification && !players[player].isComp && players[1 - player].isComp)
			//if the wav is not found, nothing happens
			PlaySound(TEXT("gomoku.wav"), NULL, SND_FILENAME|SND_ASYNC|SND_NODEFAULT);
	}
	return b;
}
//-----------------------------------------------------------------
bool undo()
{
	Psquare p1, p2;
	int s;

	if(moves<2) return false;
	cancelHilite();
	softPause();
	finished=false;
	if(getWinLine(lastMove, p2)){
		//delete winning line
		printWinLine(p2);
		p1=p2->winLineStart;
		for(s=p2->winLineDir;; prvP(p2, 1)){
			p2->winLineDir=0;
			p2->winLineStart=0;
			if(p2==p1) break;
		}
		UpdateWindow(hWin);
	}
	if(lastMove->foul) {
		lastMove->foul=false;
		UpdateWindow(hWin);
	}
	//erase square
	lastMove->z= 0;
	paintSquare(lastMove);
	//turn back time and moves counter
	player=1-player;
	players[player].time -= lastMove->time;
	players[player].Nmoves--;
	lastTick=getTickCount();
	printTime(player);
	lastMove=lastMove->nxt;
	moves--;
	printMoves();
	boardChanged();
	return true;
}
//-----------------------------------------------------------------
bool redo()
{
	if(!lastMove || !lastMove->pre) return false;
	softPause();
	finished=false;
	doMove1(lastMove->pre, 3);
	boardChanged();
	return true;
}
//-----------------------------------------------------------------

int Tplayer::level()
{
	int l;
	for(l=sizeA(levelTab)-1; l>0 && timeMove<levelTab[l]; l--);
	return l;
}

void setLevel(int l, int h)
{
	if(l>=0 && l<sizeA(levelTab)){
		players[h].timeMove=levelTab[l];
		players[h].sendInfo(INFO_TIMEMOVE);
		if(sameTime){
			players[1-h].timeMove=levelTab[l];
			players[1-h].sendInfo(INFO_TIMEMOVE);
		}
		levelChanged=true;
		printLevel();
	}
}

void switchPlayer(int h1, int h2)
{
	if(turNplayers || isNetGame) return;
	killBrains();
	players[0].isComp=h1;
	players[1].isComp=h2;
	printLevel();
	resume();
}
//-----------------------------------------------------------------
