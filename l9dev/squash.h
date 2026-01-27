// 68000 Acode squasher program (for Linux)
// 
// Copyright (C) 1986-1988 Level 9 Computing
//

#include "common.h"

// constants
#define maxwordlen 64
#define squasherprs prs
#define ramsize 0x100000
// Version
#define MAJOR "2"
#define MINOR "3"
#define major '2'
#define minor '3'
#define lengthpointers 0x14 // message data pointers
// must be at least big enough to hold largest text file
#define spaceforsquashdat 100000
// Message Descriptor Header:
#define jump    0x80    // Jump header or Message header.
#define parse   0x40    // Message contains keywords.
// Word reference:
#define longf   0x8000    // Short or Long-form reference.
// Special short-codes:
#define longc   0x1A    // Long escape code
#define header  0x1C    // Header short-code
#define endseg  0x1B    // Segment end marker
// Greater than $07, less than longc:
#define uppercasemark   0x10 // Operand to LONGC
#define maxheaderlength 3    // Max similarity count in header
// Character codes
#define modenull   1  // Initial/Follows-word state
#define modespace  2  // Stored-space
#define modechar   3  // Stored-character (in p6char)
#define modesc     4  // Space followed by character (in p6char)
#define modecs     5  // Character followed by space (character in p6char)
#define modeinit   6  // Start of message
#define modelead   7  // Spaces at start of message
// (un)pack pointers
#define unpack1 ((uint8_t *)buffer+0)
#define unpack2 ((uint8_t *)buffer+1)
#define unpack3 ((uint8_t *)buffer+2)
#define unpack4 ((uint8_t *)buffer+3)
#define unpack5 ((uint8_t *)buffer+4)
#define unpack6 ((uint8_t *)buffer+5)
#define unpack7 ((uint8_t *)buffer+6)
#define unpack8 ((uint8_t *)buffer+7)
#define pack1 ((uint8_t *)buffer+8)
#define pack2 ((uint8_t *)buffer+9)
#define pack3 ((uint8_t *)buffer+10)
#define pack4 ((uint8_t *)buffer+11)
#define pack5 ((uint8_t *)buffer+12)

// external declarations
// driver.c
extern void prs(char const *, ...);

// stage1.c
extern void squasherflush();
extern void phase01();
extern void phase02();
extern void phase03();
extern void phase04();
extern void phase05();
extern int compare();
extern void loadbuffer();
extern char buffer[];
extern bool noisy;
extern bool debug;
extern int commonmaxentries;
extern void *ram;
extern void *endmemory;
extern void *starttablememory;
extern void *startfilebuffer;
extern void *endcwt;
extern int msgnum;
extern int frequencytimes;
extern int inputlength;
extern int lengthsmt;
extern int linenum;
extern int numcharacters;
extern int numwords;
extern void *startcwt;
extern void *startft;
extern char *ftptr;
extern void *endft;
extern char *startmsg;
extern int messagelength;
extern int wordnumber;
extern char *readptr;
extern char *writeptr;
extern char *cwtptr;
extern void displaycode(char);

// stage2.c
extern void phase06();
extern void phase07();
extern void phase08();
extern void phase09();
extern bool oldgame;

// forward declarations
// stage1.c
void sortfile();
void sortmessage();
void phase2error();
void displaywhere();
void displaylinenum();
char p1getlastchar();
char casemodify(char);
int checktype(char);
char gettype(int);
void phase3error();
void countsentence();
void p3badwordtype();
bool validwordstart();
void loadbuffer();
void addfrequency();
void uppointer();
void downpointer();
int compare();
void displayfrequency();
void displaysortedmessage();
void displayheader();
void assesscommonword();
void assesscommonref();
void displayword();

// stage2.c
void convertmessage();
void writeicf(uint8_t);
void writeicf16(uint16_t);
void writesquash(uint8_t);
void writesquash16(uint16_t);
int calccompression();
void mdtfull();
void p6error();
void storecharacter(uint16_t);
void processwordtype();
bool findfrequency();
void storewordnum();
void modeeof();
void examinekeywords();
void storereference(uint16_t);
int getwordnumber();
void clearthree();
void storeworddictionary(uint8_t);
void bytealign();
void convertword();
void doconversion();
bool uppercaseword(int);
