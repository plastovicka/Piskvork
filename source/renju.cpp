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

#define COMB(X) (0x100 | (X))
#define COMC(X, Y) (0x10000 | ((X)<<8) | (Y))

#define N 60
static int S;
typedef signed char Tsign;

class line
{
	Tsign *x;
	int p;
public:
	void set(Tsign a[], int _p) { x = a+2; x[p = _p] = 1; }
	int A6();
	int A5();
	int B4();
	int A3();
};

int line::A6()
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

int line::A5()
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

int line::B4()
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

int line::A3()
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
	Tsign x1[N][N+4], x2[N][N+4], x3[2 * N - 1][N+4], x4[2 * N - 1][N+4];
	line l1, l2, l3, l4;
	int x, y;
	void pad(Tsign(*x)[N+4], int count, int len);
	int A3(line &l, int (line4v::*f)(int));
	int f1(int r);
	int f2(int r);
	int f3(int r);
	int f4(int r);
public:
	line4v();
	int foulr(int x, int y, int five);
};

int line4v::foulr(int x, int y, int five)
{
	int result = 0;

	if(x1[x][y + 2] != -1)
	{
		line m1 = l1, m2 = l2, m3 = l3, m4 = l4;
		int x0 = this->x, y0 = this->y;
		this->x = x; this->y = y;
		Tsign sign = x1[x][y + 2];

		//move to x,y
		l1.set(x1[x], y);
		l2.set(x2[y], x);
		l3.set(x3[x+y], y);
		l4.set(x4[S-1-y+x], S-1-y);

		if(l1.A5() || l2.A5() || l3.A5() || l4.A5())
		{
			result = five; //five in a row
		}
		else if(l1.B4() + l2.B4() + l3.B4() + l4.B4() >= 2)
		{
			result = 2; //double-four
		}
		else if(A3(l1, &line4v::f1) + A3(l2, &line4v::f2) + A3(l3, &line4v::f3) + A3(l4, &line4v::f4) >= 2)
		{
			result = 1; //double-three
		}
		else if(l1.A6() || l2.A6() || l3.A6() || l4.A6())
		{
			result = 3; //overline
		}

		//restore
		x1[x][y + 2] = x2[y][x + 2] = x3[x+y][y + 2] =
				x4[S-1-y+x][S-1-y + 2] = sign;
		l1 = m1, l2 = m2, l3 = m3, l4 = m4;
		this->x = x0; this->y = y0;
	}
	return result;
}

int line4v::A3(line &l, int (line4v::*f)(int))
{
	int r = l.A3();
	return r && (!(this->*f)(r & 0xff) || r >= 0x10000 && !(this->*f)((r>>8) & 0xff));
}

int line4v::f1(int r)
{
	return foulr(x, r, 1);
}

int line4v::f2(int r)
{
	return foulr(r, y, 1);
}

int line4v::f3(int r)
{
	return foulr(x + y - r, r, 1);
}

int line4v::f4(int r)
{
	return foulr(S-1 + x - y - r, S-1 - r, 1);
}

void line4v::pad(Tsign(*x)[N+4], int count, int len)
{
	for(int i = 0; i < count; i++)
	{
		x[i][0] = x[i][1] = 20;
		for(int j = len; j<S+2; j++)
			x[i][j+2] = 20;
	}
}

line4v::line4v()
{
	pad(x1, width, height);
	pad(x2, height, width);
	pad(x3, width+height-1, 0);
	pad(x4, width+height-1, 0);

	Tsign conv[4];
	conv[0] = 0;
	conv[lastMove->z] = 1;
	conv[3-lastMove->z] = -1;

	for(int i = 0; i < width; i++)
		for(int j = 0; j < height; j++)
		{
			Psquare p = Square(i, j);
			x1[i][j+2] = x2[j][i+2] = x3[i+j][j+2] = x4[S-1-j+i][S-1-j+2] =
				p->blocked() ? (Tsign)20 : conv[p->z];
		}
}

bool checkforbid()
{
	S = max(width, height);
	if(S > N) return true;
	if((moves & 1) == 0) return false;

	line4v lin4v;
	return lin4v.foulr(lastMove->x - 1, lastMove->y - 1, 0) != 0;
}
