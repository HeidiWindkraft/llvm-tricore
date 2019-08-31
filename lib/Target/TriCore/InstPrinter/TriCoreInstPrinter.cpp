//==-- TriCoreInstPrinter.cpp - Convert TriCore MCInst to assembly syntax -===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This class prints an TriCore MCInst to a .s file.
//
//===----------------------------------------------------------------------===//

#include "TriCoreInstPrinter.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/CodeGen/ISDOpcodes.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCInstrInfo.h"
#include "llvm/MC/MCRegisterInfo.h"
#include "llvm/MC/MCSymbol.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/raw_ostream.h"
#include "../TriCore.h"
using namespace llvm;

#define DEBUG_TYPE "asm-printer"

#include "TriCoreGenAsmWriter.inc"

TriCoreInstPrinter::TriCoreInstPrinter(const MCAsmInfo &MAI, const MCInstrInfo &MII,
                               const MCRegisterInfo &MRI)
    : MCInstPrinter(MAI, MII, MRI) {}

void TriCoreInstPrinter::printRegName(raw_ostream &OS, unsigned RegNo) const {
  OS << "%" << StringRef(getRegisterName(RegNo)).lower();
}

void TriCoreInstPrinter::printInst(const MCInst *MI, raw_ostream &O,
                               StringRef Annot, const MCSubtargetInfo &STI) {

  unsigned Opcode = MI->getOpcode();

  switch (Opcode) {
    // Combine 2 AddrRegs from disassember into a PairAddrRegs to match
    // with instr def. load/store require even/odd AddrReg pair. To enforce
    // this constraint, a single PairAddrRegs reg operand is used in the .td
    // file to replace the two AddrRegs. However, when decoding them, the two
    // AddrRegs cannot be automatically expressed as a PairAddrRegs, so we
    // have to manually merge them.
    // FIXME: We would really like to be able to tablegen'erate this.
    case TriCore::LD_DAabs:
    case TriCore::LD_DAbo:
    case TriCore::LD_DApreincbo:
    case TriCore::LD_DApostincbo:
    case TriCore::ST_Bcircbo:
    case TriCore::ST_Hcircbo:
    case TriCore::ST_Wcircbo:
    case TriCore::ST_Dcircbo:
    case TriCore::ST_Qcircbo:
    case TriCore::ST_Acircbo:
    case TriCore::ST_Bbitrevbo:
    case TriCore::ST_Hbitrevbo:
    case TriCore::ST_Wbitrevbo:
    case TriCore::ST_Dbitrevbo:
    case TriCore::ST_Qbitrevbo:
    case TriCore::ST_Abitrevbo: {
      const MCRegisterClass &MRC = MRI.getRegClass(TriCore::AddrRegsRegClassID);
      unsigned Reg = MI->getOperand(0).getReg();
      if (MRC.contains(Reg)) {
        MCInst NewMI;
        MCOperand NewReg;
        NewMI.setOpcode(Opcode);

        NewReg = MCOperand::createReg(MRI.getMatchingSuperReg(
            Reg, TriCore::subreg_even,
            &MRI.getRegClass(TriCore::PairAddrRegsRegClassID)));
        NewMI.addOperand(NewReg);

        // Copy the rest operands into NewMI.
        for (unsigned i = 2; i < MI->getNumOperands(); ++i)
          NewMI.addOperand(MI->getOperand(i));
        printInstruction(&NewMI, O);
        return;
      }
      break;
    }
    case TriCore::ST_DAabs:
    case TriCore::ST_DAbo:
    case TriCore::ST_DApreincbo:
    case TriCore::ST_DApostincbo:
    case TriCore::LD_Bcircbo:
    case TriCore::LD_BUcircbo:
    case TriCore::LD_Hcircbo:
    case TriCore::LD_HUcircbo:
    case TriCore::LD_Wcircbo:
    case TriCore::LD_Dcircbo:
    case TriCore::LD_Acircbo:
    case TriCore::LD_Bbitrevbo:
    case TriCore::LD_BUbitrevbo:
    case TriCore::LD_Hbitrevbo:
    case TriCore::LD_HUbitrevbo:
    case TriCore::LD_Wbitrevbo:
    case TriCore::LD_Dbitrevbo:
    case TriCore::LD_Abitrevbo: {
      const MCRegisterClass &MRC = MRI.getRegClass(TriCore::AddrRegsRegClassID);
      unsigned Reg = MI->getOperand(1).getReg();      
      if (MRC.contains(Reg)) {
        MCInst NewMI;
        MCOperand NewReg;
        NewMI.setOpcode(Opcode);
        
        NewMI.addOperand(MI->getOperand(0));
        NewReg = MCOperand::createReg(MRI.getMatchingSuperReg(
            Reg, TriCore::subreg_even,
            &MRI.getRegClass(TriCore::PairAddrRegsRegClassID)));
        NewMI.addOperand(NewReg);

        // Copy the rest operands into NewMI.
        for (unsigned i = 3; i < MI->getNumOperands(); ++i)
          NewMI.addOperand(MI->getOperand(i));
        printInstruction(&NewMI, O);
        return;
      }
      break;
    }    
    case TriCore::LD_DAcircbo:
    case TriCore::ST_DAcircbo:
    case TriCore::LD_DAbitrevbo:
    case TriCore::ST_DAbitrevbo: {
      const MCRegisterClass &MRC = MRI.getRegClass(TriCore::AddrRegsRegClassID);
      unsigned Reg1 = MI->getOperand(0).getReg();
      unsigned Reg2 = MI->getOperand(2).getReg();
      if (MRC.contains(Reg2)) {
        MCInst NewMI;
        NewMI.setOpcode(Opcode);
        
        NewMI.addOperand(MCOperand::createReg(MRI.getMatchingSuperReg(
          Reg1, TriCore::subreg_even,
          &MRI.getRegClass(TriCore::PairAddrRegsRegClassID))));
       
        NewMI.addOperand(MCOperand::createReg(MRI.getMatchingSuperReg(
          Reg2, TriCore::subreg_even,
          &MRI.getRegClass(TriCore::PairAddrRegsRegClassID))));

        // Copy the rest operands into NewMI.
        for (unsigned i = 4; i < MI->getNumOperands(); ++i)
          NewMI.addOperand(MI->getOperand(i));
        printInstruction(&NewMI, O);
        return;
      }
      break;
    }
  }

  printInstruction(MI, O);
  printAnnotation(O, Annot);
}

