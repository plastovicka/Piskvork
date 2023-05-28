/*
	(C) Petr Lastovicka

	This program is free software; you can redistribute it and/or
	modify it under the terms of the GNU General Public License.
	*/
#include <windows.h>
#pragma hdrstop
#include <assert.h>
#include "board.h"

#define EVAL_RAND1 20  //randomness
#define EVAL_RAND2 30  //maximal randomness for low evaluation
#define AHEAD_DEPTH 4  //depth search to find good total evaluation
#define AHEAD_IRRELEVANCE1 12
#define AHEAD_IRRELEVANCE2 3
#define NUM_GOOD0 100
#define NUM_GOOD1 30
#define OFFENSIVE 1   
#define DEFEND_HIGHEVAL 70

int
height2,   //height+2
 moves,     //moves count
 blockCount,//blocked squares count (continuous game or renju fouls)
 depth,     //maximal recursion depth (is increased until timeout)
 diroff[9];	//pointer difference to adjacent squares

int
winEval[MwinMoves], //evaluation of winMoves1
 winMoveDepth[2],    //depth of win combination
 sum[2],    //total evaluation for both players
 dpth=0;    //current recursion depth

Psquare
board=0,        //board array
 boardb, boardk, //pointer to the beginning and end of board
 loss4,          //opponent can win through foursomes
 resultMove,     //the best move
 highestEval,    //square which has the best evaluation
 lastMove,
 holdMove;

Psquare
goodMoves[4][2],     //linked lists of highly evaluated squares, for both players
 winMoves1[MwinMoves],//buffer for win combination
 *UwinMoves,
 winMove[2],     //the first move of win combination
 bestMove;

bool
depthReached,   //depth has beeb reached (and can be increased)
 attackDone, defendDone, defendDone1, testDone,
 carefulAttack, carefulDefend;

const int priority[][3]={{0, 1, 1}, {H10, H11, H12}, {H20, H21, H22}, {H30, H31, 0}, {H4, 0, 0}};

#ifdef DEBUG
int mt;       //curMoves buffer utilization
int resulty;  //win/loss/unknown
#endif
int benchmark;//counter of searched moves

