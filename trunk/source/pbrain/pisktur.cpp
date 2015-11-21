/*
	(C) 2004-2006  Petr Lastovicka

	This program is free software; you can redistribute it and/or
	modify it under the terms of the GNU General Public License.
	*/
#include <windows.h>
#pragma hdrstop
#include <assert.h>
#include <stdio.h>
#include "board.h"

#define MATCH_SPARE 7      //how much is time spared for the rest of game
#define TIMEOUT_PREVENT 3  //how much is alfabeta slower when the depth is increased

const char *infotext="author=\"Petr Lastovicka\", version=\"7.6\", country=\"Czech Republic\",  www=\"http://petr.lastovicka.sweb.cz\"";


void brain_init()
{
	if(width<5 || width>127 || height<5 || height>127){
		pipeOut("ERROR size of the board");
		return;
	}
	init();
	pipeOut("OK");
}

void brain_restart()
{
	brain_init();
}

int brain_takeback(int x, int y)
{
	Psquare p=Square(x, y);
	if(p<boardb || p>=boardk || !p->z) return 2;
	p->z=0;
	moves--;
	evaluate(p);
	lastMove=0;
	return 0;
}

static bool doMove0(Psquare p, int z)
{
	if(p<boardb || p>=boardk || p->z) return false;
	p->z= z;
	moves++;
	evaluate(p);
	lastMove=p;
	return true;
}

void brain_my(int x, int y)
{
	if(!doMove0(Square(x, y), 1)){
		pipeOut("ERROR my move [%d,%d]", x, y);
	}
}

void brain_opponents(int x, int y)
{
	if(!doMove0(Square(x, y), 2)){
		pipeOut("ERROR opponents's move [%d,%d]", x, y);
	}
}

void brain_block(int x, int y)
{
	if(!doMove0(Square(x, y), 3)){
		pipeOut("ERROR winning move [%d,%d]", x, y);
	}
}

bool doMove(Psquare p)
{
	if(p<boardb || p>=boardk || p->z) return false;
	if(!terminateAI || !resultMove) resultMove=p;
	return true;
}

void computer()
{
	int i;
	DWORD t0, t1;
	Psquare p;

	resultMove=0;
	holdMove=0;
	highestEval=0;
	firstMove();
	if(resultMove) return;

	for(p=boardb; p<boardk; p++){
		p->inWinMoves= winMoves1+MwinMoves;
	}
	UwinMoves=winMoves1;

	if(doMove(try4(0))){
#ifdef DEBUG
		print("win4");
#endif
		static bool w=false;
		if(!w && resultMove->h[0].pv<H31){
			w=true;
			pipeOut("MESSAGE  I win !");
		}
		return;
	}
	attackDone=defendDone=defendDone1=testDone=carefulAttack=carefulDefend=false;
	loss4=try4(1);
	if(loss4) doMove(loss4);
	else getBestEval();
	assert(resultMove);
#ifdef DEBUG
	resulty=0;
	mt=0;
#endif

	for(i=4; i<=50; i+=2){
		depth=i;
#ifdef DEBUG
		printXY(200, 0, 80, "   depth= %d   ", depth);
#endif
		benchmark=0;
		t0=GetTickCount();
		computer1();
		t1=GetTickCount();
		if(terminateAI || t1+TIMEOUT_PREVENT*(t1-t0)>=stopTime()) break;
	}

	if(terminateAI) depth-=2;
	pipeOut("DEBUG depth %d, nodes %d", depth, benchmark);
#ifdef DEBUG
	if(resulty>0) print("win");
	if(resulty<0) print("loss");
	if(resulty==0) print("---");
	printXY(60, 18, 130, "moves=%d", benchmark);
#endif
}

DWORD stopTime()
{
	return start_time + min(info_timeout_turn, info_time_left/MATCH_SPARE)-30;
}

void brain_turn()
{
	computer();
	do_mymove(resultMove->x, resultMove->y);
}

void brain_end()
{
}

//----------------------------------------------------------------------
#ifdef DEBUG

HWND wnd=FindWindow("Piskvork", 0);

void vprint(int x, int y, int w, char *format, va_list va)
{
	HDC dc= GetDC(wnd);
	RECT rc;
	rc.top=y;
	rc.bottom=y+16;
	rc.left= x - w/2;
	rc.right= rc.left + w;
	char buf[128];
	int n = vsprintf(buf, format, va);
	SetTextAlign(dc, TA_CENTER);
	ExtTextOut(dc, x, y, ETO_OPAQUE, &rc, buf, n, 0);
	ReleaseDC(wnd, dc);
}

void printXY(int x, int y, int w, char *format, ...)
{
	va_list va;
	va_start(va, format);
	vprint(x, y, w, format, va);
	va_end(va);
}

void print(char *format, ...)
{
	RECT rc;
	GetClientRect(wnd, &rc);
	va_list va;
	va_start(va, format);
	vprint(rc.right*900/1000, 0, 80, format, va);
	va_end(va);
}

void paintSquare(Psquare p)
{
	SendMessage(wnd, WM_APP+123, p->z, MAKELPARAM(p->x, p->y));
}

void brain_eval(int x, int y)
{
	Psquare p= Square(x, y);
	printXY(300, 0, 60, "%d,%d", p->h[0].pv, p->h[1].pv);
	printXY(300, 16, 60, "%d,%d", getEval(0, p), getEval(1, p));
	printXY(300, 32, 60, "%d", getEval(p));
}


#endif
