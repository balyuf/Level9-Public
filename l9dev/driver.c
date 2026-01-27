// Atari ST driver (minimal parts adapted for Linux)
// Copyright (C) 1986-1988 Level 9 Computing
// M.J.Austin 20/7/86
// first presentable Linux version 21/12/25

// note - if 'graphics' is defined elsewhere (as anything),
// the appropriate code will be defined in here.
// To make this driver public domain, simply
// remove all the code assembled within the 'ifd' instruction

// Some programs need to know which machine they are running
// on (e.g. to allow for different assemblers)
// this is catered for by the following constants
// THIS FACILITY SHOULD BE USED AS LITTLE AS POSSIBLE

#include <stdarg.h> // for va_ macros

#include "driver.h"

char driverbuffer[40];

////////////////////////////////////////////
// the Program Itself //
////////////////////////////////////////////

//---
int OutputDevice = 2; // 2 = CON: 1 = AUX:
FILE *logfile = NULL;
void openlogfile(char *filename) {
  logfile = fopen(filename, "a");
}
void closelogfile() {
  if (logfile) fclose(logfile);
  logfile = NULL;
}
void prs(char const *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);
    if (OutputDevice == 1 && logfile) {
      va_start(ap, fmt);
      vfprintf(logfile, fmt, ap);
      va_end(ap);
    }
}
char waitkey() {
  while (driver(osrdchdcode, driverbuffer), !*(char *)driverbuffer);
  return *(char *)driverbuffer;
}
//---

void driver(int code, void *buffer) {
// Standard entry point for all external routines
  switch (code) {
  case initdcode:
    init(); break;
  case checksumdcode:
    calcchecksum(buffer); break;
  case oswrchdcode:
    driveroswrch(buffer); break;
  case osrdchdcode:
    driverosrdch(buffer); break;
  case savedcode:
    driversavefile(buffer); break;
  case loaddcode:
    driverloadfile(buffer); break;
  case settextdcode:
    settext(); break;

  case taskinitdcode:
    resetginttask(); break;
  case inputlinedcode:
    driverinputline(buffer); break;
  case returntoosdcode:
    closedown(); break;
  case randomnumberdcode:
    randomnumber(buffer); break;
  }
}

//---
int getclock() {
  // return real time clock in list9(hi4,hi3,lo2,lo)
  // approx 1/50 second per unit
// move.l realtimeclock,d0
// move.l d0,(a6)
  return 0;
}
//---
void randomnumber(void *buffer) {
  // return a random word
  *(uint16_t *)buffer = (uint16_t)rand();
}
//---
void closedown() {
  // exit gracefully
  prs("\nDo you really want to leave the game? ");
  char c;
  do {
    c = toupper(osrdch());
  } while (c != 'Y' && c != 'N');
  if (c == 'Y') {
// ifd graphics
    killmultitasking();
    driver(settextdcode, NULL);
// endc // if graphics
    returntogem();
  }
}
//---
// ifd graphics
void killmultitasking() {
}
int getscreenaddress() {
// return address of screen start in a0.l and d0.l
// call_ebios _physbase
// addq.l 2,sp
// move.l d0,a0
  return 0;
}
void resetginttask() {
// lea gintstacktop(pc),a0

// lea gintstart(pc),a1
// move.l a1,-(a0)
// move.w sr,-(a0)
// lea irqswaptaskend(pc),a1
// move.l a1,-(a0) // return address from ist1

// movem.l d0-d7/a0-a6,-(a0) // dummy stack values
// lea taskstackptr,a1
// move.l a0,(a1)
// rts
}
void init1() {
  // cursorxpos = 0;
  // cursorypos = 24; // bottom left is 0,24 here

  // cyclicwriteptr = cyclicib;
  // cyclicbufferstart = cyclicib;
  // cyclicbufferend = cyclicibtop;
  // cycliccharsused = 0;
}
//---
bool init2() {
  // screenpointer = getscreenaddress();
  // now before doing anything, clear out the screen
  // system("clear"); // not really necessary
  // scrolledlines = 0;
  // and what screen resolution are we in ?
// call_ebios _getrez * move.w #4,-(sp)
// addq.l #2,sp
  // d0.b = 0 - low resolution, 1 = medium resolution, 2 = high res
  // screenresolution = 1;
  // if (screenresolution == 2) {
  //   screenheight = absscreenheight;
  // } else {
  //   screenresolution = 0; // always low res for graphics nowadays
  // }
  // return screenresolution == 2; // hi-res ?
  return false;
}
//---
void init() {
  // .initialise
  // first, some general purpose initialisation ---

  // init1();
// ifd graphics
  // initialisetasks();
  // seterrorvectors(); // address error and bus error etc.
// endc // graphics

  // bool ishires = init2();
// ifd graphics
  // if (!ishires) initsplitscreen(); // no, so set up med-low res split screen
// endc

  // screenheight = absscreenheight * 2; // hi-res height
}
void calcchecksum(struct _fcb *area) {
  // calculate checksum of area between area->start and area->end
  int length = area->end - area->start + 1;
  uint8_t checksum = 0;
  uint8_t *checkptr = (uint8_t *)area->start;
  while (length-- > 0) checksum += *checkptr++;
  *(uint8_t *)area = checksum;
}