static void printExpr(const MCExpr *Expr, const MCAsmInfo *MAI,
                      raw_ostream &OS) {
  int Offset = 0;
  const MCSymbolRefExpr *SRE;

  if (const MCBinaryExpr *BE = dyn_cast<MCBinaryExpr>(Expr)) {
    SRE = dyn_cast<MCSymbolRefExpr>(BE->getLHS());
    const MCConstantExpr *CE = dyn_cast<MCConstantExpr>(BE->getRHS());
    assert(SRE && CE && "Binary expression must be sym+const.");
    Offset = CE->getValue();
  } else {
    SRE = dyn_cast<MCSymbolRefExpr>(Expr);
    assert(SRE && "Unexpected MCExpr type.");
  }

  MCSymbolRefExpr::VariantKind Kind = SRE->getKind();

  switch (Kind) {
    default:                                 llvm_unreachable("Invalid kind!");
    case MCSymbolRefExpr::VK_None:           break;
    case MCSymbolRefExpr::VK_TRICORE_HI_OFFSET:    OS << "hi:";     break;
    case MCSymbolRefExpr::VK_TRICORE_LO_OFFSET:    OS << "lo:";     break;
  }

  SRE->getSymbol().print(OS, MAI);

  if (Offset) {
    if (Offset > 0)
      OS << '+';
    OS << Offset;
  }
}

void TriCoreInstPrinter::printPCRelImmOperand(const MCInst *MI, unsigned OpNo,
                                             raw_ostream &O) {
  const MCOperand &Op = MI->getOperand(OpNo);
  if (Op.isImm())
    O << Op.getImm();
  else {
    assert(Op.isExpr() && "unknown pcrel immediate operand");
    Op.getExpr()->print(O, &MAI);
  }
}

void TriCoreInstPrinter::printPairAddrRegsOperand(const MCInst *MI, unsigned OpNo,
                                             raw_ostream &O) {
  unsigned AddrReg = MI->getOperand(OpNo).getReg();

  O << "[";
  printRegName(O, MRI.getSubReg(AddrReg, TriCore::subreg_even));
  O << "/";
  printRegName(O, MRI.getSubReg(AddrReg, TriCore::subreg_odd));
  O << "]";
}


//===----------------------------------------------------------------------===//
// PrintSExtImm<unsigned bits>
//===----------------------------------------------------------------------===//
template <unsigned bits>
void TriCoreInstPrinter::printSExtImm(const MCInst *MI, unsigned OpNo,
                                       raw_ostream &O) {
  if (MI->getOperand(OpNo).isImm()) {
    int64_t Value = MI->getOperand(OpNo).getImm();
    Value = SignExtend32<bits>(Value);
    assert(isInt<bits>(Value) && "Invalid simm argument");
    O << Value;
  }
  else
    printOperand(MI, OpNo, O);
}

