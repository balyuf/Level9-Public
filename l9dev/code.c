// Generic code generation section
// (adapted for Linux)
//
// based on 68000 version. M.J.Austin 2/6/88
// modified to support 8086 version
//
// Copyright (C) 1988-1989 Level 9 Computing
//
// last change 13/6/88
//
// started Linux version on 23/01/26
// first presentable Linux version: 26/01/26
//

#include "code.h"

// m68k code data tables
extern uint8_t *m68kMCIfVVTable[8];
extern uint8_t *m68kMCIfVCTable[8];
extern struct _datatable m68kDataTable;

// i8086 code data tables
extern uint8_t *i8086MCIfVVTable[8];
extern uint8_t *i8086MCIfVCTable[8];
extern struct _datatable i8086DataTable;

// code data table pointers (initialized to the default m68k)
uint8_t **MCIfVVTable = m68kMCIfVVTable;
uint8_t **MCIfVCTable = m68kMCIfVCTable;
struct _datatable *Data = &m68kDataTable;

// set code data table pointers to i8086
void Mc8086() {
  MCIfVVTable = i8086MCIfVVTable;
  MCIfVCTable = i8086MCIfVCTable;
  Data = &i8086DataTable;
}

// deal with endianness
uint8_t first(uint16_t val) {
  if (i8086) return val & 0xff;
  else return val >> 8;
}

uint8_t second(uint16_t val) {
  if (i8086) return val >> 8;
  else return val & 0xff;
}

//
// Code generation for ALL instructions
//

uint16_t TableNumberInReg = 0; // 1.5 optimization

void MCPush() {
  if (testcomp) {
    compnotimp();
    return;
  }
  int var = compgetvar();
  if (!var) return;
  MCPushPop(Data->Push, var);
}

void MCPop() {
  int var = compgetvar();
  if (!var) return;
  MCPushPop(Data->Pop, var);
}

void MCPushPop(uint8_t *data, int var) {
  compsearch();

  int arg1 = var * 2;
  if (i8086) arg1 += PCvarsoffset;
  Generate(data, arg1, 0, 0); // double var number
}

void MCASR() {
  int var = compgetvar();
  if (!var) return;
  compsearch();

  int arg1 = var * 2;
  if (i8086) arg1 += PCvarsoffset;
  Generate(Data->ASR, arg1, 0, 0); // double var number
}

void MCAnd() {
  MCAddSub(Data->AndVV);
}

void MCOr() {
  MCAddSub(Data->OrVV);
}

void MCXor() {
  MCAddSub(Data->XorVV);
}

void MCData() {
  char c = 0;
  do {
    opcodeaddress = jumpdummyopcode;
    c = compsearch();
    if (c == '@') ptr++; // skip @
    if (!MCDataEntry()) return;
    c = compsearch();
    ptr++; // skip over comma
  } while (c == ',');
  if (c == ';') compssskiptoeol();
}

bool MCDataEntry() {
  if (!i8086) opcodeaddress = dummylongopcode; // In 1.5
  struct _symbol *sym = NULL;
  int label = getlabel(&sym);
  if (label < 0) return false;
  if (!sym) return false;

  // try to ignore code state for data statements
  if (false && sym->codestate && sym->codestate != CodeState) {
    WrongCodeState();
    return false;
  }

  // label is value of label relative to start of acode
  code(first(label));
  code(second(label));

  return true;
}

void MCToAcode() {
  // generate code to switch execution from machine code to ACODE
  int var1 = 0;
  int var2 = 0;

  // arg1 is acode pointer relative to start of acode
  // allow for code generated: i.e. add 12 bytes for ST
  // or add 9 bytes for PC
  var1 = compacodeptr - startacode + (i8086 ? 9 : 12);

  // and second argument gives offset of the first
  // byte of acode - i.e. the initial jump
  // arg2 is offset of start of acode from current.
  // subtract 2 to allow for it being relative to offset word
  // not used for PC
  var2 = i8086 ? 0 : (startacode - compacodeptr - 2);

  // to the start of the first acode instruction to be executed
  Generate(Data->ToAcode, var1, var2, 0);
}

void MCReturn() {
  Generate(Data->Return, 0, 0, 0);
}

void MCLetVC(int var, int con) {
  int arg1 = var * 2;
  if (i8086) arg1 += PCvarsoffset;
  Generate(Data->LetVC, arg1, con, 0); // double var number
}

void MCLetVV(int var1, int var2) {
  int arg1 = var1 * 2;
  int arg2 = var2 * 2;
  if (i8086) arg1 += PCvarsoffset;
  if (i8086) arg2 += PCvarsoffset;
  Generate(Data->LetVV, arg1, arg2, 0); // double var numbers
}

