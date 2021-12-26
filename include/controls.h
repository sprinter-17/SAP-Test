#ifndef CONTROLS_H
#define CONTROLS_H

#define TRANSFER 0b01000000
#define CALCULATION 0b10000000
#define NEXT 0b11000000

#define FROM_INPUT 0b00011000
#define TO_OUTPUT 0b00000111
#define TO_ADDRESS 0b00000110
#define FROM_STORE 0b00001000
#define TO_STORE 0b00000001
#define FROM_A 0b00000000
#define TO_A 0b00000000
#define FROM_B 0b00100000
#define TO_B 0b00000100
#define FROM_PC 0b00010000
#define TO_PC 0b00000011
#define FROM_PROG 0b00110000

#define TST_OP 0b00101000
#define NOT_OP 0b00101010
#define OR_OP 0b00001000
#define NOR_OP 0b00001010
#define AND_OP 0b00010000
#define NAND_OP 0b00010010
#define ROL_OP 0b00011000
#define SHL_OP 0b00011100
#define ROR_OP 0b00111000
#define SHR_OP 0b00111100
#define ADD_OP 0b00110000
#define SUB_OP 0b00110100
#define TO_FLAGS 0b00000001

#define ZERO_FLAG_CLR 0b00100000
#define ZERO_FLAG_SET 0b00100100
#define NEG_FLAG_CLR 0b00010000
#define NEG_FLAG_SET 0b00010010
#define CARRY_FLAG_CLR 0b00001000
#define CARRY_FLAG_SET 0b00001001

#endif