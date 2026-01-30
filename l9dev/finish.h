// finish program (for Linux)
//
// Copyright (C) 1986-1988 Level 9 Computing

#include "common.h"

// constants
#define lengthpointeroffset 0
#define mdptroffset 2 // message descriptor ptr
#define mdlengthoffset 4
#define dictionaryptroffset 6
#define dictionarylengthoffset 8
#define indextableptroffset 10
#define numbersegments 12
#define numbercommonwordtable 14
#define versionptroffset 16
#define exitptroffset 18
#define list1ptroffset 22
#define list2ptroffset 24
#define list9ptroffset 38
#define acodeptroffset 40
#define startofdataoffset 42 // first real data starts here

// function declarations needed for forward references in finish.c
void finishstart();
int getdec();
int getbin();
int z80load();
void z80write(int val);
void processacode();
void processacodeptr();
void processexits();
void processtables();
void savedirectory();
void selectdirectory();
void processsquash();
void climb();
void processacodeptr();
void finishchecksum();
void descend();
void errorindata();
void ssskiptoeol();
void skiptoeol();
int search();
int supersearch();

// defined in comp.c
extern bool oddacode;
extern bool splitdata;