void MCAdd() {
  MCAddSub(Data->AddVV);
}

void MCSub() {
  MCAddSub(Data->SubVV);
}

void MCAddSub(uint8_t *data) {
  // first var - operand
  int var1 = compgetvar();
  if (!var1) return;
  if (compsearch() != ',') {
    commaexpected();
    return;
  }
  ptr++; // skip over comma
  // second var - result of add
  int var2 = compgetvar();
  if (!var2) return;

  int arg1 = var1 * 2;
  int arg2 = var2 * 2;
  if (i8086) arg1 += PCvarsoffset;
  if (i8086) arg2 += PCvarsoffset;
  Generate(data, arg1, arg2, 0); // double var numbers
}

void MCAttVar(int table, int index) {
  if (compsearch() != ')') {
    bracketsexpected();
    return;
  }
  ptr++; // skip bracket

  if (compsearch() != '=') {
    equalsexpected();
    return;
  }
  ptr++; // skip equals

  // and finally the var not inside the brackets
  int var = compgetvar();
  if (!var) return;

  // quadruple table number to give ptr table offset
  // double var numbers to give vartble offsets
  int arg1 = table * 4;
  int arg2 = index * 2;
  int arg3 = var * 2;
  if (i8086) arg1 += PCListVector;
  if (i8086) arg2 += PCvarsoffset;
  if (i8086) arg3 += PCvarsoffset;
  TableGenerate(Data->AttVV, Data->AttVV16, arg1, arg2, arg3);
}

void MCAttConst(int table, int index) {
  if (compsearch() != ')') {
    bracketsexpected();
    return;
  }
  ptr++; // skip bracket

  if (compsearch() != '=') {
    equalsexpected();
    return;
  }
  ptr++; // skip equals

  // and finally the var not inside the brackets
  int var = compgetvar();
  if (!var) return;

  // index - constant - no change
  // double var to give vartbl offset
  // quadruple table number to give ptr table offset
  int arg1 = table * 4;
  int arg2 = index;
  int arg3 = var * 2;
  if (i8086) arg1 += PCListVector;
  if (i8086) arg3 += PCvarsoffset;
  TableGenerate(Data->AttCV, Data->AttCV16, arg1, arg2, arg3);
}

void MCAft(int var, int table) { // ttVar
  // called as soon as the table reference has been parsed
  int index = 0;
  int arg1 = 0;
  int arg2 = 0;
  int arg3 = 0;

  if (compsearch() != '(') {
    bracketsexpected();
    return;
  }
  ptr++; // skip bracket

  struct _symbol *sym3 = NULL;
  bool found = findsymbol(&sym3);
  if (!sym3) return;
  if (found) {
    int type = sym3->type & 0x7f;
    sym3->type |= 0x80;
    if (type == vartype) {
      // a variable
      index = sym3->value; // value of var

      if (compsearch() != ')') {
        bracketsexpected();
        return;
      }
      ptr++; // skip bracket

      // now generate the code
      arg1 = index * 2; // index - first arg
      arg2 = table * 4; // table number - second arg
      arg3 = var * 2; // var to assign to - third arg
      if (i8086) arg1 += PCvarsoffset;
      if (i8086) arg2 += PCListVector;
      if (i8086) arg3 += PCvarsoffset;
      TableGenerate(Data->AftVV, Data->AftVV16, arg1, arg2, arg3);
      return;
    } else if (type != constanttype) {
      badindex();
      return;
    } else {
      // a manifest constant
      index = sym3->value; // value of constant
    }
  } else {
    // a numeric value?
    index = getnumberconstant(); // will print an error message if not
    if (index < 0) return;
  }

  if (compsearch() != ')') {
    bracketsexpected();
    return;
  }
  ptr++; // skip bracket

  // now generate the code
  arg1 = var * 2; // double v1
  arg2 = table * 4; // quadruple table number
  arg3 = index; // index
  if (i8086) arg1 += PCvarsoffset;
  if (i8086) arg2 += PCListVector;
  TableGenerate(Data->AftVC, Data->AftVC16, arg1, arg2, arg3);
}

void TableGenerate(uint8_t *data8, uint8_t *data16, int arg1, int arg2, int arg3) {
  // avoid maintaining address register across call.
  // this seems to be a way to disable the optimization
  // however BTK uses list0 which still matches but
  // if the register in the interpreter is actually kept zero
  // then this should not be a problem as the occurrence
  // is one that indeed has the table in arg1
  // also it is only in demo.txt which is not used by GM
  // in this case Data->AttVV16 is used
  TableNumberInReg = 0; // 1.5 optimization (partially disabled)

  // use data as generation table pointer in 8 bit table mode,
  // or data16 in 16 bit mode
  uint8_t *data = (SixteenFlag) ? data16 : data8;

  // table is not always linked to arg1 !?
  // so this is a potential bug, but GM and BTK do not seem to need it
  // to solve this issue, better name the arguments by role, e.g.
  // TableGenerate(data8, data16, table, index, arg)
  uint16_t table = arg1; // table is not always linked to arg1 !?
  if (!i8086 && table == TableNumberInReg) { // not used for PC
    // Don't load first part of table
    uint8_t len = *data++;
    data += 8;
    len -= 4;
    GenerateLen(data, len, arg1, arg2, arg3);
  } else {
    // Load full table but store (double of) tablenumber
    TableNumberInReg = table;
    Generate(data, arg1, arg2, arg3);
  }
}

