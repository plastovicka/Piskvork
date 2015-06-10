/*
 (C) Petr Lastovicka
 
 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License.
*/  
#include "lang.h"
#include "taskbarprogress.h"

#define myAI "pbrain-pela"
#define fnHtmlTable "_table.html"
#define fnResult "_result.txt"
#define fnState "state.tur"
#define fnStateTmp "turstate.tmp"
#define fnMsg "messages.txt"
#define fnOpenings "openings.txt"
#define tmpOpenings "tmpOpening.txt"

#define Mplayer 127
#define NET_PLAYER 0

typedef int Tsymbol;
struct Tsquare;
typedef Tsquare *Psquare;
typedef char TfileName[256];
typedef char TdirName[224];
enum{ INFO_TIMEMOVE=1,INFO_TIMEGAME=2,INFO_MEMORY=4,INFO_RULE=8,
  INFO_GAMETYPE=16,INFO_TIMELEFT=32,INFO_DIR=64, INFO_ALL=0xffff };

struct Tsquare                   
{
  Tsymbol z;	  //0=nothing, 1=X, 2=O, 3=outside
  Psquare nxt,pre;//linked list for undo, redo
  int x,y;        //coordinates <1..width, 1..height>
  int time;       //total thinking time
  Psquare winLineStart;//0 or beginning of a winning line
  int winLineDir; //direction offset of a winning line
};

struct Txyp {
  int x,y;
  Psquare p;
};

struct ThreadItem
{
  ThreadItem *nxt,*prv;
  HANDLE h;
  DWORD id;
  ThreadItem(){ nxt=prv=this; }
  void init(){ nxt=prv=this; }
  ~ThreadItem(){ prv->nxt= nxt; nxt->prv= prv; }
  void append(ThreadItem *item); 	
  int isEmpty(){ return nxt==this; }
};

struct Tplayer
{
 int isComp;   //0=human, 1=computer
 int timeMove; //time for a move (ms)
 int timeGame; //time for a game (s)
 int score;    //number of wins
 int time;     //total time (ms)
 int timeInit; //time of initialization, -1 during initialization, 0 after the first move
 int memory;   //used memory (bytes)
 int turPlayerId;  //number of player in tournament
 int Nmoves;       //moves counter
 HANDLE process;   //process handle
 DWORD processId;  //process identifier
 HANDLE pipeToAI;  //pipe handle
 HANDLE pipeFromAI;//pipe handle
 HANDLE dbgThread;   //debugger thread (only pipe protocol)
 ThreadItem threads; //list of threads (only pipe protocol)
 Txyp *lastBoard;
 int DlastBoard;
 bool mustSendBoard; //whole board must be send to AI
 bool isTimeOut;     //time out
 bool mustSendInfo;  //INFO timeout_turn must be send to AI
 TfileName brain;    //name of the EXE or ZIP file
 TfileName brain2;   //name of the EXE file
 TdirName temp;      //temporary folder 

 int level();
 void getName(char *buf, int n);
 char *getFilePart(); 
 void rdBrain(FILE *f);
 int timeLeft();
 void checkTimeOut(int currentTime);
 int move();
 void getMem();
 int createProcess(bool pipe, STARTUPINFO &si);
 void stopAI();
 void startPipeAI();
 bool notRunning();
 void sendCommand(char *text,...);
 int sendInfo(int mask=INFO_ALL);
 void sendInfoCmd(char *key, int value);
 void sendInfoCmd(char *key, char *value);
 int storeBoard();
 int readCommand(char *buf, int n);
 int brainChanged();
 void wrMsg(char *s);
 void wrDebugLog(char *s);
 void openThreads();

 FILE *infoDatFile;
};

struct TturPlayer
{
  int wins,losses,timeouts,errors,wins1,winsE,losses1;
  int time,memory,maxTurnTime;
  int Nmoves,Ngames;
  DWORD crc;
  int points;
  float ratio;
};

struct TturCell
{
  short start,notStart,error;
  int sum(){ return start+notStart+error; }
};

struct Tclient {
  SOCKET socket;
  HANDLE thread;
  int player[2];
  int repeatCount;
  int gameCount;
  int opening;
  u_long IP;
  void gameFinished();
};

struct PROCESS_MEMORY_COUNTERS {
  DWORD cb,PageFaultCount,PeakWorkingSetSize,WorkingSetSize,
    QuotaPeakPagedPoolUsage,QuotaPagedPoolUsage,
    QuotaPeakNonPagedPoolUsage,QuotaNonPagedPoolUsage,
    PagefileUsage,PeakPagefileUsage;
};
typedef BOOL (__stdcall *TmemInfo)(HANDLE,PROCESS_MEMORY_COUNTERS*,DWORD);

//-----------------------------------------------------------------
extern int player,moves,width,height,tolerance,maxMemory,turNplayers,logDebug,logMessage,suspendAI,debugAI,port,turCurRepeat,autoBegin,turRepeat,turRecord,turDelay,priority,terminate,turGamesCounter,turTieRepeat,turTieCounter,startMoves,errDelay,turOnlyLosses,turFormat,turGamesTotal,turOpening,invert,turNet,turRule,mx,my,height2,lastTurnTime[2],hardTimeOut,humanTimeOut,opening,logMoves,moveStart,coordStart,sameTime,turMatchRepeat,turLogMsg,infoEval,saveLock,debugPipe,exactFive,continuous,undoRequest,netGameVersion,ignoreErrors,openingRandomShift1,openingRandomShiftT,affinity, Nopening, includeUndoMoves, soundNotification,cmdlineGameOutFileFormat;
extern DWORD openingCRC;
extern bool paused,finished,disableScore,isWin9X,isClient,isServer,isNetGame,isListening,turTimerAvail,levelChanged,cmdLineGame,autoBeginForce,tmpPsq;