//-------------------------------------------------------------------
signed char data[]={
	15, 1, 4, 0, 0, 0, 1, 1, 2, 3, 2, 2, 4, 2, 5, 3, 3, 4, 3, 3, 5, 5, 4, 3, 3, 6, 4, 5, 6, 5, 5, 4, 3, 1,
	11, 1, 1, 1, 0, 0, 2, 2, 3, 2, 3, 3, 1, 3, 2, 4, 1, 5, 3, 3, 3, 2, 5, 1, 4, 3,
	11, 1, 1, 0, 0, 0, 1, 1, 3, 2, 2, 2, 2, 3, 3, 3, 2, 4, 2, 5, 1, 5, 1, 4, 0, 3,
	 9, 1, 0, 0, 0, 3, 0, 1, 1, 2, 0, 2, 2, 3, 3, 3, 3, 4, 2, 4, 1, 1,
	 9, 1, 1, 1, 0, 0, 2, 2, 3, 2, 3, 3, 2, 3, 1, 4, 1, 3, 1, 5, 2, 4,
	 9, 1, 1, 0, 0, 0, 0, 1, 1, 1, 2, 2, 2, 1, 2, 3, 1, 2, 0, 3, 1, 4,
	 9, 1, 0, 0, 2, 0, 1, 0, 1, 1, 0, 1, 2, 2, 3, 2, 3, 3, 2, 3, 0, 2,
	 9, 1, 1, 1, 0, 0, 2, 2, 0, 2, 2, 1, 3, 1, 1, 2, 1, 3, 0, 3, 3, 0,
	 9, 1, 0, 0, 2, 0, 1, 1, 1, 2, 2, 1, 1, 4, 3, 2, 4, 3, 2, 3, 0, 1,
	 8, 1, 0, 2, 1, 1, 1, 2, 2, 0, 2, 2, 3, 0, 3, 1, 3, 2, 4, 0,
	 8, 1, 1, 1, 3, 0, 0, 2, 3, 1, 1, 3, 2, 2, 2, 3, 1, 4, 3, 3,
	 7, 1, 0, 0, 1, 1, 0, 1, 2, 2, 3, 2, 3, 3, 2, 3, 0, 2,
	 7, 1, 3, 2, 2, 1, 2, 2, 1, 1, 1, 0, 0, 0, 0, 1, 1, 2,
	 7, 1, 0, 0, 0, 1, 1, 0, 0, 3, 2, 1, 3, 2, 1, 2, 1, -1,
	 7, 1, 1, 0, 0, 1, 0, 2, 2, 2, 2, 1, 3, 2, 5, 2, 3, 0,
	 7, 1, 0, 0, 0, 1, 2, 0, 0, 3, 2, 1, 1, 3, 1, 2, 2, 3,
	 7, 1, 1, 0, 0, 0, 0, 1, 1, 1, 2, 2, 1, 2, 1, 3, 2, 3,
	 7, 1, 0, 1, 1, 1, 2, 0, 0, 2, 1, 2, 3, 4, 2, 3, 2, 1,
	 6, 1, 1, 0, 0, 0, 0, 2, 1, 1, 3, 3, 2, 2, 1, -1,
	 6, 1, 1, 0, 0, 0, 1, 1, 2, 1, 2, 2, 1, 2, 0, -1,
	 6, 1, 0, 0, 1, 0, 2, 1, 3, 1, 3, 2, 2, 2, 1, -1,
	 6, 1, 3, 0, 0, 0, 1, 1, 2, 1, 2, 2, 1, 2, -1, 0,
	 6, 1, 0, 0, 1, 1, 3, 1, 2, 2, 4, 3, 3, 3, 2, 4,
	 6, 1, 0, 2, 0, 0, 1, 1, 1, 2, 2, 2, 2, 1, 3, 0,
	 6, 1, 2, 1, 0, 0, 2, 2, 0, 1, 2, 3, 1, 2, 2, 0,
	 6, 1, 2, 1, 0, 0, 1, 3, 0, 1, 2, 3, 1, 2, 2, 0,
	 5, 2, 2, 2, 0, 0, 1, 0, 1, 1, 0, 1, 2, 1, 1, 2,
	 5, 1, 0, 0, 2, 1, 0, 1, 2, 3, 1, 2, 1, 3,
	 5, 1, 1, 0, 0, 0, 1, 1, 1, 2, 2, 1, 1, -1,
	 5, 1, 1, 1, 1, 0, 0, 2, 1, 3, 1, 2, 2, 2,
	 5, 1, 1, 0, 0, 0, 0, 1, 1, 1, 2, 3, 2, 2,
	 5, 2, 0, 0, 1, 1, 1, 2, 0, 1, 2, 1, 2, 0, 3, 0,
	 5, 1, 0, 2, 1, 0, 2, 0, 2, 1, 1, 1, -1, 3,
	 5, 1, 1, 0, 1, 1, 0, 1, 2, 1, 3, 2, 2, -1,
	 5, 1, 1, 1, 0, 0, 2, 1, 0, 2, 3, 1, 0, 1,
	 5, 2, 1, 1, 0, 0, 2, 1, 1, 3, 1, 2, 0, 1, 3, 1,
	 5, 2, 1, 0, 0, 0, 0, 1, 2, 1, 1, 1, 1, 2, -1, 2,
	 5, 1, 1, 0, 1, 2, 0, 1, 3, 2, 2, 1, 2, -1,
	 5, 2, 0, 1, 1, 2, 2, 1, 3, 0, 0, 3, 1, 1, 0, 2,
	 5, 1, 2, 0, 0, 2, 1, 1, 1, 2, 2, 2, 0, 0,
	 5, 1, 0, 0, 1, 1, 1, 0, 1, 2, 0, 2, 0, 1,
	 4, 1, 1, 0, 0, 1, 1, 3, 1, 2, -1, 0,
	 4, 1, 1, 0, 0, 1, 1, 1, 1, 2, -1, 0,
	 4, 1, 1, 0, 1, 1, 0, 2, 2, 2, 0, 0,
	 4, 1, 2, 0, 0, 0, 0, 2, 1, 1, 2, 2,
	 4, 1, 0, 0, 1, 1, 3, 1, 2, 2, 3, 3,
	 4, 2, 2, 0, 0, 0, 2, 2, 1, 1, 0, -1, 1, -1,
	 4, 1, 2, 0, 0, 0, 2, 1, 1, 1, 2, 2,
	 4, 1, 1, 0, 0, 1, 2, 0, 1, 2, -1, 0,
	 4, 1, 1, 0, 0, 0, 0, 2, 1, 1, -1, -1,
	 4, 1, 1, 0, 0, 1, 2, 2, 1, 2, -1, 0,
	 4, 1, 0, 0, 1, 1, 2, 1, 2, 2, 3, 3,
	 3, 1, 0, 1, 0, 0, 1, 0, 1, 1,
	 3, 1, 0, 0, 2, 0, 1, 1, 2, 2,
	 3, 4, 1, 0, 0, 1, 1, 2, 0, 2, 0, 0, 2, 0, 2, 2,
	 3, 1, 1, 2, 0, 0, 1, 1, 1, 3,
	 3, 1, 1, 0, 0, 0, 1, 1, 1, 2,
	 3, 1, 0, 0, 0, 2, 0, 1, 0, -1,
	 3, 1, 0, 0, 2, 1, 1, 1, 2, 2,
	 3, 1, 0, 0, 0, 1, 1, 2, 1, 1,
	 3, 2, 0, 0, 2, 2, 1, 1, 2, 1, 1, 2,
	 3, 1, 0, 1, 3, 0, 2, 0, 1, 1,
	 3, 1, 0, 0, 2, 1, 2, 2, 1, 1,
	 3, 4, 0, 0, 1, 1, 2, 2, 1, -1, -1, 1, 3, 1, 1, 3,
	 3, 1, 0, 0, 0, 2, 1, 2, 1, 0,
	 3, 2, 1, 0, 0, 3, 1, 2, 2, 3, 1, 1,
	 3, 2, 0, 0, 0, 3, 0, 2, 1, 3, -1, 3,
	 3, 1, 0, 0, 0, 3, 1, 2, 1, 1,
	 3, 1, 1, 0, 0, 2, 1, 2, 1, 1,
	 3, 1, 0, 0, 2, 1, 1, 2, 0, 2,
	 3, 1, 0, 0, 2, 2, 1, 2, 0, 1,
	 3, 1, 0, 0, 3, 3, 2, 2, 1, 1,
	 3, 1, 0, 0, 3, 2, 2, 2, 1, 1,
	 3, 1, 0, 0, 3, 1, 2, 2, 3, 3,
	 3, 6, 0, 0, 1, 0, 2, 0, 1, 1, 1, -1, 0, 2, 0, -2, 2, 2, 2, -2,
	 3, 3, 0, 0, 3, 2, 2, 1, 1, 0, 1, 1, 0, 1,
	 2, 3, 0, 0, 1, 1, 0, 2, 2, 0, 1, 2,
	 2, 8, 0, 0, 1, 0, -1, -1, 0, -1, 1, -1, 2, -1, -1, 1, 0, 1, 1, 1, 2, 1,
	 2, 2, 0, 0, 2, 2, 1, 3, 3, 1,
	 1, 8, 0, 0, -1, 0, 1, 0, 0, 1, 0, -1, 1, 1, -1, 1, 1, -1, -1, -1,
	 0, 0
};
//-------------------------------------------------------------------
Psquare Square(int x, int y) //indexed from zero
{
	return boardb + x*height2+(y+1);
}
//-------------------------------------------------------------------
struct PR2 { short h[2]; };
PR2 K[262144];         //evaluation for all combinations of 9 squares
static int comb[10];   //current combination
static PR2 *genOutput; //write position in array K
static int genCount[4];//number of empty fields, stones and borders
//-------------------------------------------------------------------
void gen2(int *pos)
{
	int *pb, *pe;
	int n[4], a1, a2;
	int s;
	bool blocked[2];

	if(pos==comb+9){
		a1=a2=0;
		if(comb[4]==0){ //the middle square must be empty
			n[1]=genCount[1]; n[2]=genCount[2]; n[3]=genCount[3];
			blocked[0]=blocked[1]=false;
			pb=comb;
			pe=pb+4;
			while(pe!=comb+9){
				if(!n[3]){ //must not be outside the board
					if(!n[2]){
						//my chances
						s=0;
						if(!*pb && pe[1]<2 && pb!=comb+4){
							s++;
							if(!*pe && pe!=comb+4) s++;
						}
						if(info_exact5==1 && n[1]==4 && 
							(pe<comb+8 && pe[1]==1 || pb>comb && pb[-1]==1)
							|| info_caro==1 && 
							(pe<comb+8 && pe[1]==2 && pb>comb && pb[-1]==2)){
							blocked[0]=true;
						}
						amin(a1, priority[n[1]][s]);
					}
					if(!n[1]){
						//opponent's chances
						s=0;
						if(!*pb && !(pe[1]&1) && pb!=comb+4){
							s++;
							if(!*pe && pe!=comb+4) s++;
						}
						if(info_exact5==1 && n[2]==4 &&
							(pe<comb+8 && pe[1]==2 || pb>comb && pb[-1]==2)
							|| info_caro==1 &&
							(pe<comb+8 && pe[1]==1 && pb>comb && pb[-1]==1)){
							blocked[1]=true;
						}
						amin(a2, priority[n[2]][s]);
					}
				}
				n[*++pe]++;
				n[*pb++]--;
			}
		}
		//store computed evaluation to the array
		genOutput->h[0]= short(blocked[0] ? 0 : a1);
		genOutput->h[1]= short(blocked[1] ? 0 : a2);
		genOutput++;
	}
	else{
		//generate last five squares of combination
		for(int z=0; z<4; z++){
			*pos=z;
			gen2(pos+1);
		}
	}
}