void MCgoto() {
  if (compsearch() != '@')
    MCShortGoto();
  else {
    ptr++;
    MCLongGoto();
  }
}

void MCLongGoto() {
  Generate(Data->LongGoto, 0, 0, 0);
  MCLongJump();
}

void MCShortGoto() {
  if (forcedlongjumps)
    MCLongGoto();
  else {
    Generate(Data->ShortGoto, 0, 0, 0);
    MCShortJump();
  }
}

void MCgosub() {
  // avoid maintaining address register across call.
  // should be 0xffff (-1) to avoid matching list0
  // yet more cases need the reset?
  // like goto, labels, etc, anything which is not atomic
  TableNumberInReg = 0; // 1.5 optimization

  if (compsearch() != '@')
    if (i8086) MCLongGosub();
    else MCShortGosub(); // PC does not have short gosub
  else {
    ptr++;
    MCLongGosub();
  }
}

void MCLongGosub() {
  Generate(Data->LongGosub, 0, 0, 0);
  MCLongJump();
}

void MCShortGosub() {
  if (forcedlongjumps)
    MCLongGosub();
  else {
    Generate(Data->ShortGosub, 0, 0, 0);
    MCShortJump();
  }
}

void MCLongJump() {
  opcodeaddress = dummylongopcode;
  struct _symbol *sym = NULL;
  int label = getlabel(&sym);
  if (label < 0) return;
  if (!sym) return;

  if (sym->codestate && sym->codestate != CodeState) {
    WrongCodeState();
    return;
  }

  // label is value of label relative to start of acode
  // find current address relative to start
  int delta = label - (compacodeptr - startacode);
  // for PC: relative to instruction AFTER the current one
  if (i8086) delta -= 2; // In 1.5
  if (i8086 || (delta < 32768 && delta >= -32768)) {
    code(first(delta));
    code(second(delta));
  } else {
    MCTooFar();
    return;
  }
}

void MCShortJump() {
  if (forcedlongjumps)
    MCLongJump();
  else {
    opcodeaddress = dummyshortopcode;
    struct _symbol *sym = NULL;
    int label = getlabel(&sym);
    if (label < 0) return;
    if (!sym) return;

    if (sym->codestate && sym->codestate != CodeState) {
      WrongCodeState();
      return;
    }

    // label is value of label relative to start of acode
    // find current address relative to start
    // short relative is relative to start of next instruction
    // how close are current and destination?
    int delta = label - (compacodeptr - startacode + 1);
    if (delta < 128 && delta >= -128) code(delta);
    else {
      reljumpoutofrange();
      return;
    }
  }
}

void MCrfr(struct _symbol *sym, struct _forwardentry *entry) {
  int label = compacodeptr - startacode;
  char *ref = (char *)(startacode + entry->ref);
  void *address = startacode + entry->opcode;
  char opcode = *(char *)address;
  bool longjump = false;
  if (i8086) {
    // for PC, DATA is relative to acode start, unlike everything else
    // and counteract the correction (2) for instruction relativity
    if (address == jumpdummyopcode) label += entry->ref + 2;
    // is this PC specific, or can we do this in general?
    if (address != dummyshortopcode) longjump = true;
  } else longjump = !(opcode & relmask);
  if (longjump) { // is this a long jump?
    // poke back in address for a long jump
    // make it relative to address of reference
    int delta = label - entry->ref;
    // for PC: relative to instruction AFTER the current one
    if (i8086) delta -= 2; // In 1.5
    if (delta >= 32768) {
      forwardoutofrange(sym, entry); // note special line number treatment
      return;
    }
    // poke back two-byte reference
    *ref++ = first(delta); // high byte
    *ref   = second(delta); // low byte
  } else { // no!
    // poke back in address for short jump
    // make it relative to address of reference
    // relative to start of next instruction after Bra.s
    int delta = label - (entry->ref + 1);
    if (delta >= 128) { // was 0x100!
      forwardoutofrange(sym, entry); // note special line number treatment
      return;
    }
    *ref = delta; // poke in single byte reference
  }
}

