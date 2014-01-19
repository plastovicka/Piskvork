/*
 (C) Petr Lastovicka
 
 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License.
*/  
#ifndef langH
#define langH

struct Txywh {
  int x,y,w,h,oldW,oldH;
};

extern char lang[64];

#define Maccel 96
extern ACCEL accel[Maccel];
extern int Naccel;

char *lng(int i, char *s);
void initLang();
int setLang(int cmd);
HMENU loadMenu(char *name, int *subId);
void loadMenu(HWND hwnd, char *name, int *subId);
void changeDialog(HWND &wnd, Txywh &pos, LPCTSTR dlgTempl, DLGPROC dlgProc, HWND owner=0);
void setDlgTexts(HWND hDlg);
void setDlgTexts(HWND hDlg, int id);
void getExeDir(char *fn, char *e);
char *cutPath(char *s);
inline const char *cutPath(const char *s){ return cutPath((char*)s); }
void printKey(char *s, ACCEL *a);


extern void langChanged();
extern void msg(char *text, ...);
void msglng(int id, char *text, ...);
extern HINSTANCE inst;

#define sizeA(A) (sizeof(A)/sizeof(*A))

#endif
