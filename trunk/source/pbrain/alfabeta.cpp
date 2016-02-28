/*
	(C) 2001-2005  Petr Lastovicka

	This program is free software; you can redistribute it and/or
	modify it under the terms of the GNU General Public License.
	*/
#include <windows.h>
#pragma hdrstop
#include <assert.h>
#include "board.h"

#define MOVE_NUM1 -3   //number of searched moves
#define MOVE_NUM2 2
#define DEFEND_MAX 20  //maximal number of defend moves
#define LOST_DECR 140
#define McurMoves 1024 //buffer size for searched moves

Psquare
curMoves[McurMoves]; //buffer for moves

static bool careful; //search attacks of both players

//-------------------------------------------------------------------

#ifdef DEBUG
int sl=0;  //visualization (ms)

void vis(Psquare p){
	if(sl>0) paintSquare(p);
	if(sl>1) Sleep(sl);
}
#endif

int distance(Psquare p1, Psquare p2)
{
	return max(abs(p1->x - p2->x), abs(p1->y - p2->y));
}

bool notInWinMove(Psquare p)
{
	return !p || p->inWinMoves>=UwinMoves || *p->inWinMoves!=p;
}

void addWinMove(Psquare p)
{
	if(p->inWinMoves>=UwinMoves || *p->inWinMoves!=p){
		if(UwinMoves>=winMoves1+MwinMoves){ assert(0); return; }
		p->inWinMoves=UwinMoves;
		*UwinMoves++=p;
	}
}

