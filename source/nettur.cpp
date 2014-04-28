/*
	(C) 2005-2011  Petr Lastovicka

	This program is free software; you can redistribute it and/or
	modify it under the terms of the GNU General Public License.
	*/
#include "hdr.h"
#pragma hdrstop
#ifndef TF_DISCONNECT
#include <mswsock.h>
#endif
#include "piskvork.h"

#define STATE_VERSION 0x805


//info about game that is sent to a client
struct TgameSettings {
	DWORD timeGame, timeMove, tolerance, memory;
	char width, height;
	char suspend;
	char sendMoves;
	char tieRepeat;
	char messages;
	char openingData[1];
	void fill();
	void use();
};

//tournament state
struct TturSettings {

	//STATE_VERSION 0x703 and less
	SYSTEMTIME date;
	int version;
	DWORD timeGame, timeMove, memory;
	char rule, autoBegin;
	char width, height;
	short id[2];
	int playerStrLen;
	short repeat;
	short gameCount;
	short opening;
	short matchRepeat;
	char openingRandomShift;
	bool autoBeginForce;

#if STATE_VERSION >=0x805
	bool exactFive;
#endif

	void fill();
	void use();
};

struct TstateClient {
	short player[2];
	short repeatCount;
	short gameCount;
	u_long IP;
};

bool isServer, isClient;
SOCKET sock;        //client socket
SOCKET sock_l;      //listening socket 
int port=3629;      //TCP/IP port
int pollDelay=1000; //interval of asking for next game in server threads
TdirName AIfolder;  //folder where AI are downloaded
HANDLE clientThread;
CRITICAL_SECTION netLock;
Tclient clients[Mplayer]; //information about clients
char servername[128]="localhost";

//---------------------------------------------------------------
int rd(SOCKET s, char *buf, int len)
{
	int i, n;

	for(n=0; n<len;){
		i=recv(s, buf+n, len-n, 0);
		if(i<=0) return -1;
		n+=i;
	}
	return 0;
}

int rd1(SOCKET s)
{
	char buf;
	if(rd(s, &buf, 1)) return -1;
	return *(unsigned char*)&buf;
}

int rd2(SOCKET s)
{
	int r=rd1(s);
	return (r<<8)|rd1(s);
}

int rd4(SOCKET s)
{
	int r=rd1(s);
	r=(r<<8)|rd1(s);
	r=(r<<8)|rd1(s);
	return (r<<8)|rd1(s);
}

int wr(SOCKET s, char *buf, int len)
{
	int i, n;

	for(n=0; n<len;){
		i=send(s, buf+n, len-n, 0);
		if(i==SOCKET_ERROR) return -1;
		n+=i;
	}
	return 0;
}

int wr1(SOCKET s, int i)
{
	char buf=(char)i;
	return wr(s, &buf, 1);
}

int rd(char *buf, int len)
{
	return rd(sock, buf, len);
}

int rd1()
{
	return rd1(sock);
}

int rd2()
{
	return rd2(sock);
}

int rd4()
{
	return rd4(sock);
}

int wr(char *buf, int len)
{
	return wr(sock, buf, len);
}

int wr1(int i)
{
	return wr1(sock, i);
}

