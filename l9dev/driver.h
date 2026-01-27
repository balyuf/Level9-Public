// Atari ST driver (minimal parts adapted for Linux)
// Copyright (C) 1986-1988 Level 9 Computing

#include "common.h"

// function declarations needed for forward references in driver.c
void randomnumber();
void closedown();
void killmultitasking();
void resetginttask();
void init1();
bool init2();
void init();
void calcchecksum(struct _fcb *area);
void driverloadfile(struct _fcb *fcb);
void driversavefile(struct _fcb *fcb);
int osrdch();
void driveroswrch(char *c);
void oswrch(char c);
void driverinputline(char *buffer);
void driverosrdch(char *c);
void settext();