//-------------------------------------------------------------------
/*
recursive function
find out whether I win or lose
set variable bestMove if dpth==0
fill array winMoves1 if logWin==1 && y!=0 && strike==(y>0)
strike:0=defend,1=attack
*/
int alfabeta(int player1, Psquare *UcurMoves, int logWin, Psquare last1, Psquare last2, int strike)
{
	Psquare p, *q, *t, pDefend, *defendMoves1, *defendMoves2, *UwinMoves0, *badMoves;
	int y, m;
	int i, j, s, n;
	int pr, hr;
	int mustDefend, mustAttack, mayAttack;
	static int cnt;

	//check timeout
	if(--cnt<0){
		cnt=1000;
		if(GetTickCount()>stopTime()){
			terminateAI=2;
#ifdef DEBUG
			printXY(55, 0, 100, "timeout");
#endif
		}
	}
	benchmark++;
#ifdef DEBUG
	if(!(benchmark%3000)) printXY(60, 18, 130, "moves=%d", benchmark);
#endif

	p=goodMoves[3][player1];
	if(p){
		do{
			if(!(info_renju && checkForbid(p, player1)))
			{
				//player1 has already four 
				assert(dpth>0 || info_renju);
				if(logWin && strike) addWinMove(p);
				return 1000-dpth; //I win
			}
			p=p->h[player1].nxt;
		} while(p);
	}

	int player2=1-player1;
	p=goodMoves[3][player2];
	if(p){
		do{
			if(!(info_renju && checkForbid(p, player2)))
			{
				//player2 has already four 
				if(info_renju && checkForbid(p, player1)){
					y=depth-999;
				}else{
					p->z=player1+1;
#ifdef DEBUG
					vis(p);
#endif
					evaluate(p);
					y= -alfabeta(player2, UcurMoves, logWin, last2, last1, strike^1);
					p->z=0;
#ifdef DEBUG
					vis(p);
#endif
					evaluate(p);
					assert(dpth>0);
				}
				if(logWin && y && ((y>0) == strike)) addWinMove(p);
				return y;
			}
			p=p->h[player2].nxt;
		} while(p);
	}

	//find reasonable moves and store them to array curMoves
	Psquare *Utahy0=UcurMoves;
	if(strike) hr=player1; else hr=player2;
	mustDefend= mustAttack= mayAttack= 0;
	pDefend=0;
	p=goodMoves[2][player1];
	if(p){
		mustAttack++;
		do{
			if(!logWin && p->h[player1].pv >= H31){
				//I have three in a line => I should win
				if(!dpth) bestMove=p;
				return 999-dpth;
			}
			if(UcurMoves==curMoves+McurMoves) break;
			//insert square p to sorted array curMoves
			pr=p->h[hr].pv;
			for(q=UcurMoves++; q>Utahy0 && (*(q-1))->h[hr].pv < pr; q--){
				*q = *(q-1);
			}
			*q = p;
			p=p->h[player1].nxt;
		} while(p);
	}
	defendMoves1=UcurMoves; //beginning of defend moves

	for(p=goodMoves[2][player2]; p; p=p->h[player2].nxt){
		if(p->h[player2].pv >= H30+H21){
			//opponent has a triple => I must defend
			if(!mustDefend) mustDefend=1;
			if(p->h[player2].pv >= H31) mustDefend=2;
		}
		else{
			//don't bother with opponent's two pairs if I have triple
			if(mustAttack) continue;
		}
		pDefend=p;
		if(UcurMoves==curMoves+McurMoves) break; //buffer overflow
		//insert square p to sorted array curMoves
		pr=p->h[hr].pv;
		for(q=UcurMoves++; q>defendMoves1 && (*(q-1))->h[hr].pv < pr; q--){
			*q = *(q-1);
		}
		*q = p;
	}
	defendMoves2=UcurMoves;

	if(dpth>=depth){
		depthReached=true;
		if(!mustAttack && !mustDefend) return 0;
	}
	else if(!terminateAI){
		//defend
		if(pDefend){
			//look around opponent's move
			for(i=0; i<8; i++){
				s=diroff[i];
				p=pDefend; nxtP(p, 1);
				for(j=0; j<4 && (p->z!=3); j++, nxtP(p, 1)){
					if(p->h[player2].i==1){
						if(UcurMoves==curMoves+McurMoves) break; //buffer overflow
						//insert square p to sorted array curMoves
						pr=p->h[hr].pv;
						for(q=UcurMoves++; q>defendMoves2 && (*(q-1))->h[hr].pv < pr; q--){
							*q = *(q-1);
						}
						*q = p;
					}
				}
			}
		}
		else if(!strike){
			for(p=goodMoves[1][player2]; p; p=p->h[player2].nxt){
				if((!last2 || distance(p, last2)<7)
					&& (!mustAttack || p->h[player2].pv >= H30)){
					if(UcurMoves==curMoves+McurMoves) break; //buffer overflow
					//insert square p to sorted array curMoves
					pr=p->h[hr].pv;
					for(q=UcurMoves++; q>defendMoves2 && (*(q-1))->h[hr].pv < pr; q--){
						*q = *(q-1);
					}
					*q = p;
				}
			}
		}
		defendMoves2=UcurMoves; //end of defend moves

		//attack
		if(strike || careful || !pDefend){
			if(last1 && !mustDefend){
				//look around my last move
				for(i=0; i<8; i++){
					s=diroff[i];
					p=last1; nxtP(p, 1);
					for(j=0; j<4 && (p->z!=3); j++, nxtP(p, 1)){
						if(p->h[player1].i==1){
							mayAttack=1;
							if(!mustDefend || p->h[player1].pv >= H30){
								if(UcurMoves==curMoves+McurMoves) break; //buffer overflow
								//insert square p to sorted array curMoves
								pr=p->h[hr].pv;
								for(q=UcurMoves++; q>defendMoves2 && (*(q-1))->h[hr].pv < pr; q--){
									*q = *(q-1);
								}
								*q = p;
							}
						}
					}
				}
			}
			else{
				for(p=goodMoves[1][player1]; p; p=p->h[player1].nxt){
					if(UcurMoves==curMoves+McurMoves) break;
					mayAttack=1;
					if(!mustDefend || p->h[player1].pv >= H30){
						if(UcurMoves==curMoves+McurMoves) break; //buffer overflow
						//insert square p to sorted array curMoves
						pr=p->h[hr].pv;
						for(q=UcurMoves++; q>defendMoves2 && (*(q-1))->h[hr].pv < pr; q--){
							*q = *(q-1);
						}
						*q = p;
					}
				}
			}
		}
		badMoves=UcurMoves;
		//low evaluated squares
		if(!last1 && !last2 && !pDefend &&
			(UwinMoves==winMoves1 || !strike || player1)){
			n= MOVE_NUM1+int(depth*MOVE_NUM2) - int(UcurMoves-Utahy0);
			if(n>0){
				for(p=boardb; p<boardk; p++){
					pr=p->h[hr].pv;
					if(pr>H11 && pr<H21){
						assert(p->z==0 && p->h[hr].i==0);
						//insert square p to sorted array curMoves
						for(q=UcurMoves; q>badMoves && (*(q-1))->h[hr].pv<pr; q--){
							*q = *(q-1);
						}
						*q = p;
						if(n>0){
							UcurMoves++;
							n--;
						}
					}
				}
			}
		}
	}

	if(Utahy0==UcurMoves) return 0; //can't attack or I'm too deep
	if(strike && !mayAttack && pDefend && !mustAttack) return 0;

#ifdef DEBUG
	//print buffer size
	if(UcurMoves-curMoves > mt){
		mt=UcurMoves-curMoves;
		printXY(55, 0, 100, "buffer= %d", mt);
	}
#endif

	//go through array curMoves (is sorted from high evaluation to low evaluation)
	UwinMoves0=UwinMoves;
	m=-0x7ffe;
	for(t=Utahy0; t<UcurMoves; t++){
		p=*t;
		if(!dpth && loss4 && strike && notInWinMove(p)) continue;
		if(info_renju && checkForbid(p, player1)) continue;
		//do move
		p->z=player1+1;
#ifdef DEBUG
		vis(p);
#endif
		evaluate(p);
		dpth++;
		//recurse
		y= -alfabeta(player2, UcurMoves, logWin, last2,
			(t>=defendMoves1 && t<defendMoves2) ? last1 : p, strike^1);
		//erase square
		p->z=0;
#ifdef DEBUG
		vis(p);
#endif
		evaluate(p);
		dpth--;
		if(y>0){
			//I win
			if(!dpth) bestMove=p;
			if(!strike) UwinMoves=UwinMoves0;
			else if(logWin) addWinMove(p);
			return y;
		}
		if(y==0){
			if(!strike){
				//I hold
				UwinMoves=UwinMoves0; //delete win combination
				if(!dpth) bestMove=p;
				return y;
			}
			m=y;
		}
		else{
			if(y>=m){
				//I will probably lose, try other moves
				if(logWin && !strike) addWinMove(p);
				if(!dpth){
					//choose move to lose not so soon
					if(y>m || p->h[player2].pv > bestMove->h[player2].pv) bestMove=p;
				}
				m=y;
			}
		}
	}
	if(info_renju && m==-0x7ffe) return 0;
	return m;
}