//generate first four squares of combination
void gen1(int *pos)
{
	if(pos==comb+5){
		gen2(pos);
	}
	else{
		for(int z=0; z<4; z++){
			*pos=z;
			genCount[z]++;
			gen1(pos+1);
			genCount[z]--;
		}
	}
}

void gen()
{
	genOutput=K;
	gen1(comb);
}
//-------------------------------------------------------------------
void init()
{
	int x, y, k;
	Psquare p;
	Tprior *pr;

	memset(goodMoves, 0, sizeof(goodMoves));
	memset(winMove, 0, sizeof(winMove));
	sum[0]= sum[1]= 0;

	//alocate the board
	delete[] board;
	height2=height+2;
	board= new Tsquare[(width+12)*(height2)];
	boardb= board + 6*height2;
	boardk= boardb+ width*height2;
	// 5 4 3
	// 6 8 2
	// 7 0 1
	diroff[0]=sizeof(Tsquare);
	diroff[4]=-diroff[0];
	diroff[1]=sizeof(Tsquare)*(1+height2);
	diroff[5]=-diroff[1];
	diroff[2]=sizeof(Tsquare)* height2;
	diroff[6]=-diroff[2];
	diroff[3]=sizeof(Tsquare)*(-1+height2);
	diroff[7]=-diroff[3];
	diroff[8]=0;

	//initialize the board
	p=board;
	for(x=-6; x<=width+5; x++){
		for(y=-1; y<=height; y++){
			p->z= (x<0 || y<0 || x>=width || y>=height) ? 3 : 0;
			p->x= (short)x;
			p->y= (short)y;
			for(k=0; k<2; k++){
				pr=&p->h[k];
				pr->i=0;
				pr->pv=4;
				pr->ps[0]=pr->ps[1]=pr->ps[2]=pr->ps[3]=1;
			}
			p++;
		}
	}
	moves=blockCount=0;
}
//-------------------------------------------------------------------
bool checkForbid(Psquare p, int player)
{
	int i, poc, s, y, n3, n4, n6;
	Psquare p1, p2;

	if((moves & 1) != player) return false;
	Tprior *pr = &p->h[player];
	if(pr->pv < 2*H21) return false;

	if(pr->pv >= H4)
	{
		n6=0;
		for(i=0; i<4; i++)
		{
			if(pr->ps[i] >= H4)
			{
				s=diroff[i];
				p1=p2=p;
				poc=-1;
				do{
					prvP(p1, 1);
					poc++;
				} while(p1->z==player+1);
				do{
					nxtP(p2, 1);
					poc++;
				} while(p2->z==player+1);
				if(poc>=5){
					if(poc==5) return false; //five in a row
					n6++;
				}
			}
		}
		return n6>0; //overline
	}

	n3=n4=0;
	for(i=0; i<4; i++)
	{
		y=pr->ps[i];
		if(y>=H30) n4++;
		else if(y>=H21) n3++;
	}
	return n4>1 || n3>1; //double-four or double-three
}
//-------------------------------------------------------------------
//evaluate squares around p0 to distance 4
void evaluate(Psquare p0)
{
	int i, k, m, s, h;
	Tprior *pr;
	Psquare p, q, qk, *up, pe, pk1;
	int *u;
	int ind;
	unsigned pattern;
	static int last_exact5=-1, last_caro=-1;

	if(info_exact5!=last_exact5 || info_caro!=last_caro){
		last_exact5=info_exact5;
		last_caro=info_caro;
		gen();
	}
	//remove not empty square from list and set its evaluation to zero
	if(p0->z){
		for(k=0; k<2; k++){
			pr=&p0->h[k];
			if(pr->pv){
				if(pr->i){
					if((*pr->pre= pr->nxt)!=0) pr->nxt->h[k].pre= pr->pre;
					pr->i=0;
				}
				sum[k]-= pr->pv;
				pr->pv= pr->ps[0]= pr->ps[1]= pr->ps[2]= pr->ps[3]= 0;
			}
		}
	}
	//cycle all directions
	for(i=0; i<4; i++){
		s=diroff[i];
		pk1=p0;
		nxtP(pk1, 5);
		pe=p0;
		p=p0;
		for(m=4; m>0; m--){
			prvP(p, 1);
			if(p->z==3){
				nxtP(pe, m);
				nxtP(p, 1);
				break;
			}
		}
		pattern=0;
		qk=pe;
		prvP(qk, 9);
		for(q=pe; q!=qk; prvP(q, 1)){
			pattern*=4;
			pattern+=q->z;
		}
		while(p->z!=3){
			if(!p->z){
				for(k=0; k<2; k++){ //both players
					pr= &p->h[k];
					//change evaluation in one direction
					u= &pr->ps[i];
					h= K[pattern].h[k];
					if(h>=H4 && info_exact5 &&
						(prvQ(p, 5)->z==k+1 && nxtQ(p, 1)->z==0 || nxtQ(p, 5)->z==k+1 && prvQ(p, 1)->z==0)
						|| info_caro &&
						(prvQ(p, 5)->z==2-k && nxtQ(p, 1)->z==2-k || nxtQ(p, 5)->z==2-k && prvQ(p, 1)->z==2-k)) {
						h=0;
					}
					m=h-*u;
					if(m){
						*u=h;
						sum[k]+=m;
						pr->pv+=m;
						//choose linked list
						ind=0;
						if(pr->pv >= H21){
							ind++;
							if(pr->pv >= 2*H21){
								ind++;
								if(pr->pv >= H4) ind++;
							}
						}
						//put square to another linked list
						if(ind!=pr->i){
							//remove
							if(pr->i){
								if((*pr->pre= pr->nxt)!=0) pr->nxt->h[k].pre= pr->pre;
							}
							//append
							if((pr->i=ind)!=0){
								up= &goodMoves[ind][k];
								pr->nxt= *up;
								if(*up) (*up)->h[k].pre= &pr->nxt;
								pr->pre= up;
								*up= p;
							}
						}
					}
				}
			}
			nxtP(p, 1);
			if(p==pk1) break;
			nxtP(pe, 1);
			pattern>>=2;
			pattern+= pe->z << 16;
		}
	}
}
//-------------------------------------------------------------------
int getEval(int player1, Psquare p0)
{
	int i, s, y, c, c1, c2, n, d1, d2;
	Psquare p;

	y=0;
	//count symbols on 8 squares around p0
	c1=c2=0;
	for(i=0; i<8; i++){
		s=diroff[i];
		p=p0;
		nxtP(p, 1);
		if(p->z==player1+1) c1++;
		if(p->z==2-player1) c2++;
	}
	c=c1+c2;
	if(c==0) y-=20; //too empty
	if(c>4) y-=4*(c-3); //too many

	//blocked directions
	d1=d2=0;
	for(i=0; i<4; i++){
		if(p0->h[player1].ps[i]<=1) d1++;
		if(p0->h[1-player1].ps[i]<=1) d2++;
	}

	//detect if there is only one player here
	if(c2==0 && c1>0 && p0->h[player1].pv>=H12){
		y+= (c1+1)*5;
	}
	if(d2==4){
		n=0;
		for(i=0; i<4; i++){
			if(p0->h[player1].ps[i]>=H12) n++;
		}
		y+=15;
		if(n>1) y+=n*64;
	}
	return y + p0->h[player1].pv;
}