extern Psquare lastMove,board,boardb,boardk,hilited;
extern DWORD lastTick;
extern TturPlayer *turTable;
extern TturCell *turCells;
extern Tplayer players[2];
extern HANDLE thread,pipeThread,endGameEvent;
extern char *title;
extern TfileName fnstate,TahDat,PlochaDat,TimeOutsDat,MsgDat,InfoDat,cmdlineGameOutFile,cmdlineLogMsgOutFile,cmdlineLogPipeOutFile;
extern TdirName fntur,tempDir,AIfolder,dataDir;
extern char servername[128],turPlayerStr[50000],cmdGameEnd[512],cmdTurEnd[512];
extern CRITICAL_SECTION threadsLock,netLock,timerLock,infoLock;
extern SYSTEMTIME turDateTime; 
extern HWND hWin,msgWnd,logDlg,resultDlg;
extern SOCKET sock,sock_l;
extern HMODULE psapi;
extern TmemInfo getMemInfo;
extern Tclient clients[Mplayer];

//-----------------------------------------------------------------

int vmsg(char *caption, char *text, int btn, va_list v);
int msg1(int btn, char *text, ...);
void wrLog(char *text, ...);
void vwrLog(char *text, va_list ap);
void writeini(bool saveToolbar=true);
char* parseCommandLine();
int close(FILE *f,char *fn);
void show(HWND wnd);
void paintSquare(Psquare p);
void printLevel();
void printLevel1();
void printScore();
void printMoves();
void printTime(int pl);
bool getWinLine(Psquare p, Psquare &p2);
void printWinLine(Psquare p);
void invalidate();
void hiliteLast();
void cancelHilite();
Psquare Square(int x,int y);
void browseFolder(char *buf,HWND wnd,int item,char *txt);
void drawTitle();
void setPriority(HANDLE process);
unsigned rnd(unsigned n);
TturCell *getCell(int loser,int winner);
bool cmpExt(char *s,char *e);
void delDir(char *path,bool d);
int checkDir(HWND hDlg, int item);
DWORD WINAPI threadLoop(LPVOID);
DWORD WINAPI pipeThreadLoop(LPVOID);

void turGetBrain(int i, char *out, int n, bool noPath);
char *getTurDir(char *buf);
void getMsgFn(char *fn);
DWORD searchAI(const char *src, int n, char *dest, char **filePart);
void wrMsgNewGame(int p1,int p2);
void wrMessage(char *fmt,...);
void executeCmd(char *cmd);
int getNplayers();
//if filename is NULL, opening is read from filename define by fnOpenings, otherwise by the parameter
void initOpeningTab(char* filename = NULL);
void shutOpeningTab();
signed char* getOpening(int i);

//parametres viz declaration of initOpeningTab and newGame
void turStart(char* openingFileName = NULL);
void turRestart();
void turNext();
bool turNext(Tclient *client);

void turLocalNext();
void turResult();
void turGameFinished();
bool doMove(Psquare p);
bool doMove1(Psquare p, int action=0);
void init();
void resetScore();

//parameter forceAutoBegin force to use automatic opening even if global autoBegin is 0
void newGame(int pl, bool openingEnabled, bool forceAutoBegin = false);
void restartGame();
void resume();
void softPause();
void hardPause();
void killBrains();
int stopThinking();
void finishGame(int errCode);
void turAddTime();
void saveRec(int errCode, bool _includeUndoMoves);
void saveRecTmp(int errCode);
void wrGameResult();
void sendInfoEval();
void boardChanged();
bool notSuspended();
bool redo();
bool undo();
void setLevel(int l,int h);
void switchPlayer(int h1, int h2);
void savePsq(char *fn, int format, int errCode, bool _includeUndoMoves);
void openPsq(char *fn);
DWORD getTickCount();

int initWSA();
int startListen();
int startConnection();
int clientStart();
void clientEnd();
void killClient(int i);
void turAbort();
void stopListen();
void serverEnd();
int serverStart();
void clientFinished();
int calcCRC(const char *fn, DWORD &result);
void wrState();
int rdState();
int rd(char *buf, int len);
int rd1();
int rd2();
int rd4();
int wr(char *buf, int len);
int wr1(SOCKET s, int i);
int wr1(int i);

int netGameStart();
void netGameEnd();
void newNetGame(LPARAM lP);
void netGameMove(Psquare p);
void netGameMsg();
void netGameUndo();
void netGameNew();

//-----------------------------------------------------------------

#define nxtP(p,i) (p=(Psquare)(((char*)p)+(i*s)))
#define prvP(p,i) (p=(Psquare)(((char*)p)-(i*s)))
#define nxtS(p,s) ((Psquare)(((char*)p)+diroff[s]))

template <class T> inline void amin(T &x,int m){ if(x<m) x=m; }
template <class T> inline void amax(T &x,int m){ if(x>m) x=m; }
template <class T> inline void aminmax(T &x,int l,int h){
 if(x<l) x=l;
 if(x>h) x=h;
}

