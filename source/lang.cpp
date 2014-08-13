/*
	(C) 2004-2011  Petr Lastovicka

	This program is free software; you can redistribute it and/or
	modify it under the terms of the GNU General Public License.
	*/
#include "hdr.h"
#pragma hdrstop
#include "lang.h"
/*
Usage:

menu:               submenu "Language", item 29999
registry:           item "language", value lang[64]
WinMain:            readini(); initLang(); ...; langChanded();
case WM_COMMAND:    setLang(cmd);
case WM_INITDIALOG: setDlgTexts(hDlg,titleId);
void langChanged(): LoadMenu, file filters, invalidate, modeless dialogs

ID ranges:
0    standard dialog buttons
11   window titles
100  commands
400  submenus and popup menus
500  file filters
510  dialog controls
700  common error messages
800  application messages
1100 tooltips
1500 end
*/

#ifndef MAXLNGSTR
#define MAXLNGSTR 1500 //maximum string ID
#endif

const int MAXLANG=60;
char lang[64];            //name of the current language

static char *langFile;     //file content (\n are replaced by \0)
static char *lngstr[MAXLNGSTR];//pointers to lines in the langFile
static char *lngNames[MAXLANG+1];//names of all foundlanguages
//-------------------------------------------------------------------------

char *lng(int i, char *s)
{
	if(i>=0 && i<sizeA(lngstr) && lngstr[i]) return lngstr[i];
	return s;
}

//return pointer to the file name after the path
char *cutPath(char *s)
{
	char *t;
	t=strchr(s, 0);
	while(t>=s && *t!='\\' && *t!='/') t--;
	t++;
	return t;
}

//write working dir to fn and append string e
void getExeDir(char *fn, char *e)
{
	GetModuleFileName(0, fn, 192);
	strcpy(cutPath(fn), e);
}
//-------------------------------------------------------------------------
static BOOL CALLBACK enumControls(HWND hwnd, LPARAM)
{
	int i=GetDlgCtrlID(hwnd);
	if((i>=300 && i<sizeA(lngstr) || i<11 && i>0) && lngstr[i]){
		SetWindowText(hwnd, lngstr[i]);
	}
	return TRUE;
}

void setDlgTexts(HWND hDlg)
{
	EnumChildWindows(hDlg, (WNDENUMPROC)enumControls, 0);
}

void setDlgTexts(HWND hDlg, int id)
{
	char *s=lng(id, 0);
	if(s) SetWindowText(hDlg, s);
	setDlgTexts(hDlg);
}

//reload modeless dialog or create a new dialog at the position x,y
void changeDialog(HWND &wnd, Txywh &pos, LPCTSTR dlgTempl, DLGPROC dlgProc, HWND owner)
{
	HWND a, w;

	a=GetActiveWindow();
	w=CreateDialog(inst, dlgTempl, owner, dlgProc);
	if(wnd){
		RECT rc;
		GetWindowRect(wnd, &rc);
		MoveWindow(w, rc.left, rc.top, rc.right-rc.left, rc.bottom-rc.top, FALSE);
		if(IsWindowVisible(wnd)) ShowWindow(w, SW_SHOW);
		DestroyWindow(wnd);
	}
	else{
		SetWindowPos(w, 0, pos.x, pos.y, pos.w, pos.h,
			pos.w ? SWP_NOZORDER : SWP_NOSIZE|SWP_NOZORDER);
	}
	wnd=w;
	if(a) SetActiveWindow(a);
}

//-------------------------------------------------------------------------
void printKey(char *s, ACCEL *a)
{
	*s=0;
	if(a->key==0) return;
	if(a->fVirt&FCONTROL) strcat(s, lng(97, "Ctrl+"));
	if(a->fVirt&FSHIFT) strcat(s, lng(98, "Shift+"));
	if(a->fVirt&FALT) strcat(s, lng(99, "Alt+"));
	s+=strlen(s);
	if((a->fVirt&FVIRTKEY) && (a->key<'0' || a->key>'9')){
		UINT scan=MapVirtualKey(a->key, 0);
		if(a->key==VK_DIVIDE){
			strcpy(s, "Num /");
		}
		else{
			if(a->key>0x20 && a->key<0x2f || a->key==VK_NUMLOCK){
				scan+=256; //Ins,Del,Home,End,PgUp,PgDw,Left,Right,Up,Down
			}
			GetKeyNameText(scan<<16, s, 16);
		}
	}
	else{
		*s++= (char)a->key;
		*s=0;
	}
}
//-------------------------------------------------------------------------
static int *subPtr;

//change names in the menu h, recursively
static void fillPopup(HMENU h)
{
	int i, id, j;
	char *s;
	HMENU sub;
	ACCEL *a;
	MENUITEMINFO mii;
	char buf[128];

	for(i=GetMenuItemCount(h)-1; i>=0; i--){
		id=GetMenuItemID(h, i);
		if(id==29999){
			for(j=0; lngNames[j]; j++){
				InsertMenu(h, 0xFFFFFFFF,
					MF_BYPOSITION|(_stricmp(lngNames[j], lang) ? 0 : MF_CHECKED),
					30000+j, lngNames[j]);
			}
			DeleteMenu(h, 0, MF_BYPOSITION);
		}
		else{
			if(id<0 || id>=0xffffffff){
				sub=GetSubMenu(h, i);
				if(sub){
					id=*subPtr++;
					fillPopup(sub);
				}
			}
			mii.cbSize=sizeof(MENUITEMINFO);
			mii.fMask=MIIM_TYPE;
			mii.dwTypeData=buf;
			mii.cch= sizeA(buf);
			GetMenuItemInfo(h, i, TRUE, &mii);
			if(mii.fType&MFT_SEPARATOR) continue;
			s=lng(id, 0);
			if(s) lstrcpyn(buf, s, sizeA(buf)-32);
			for(a=accel+Naccel-1; a>=accel; a--){
				if(a->cmd==id){
					s=buf+strlen(buf);
					*s++='\t';
					printKey(s, a);
					break;
				}
			}
			mii.fMask=MIIM_TYPE|MIIM_STATE;
			mii.fType=MFT_STRING;
			mii.fState=MFS_ENABLED;
			mii.dwTypeData=buf;
			mii.cch= (UINT)strlen(buf);
			SetMenuItemInfo(h, i, TRUE, &mii);
		}
	}
}