int getEval(Psquare p)
{
	int a, b;
	a=getEval(0, p);
	b=getEval(1, p);
	return a>b ? a+b/2 : a/2+b;
}

//evaluate all squares in array winMoves
void evalWinMoves()
{
	int Nwins;
	int *e;
	Psquare *t, p;

	carefulAttack=true;
	//add high priority squares
	for(p=goodMoves[2][1]; p; p=p->h[1].nxt){
		addWinMove(p);
	}
	//evaluate
	Nwins= int(UwinMoves-winMoves1);
	assert(Nwins<=MwinMoves);
	for(t=UwinMoves-1, e=winEval+Nwins-1; t!=winMoves1-1; t--, e--){
		*e= getEval(1, *t);
		if(*t==highestEval) *e+=DEFEND_HIGHEVAL;
	}
	//add my chances to make four
	for(p=goodMoves[1][0]; p; p=p->h[0].nxt){
		if(p->h[0].pv<H30) continue;
		addWinMove(p);
		if(p->inWinMoves==UwinMoves-1){
			winEval[Nwins++]= p->h[0].pv>>3;
		}
	}
}

//-------------------------------------------------------------------
//find square which has greatest evaluation
//return 0 if evaluation is too low
Psquare findMax()
{
	Psquare p, t;
	int m, r;
	int i, k;

	m=-1;
	t=0;
	for(i=2; i>0 && !t; i--){
		for(k=0; k<2; k++){
			for(p=goodMoves[i][k]; p; p=p->h[k].nxt){
				r= getEval(p);
				if(r>m){
					m=r;
					t=p;
				}
			}
		}
	}
	return t;
}


