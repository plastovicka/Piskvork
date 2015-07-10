/*
 (C) Petr Lastovicka
 
 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License.
*/  

#include "..\skelet\pisqpipe.h"

const int //evaluation constants (very important)
 H10=2,  H11=6,   H12=10,
 H20=23, H21=158, H22=175,
 H30=256,H31=511, H4=2047;

typedef int Tsymbol;
struct Tsquare;
typedef Tsquare *Psquare;

struct Tprior
{
  int pv,ps[4];  //evaluation in 4 directions and sum
  int i;         //in which linked list
  Psquare nxt,*pre;//linked list pointers
};

struct Tsquare                   
{
  Tsymbol z;   //0=nothing, 1=my, 2=opponent, 3=outside
  Tprior h[2]; //evaluation for both players
  short x,y;   //coordinates 0..width-1, 0..height-1
  Psquare *inWinMoves; //pointer to winMoves1 array
};

#define MwinMoves 512  //buffer size for win combination

extern int diroff[9],moves,width,height,height2,depth,resulty,benchmark,mt,dpth,winEval[MwinMoves];
extern bool attackDone,defendDone,defendDone1,testDone,depthReached,carefulAttack,carefulDefend;
extern Psquare board,boardb,boardk,resultMove,loss4,highestEval,lastMove,bestMove,holdMove,goodMoves[4][2],*UwinMoves,winMoves1[MwinMoves],winMove[2];

DWORD stopTime();
void paintSquare(Psquare p);
void computer1();
void evaluate(Psquare p0);
int alfabeta(bool _careful, int strike, int player1, int logWin, Psquare last=0);
int defend();
void init();
bool doMove(Psquare p);
void getBestEval();
void firstMove();
void evalWinMoves();
void addWinMove(Psquare p);
bool notInWinMove(Psquare p);
Psquare try4(int player1);
Psquare Square(int x,int y);
int getEval(int player,Psquare p);
int getEval(Psquare p);
void print(char *format, ...);
void printXY(int x, int y, int w, char *format, ...);

#define nxtQ(p,i) ((Psquare)(((char*)(p))+(i*s)))
#define prvQ(p,i) ((Psquare)(((char*)(p))-(i*s)))
#define nxtP(p,i) (p=(Psquare)(((char*)(p))+(i*s)))
#define prvP(p,i) (p=(Psquare)(((char*)(p))-(i*s)))
#define nxtS(p,s) ((Psquare)(((char*)(p))+diroff[s]))
#define nxtI(p,s,i) ((Psquare)(((char*)(p))+diroff[((s)+(i))&7]))

template <class T> inline void amin(T &x,int m){ if(x<m) x=m; }
template <class T> inline void amax(T &x,int m){ if(x>m) x=m; }
template <class T> inline void aminmax(T &x,int l,int h){
 if(x<l) x=l;
 if(x>h) x=h;
}