template <unsigned bits>
void TriCoreInstPrinter::printZExtImm(const MCInst *MI, int OpNo,
                                       raw_ostream &O) {
  if (MI->getOperand(OpNo).isImm()) {
    unsigned int Value = MI->getOperand(OpNo).getImm();
    assert(Value <= ((unsigned int)pow(2,bits) -1 )  && "Invalid uimm argument!");
    O << (unsigned int)Value;
  }
  else
    printOperand(MI, OpNo, O);
}

// Print a 'bo' operand which is an addressing mode
// Base+Offset
void TriCoreInstPrinter::printAddrBO(const MCInst *MI, unsigned OpNum,
                                         raw_ostream &O) {

  const MCOperand &Base = MI->getOperand(OpNum);
  const MCOperand &Offset = MI->getOperand(OpNum+1);

  unsigned Opcode = MI->getOpcode();

  switch (Opcode) {
    default:
      // Print register base field
      if (Base.isReg())
          O << "[%" << StringRef(getRegisterName(Base.getReg())).lower() << "]";

      if (Offset.isExpr())
        Offset.getExpr()->print(O, &MAI);
      else {
        assert(Offset.isImm() && "Expected immediate in displacement field");
        O << " " << Offset.getImm();
      }
      break;
    }
}

// Print a 'preincbo' operand which is an addressing mode
// Pre-increment Base+Offset
void TriCoreInstPrinter::printAddrPreIncBO(const MCInst *MI, unsigned OpNum,
                                         raw_ostream &O) {

  const MCOperand &Base = MI->getOperand(OpNum);
  const MCOperand &Offset = MI->getOperand(OpNum+1);

  // Print register base field
  if (Base.isReg())
      O << "[+%" << StringRef(getRegisterName(Base.getReg())).lower() << "]";

  if (Offset.isExpr())
    Offset.getExpr()->print(O, &MAI);
  else {
    assert(Offset.isImm() && "Expected immediate in displacement field");
    O << " " << Offset.getImm();
  }
}

// Print a 'postincbo' operand which is an addressing mode
// Post-increment Base+Offset
void TriCoreInstPrinter::printAddrPostIncBO(const MCInst *MI, unsigned OpNum,
                                         raw_ostream &O) {

  const MCOperand &Base = MI->getOperand(OpNum);
  const MCOperand &Offset = MI->getOperand(OpNum+1);

  // Print register base field
  if (Base.isReg())
      O << "[%" << StringRef(getRegisterName(Base.getReg())).lower() << "+]";

  if (Offset.isExpr())
    Offset.getExpr()->print(O, &MAI);
  else {
    assert(Offset.isImm() && "Expected immediate in displacement field");
    O << " " << Offset.getImm();
  }
}

// Print a 'circbo' operand which is an addressing mode
// Circular Base+Offset
void TriCoreInstPrinter::printAddrCircBO(const MCInst *MI, unsigned OpNum,
                                         raw_ostream &O) {

  const MCOperand &Base = MI->getOperand(OpNum);
  const MCOperand &Offset = MI->getOperand(OpNum+1);

  // Print register base field
  if (Base.isReg()) {
      O << "[";
      printRegName(O, MRI.getSubReg(Base.getReg(), TriCore::subreg_even));
      O << "/";
      printRegName(O, MRI.getSubReg(Base.getReg(), TriCore::subreg_odd));
      O << "+c]";
  }
  if (Offset.isExpr())
    Offset.getExpr()->print(O, &MAI);
  else {
    assert(Offset.isImm() && "Expected immediate in displacement field");
    O << " " << Offset.getImm();
  }
}

// Print a 'bitrevbo' operand which is an addressing mode
// Bit-Reverse Base+Offset
void TriCoreInstPrinter::printAddrBitRevBO(const MCInst *MI, unsigned OpNum,
                                         raw_ostream &O) {

  const MCOperand &Base = MI->getOperand(OpNum);

  // Print register base field
  if (Base.isReg()) {
      O << "[";
      printRegName(O, MRI.getSubReg(Base.getReg(), TriCore::subreg_even));
      O << "/";
      printRegName(O, MRI.getSubReg(Base.getReg(), TriCore::subreg_odd));
      O << "+r]";
  }
}

void TriCoreInstPrinter::printOperand(const MCInst *MI, unsigned OpNo,
                                  raw_ostream &O) {

  const MCOperand &Op = MI->getOperand(OpNo);

  if (Op.isReg()) {
    printRegName(O, Op.getReg());
    return;
  }

  if (Op.isImm()) {
    O << Op.getImm();
    return;
  }

  assert(Op.isExpr() && "unknown operand kind in printOperand");
  printExpr(Op.getExpr(), &MAI, O);
}