//find out what total evaluation will be after AHEAD_DEPTH moves
int lookAhead(int player1)
{
	Psquare p;
	int y;

	if(goodMoves[3][player1]) return 700;
	int player2=1-player1;
	p=goodMoves[3][player2];
	if(!p && dpth<AHEAD_DEPTH) p=findMax();
	if(!p){
		if(info_timeout_turn < 100)
		{
			y= sum[0]-sum[1];
		}
		else{
			//evaluate entire board
			y = 0;
			for(Psquare q = boardb; q < boardk; q++)
			{
				if(q->z || q->h[0].pv == q->h[1].pv) continue;

				int count[4];
				count[0]=count[1]=count[2]=0; //empty, our, opponent

				//count of symbols on 5x5 square
				Psquare q2 = q - 2*height2 - 2;
				for(int i=0; i < 5; q2+=height2, i++)
					for(int j=0; j<5; j++)
						count[q2[j].z]++;

				//thank Farmer Lu for this formula
				y += (q->h[0].pv - q->h[1].pv) * (count[0] + (count[1] - count[2])/2 - 1);
			}
			y /= AHEAD_IRRELEVANCE1;
		}

		for(int i=2; i>0; i--){
			for(p=goodMoves[i][0]; p; p=p->h[0].nxt) y+=NUM_GOOD0;
			for(p=goodMoves[i][1]; p; p=p->h[1].nxt) y-=NUM_GOOD1;
		}
		if(player1) y=-y;
		return int(y/AHEAD_IRRELEVANCE2);
	}
	dpth++;
	p->z=player1+1;
	evaluate(p);
	y= -lookAhead(player2);
	p->z=0;
	evaluate(p);
	dpth--;
	return y;
}