//---
void getfilename(struct _fcb *fcb) {
  char *f = fcb->filename;
  if (!*f) {
    prs("\nFilename ? ");
    scanf("%s", f);
  }
}

void driverloadfile(struct _fcb *fcb) {
  getfilename(fcb);
  FILE *fp = fopen(fcb->filename, "r");
  if (fp) {
    int r = fread(fcb->start, 1, 0x30000, fp);
    fclose(fp);
    fcb->end = fcb->start + r;
    *(char *)fcb = 0; // signify load ok
  } else {
    prs("Can't find file on disk.\n");
    *(char *)fcb = 1; // signify load error
  }
}

void driversavefile(struct _fcb *fcb) {
  getfilename(fcb);
  FILE *fp = fopen(fcb->filename, "w");
  if (fp) {
    int size = fcb->end - fcb->start;
    int written = fwrite(fcb->start, 1, fcb->end - fcb->start, fp);
    fclose(fp);
    if (written == size) {
      *(char *)fcb = 0; // signify save ok
    } else fp = NULL;
  }
  if (!fp) {
    prs("Save error.\n");
    *(char *)fcb = 1; // signify save error
  }
}
void hexlonga0(uint32_t n) {
  prs("%08X", n);
}
void hexworda0(uint16_t n) {
  prs("%04X", n);
}
void hexbyted0(uint8_t n) {
  prs("%02X", n & 0xff);
}
void printhexdigit(uint8_t n) {
  prs("%01X", n & 0xf);
}
void printdecimald0(int n) {
  // print n as a decimal number,
  // suppressing leading zeros
  prs("%d", n);
}
int readdecimal(char **s) {
  // given a decimal number as an ascii string at *s, return
  // its value
  // and *s = character after the number
  return strtol(*s, s, 10);
}
//---
bool isd0alphanumeric(char c) {
  c = toupper(c);
  return ((c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z'));
}
//--- 
#ifdef __linux__
#include <unistd.h>
#include <termios.h>
struct termios saved_attributes;
#endif
void reset_input_mode() {
#ifdef __linux__
  if (!isatty(STDIN_FILENO)) return;
  tcsetattr(STDIN_FILENO, TCSANOW, &saved_attributes);
#endif
}
void set_input_mode() {
#ifdef __linux__
  struct termios tattr;
  if (!isatty(STDIN_FILENO)) return;

  tcgetattr(STDIN_FILENO, &saved_attributes);
  atexit(reset_input_mode);

  tcgetattr(STDIN_FILENO, &tattr);
  tattr.c_lflag &= ~(ICANON|ECHO);
  tattr.c_cc[VMIN] = 1;
  tattr.c_cc[VTIME] = 0;
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &tattr);
#endif
}
int osrdch() {
  char c;
  set_input_mode();
  c = getchar();
  reset_input_mode();
  if (c == EOF) c = 0;
  return c;
}
void driveroswrch(char *c) {
  oswrch(*c);
}
void oswrch(char c) {
  prs("%c", c);
}
void driverinputline(char *buffer) {
  scanf("%s\n", buffer);
}
void driverosrdch(char *c) {
  *c = osrdch();
}
void returntogem() {
// ifd graphics
  killmultitasking();
  driver(settextdcode, NULL);
// endc // if graphics
  prs("\n");
// call_bdos p_term
  exit(0);

//----------

// ifnd graphics // only assembled if graphics has not been defined
//  settext
//  setgraphics
//  driverclg
//  driverchgcol
//  line
//  fill
//  gintstart
//  killmultitasking
//  rts
// endc
}

//---

// ifd graphics // only assembled if graphics has been defined


void settext() {
  // clear top half of screen
// move.l screenpointer(pc),a0
// move.w #$3bfe,d0 * length to clear
// ...

  // and kill the split screen, restore full screen scrolling etc.
  // drivergraphicsmode = 0;
}