//---------------------------------------------------------------
const DWORD crcTable[256]={
	0x00000000, 0x77073096, 0xEE0E612C, 0x990951BA,
	0x076DC419, 0x706AF48F, 0xE963A535, 0x9E6495A3,
	0x0EDB8832, 0x79DCB8A4, 0xE0D5E91E, 0x97D2D988,
	0x09B64C2B, 0x7EB17CBD, 0xE7B82D07, 0x90BF1D91,
	0x1DB71064, 0x6AB020F2, 0xF3B97148, 0x84BE41DE,
	0x1ADAD47D, 0x6DDDE4EB, 0xF4D4B551, 0x83D385C7,
	0x136C9856, 0x646BA8C0, 0xFD62F97A, 0x8A65C9EC,
	0x14015C4F, 0x63066CD9, 0xFA0F3D63, 0x8D080DF5,
	0x3B6E20C8, 0x4C69105E, 0xD56041E4, 0xA2677172,
	0x3C03E4D1, 0x4B04D447, 0xD20D85FD, 0xA50AB56B,
	0x35B5A8FA, 0x42B2986C, 0xDBBBC9D6, 0xACBCF940,
	0x32D86CE3, 0x45DF5C75, 0xDCD60DCF, 0xABD13D59,
	0x26D930AC, 0x51DE003A, 0xC8D75180, 0xBFD06116,
	0x21B4F4B5, 0x56B3C423, 0xCFBA9599, 0xB8BDA50F,
	0x2802B89E, 0x5F058808, 0xC60CD9B2, 0xB10BE924,
	0x2F6F7C87, 0x58684C11, 0xC1611DAB, 0xB6662D3D,
	0x76DC4190, 0x01DB7106, 0x98D220BC, 0xEFD5102A,
	0x71B18589, 0x06B6B51F, 0x9FBFE4A5, 0xE8B8D433,
	0x7807C9A2, 0x0F00F934, 0x9609A88E, 0xE10E9818,
	0x7F6A0DBB, 0x086D3D2D, 0x91646C97, 0xE6635C01,
	0x6B6B51F4, 0x1C6C6162, 0x856530D8, 0xF262004E,
	0x6C0695ED, 0x1B01A57B, 0x8208F4C1, 0xF50FC457,
	0x65B0D9C6, 0x12B7E950, 0x8BBEB8EA, 0xFCB9887C,
	0x62DD1DDF, 0x15DA2D49, 0x8CD37CF3, 0xFBD44C65,
	0x4DB26158, 0x3AB551CE, 0xA3BC0074, 0xD4BB30E2,
	0x4ADFA541, 0x3DD895D7, 0xA4D1C46D, 0xD3D6F4FB,
	0x4369E96A, 0x346ED9FC, 0xAD678846, 0xDA60B8D0,
	0x44042D73, 0x33031DE5, 0xAA0A4C5F, 0xDD0D7CC9,
	0x5005713C, 0x270241AA, 0xBE0B1010, 0xC90C2086,
	0x5768B525, 0x206F85B3, 0xB966D409, 0xCE61E49F,
	0x5EDEF90E, 0x29D9C998, 0xB0D09822, 0xC7D7A8B4,
	0x59B33D17, 0x2EB40D81, 0xB7BD5C3B, 0xC0BA6CAD,
	0xEDB88320, 0x9ABFB3B6, 0x03B6E20C, 0x74B1D29A,
	0xEAD54739, 0x9DD277AF, 0x04DB2615, 0x73DC1683,
	0xE3630B12, 0x94643B84, 0x0D6D6A3E, 0x7A6A5AA8,
	0xE40ECF0B, 0x9309FF9D, 0x0A00AE27, 0x7D079EB1,
	0xF00F9344, 0x8708A3D2, 0x1E01F268, 0x6906C2FE,
	0xF762575D, 0x806567CB, 0x196C3671, 0x6E6B06E7,
	0xFED41B76, 0x89D32BE0, 0x10DA7A5A, 0x67DD4ACC,
	0xF9B9DF6F, 0x8EBEEFF9, 0x17B7BE43, 0x60B08ED5,
	0xD6D6A3E8, 0xA1D1937E, 0x38D8C2C4, 0x4FDFF252,
	0xD1BB67F1, 0xA6BC5767, 0x3FB506DD, 0x48B2364B,
	0xD80D2BDA, 0xAF0A1B4C, 0x36034AF6, 0x41047A60,
	0xDF60EFC3, 0xA867DF55, 0x316E8EEF, 0x4669BE79,
	0xCB61B38C, 0xBC66831A, 0x256FD2A0, 0x5268E236,
	0xCC0C7795, 0xBB0B4703, 0x220216B9, 0x5505262F,
	0xC5BA3BBE, 0xB2BD0B28, 0x2BB45A92, 0x5CB36A04,
	0xC2D7FFA7, 0xB5D0CF31, 0x2CD99E8B, 0x5BDEAE1D,
	0x9B64C2B0, 0xEC63F226, 0x756AA39C, 0x026D930A,
	0x9C0906A9, 0xEB0E363F, 0x72076785, 0x05005713,
	0x95BF4A82, 0xE2B87A14, 0x7BB12BAE, 0x0CB61B38,
	0x92D28E9B, 0xE5D5BE0D, 0x7CDCEFB7, 0x0BDBDF21,
	0x86D3D2D4, 0xF1D4E242, 0x68DDB3F8, 0x1FDA836E,
	0x81BE16CD, 0xF6B9265B, 0x6FB077E1, 0x18B74777,
	0x88085AE6, 0xFF0F6A70, 0x66063BCA, 0x11010B5C,
	0x8F659EFF, 0xF862AE69, 0x616BFFD3, 0x166CCF45,
	0xA00AE278, 0xD70DD2EE, 0x4E048354, 0x3903B3C2,
	0xA7672661, 0xD06016F7, 0x4969474D, 0x3E6E77DB,
	0xAED16A4A, 0xD9D65ADC, 0x40DF0B66, 0x37D83BF0,
	0xA9BCAE53, 0xDEBB9EC5, 0x47B2CF7F, 0x30B5FFE9,
	0xBDBDF21C, 0xCABAC28A, 0x53B39330, 0x24B4A3A6,
	0xBAD03605, 0xCDD70693, 0x54DE5729, 0x23D967BF,
	0xB3667A2E, 0xC4614AB8, 0x5D681B02, 0x2A6F2B94,
	0xB40BBE37, 0xC30C8EA1, 0x5A05DF1B, 0x2D02EF8D};