//-------------------------------------------------------------------

DWORD seed= GetTickCount();

unsigned rnd(unsigned maxValue)
{
	seed=seed*367413989+174680251;
	return (unsigned)(UInt32x32To64(maxValue, seed)>>32);
}

void databaseMove()
{
	signed char *s, *sn;
	Psquare p, k;
	int i, x, y, x1, y1, flip, len1, len2, left, top, right, bottom;

	if(moves==blockCount) return;

	//board rectangle
	for(p=boardb; p->z!=1 && p->z!=2; p++);
	left=p->x;
	for(k=boardk; k->z!=1 && k->z!=2; k--);
	right=k->x;
	top=bottom=k->y;
	for(; p<k; p++){
		if(p->z==1 || p->z==2){
			amax(top, p->y);
			amin(bottom, p->y);
		}
	}
	//find current board in the database
	for(s=data;; s=sn){
		len1=*s++;
		len2=*s++;
		sn= s + 2*(len1+len2);
		if(len1!=moves){
			if(len1<moves) return;
			continue;
		}
		//try all symmetries
		for(flip=0; flip<8; flip++){
			for(i=0;; i++){
				x1=s[2*i];
				y1=s[2*i+1];
				if(i==len1){
					s += 2*(len1+rnd(len2));
					x1=*s++;
					y1=*s;
				}
				switch(flip){
					case 0: x=left+x1; y=top+y1; break;
					case 1: x=right-x1; y=top+y1; break;
					case 2: x=left+x1; y=bottom-y1; break;
					case 3: x=right-x1; y=bottom-y1; break;
					case 4: x=left+y1; y=top+x1; break;
					case 5: x=right-y1; y=top+x1; break;
					case 6: x=left+y1; y=bottom-x1; break;
					default: x=right-y1; y=bottom-x1; break;
				}
				p=Square(x, y);
				if(i==len1){
					//opening found
					const int B = 4; //distance from border
					if(x>=B && y>=B && x<width-B && y<height-B) doMove(p);
					return;
				}
				if(p->z != 2-(i&1)) break;
			}
		}
	}
}

