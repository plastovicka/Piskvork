/*
	(C) 2012-2015  Tianyi Hao
	(C) 2016  Petr Lastovicka

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

#define GA(x) ((x)&0xff)
#define GB(x) (((x)&0xff00)>>8)
#define COMB(X) (0x100 | (X))
#define COMC(X, Y) (0x10000 | ((X)<<8) | (Y))

#define N 60
static int S;
typedef signed char Tsign;

class line
{
public:
	Tsign *x;
	line(Tsign a[]) { x = a+2; }
	int A6(int p);
	int A5(int p);
	int B4(int p);
	int A3(int p);
};

int line::A6(int p)
{
	int xmin = max(p - 5, 0);
	int xmax = min(p, S - 6);
	for(int i = xmin; i <= xmax; i++)
	{
		if(x[i] + x[i + 1] + x[i + 2] + x[i + 3] + x[i + 4] + x[i + 5] == 6)
			return 1;
	}
	return 0;
}

int line::A5(int p)
{
	int xmin = max(p - 4, 0);
	int xmax = min(p, S - 5);
	for(int i = xmin; i <= xmax; i++)
	{
		if(x[i] + x[i + 1] + x[i + 2] + x[i + 3] + x[i + 4] == 5 
			&& x[i - 1] != 1 && x[i + 5] != 1) //XXXXX
			return 1;
	}
	return 0;
}

int line::B4(int p)
{
	int xmin = max(p - 4, 0);
	int xmax = min(p, S - 5);
	for(int i = xmin; i <= xmax; i++)
	{
		if(x[i] + x[i + 1] + x[i + 2] + x[i + 3] + x[i + 4] == 4
			&& x[i - 1] != 1 && x[i + 5] != 1)
		{
			if(x[i + 4] == 0) //XXXX_
			{
				return 1; //COMB(i+4);
			}
			else if(x[i + 3] == 0) //XXX_X
			{
				if(p == i + 4 && x[i + 5] == 0 && x[i + 6] == 1 
					&& x[i + 7] == 1 && x[i + 8] == 1 && x[i + 9] != 1) //XXX_X_XXX
					return 2; //COMC(i+3,i+5);

				return 1;
			}
			else if(x[i + 2] == 0) //XX_XX
			{
				if((p == i + 4 || p == i + 3) && x[i + 5] == 0
					&& x[i + 6] == 1 && x[i + 7] == 1 && x[i + 8] != 1) //XX_XX_XX
					return 2; //COMC(i+2,i+5);

				return 1;
			}
			else if(x[i + 1] == 0) //X_XXX
			{
				if(x[i + 5] == 0 && x[i + 6] == 1 && x[i + 7] != 1
					&& (p == i + 4 || p == i + 3 || p == i + 2)) //X_XXX_X
					return 2; //COMC(i+1,i+5);

				return 1; //COMB(i+1);
			}
			else //_XXXX
			{
				return 1; //COMB(i);
			}
		}
	}
	return 0;
}

int line::A3(int p)
{
	int xmin = max(p - 3, 0);
	int xmax = min(p, S - 4);
	for(int i = xmin; i <= xmax; i++)
	{
		if(x[i] + x[i + 1] + x[i + 2] + x[i + 3] == 3 
			&& x[i - 1] == 0 && x[i - 2] != 1)
		{
			if(x[i + 3] == 0) //XXX_
			{
				if(x[i + 4] != 1)
				{
					if(x[i - 2] == 0 && x[i - 3] != 1) //__XXX_
					{
						if(x[i + 4] == 0 && x[i + 5] != 1) //__XXX__
							return COMC(i - 1, i + 3);
						return COMB(i - 1);
					}
					if(x[i + 4] == 0 && x[i + 5] != 1) //_XXX__
						return COMB(i + 3);
				}
			}
			else if(x[i + 2] == 0) //XX_X
			{
				if(x[i + 4] == 0 && x[i + 5] != 1)
					return COMB(i + 2);
			}
			else if(x[i + 1] == 0) //X_XX
			{
				if(x[i + 4] == 0 && x[i + 5] != 1)
					return COMB(i + 1);
			}
		}
	}
	return 0;
}

class line4v
{

private:
public:
	Tsign x1[N][N+4], x2[N][N+4], x3[2 * N - 1][N+4], x4[2 * N - 1][N+4];
	line4v(Tsign board[N][N]);
	void pad(Tsign (*x)[N+4], int count, int len);
	int A6(int x, int y);
	int A5(int x, int y);
	int B4(int x, int y);
	int A3(int x, int y);
	int foulr(int x, int y);
};

int line4v::foulr(int x, int y)
{
	int result=0;
	if(x1[x][y + 2] != -1)
	{
		int sign = 0;
		if(x1[x][y + 2] == 0)
		{
			sign = 1;
			x1[x][y + 2] = 1;
			x2[y][x + 2] = 1;
			x3[x + y][y + 2] = 1;
			x4[S - 1 - y + x][S - 1 - y + 2] = 1;
		}
		if(A5(x, y))
		{
			result = 0;
		}
		else if(B4(x, y) >= 2)
		{
			result = 2;
		}
		else if(A3(x, y) >= 2)
		{
			result = 1;
		}
		else if(A6(x, y) > 0)
		{
			result = 3;
		}
		if(sign)
		{
			x1[x][y + 2] = 0;
			x2[y][x + 2] = 0;
			x3[x + y][y + 2] = 0;
			x4[S - 1 - y + x][S - 1 - y + 2] = 0;
		}
	}
	return result;
}

int line4v::A5(int x,int y)
{
	line l1(x1[x]),l2(x2[y]),l3(x3[x+y]),l4(x4[S-1-y+x]);
	int p1=y,p2=x,p3=y,p4=S-1-y;
	return l1.A5(p1) || l2.A5(p2) || l3.A5(p3) || l4.A5(p4);
}

int line4v::A3(int x,int y)
{
	line l1(x1[x]),l2(x2[y]),l3(x3[x+y]),l4(x4[S-1-y+x]);
	int p1=y,p2=x,p3=y,p4=S-1-y;
	int count=0;
	int ll1 = l1.A3(p1);
	if(ll1)
	{
		int r=GA(ll1);
		if(!foulr(x,r)) 
			count++;
		else if(ll1 >= 65536)
		{
			r=GB(ll1);
			if(!foulr(x,r))
				count++;
		}
	}
	int ll2 = l2.A3(p2);
	if(ll2)
	{
		int r=GA(ll2);
		if(!foulr(r,y))
			count++;
		else if(ll2 >= 65536)
		{
			r=GB(ll2);
			if(!foulr(r,y))
				count++;
		}
	}
	int ll3 = l3.A3(p3);
	if(ll3)
	{
		int r=GA(ll3);
		if(!foulr(x+y-r,r))
			count++;
		else if(ll3 >= 65536)
		{
			r=GB(ll3);
			if(!foulr(x+y-r,r))
				count++;
		}
	}
	int ll4 = l4.A3(p4);
	if(ll4)
	{
		int r=GA(ll4);
		if(!foulr(S-1 + x - y - r, S-1 - r))
			count++;
		else if(ll4 >= 65536)
		{
			r=GB(ll4);
			if(!foulr(S-1 + x - y - r, S-1 - r))
				count++;
		}
	}
	return count;
}

int line4v::B4(int x,int y)
{
	line l1(x1[x]),l2(x2[y]),l3(x3[x+y]),l4(x4[S-1-y+x]);
	int p1=y,p2=x,p3=y,p4=S-1-y;
	return l1.B4(p1) + l2.B4(p2) + l3.B4(p3) + l4.B4(p4);
}

int line4v::A6(int x,int y)
{
	line l1(x1[x]),l2(x2[y]),l3(x3[x+y]),l4(x4[S-1-y+x]);
	int p1=y,p2=x,p3=y,p4=S-1-y;
	return l1.A6(p1) || l2.A6(p2) || l3.A6(p3) || l4.A6(p4);
}

void line4v::pad(Tsign (*x)[N+4], int count, int len)
{
	for(int i = 0; i < count; i++)
	{
		x[i][0] = x[i][1] = 20;
		for(int j = len; j<S+2; j++)
			x[i][j+2] = 20;
	}
}

line4v::line4v(Tsign board[N][N])
{
	pad(x1, width, height);
	pad(x2, height, width);
	pad(x3, width+height-1, 0);
	pad(x4, width+height-1, 0);

	for(int i = 0; i < width; i++)
		for(int j = 0; j < height; j++)
			x1[i][j+2] = x2[j][i+2] = x3[i+j][j+2] =
			x4[S-1-j+i][S-1-j+2] = board[i][j];
}

bool checkforbid(Psquare lastMove)
{
	Tsign board_[N][N];

	S = max(width, height);
	if((moves & 1) == 0 || S > N) return false;

	for(int i = 0; i<width; i++)
		for(int j = 0; j<height; j++)
			board_[i][j] = 0;

	Tsign sign = 1;
	for(Psquare p = lastMove; p; p = p->nxt)
	{
		board_[p->x - 1][p->y - 1] = sign;
		sign *= -1;
	}

	line4v lin4v(board_);
	return lin4v.foulr(lastMove->x - 1, lastMove->y - 1) != 0;
}