int alfabeta(bool _careful, int strike, int player1, int logWin, Psquare last)
{
	careful=_careful;
	return alfabeta(player1, curMoves, logWin, last, 0, strike);
}
//-------------------------------------------------------------------
Psquare try4(int player1, Psquare last)
{
	int i, j, s;
	Psquare p, p2=0, y=0, result=0;

	//attack
	last->z=player1+1;
	evaluate(last);
	if(!goodMoves[3][1-player1]){
		p2=goodMoves[3][player1];
		if(p2){
			//defend - only one possibility
			p2->z=2-player1;
			evaluate(p2);

			p=goodMoves[3][player1];
			if(p){
				addWinMove(p);
				result=p; //player1 wins
			}
			else{
				p=goodMoves[3][1-player1];
				if(p){
					if(p->h[player1].pv>=H30){
						y=try4(player1, p);
						if(y) result=p;
					}
				}
				else{
					//search triples around last move
					for(i=0; i<8; i++){
						s=diroff[i];
						p=last; nxtP(p, 1);
						for(j=0; j<4 && (p->z!=3); j++, nxtP(p, 1)){
							if(p->h[player1].pv>=H30){
								//recurse
								y=try4(player1, p);
								if(y){
									result=p;
									goto e;
								}
							}
						}
					}
				}
			}
		e:
			p2->z=0;
			evaluate(p2);
		}
	}
	last->z=0;
	evaluate(last);
	if(result){
		addWinMove(p2);
		addWinMove(last);
	}
	return result;
}
//-------------------------------------------------------------------
//search only forced moves when attacker makes fours
//depth of recursion is unlimited
Psquare try4(int player1)
{
	Psquare p, y=0, *t;
	int j;

	assert(!goodMoves[3][0] && !goodMoves[3][1] || info_renju);

	//find triples of player1
	t=curMoves;
	for(j=1; j<=2; j++){
		for(p=goodMoves[j][player1]; p; p=p->h[player1].nxt){
			if(p->h[player1].pv>=H30){
				if(t==curMoves+McurMoves) break;
				*t++=p;
			}
		}
	}
	for(t--; t>=curMoves; t--){
		p=*t;
		y=try4(player1, p);
		if(y){
			evalWinMoves();
			return p;
		}
	}
	return 0;
}
//-------------------------------------------------------------------
//try to move on squares from array winMoves1
int defend()
{
	Psquare p;
	int m, mv, mh, y, yh, Nwins, i, j, jm=0, e, mi;
	const int player1=0;
	const int player2=1;
	static int winIndex[MwinMoves];

	dpth++;
	Nwins= UwinMoves-winMoves1;
	assert(Nwins>0);
	for(i=0; i<Nwins; i++){
		winIndex[i]=i;
	}

	mh=m=-0x7ffe;
	for(i=0; Nwins>0 && i<DEFEND_MAX; i++){
		//chose square which has the greatest evaluation
		mv=-0x7ffe;
		for(j=Nwins-1; j>=0; j--){
			e= winEval[winIndex[j]];
			if(e>mv){ mv=e; jm=j; }
		}
		if(mv < H12) break;
		//remove square from the list
		mi=winIndex[jm];
		p= winMoves1[mi];
		Nwins--;
		winIndex[jm]=winIndex[Nwins];
		assert(p>=boardb && p<boardk && !p->z);
		p->z=player1+1;
#ifdef DEBUG
		vis(p);
#endif
		evaluate(p);
		//opponent's attack
		y = -alfabeta(carefulDefend, 1, player2, 0);
		p->z=0;
#ifdef DEBUG
		vis(p);
#endif
		evaluate(p);
		//evaluation
		yh= mv + y*20;
		if(yh>mh){
			m=y;
			mh=yh;
			bestMove=p;
			if(y>0 || y==0 && !winMove[player1]){
				holdMove=p;
				break; //I hold
			}
		}
		else if(y<0){
			winEval[mi]-=LOST_DECR;
		}
	}
	if(m<0 && holdMove) bestMove=holdMove;
	dpth--;
	return m;
}
//-------------------------------------------------------------------
