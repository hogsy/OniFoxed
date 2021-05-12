/*
	FILE:	MS_Mac_PPCAsm.h
	
	AUTHOR:	Brent H. Pease
	
	CREATED: April 2, 1997
	
	PURPOSE: platform specific parts
	
	Copyright 1997

*/
#ifndef MS_MAC_PPCASM_H
#define MS_MAC_PPCASM_H

void MSrPPCAsm_BuildOp_LI(
	unsigned long	generateCode,
	unsigned long	**curInstrPtr,
	unsigned long	*procSize,
	unsigned long	gpr,
	unsigned long	value);

void MSrPPCAsm_BuildOp_L32(
	unsigned long	generateCode,
	unsigned long	**curInstrPtr,
	unsigned long	*procSize,
	unsigned long	gpr,
	unsigned long	value);

void MSrPPCAsm_BuildOp_MTCTR(
	unsigned long	generateCode,
	unsigned long	**curInstrPtr,
	unsigned long	*procSize,
	unsigned long	gpr);

void MSrPPCAsm_BuildOp_BDNZ(
	unsigned long	generateCode,
	unsigned long	**curInstrPtr,
	unsigned long	*procSize,
	unsigned long	*pc);

void MSrPPCAsm_BuildOp_BLR(
	unsigned long	generateCode,
	unsigned long	**curInstrPtr,
	unsigned long	*procSize);

void MSrPPCAsm_BuildOp_ADDI(
	unsigned long	generateCode,
	unsigned long	**curInstrPtr,
	unsigned long	*procSize,
	unsigned long	gprDest,
	unsigned long	gprA,
	unsigned long	value);

void MSrPPCAsm_BuildOp_STB(
	unsigned long	generateCode,
	unsigned long	**curInstrPtr,
	unsigned long	*procSize,
	unsigned long	gprS,
	unsigned long	gprAddr,
	unsigned long	disp);

void MSrPPCAsm_BuildOp_STH(
	unsigned long	generateCode,
	unsigned long	**curInstrPtr,
	unsigned long	*procSize,
	unsigned long	gprS,
	unsigned long	gprAddr,
	unsigned long	disp);

void MSrPPCAsm_BuildOp_STW(
	unsigned long	generateCode,
	unsigned long	**curInstrPtr,
	unsigned long	*procSize,
	unsigned long	gprS,
	unsigned long	gprAddr,
	unsigned long	disp);

void MSrPPCAsm_BuildOp_STFD(
	unsigned long	generateCode,
	unsigned long	**curInstrPtr,
	unsigned long	*procSize,
	unsigned long	fS,
	unsigned long	gprAddr,
	unsigned long	disp);

void MSrPPCAsm_BuildOp_LBZ(
	unsigned long	generateCode,
	unsigned long	**curInstrPtr,
	unsigned long	*procSize,
	unsigned long	gprD,
	unsigned long	gprAddr,
	unsigned long	disp);

void MSrPPCAsm_BuildOp_LHZ(
	unsigned long	generateCode,
	unsigned long	**curInstrPtr,
	unsigned long	*procSize,
	unsigned long	gprD,
	unsigned long	gprAddr,
	unsigned long	disp);

void MSrPPCAsm_BuildOp_LWZ(
	unsigned long	generateCode,
	unsigned long	**curInstrPtr,
	unsigned long	*procSize,
	unsigned long	gprD,
	unsigned long	gprAddr,
	unsigned long	disp);

void MSrPPCAsm_BuildOp_LFD(
	unsigned long	generateCode,
	unsigned long	**curInstrPtr,
	unsigned long	*procSize,
	unsigned long	fD,
	unsigned long	gprAddr,
	unsigned long	disp);

void MSrPPCAsm_BuildOp_DCBZ(
	unsigned long	generateCode,
	unsigned long	**curInstrPtr,
	unsigned long	*procSize,
	unsigned long	gprA,
	unsigned long	gprB);

void MSrPPCAsm_BuildOp_DCBT(
	unsigned long	generateCode,
	unsigned long	**curInstrPtr,
	unsigned long	*procSize,
	unsigned long	gprA,
	unsigned long	gprB);

void MSrPPCAsm_CallProc(
	register void					*proc,
	register MStDrawContextPrivate	*drawContext,
	register long					param1);

#endif