void firstMove()
{
	//the first move is in the middle of the board
	if(moves==0){
		doMove(Square(width/2+rnd(5)-2, height/2+rnd(5)-2));
		return;
	}

	if(moves==2){
		//find both symbols 
		Psquare p, p1=0, p2=0;
		int i, y1, y2;
		for(p = boardb; p<boardk; p++) {
			if(p->z == 1) p1 = p;
			if(p->z == 2) p2 = p;
		};
		//evaluate
		y1 = 5;  y2 = 0;
		for(i = 0; i<8; i++) {
			y1 += nxtS(p1, i)->h[0].pv;
			y2 += nxtS(p2, i)->h[1].pv;
		}
		//random adjacent square
		doMove(nxtS(y1 > y2 ? p1 : p2, rnd(8)));
	}

	databaseMove();
	//already four in a line -> don't think
	if(goodMoves[3][0] && (!info_continuous || !goodMoves[3][0]->h[0].nxt || !goodMoves[3][1])){
		if(doMove(goodMoves[3][0])) return; //I win
	}
	doMove(goodMoves[3][1]); //I have to defend
}


//find square which has very good evaluation
void getBestEval()
{
	int i, r, m, d, a, b;
	Psquare p, _bestMove;
	int Nresults=0;

	_bestMove=0;
	if(moves>4){
		m=-0x7ffffffe;
		for(p=boardb; p<boardk; p++){
			if(!p->z && (p->h[0].pv>10 || p->h[1].pv>10)){
				a=getEval(0, p);
				b=getEval(1, p);
				r= (a>b) ? int((a + b/2)*OFFENSIVE) : (a/2 + b);
				p->z=1;
				evaluate(p);
				r-= lookAhead(1);
				p->z=0;
				evaluate(p);
				if(r>m && !(info_renju && checkForbid(p,0))){
					m=r;
					_bestMove=p;
					Nresults=1;
				}
				else if(r>m-EVAL_RAND1 && !(info_renju && checkForbid(p, 0))){
					Nresults++;
					if(!rnd(Nresults)) _bestMove=p;
				}
			}
		}
	}
	if(!_bestMove){
		m=-0x7fffff;
		for(p=boardb; p<boardk; p++){
			if(!p->z){
				r=getEval(p);
				if(r>m) m=r;
			}
		}
		d= abs(m/12);
		amax(d, EVAL_RAND2);
		Nresults=0;
		for(p=boardb; p<boardk; p++){
			if(!p->z){
				if(getEval(p) >= m-d && !(info_renju && checkForbid(p, 0))){
					Nresults++;
					if(!rnd(Nresults)) _bestMove=p;
				}
			}
		}
	}
	doMove(_bestMove);
	highestEval=_bestMove;

	if(lastMove){
		p=lastMove;
		for(i=0;; i++){
			if(i==8){
				//opponent made an isolated move -> defend it
				doMove(nxtS(p, rnd(8)));
				break;
			}
			if(nxtS(p, i)->h[1].ps[i&3]!=H12) break;
		}
	}
}