void MCIF() {
  // handle if v = <> < > then label
  opcodeaddress = compacodeptr;
  int value = 0;
  int arg1 = 0;
  int arg2 = 0;
  uint8_t **table = NULL;

  int v1 = compgetvar();
  if (!v1) return;
  int op = evaloperator(); // op = operator type
  if (op < 0) return;
  struct _symbol *sym = NULL;
  bool found = findsymbol(&sym);
  if (!sym) return;
  if (!found) {
    // only other possibility is a number
    value = getnumberconstant();
    if (value < 0) return;
    arg1 = v1 * 2;
    arg2 = value;
    if (i8086) arg1 += PCvarsoffset;
    table = MCIfVCTable;
  } else {
    int type = sym->type & 0x7f;
    sym->type |= 0x80; // set "USED" bit
    if (type == vartype) {
      arg1 = v1 * 2;
      arg2 = sym->value * 2;
      if (i8086) arg1 += PCvarsoffset;
      if (i8086) arg2 += PCvarsoffset;
      table = MCIfVVTable;
    } else if (type != constanttype) {
      badtype();
      return;
    } else {
      arg1 = v1 * 2;
      arg2 = sym->value;
      if (i8086) arg1 += PCvarsoffset;
      table = MCIfVCTable;
    }
  }

  // given arg1, arg2 as two args for the comparison stage,
  // and table as basic IF V C/V Table
  // and op as the comparator type, generate the code:
  table = table + (op & 0xff);

  // skip over "THEN" if present, then evaluate jump address
  compsearch();
  stringcompare("THEN");
  // stringcompare automatically skips string if matched

  // is this a short or a long jump?
  char c = compsearch();
  if (c == '@') ptr++; // skip over '@'
  if (c == '@' || forcedlongjumps) {
    table += 4; // move to long if table section
    Generate(*table, arg1, arg2, 0);
    MCLongJump();
  } else {
    Generate(*table, arg1, arg2, 0);
    MCShortJump();
  }
}

void MCPrint() {
  NotAllowedInMC();
}

void MCMessage() {
  // MCMessageC
  NotAllowedInMC();
}

// void MCFunction() {
//   NotAllowedInMC();
// }

void MCInput() {
  NotAllowedInMC();
}

// void MCCHangeCode() {
//   NotAllowedInMC();
// }

// void MCCall() {
//   NotAllowedInMC();
// }

void MCExit() {
  NotAllowedInMC();
}

void MCScreen() {
  NotAllowedInMC();
}

void MCPicture() {
  NotAllowedInMC();
}

void MCGetNext() {
  NotAllowedInMC();
}

void MCPrintInput() {
  NotAllowedInMC();
}

void MCDriverOpcode() {
  NotAllowedInMC();
}

void MCRandom() {
  NotAllowedInMC();
}

void MCSave() {
  NotAllowedInMC();
}

void MCRestore() {
  NotAllowedInMC();
}

void MCClear() {
  NotAllowedInMC();
}

void MCStack() {
  NotAllowedInMC();
}

// void MCPrs() {
//   NotAllowedInMC();
// }

void MCCLS() {
  NotAllowedInMC();
}

void MCJump() {
  NotAllowedInMC();
}

void MCAcodePrs() {
  NotAllowedInMC();
}

void MCBreakPt() {
  Generate(Data->BreakPt, 0, 0, 0);
}

void GenerateLen(uint8_t *data, uint8_t len, int arg1, int arg2, int arg3) {
  // given data as the code data for the current instruction
  // arg1 is first argument, arg2 is second argument, arg3 is third argument

  // check that code ptr is even.
  if (!i8086 && (compacodeptr - startacode) & 1) {
    EvenError(); // not done for PC in 1.5
    return;
  }

  // 1.5 optimization where table number might be kept in register
  // In that case the first part of the table is skipped
  // And hence the length must be a parameter rather than *data
  while (len--) { // end of code?
    // fetch instruction
    uint16_t ins = *data++ << 8;
    ins |= *data++;

    // which argument do we want?
    uint16_t val = 0;
    if (ins & 0x4000) val = arg1;
    else if (ins & 0x2000) val = arg2;
    else if (ins & 0x1000) val = arg3;
    // else don't use argument - use ins instead (i.e. low byte from table)

    // add anything to argument?
    val += ins & 0xff; // low byte of ins is 8-bit offset
    // use high/low byte of argument?
    if (ins & 0x8000) val >>= 8;

    // write code
    code(val);
  }
}

void Generate(uint8_t *data, int arg1, int arg2, int arg3) {
  // 1.5 optimization where table number might be kept in a register
  // This is the original call with the full table
  uint8_t len = *data++; // length of this code fragment
  GenerateLen(data, len, arg1, arg2, arg3);
}