//load menu from resources
//subId are id numbers of names for submenus
HMENU loadMenu(char *name, int *subId)
{
	HMENU hMenu= LoadMenu(inst, name);
	subPtr=subId;
	fillPopup(hMenu);
	return hMenu;
}

void loadMenu(HWND hwnd, char *name, int *subId)
{
	if(!hwnd) return;
	HMENU m= GetMenu(hwnd);
	SetMenu(hwnd, loadMenu(name, subId));
	DestroyMenu(m);
}
//-------------------------------------------------------------------------
static void parseLng()
{
	char *s, *d, *e;
	int id, err=0, line=1;

	for(s=langFile; *s; s++){
		if(*s==';' || *s=='#' || *s=='\n' || *s=='\r'){
			//comment
		}
		else{
			id=(int)strtol(s, &e, 10);
			if(s==e){
				if(!err) msglng(755, "Error in %s\nLine %d", lang, line);
				err++;
			}
			else if(id<0 || id>=sizeA(lngstr)){
				if(!err) msglng(756, "Error in %s\nMessage number %d is too big", lang, id);
				err++;
			}
			else if(lngstr[id]){
				if(!err) msglng(757, "Error in %s\nDuplicated number %d", lang, id);
				err++;
			}
			else{
				s=e;
				while(*s==' ' || *s=='\t') s++;
				if(*s=='=') s++;
				lngstr[id]=s;
			}
		}
		for(d=s; *s!='\n' && *s!='\r'; s++){
			if(*s=='\\'){
				s++;
				if(*s=='\r'){
					line++;
					if(s[1]=='\n') s++;
					continue;
				}
				else if(*s=='\n'){
					line++;
					continue;
				}
				else if(*s=='0'){
					*s='\0';
				}
				else if(*s=='n'){
					*s='\n';
				}
				else if(*s=='r'){
					*s='\r';
				}
				else if(*s=='t'){
					*s='\t';
				}
			}
			*d++=*s;
		}
		if(*s!='\r' || s[1]!='\n') line++;
		*d='\0';
	}
}
//-------------------------------------------------------------------------
void scanLangDir()
{
	int n;
	HANDLE h;
	WIN32_FIND_DATA fd;
	char buf[256];

	lngNames[0]="English";
	getExeDir(buf, "language\\*.lng");
	h = FindFirstFile(buf, &fd);
	if(h!=INVALID_HANDLE_VALUE){
		n=1;
		do{
			if(!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)){
				int len= (int)strlen(fd.cFileName)-4;
				if(len>0){
					char *s= new char[len+1];
					memcpy(s, fd.cFileName, len*sizeof(char));
					s[len]='\0';
					lngNames[n++]=s;
				}
			}
		} while(FindNextFile(h, &fd) && n<MAXLANG);
		FindClose(h);
	}
}
//-------------------------------------------------------------------------
static void loadLang()
{
	memset(lngstr, 0, sizeof(lngstr));
	char buf[256];
	GetModuleFileName(0, buf, sizeA(buf)-strlen(lang)-14);
	strcpy(cutPath(buf), "language\\");
	char *fn= buf + strlen(buf);
	strcpy(fn, lang);
	strcat(buf, ".lng");
	HANDLE f=CreateFile(buf, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
	if(f!=INVALID_HANDLE_VALUE){
		DWORD len=GetFileSize(f, 0);
		if(len>10000000){
			msglng(753, "File %s is too long", fn);
		}
		else{
			delete[] langFile;
			char *langFileA= new char[len+3];
			DWORD r;
			ReadFile(f, langFileA, len, &r, 0);
			if(r<len){
				msglng(754, "Error reading file %s", fn);
			}
			else{
				langFile= langFileA;
				langFile[len]='\n';
				langFile[len+1]='\n';
				langFile[len+2]='\0';
				parseLng();
			}
		}
		CloseHandle(f);
	}
}
//---------------------------------------------------------------------------
int setLang(int cmd)
{
	if(cmd>=30000 && cmd<30000+MAXLANG && lngNames[cmd-30000]){
		strcpy(lang, lngNames[cmd-30000]);
		loadLang();
		langChanged();
		return 1;
	}
	return 0;
}
//---------------------------------------------------------------------------
void initLang()
{
	scanLangDir();
	if(!lang[0]){
		//language detection
		const char* s;
		switch(PRIMARYLANGID(GetUserDefaultLangID()))
		{
			case LANG_CATALAN: s="Catalan"; break;
			case LANG_CZECH: s="Èesky"; break;
			case LANG_FRENCH: s="French"; break;
			case LANG_POLISH: s="Polski"; break;
			default: s="English"; break;
		}
		strcpy(lang, s);
	}
	loadLang();
}
//---------------------------------------------------------------------------