int calcCRC(const char *fn, DWORD &result)
{
	HANDLE h;
	DWORD crc;
	DWORD i, r;
	BYTE buf[4096];
	TfileName path;

	r=searchAI(fn, sizeof(path), path, 0);
	h=CreateFile((r>0 && r<sizeof(path)) ? path : fn, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, 0);
	if(h==INVALID_HANDLE_VALUE) return 1;
	crc=0xffffffff;
	do{
		if(!ReadFile(h, buf, sizeof(buf), &r, 0)){ CloseHandle(h); return 2; }
		for(i=0; i<r; i++){
			crc = (crc>>8) ^ crcTable[buf[i] ^ (crc & 0xFF)];
		}
	} while(r);
	CloseHandle(h);
	crc= ~crc;
	result=crc;
	return 0;
}
//---------------------------------------------------------------
int recvFile(SOCKET s, const char *fn, int len)
{
	HANDLE h;
	int i, n;
	DWORD w;
	char buf[4096];

	h=CreateFile(fn, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
	if(h==INVALID_HANDLE_VALUE) return 2;

	for(n=0; n<len;){
		i=recv(s, buf, min(len-n, sizeof(buf)), 0);
		if(i<=0) break;
		WriteFile(h, buf, i, &w, 0);
		n+=i;
	}
	CloseHandle(h);
	return n!=len;
}
//---------------------------------------------------------------
//get players id for a next game
bool turNext(Tclient *client)
{
	bool result=false, stillActive=false;

	EnterCriticalSection(&netLock);
	if(turNplayers){
		if(client->player[0]>=0){
			result=true;
		}
		else if(turCurRepeat>0){
			turNext();
			if(turCurRepeat>0){
				client->player[0]= players[0].turPlayerId;
				client->player[1]= players[1].turPlayerId;
				client->gameCount=0;
				client->repeatCount= turRepeat-turCurRepeat;
				result=true;
			}
		}
		if(!result){
			for(int i=0; i<Mplayer; i++){
				Tclient *c=&clients[i];
				if(c->player[0]>=0){
					if(c->thread){
						stillActive=true;
					}
					else{
						client->player[0]= c->player[0];
						client->player[1]= c->player[1];
						client->repeatCount= c->repeatCount;
						client->gameCount= c->gameCount;
						c->player[0]=-1;
						result=true;
						break;
					}
				}
			}
		}
		if(!result && !stillActive){
			wrLog(lng(850, "Tournament finished"));
			show(resultDlg);
			turNplayers=0;
			killBrains();
			opening=turOpening + turMatchRepeat*turRepeat/2;
			executeCmd(cmdTurEnd);
		}
		client->opening= turOpening + (client->repeatCount*turMatchRepeat + client->gameCount)/2;
		players[0].isComp=players[1].isComp=1;
		players[1].timeMove=players[0].timeMove;
		players[1].timeGame=players[0].timeGame;
	}
	LeaveCriticalSection(&netLock);
	return result;
}
//---------------------------------------------------------------
void TgameSettings::use()
{
	players[1].timeMove= players[0].timeMove= timeMove;
	players[1].timeGame= players[0].timeGame= timeGame;
	::tolerance= tolerance;
	maxMemory= memory;
	::width=width;
	::height=height;
	suspendAI=suspend&1;
	debugAI=(suspend>>1)&1;
	turTieRepeat=tieRepeat;
	turLogMsg=messages;
}

void TgameSettings::fill()
{
	timeGame=players[0].timeGame;
	timeMove=players[0].timeMove;
	tolerance=::tolerance;
	memory=maxMemory;
	width=(char)::width;
	height=(char)::height;
	suspend=(char)((suspendAI!=0)|((debugAI!=0)<<1));
	sendMoves=(char)turRecord;
	tieRepeat=(char)turTieRepeat;
	messages=(char)turLogMsg;
	openingData[0]=0;
}

void TturSettings::use()
{
	players[1].timeMove= players[0].timeMove= timeMove;
	players[1].timeGame= players[0].timeGame= timeGame;
	maxMemory= memory;
	::width=width;
	::height=height;
	turDateTime=date;
	players[0].turPlayerId=id[0];
	players[1].turPlayerId=id[1];
	turOpening=opening;
	::autoBegin=autoBegin;
	turRule=rule;
	turGamesCounter=gameCount;
	amin(turRepeat, repeat);
	turCurRepeat=turRepeat-repeat;
	turMatchRepeat= matchRepeat ? matchRepeat : 2;
	openingRandomShiftT=openingRandomShift;
	::autoBeginForce=autoBeginForce;
	::exactFive = exactFive;
}

void TturSettings::fill()
{
	version=STATE_VERSION;
	playerStrLen=strlen(turPlayerStr);
	timeGame=players[0].timeGame;
	timeMove=players[0].timeMove;
	memory=maxMemory;
	width=(char)::width;
	height=(char)::height;
	date=turDateTime;
	id[0]=(short)players[0].turPlayerId;
	id[1]=(short)players[1].turPlayerId;
	opening=(short)turOpening;
	autoBegin=(char)::autoBegin;
	rule=(char)turRule;
	gameCount=(short)turGamesCounter;
	repeat=(short)(turRepeat-turCurRepeat);
	matchRepeat=(short)turMatchRepeat;
	openingRandomShift=(char)openingRandomShiftT;
	autoBeginForce=::autoBeginForce;
	exactFive = ::exactFive!=0;
}

//reload tournament state from a file
int rdState()
{
	FILE *f;
	int i, n, len, err=1;
	Tclient *c;
	char buf[512];

	f=fopen(fnstate, "rb");
	if(f){
		{
			len=fgetc(f);
			if(len==EOF) goto le;
			memset(buf, 0, sizeof(TturSettings));
			if(fread(buf, len, 1, f)!=1) goto le;
			TturSettings &ts=*(TturSettings*)buf;
			if(ts.version!=STATE_VERSION) goto le;
			ts.use();
			if(ts.playerStrLen>=sizeof(turPlayerStr)) goto le;
			if(fread(turPlayerStr, ts.playerStrLen, 1, f)!=1) goto le;
			turPlayerStr[ts.playerStrLen]=0;
		}
		if(getNplayers()) goto le;
		n=fgetc(f);
		if(n>Mplayer) goto le;
		len=fgetc(f);
		if(len==EOF) goto le;
		for(i=0; i<n; i++){
			memset(buf, 0, sizeof(TstateClient));
			if(fread(buf, len, 1, f)!=1) goto le;
			TstateClient &sc=*(TstateClient*)buf;
			c=&clients[i];
			c->player[0]=sc.player[0];
			c->player[1]=sc.player[1];
			c->repeatCount= sc.repeatCount;
			c->gameCount= sc.gameCount;
			c->IP= sc.IP;
		}
		len=fgetc(f);
		if(len<0 || len>sizeof(TturPlayer)) goto le;
		memset(turTable, 0, turNplayers*sizeof(TturPlayer));
		for(i=0; i<turNplayers; i++){
			fread(turTable+i, len, 1, f);
		}
		len=fgetc(f);
		if(len<0 || len>sizeof(TturCell)) goto le;
		memset(turCells, 0, turNplayers*turNplayers*sizeof(TturCell));
		for(i=0; i<turNplayers*turNplayers; i++){
			fread(turCells+i, len, 1, f);
		}
		if(fgetc(f)==156) err=0;
	le:
		fclose(f);
	}
	if(err){
		for(int i=0; i<Mplayer; i++){
			clients[i].player[0]=-1;
		}
		turNplayers=0;
		msglng(815, "Cannot load a tournament state");
	}
	return err;
}

//store tournament state to a file
void wrState()
{
	FILE *f;
	int i, n;
	Tclient *c;
	TstateClient sc;
	TturSettings ts;
	TfileName fn, fn2;

	if(!turNplayers) return;
	strcpy(getTurDir(fn), fnStateTmp);
	f=fopen(fn, "wb");
	if(!f) return;
	ts.fill();
	fputc(sizeof(TturSettings), f);
	fwrite(&ts, sizeof(ts), 1, f);
	fputs(turPlayerStr, f);
	n=0;
	for(i=0; i<Mplayer; i++){
		if(clients[i].player[0]>=0) n++;
	}
	fputc(n, f);
	fputc(sizeof(TstateClient), f);
	for(i=0; i<Mplayer; i++){
		c=&clients[i];
		if(c->player[0]>=0){
			sc.player[0]=(short)c->player[0];
			sc.player[1]=(short)c->player[1];
			sc.repeatCount=(short)c->repeatCount;
			sc.gameCount=(short)c->gameCount;
			sc.IP=c->IP;
			fwrite(&sc, sizeof(sc), 1, f);
		}
	}
	fputc(sizeof(TturPlayer), f);
	fwrite(turTable, sizeof(TturPlayer), turNplayers, f);
	fputc(sizeof(TturCell), f);
	fwrite(turCells, sizeof(TturCell), turNplayers*turNplayers, f);
	fputc(156, f);
	if(!fclose(f)){
		strcpy(getTurDir(fn2), fnState);
		DeleteFile(fn2);
		MoveFile(fn, fn2);
	}
}
//---------------------------------------------------------------
//cleanup on client
void clientDone()
{
	EnterCriticalSection(&netLock);
	if(clientThread){
		killBrains();
		closesocket(sock);
		sock=INVALID_SOCKET;
		WSACleanup();
		wrLog(lng(853, "Disconnected from server"));
		CloseHandle(clientThread);
		clientThread=0;
		isClient=false;
		turNplayers=0;
	}
	LeaveCriticalSection(&netLock);
	drawTitle();
}
//---------------------------------------------------------------
//main function for a client thread
DWORD WINAPI clientLoop(void *)
{
	int i, j, len, x, y, c, pathLen;
	Tplayer *p;
	TgameSettings *b;
	DWORD crcC, crcS, fileSize;
	signed char *o;
	Psquare q;
	HANDLE h;
	char buf[512], *A, *s;
	char sendMoves;
	TfileName fnmsg;

	pathLen=(int)strlen(AIfolder);
	getMsgFn(fnmsg);
lstart:
	for(;;){
		killBrains();
		//wait for server
		c=rd1();
		if(c!=92) break;
		//receive players
		for(i=0; i<2; i++){
			p=&players[i];
			p->turPlayerId=i;
			p->isComp=1;
			if(rd(buf, 9)<0) goto lend;
			len=(unsigned char)buf[8];
			if(!len) goto lstart;
			strcpy(p->brain, AIfolder);
			s=p->brain+pathLen;
			if(pathLen && s[-1]!='\\') *s++='\\';
			if(pathLen+len+1>=sizeof(p->brain)){
				wrLog("File name is too long");
				wr1(0);
				goto lend;
			}
			if(rd(s, len)<0) goto lend;
			s[len]=0;
			len=*(DWORD*)buf;
			crcS=*(DWORD*)(buf+4);
			if(calcCRC(p->brain, crcC) || crcC!=crcS){
				wr1(229);
				wrLog(lng(851, "Receiving %s"), p->brain);
				recvFile(sock, p->brain, len);
				if(calcCRC(p->brain, crcC) || crcC!=crcS){
					p->brain2[0]=0;
					wr1(221);
					wrLog("Download failed");
					goto lend;
				}
			}
			p->brainChanged();
			wr1(118);//OK
		}
		//receive game settings
		c=rd1();
		if(c<8 || c==231) break;
		len=c;
		memset(buf, 0, sizeof(TgameSettings));
		width=height=20;
		tolerance=1000;
		if(rd(buf, len)<0) goto lend;
		b=(TgameSettings*)buf;
		j=b->openingData[0];
		if(rd(buf+len, j*2)<0) goto lend;
		b->use();
		sendMoves=b->sendMoves;
		//start game
		memset(turTable, 0, 2*sizeof(TturPlayer));
		turTieCounter=0;
		DeleteFile(fnmsg);
		newGame(0, false);
		//automatic opening
		if(j){
			o=(signed char*)b->openingData;
			for(; j>0; j--){
				x= *++o;
				y= *++o;
				doMove1(Square(x, y));
			}
			startMoves=moves;
		}
		resume();
		//wait until the match is finished
		c=rd1();
		if(c!=137) break;
		//send results
		wr1(sizeof(TturPlayer));
		for(i=0; i<2; i++){
			wr((char*)&turTable[i], sizeof(TturPlayer));
		}
		//send board
		len=3;
		if(sendMoves){
			len+=moves*6;
		}
		s=A=new char[len];
		*s++=(char)(moves>>8);
		*s++=(char)moves;
		if(sendMoves && moves){
			*s++=6;
			for(q=lastMove; q->nxt; q=q->nxt);
			for(; q; q=q->pre){
				*s++= (char)q->x;
				*s++= (char)q->y;
				*(DWORD*)s= q->time;
				s+=4;
			}
		}
		else{
			*s=0;
		}
		wr(A, len);
		delete[] A;
		//send messages
		fileSize=0;
		if(turLogMsg){
			h=CreateFile(fnmsg, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
			if(h!=INVALID_HANDLE_VALUE){
				fileSize=GetFileSize(h, 0);
				wr((char*)&fileSize, 4);
				TransmitFile(sock, h, 0, 0, 0, 0, 0);
			}
			CloseHandle(h);
		}
		if(!fileSize) wr((char*)&fileSize, 4);
	}
	if(c==231) wrLog(lng(852, "Tournament aborted"));
lend:
	clientDone();
	return 0;
}
//---------------------------------------------------------------
//main function for server threads
DWORD WINAPI serverLoop(void *param)
{
	int clientId= (int)param;
	Tclient *client= &clients[clientId];
	TturCell *ce;
	SOCKET sock_c;
	TturPlayer *tt[2], *rr[2], *t, *r;
	int i, c, len, len1, m, id[2], movesOld;
	DWORD w, fileSize;
	HANDLE h;
	TgameSettings *b;
	Psquare p, B, lastMoveOld;
	u_long u;
	char *s, *A, *q, *M;
	signed char *o1, *o2;
	char buf[512];
	char *resultBuf[2]={buf, buf+256};
	TfileName fn;

	for(;;){
		//wait for a tournament
		while(!turNext(client)){
			if(ioctlsocket(client->socket, FIONREAD, &u) || u) goto lend;
			Sleep(pollDelay);
		}
		sock_c= client->socket;
		if(wr1(sock_c, 92)<0) goto lend;
		//send players
		for(i=0; i<2; i++){
			tt[i]=&turTable[client->player[i]];
			const int H=9;
			s=buf+H;
			turGetBrain(client->player[i], fn, sizeof(fn), false);
			searchAI(fn, sizeof(buf)-H, s, 0);
			h=CreateFile(s, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
			s=cutPath(s);
			if(h==INVALID_HANDLE_VALUE){
				wrLog("Cannot open %s", fn);
				goto lend;
			}
			fileSize=GetFileSize(h, 0);
			len=(int)strlen(s);
			s[-1]=(char)len;
			*(DWORD*)(s-9)=fileSize;
			*(DWORD*)(s-5)=tt[i]->crc;
			wr(sock_c, s-H, len+H);
			c=rd1(sock_c);
			if(c==229){
				//client does not have the AI
				wrLog(lng(854, "Transmit %s to Client %d, size=%u"), s, clientId, fileSize);
				TransmitFile(sock_c, h, 0, 0, 0, 0, 0);
				c=rd1(sock_c);
			}
			CloseHandle(h);
			if(c!=118) goto lend;
		}
		//send game settings
		b=(TgameSettings*)(buf+1);
		buf[0]=sizeof(TgameSettings);
		b->fill();
		//send opening
		if(autoBegin || autoBeginForce){
			int x0, y0, r, j, x, y;
			o1= getOpening(client->opening);
			o2= (signed char*)b->openingData;
			j= *o2 = *o1;
			x0= width/2;
			y0= height/2;
			r= 0;
			if(openingRandomShiftT){
				x0 += rnd(4)-2;
				y0 += rnd(4)-2;
				r = rnd(8);
			}
			for(; j>0; j--){
				x= *++o1;
				y= *++o1;
				if(r&1) x=-x;
				if(r&2) y=-y;
				if(r&4) w=x, x=y, y=w;
				*++o2 = (signed char)(x0+x);
				*++o2 = (signed char)(y0+y);
			}
		}
		wr(sock_c, buf, 1+sizeof(TgameSettings)+2*b->openingData[0]);
		wrLog(lng(855, "Game started: players %d x %d,  Client %d"),
			client->player[0], client->player[1], clientId);
		//wait
		if(rd1(sock_c)!=173) goto lend;
		wr1(sock_c, 137);
		//receive game results
		len=rd1(sock_c);
		if(len<0) goto lend;
		for(i=0; i<2; i++){
			memset(resultBuf[i], 0, sizeof(TturPlayer));
			if(rd(sock_c, resultBuf[i], len)<0) goto lend;
			r=rr[i]=(TturPlayer*)resultBuf[i];
			if(unsigned(r->wins+r->winsE+r->losses+r->errors+r->timeouts)>1){
				wrLog("Invalid result received !");
				goto lend;
			}
		}
		//receive board
		m=(rd1(sock_c)<<8)|rd1(sock_c);
		len1=rd1(sock_c);
		if(len1<0) goto lend;
		len=m*len1;
		A=0;
		if(len>0){
			A=new char[len];
			if(rd(sock_c, A, len)<0){ delete[] A; goto lend; }
		}
		//receive messages
		if(rd(sock_c, (char*)&fileSize, 4)<0 || fileSize>16000000){ delete[] A; goto lend; }
		M=0;
		if(fileSize>0){
			M=new char[fileSize];
			if(rd(sock_c, M, fileSize)<0){ delete[] A; delete[] M; goto lend; }
		}
		//add game result to the tournament table
		EnterCriticalSection(&netLock);
		for(i=0; i<2; i++){
			r=rr[i];
			t=tt[i];
			t->Ngames++;
			t->Nmoves+=r->Nmoves;
			t->time+=r->time;
			t->losses+=r->losses;
			t->losses1+=r->losses1;
			t->errors+=r->errors;
			t->timeouts+=r->timeouts;
			t->wins+=r->wins;
			t->wins1+=r->wins1;
			t->winsE+=r->winsE;
			amin(t->memory, r->memory);
			amin(t->maxTurnTime, r->maxTurnTime);
			ce= getCell(client->player[1-i], client->player[i]);
			if(r->wins) if(i==0) ce->start++; else ce->notStart++;
			if(r->winsE) ce->error++;
		}
		//save messages
		wrMsgNewGame(client->player[0], client->player[1]);
		if(M){
			getMsgFn(fn);
			h=CreateFile(fn, GENERIC_WRITE, 0, 0, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
			if(h!=INVALID_HANDLE_VALUE){
				SetFilePointer(h, 0, 0, FILE_END);
				WriteFile(h, M, fileSize, &w, 0);
				CloseHandle(h);
			}
			delete[] M;
		}
		//save position
		if(A){
			lastMoveOld=lastMove;
			movesOld=moves;
			B=new Tsquare[m];
			q=A;
			p=B;
			for(i=0; i<m; i++){
				p->nxt=p-1;
				p->pre=p+1;
				p->x=*(unsigned char*)q;
				p->y=*(unsigned char*)(q+1);
				p->time= (len1>2) ? *(DWORD*)(q+2) : 0;
				q+=len1;
				p++;
			}
			lastMove=p-1;
			lastMove->pre= B->nxt= 0;
			moves=m;
			player=m&1;
			for(i=0; i<2; i++){
				id[i]= players[i].turPlayerId;
				players[i].turPlayerId=client->player[i];
				turGetBrain(client->player[i], players[i].brain, sizeof(players[0].brain), false);
				players[i].brain2[0]=0;
			}
			// Tomas Kubes: I should save errCode in saveRec,
			// but it is not probably available on the server,
			// unless communication protocol is changed and I don't have any idea how to do it.
			// so I put -1, it means unavailable
			saveRec(-1, false);
			players[0].turPlayerId=id[0];
			players[1].turPlayerId=id[1];
			lastMove=lastMoveOld;
			moves=movesOld;
			delete[] B;
			delete[] A;
		}
		wrLog(lng(856, "Game finished: players %d x %d, Client %d, moves=%d"),
			client->player[0], client->player[1], clientId, m);
		//incr. game counter, update result window, save state and html table
		client->gameFinished();
		LeaveCriticalSection(&netLock);
		Sleep(turDelay);
	}
lend:
	EnterCriticalSection(&netLock);
	closesocket(client->socket);
	client->socket=INVALID_SOCKET;
	CloseHandle(client->thread);
	client->thread=0;
	wrLog(lng(857, "Client %d disconnected"), clientId);
	LeaveCriticalSection(&netLock);
	return 0;
}
//---------------------------------------------------------------
//main function for a listening thread on server
DWORD WINAPI listenLoop(void *)
{
	SOCKET sock_c;
	int i, sl;
	char *a;
	Tclient *c;
	struct sockaddr_in sa;

	wrLog(lng(858, "Server started"));
	for(;;){
		sl= sizeof(sa);
		if((sock_c= accept(sock_l, (sockaddr *)&sa, &sl))==INVALID_SOCKET){
			break;
		}
		for(i=0;; i++){
			if(i==Mplayer){
				for(i=0; i<Mplayer; i++){
					if(!clients[i].thread) break;
				}
				break;
			}
			if(clients[i].IP==sa.sin_addr.S_un.S_addr && !clients[i].thread) break;
		}
		if(i==Mplayer){
			wrLog("Too many clients");
			closesocket(sock_c);
		}
		else{
			c= &clients[i];
			a= inet_ntoa(sa.sin_addr);
			if(!a) a="???";
			wrLog(lng(859, "Client %d connected: %s"), i, a);
			c->IP= sa.sin_addr.S_un.S_addr;
			c->socket=sock_c;
			DWORD id;
			ResumeThread(c->thread=CreateThread(0, 0, serverLoop, (void*)i, CREATE_SUSPENDED, &id));
		}
	}
	if(sock_l!=INVALID_SOCKET) serverEnd();
	WSACleanup();
	isServer=false;
	drawTitle();
	wrLog(lng(860, "Server terminated"));
	return 0;
}
//---------------------------------------------------------------

//send notification that a game finished on client
void clientFinished()
{
	wr1(173);
}

//disconnect client
void clientEnd()
{
	if(!isClient) return;
	wr1(0);
	for(int i=0; i<50; i++){
		Sleep(200);
		if(!isClient) return;
	}
	EnterCriticalSection(&netLock);
	if(clientThread){
		wrLog("Server is not responding");
		clientDone();
		TerminateThread(clientThread, 1);
	}
	LeaveCriticalSection(&netLock);
}

//disconnect client on server
void killClient(int i)
{
	HANDLE t, t2=0;

	if(i<0 || i>=Mplayer) return;
	Tclient *c=&clients[i];
	EnterCriticalSection(&netLock);
	t= c->thread;
	if(t) DuplicateHandle(GetCurrentProcess(), t, GetCurrentProcess(), &t2, 0, FALSE, DUPLICATE_SAME_ACCESS);
	LeaveCriticalSection(&netLock);
	if(t){
		wr1(c->socket, 231);
		closesocket(c->socket);
		c->socket=INVALID_SOCKET;
		WaitForSingleObject(t2, 30000);
		CloseHandle(t2);
	}
}

//cancel tournament on server, don't stop listening
void turAbort()
{
	EnterCriticalSection(&netLock);
	if(turNplayers) wrLog(lng(852, "Tournament aborted"));
	turNplayers=0;
	LeaveCriticalSection(&netLock);
	for(int i=0; i<Mplayer; i++){
		killClient(i);
		clients[i].player[0]=-1;
	}
}

void stopListen()
{
	SOCKET s=sock_l;
	sock_l=INVALID_SOCKET;
	closesocket(s);
}

//disconnect server
void serverEnd()
{
	if(!isServer) return;
	//terminate clients threads
	turAbort();
	//close listening socket
	stopListen();
}

//---------------------------------------------------------------
int initWSA()
{
	WSAData wd;
	int err= WSAStartup(0x0202, &wd);
	if(err) msg("Cannot initialize WinSock");
	return err;
}

int startListen()
{
	int err=1;
	struct sockaddr_in sa;
	char host[128];

	if(isServer || isNetGame || isListening){
		err=4;
	}
	else if(!initWSA()){
		if((sock_l= socket(PF_INET, SOCK_STREAM, 0))==INVALID_SOCKET){
			msg("Error: socket");
		}
		else{
			memset(&sa, 0, sizeof(sa));
			sa.sin_family = AF_INET;
			sa.sin_addr.s_addr = INADDR_ANY;
			sa.sin_port=htons((u_short)port);
			if(bind(sock_l, (sockaddr *)&sa, sizeof(sa)) != 0){
				msglng(861, "Cannot bind to port %d", port);
				err=5;
			}
			else{
				if(listen(sock_l, Mplayer)){
					msg("Error: listen");
				}
				else{
					wrLog(0);
					if(!gethostname(host, sizeof(host))){
						wrLog(lng(862, "Hostname: %s"), host);
					}
					show(logDlg);
					return 0;
				}
			}
			closesocket(sock_l);
		}
		WSACleanup();
	}
	return err;
}

int serverStart()
{
	if(startListen()) return 1;
	isServer=true;
	drawTitle();
	DWORD id;
	CloseHandle(CreateThread(0, 0, listenLoop, 0, 0, &id));
	return 0;
}

int startConnection()
{
	int err=1;
	unsigned long a;
	sockaddr_in sa;
	hostent *he;

	if(!initWSA()){
		memset(&sa, 0, sizeof(sa));
		sa.sin_family = AF_INET;
		a= inet_addr(servername);
		if(a!=INADDR_NONE){
			sa.sin_addr.S_un.S_addr=a;
		}
		else if((he= gethostbyname(servername))!=0){
			memcpy(&sa.sin_addr, he->h_addr, he->h_length);
		}
		else{
			msglng(863, "Server not found");
			err=2;
			goto le;
		}
		if((sock= socket(PF_INET, SOCK_STREAM, 0))==INVALID_SOCKET){
			msg("Error: socket");
		}
		else{
			sa.sin_port = htons((u_short)port);
			if(connect(sock, (sockaddr*)&sa, sizeof(sa))!=0){
				err=3;
			}
			else{
				return 0;
			}
			closesocket(sock);
		}
	le:
		WSACleanup();
	}
	return err;
}

int clientStart()
{
	int err=startConnection();
	if(err){
		if(err==3) msglng(864, "Cannot connect to server");
		return err;
	}
	isClient=true;
	drawTitle();
	delete[] turTable;
	turTable= new TturPlayer[turNplayers=2];
	delete[] turCells;
	turCells= new TturCell[4];
	DWORD id;
	ResumeThread(clientThread=CreateThread(0, 0, clientLoop, 0, CREATE_SUSPENDED, &id));
	return 0;
}