//-------------------------------------------------------------------

void computer1()
{
	int y=0;
	const int player1=0, player2=1;

	if(loss4){
		winMove[player2]=loss4;
		winMoveDepth[player2]=10;
		y=-1;
	}
	bestMove=0;
	//attack
	if(!attackDone && (!loss4 || winMove[player1])){
#ifdef DEBUG
		print(carefulAttack ? "attack2" : "attack");
#endif
		depthReached=false;
		//I have already found win
		if(winMove[player1]){
			y= alfabeta(carefulAttack, 1, player1, 0, winMove[player1]);
			if(y<=0){
				if(winMoveDepth[player1]<=depth) winMove[player1]= 0;
			}
		}
		if(!bestMove){
			y= alfabeta(carefulAttack, 1, player1, 0);
		}
		if(y>0 && bestMove){
			doMove(bestMove);
			winMove[player1]=bestMove;
			winMoveDepth[player1]=depth;
#ifdef DEBUG
			if(!terminateAI) resulty=y;
#endif
			if(!carefulAttack){
				carefulAttack=true;
			}
			else{
				terminateAI=3; //win
			}
			return;
		}
		if(!depthReached) attackDone=true;
	}
	//find out if opponent can win
	if(UwinMoves==winMoves1 && !testDone){
#ifdef DEBUG
		print("thinking");
#endif
		depthReached=false;
		y=0;
		if(winMove[player2]){
			y= alfabeta(0, 1, player2, 1, winMove[player2]);
			if(y<=0){
				if(winMoveDepth[player2]<=depth) winMove[player2]=0;
				UwinMoves=winMoves1;
			}
		}
		if(y<=0){
			y= alfabeta(0, 1, player2, 1);
			if(y>0){
				winMove[player2]=bestMove;
				winMoveDepth[player2]=depth;
			}
			else{
				UwinMoves=winMoves1;
			}
		}
		if(y>0) evalWinMoves();
		if(!depthReached) testDone=true;
	}

	//defend
	if(UwinMoves>winMoves1 && !defendDone &&
		(!defendDone1 || carefulDefend)){
#ifdef DEBUG
		print(carefulDefend ? "defend2" : "defend");
#endif
		depthReached=false;
		bestMove=0;
		y=defend();
		if(!bestMove){
			y=alfabeta(1, 0, player1, 0);
		}
#ifdef DEBUG
		if(y<=0 && !terminateAI) resulty=y;
#endif
		if(!depthReached){
			if(!carefulDefend) defendDone1=true;
			else defendDone=true;
		}
		if(y<0) carefulDefend=true;
		doMove(bestMove);
	}
	assert(dpth==0);
}

