#include "controls.h"
#include <Arduino.h>

#define RESET 2
#define MAIN_CLOCK 3
#define SHIFT_CLEAR 4
#define SHIFT_DATA 5
#define COMMAND_CLOCK 6
#define BUS_OUT_CLOCK 7
#define BUS_IN_CLOCK 8
#define BUS_IN_LOAD 9
#define BUS_OUT_ENABLE 10
#define PROG_MEM 11

#define ZERO_MICRO_COUNTER A5
#define ZERO_FLAG A4
#define NEG_FLAG A3
#define CARRY_FLAG A2

#define CLOCK_DELAY 7

#define PC_SAMPLE 100
#define STORE_SAMPLE 100
#define REGISTER_SAMPLE 100
#define ALU_SAMPLE 20

uint8_t count;
uint8_t operand;

char text[100];

void pulseHigh(uint8_t pin) {
    digitalWrite(pin, LOW);
    digitalWrite(pin, HIGH);
    digitalWrite(pin, LOW);
}

void pulseLow(uint8_t pin) {
    digitalWrite(pin, HIGH);
    digitalWrite(pin, LOW);
    digitalWrite(pin, HIGH);
}

void writeCommand(uint8_t command) {
    pinMode(SHIFT_DATA, OUTPUT);
    uint8_t i;
    for (i = 0; i < 8; i++) {
        digitalWrite(SHIFT_DATA, command & 1);
        command >>= 1;
        pulseHigh(COMMAND_CLOCK);
    }
}

void writeBus(uint8_t value) {
    pinMode(SHIFT_DATA, OUTPUT);
    digitalWrite(BUS_OUT_ENABLE, HIGH);
    uint8_t i;
    for (i = 0; i < 8; i++) {
        digitalWrite(SHIFT_DATA, value & 1);
        value >>= 1;
        pulseHigh(BUS_OUT_CLOCK);
    }
    pulseHigh(BUS_OUT_CLOCK);
}

uint8_t readBus( ) {
    pinMode(SHIFT_DATA, INPUT_PULLUP);
    digitalWrite(BUS_OUT_ENABLE, HIGH);
    pulseLow(BUS_IN_LOAD);
    uint8_t result = 0;
    uint8_t i;
    for (i = 0; i < 8; i++) {
        result |= digitalRead(SHIFT_DATA) << i;
        pulseHigh(BUS_IN_CLOCK);
    }
    return result;
}

void sendCommand(uint8_t command) {
    pulseLow(SHIFT_CLEAR);
    writeCommand(command);
    digitalWrite(BUS_OUT_ENABLE, HIGH);
    pulseHigh(MAIN_CLOCK);
    delay(CLOCK_DELAY);
}

void sendCommandWithValue(uint8_t command, uint8_t value) {
    pulseLow(SHIFT_CLEAR);
    writeCommand(command);
    writeBus(value);
    digitalWrite(BUS_OUT_ENABLE, LOW);
    pulseHigh(MAIN_CLOCK);
    delay(CLOCK_DELAY);
}

uint8_t getValue(uint8_t command) {
    pulseLow(SHIFT_CLEAR);
    writeCommand(command);
    delay(CLOCK_DELAY);
    return readBus( );
}

bool testRAM( ) {
    bool passed = true;
    Serial.println("Testing RAM");
    uint8_t values[STORE_SAMPLE];
    for (int i = 0; i < STORE_SAMPLE; i++) {
        values[i] = random(256);
        sendCommandWithValue(TRANSFER | FROM_INPUT | TO_ADDRESS, i);
        sendCommandWithValue(TRANSFER | FROM_INPUT | TO_STORE, values[i]);
    }
    for (int i = 0; i < STORE_SAMPLE; i++) {
        sendCommandWithValue(TRANSFER | FROM_INPUT | TO_ADDRESS, i);
        uint8_t value = getValue(TRANSFER | FROM_STORE | TO_OUTPUT);
        if (value != values[i]) {
            passed = false;
            sprintf(text, "**** RAM failed: [%d] was %d expecting %d", i, value, values[i]);
            Serial.println(text);
        }
    }
    if (passed) {
        Serial.println("RAM passed");
    }
    return passed;
}

bool testROM( ) {
    bool passed = true;
    Serial.println("Testing ROM");
    uint8_t values[STORE_SAMPLE];
    digitalWrite(PROG_MEM, LOW);
    for (int i = 0; i < STORE_SAMPLE; i++) {
        values[i] = random(256);
        sendCommandWithValue(TRANSFER | FROM_INPUT | TO_ADDRESS, i);
        sendCommandWithValue(TRANSFER | FROM_INPUT | TO_STORE, values[i]);
    }
    digitalWrite(PROG_MEM, HIGH);
    for (int i = 0; i < STORE_SAMPLE; i++) {
        sendCommandWithValue(TRANSFER | FROM_INPUT | TO_ADDRESS, i);
        uint8_t value = getValue(TRANSFER | FROM_PROG | TO_OUTPUT);
        if (value != values[i]) {
            passed = false;
            sprintf(text, "**** ROM failed: [%d] was %d expecting %d", i, value, values[i]);
            Serial.println(text);
        }
    }
    if (passed) {
        Serial.println("ROM passed");
    }
    return passed;
}

bool testRegisters( ) {
    bool passed = true;
    Serial.println("Testing Registers");
    for (int i = 0; i < REGISTER_SAMPLE; i++) {
        uint8_t value = random(256);
        sendCommandWithValue(TRANSFER | FROM_INPUT | TO_A, value);
        uint8_t actual = getValue(TRANSFER | FROM_A | TO_OUTPUT);
        if (actual != value) {
            passed = false;
            sprintf(text, "**** Register A failed: was %d expecting %d", actual, value);
            Serial.println(text);
        }
    }
    for (int i = 0; i < REGISTER_SAMPLE; i++) {
        uint8_t value = random(256);
        sendCommandWithValue(TRANSFER | FROM_INPUT | TO_B, value);
        uint8_t actual = getValue(TRANSFER | FROM_B | TO_OUTPUT);
        if (actual != value) {
            passed = false;
            sprintf(text, "**** Register B failed: was %d expecting %d", actual, value);
            Serial.println(text);
        }
    }
    if (passed) {
        Serial.println("Registers passed");
    }
    return passed;
}

bool testUnaryOperation(uint8_t op1, uint8_t expected, uint8_t OP_CODE, char const *desc, char const *symbol) {
    sendCommandWithValue(TRANSFER | FROM_INPUT | TO_A, op1);
    uint8_t actual = getValue(CALCULATION | OP_CODE);
    if (actual != expected) {
        sprintf(text, "**** %s operation failed: %s %d was %d expecting %d", desc, symbol, op1, actual, expected);
        Serial.println(text);
        return false;
    } else {
        return true;
    }
}

bool testBinaryOperation(uint8_t op1, uint8_t op2, uint8_t expected, uint8_t OP_CODE, char const *desc, char const *symbol) {
    sendCommandWithValue(TRANSFER | FROM_INPUT | TO_A, op1);
    sendCommandWithValue(TRANSFER | FROM_INPUT | TO_B, op2);
    uint8_t actual = getValue(CALCULATION | OP_CODE);
    if (actual != expected) {
        sprintf(text, "**** %s operation failed: %d %s %d was %d expecting %d", desc, op1, symbol, op2, actual, expected);
        Serial.println(text);
        return false;
    } else {
        return true;
    }
}

bool testALU( ) {
    bool passed = true;
    Serial.println("Testing ALU");
    for (int i = 0; i < ALU_SAMPLE; i++) {
        uint8_t op1 = random(256);
        uint8_t op2 = random(256);
        passed &= testUnaryOperation(op1, op1, TST_OP, "Test", "?");
        passed &= testUnaryOperation(op1, ~op1, NOT_OP, "Not", "~");
        passed &= testBinaryOperation(op1, op2, op1 | op2, OR_OP, "Or", "|");
        passed &= testBinaryOperation(op1, op2, ~(op1 | op2), NOR_OP, "Nor", "~|");
        passed &= testBinaryOperation(op1, op2, op1 & op2, AND_OP, "And", "&");
        passed &= testBinaryOperation(op1, op2, ~(op1 & op2), NAND_OP, "Nand", "~&");
        passed &= testUnaryOperation(op1, op1 << 1 | op1 >> 7, ROL_OP, "RoL", "<<");
        passed &= testUnaryOperation(op1, op1 << 1, SHL_OP, "ShL", "|<<");
        passed &= testUnaryOperation(op1, op1 >> 1 | op1 << 7, ROR_OP, "RoR", ">>");
        passed &= testUnaryOperation(op1, op1 >> 1, SHR_OP, "ShR", ">>|");
        passed &= testBinaryOperation(op1, op2, op1 + op2, ADD_OP, "Add", "+");
        passed &= testBinaryOperation(op1, op2, op1 - op2, SUB_OP, "Sub", "-");
    }
    if (passed) {
        Serial.println("ALU passed");
    }
    return passed;
}

bool testProgramCounter( ) {
    bool passed = true;
    Serial.println("Testing PC");
    for (int i = 0; i < PC_SAMPLE; i++) {
        sendCommand(TRANSFER | FROM_PC | TO_A);
        uint8_t actual = getValue(TRANSFER | FROM_PC | TO_OUTPUT);
        if (actual != i) {
            passed = false;
            sprintf(text, "**** PC failed: was %d expecting %d", actual, i);
            Serial.println(text);
        }
    }
    if (passed) {
        Serial.println("PC passed");
    }
    return passed;
}

void resetMicroCounter( ) {
    while (digitalRead(ZERO_MICRO_COUNTER) == LOW) {
        pulseHigh(MAIN_CLOCK);
    }
}

bool testMicroCounter( ) {
    bool passed = true;
    Serial.println("Testing micro counter");
    resetMicroCounter( );
    int counter = 1;
    pulseHigh(MAIN_CLOCK);
    while (digitalRead(ZERO_MICRO_COUNTER) == LOW) {
        pulseHigh(MAIN_CLOCK);
        counter++;
    }
    int expected = 8;
    if (counter != expected) {
        passed = false;
        sprintf(text, "**** Micro counter failed: was %d cycles expecting %d", counter, expected);
        Serial.println(text);
    }
    if (passed) {
        Serial.println("Micro counter passed");
    }
    return passed;
}

bool testNext( ) {
    bool passed = true;
    Serial.println("Testing next");
    resetMicroCounter( );
    pulseHigh(MAIN_CLOCK);
    sendCommand(NEXT);
    if (digitalRead(ZERO_MICRO_COUNTER) == LOW) {
        passed = false;
        Serial.println("**** Next failed: micro counter not reset");
    }
    if (passed) {
        Serial.println("Next passed");
    }
    return passed;
}

bool testFlagValue(uint8_t flag, bool expected, char const * desc) {
    if (expected && digitalRead(flag) == LOW) {
        sprintf(text, "**** %s failed: was L expecting H", desc);
        Serial.println(text);
        return false;
    } else if (!expected && digitalRead(flag) == HIGH) {
        sprintf(text, "**** %s failed: was H expecting L", desc);
        Serial.println(text);
        return false;
    } else {
        return true;
    }
}

bool testFlags( ) {
    bool passed = true;
    Serial.println("Testing flags");

    sendCommandWithValue(TRANSFER | FROM_INPUT | TO_A, 1);
    sendCommand(CALCULATION | TST_OP | TO_FLAGS);

    passed &= testFlagValue(ZERO_FLAG, false, "zero flag for 1");
    passed &= testFlagValue(NEG_FLAG, false, "neg flag for 1");

    sendCommandWithValue(TRANSFER | FROM_INPUT | TO_A, 0);
    sendCommand(CALCULATION | TST_OP | TO_FLAGS);

    passed &= testFlagValue(ZERO_FLAG, true, "zero flag for 0");
    passed &= testFlagValue(NEG_FLAG, false, "neg flag for 1");

    sendCommandWithValue(TRANSFER | FROM_INPUT | TO_A, -1);
    sendCommand(CALCULATION | TST_OP | TO_FLAGS);

    passed &= testFlagValue(ZERO_FLAG, false, "zero flag for -1");
    passed &= testFlagValue(NEG_FLAG, true, "neg flag for -1");

    sendCommandWithValue(TRANSFER | FROM_INPUT | TO_A, 127);
    sendCommandWithValue(TRANSFER | FROM_INPUT | TO_B, 127);
    sendCommand(CALCULATION | ADD_OP | TO_FLAGS);

    passed &= testFlagValue(CARRY_FLAG, false, "carry flag for 127+127");

    sendCommandWithValue(TRANSFER | FROM_INPUT | TO_A, 128);
    sendCommandWithValue(TRANSFER | FROM_INPUT | TO_B, 127);
    sendCommand(CALCULATION | ADD_OP | TO_FLAGS);

    passed &= testFlagValue(CARRY_FLAG, false, "carry flag for 128+127");

    sendCommandWithValue(TRANSFER | FROM_INPUT | TO_A, 128);
    sendCommandWithValue(TRANSFER | FROM_INPUT | TO_B, 128);
    sendCommand(CALCULATION | ADD_OP | TO_FLAGS);

    passed &= testFlagValue(CARRY_FLAG, true, "carry flag for 128+128");

    if (passed) {
        Serial.println("Flags passed");
    }
    return passed;
}

bool testBranchFlag(uint8_t FLAG_OPTION, bool expectedReset, char const *desc) {
    resetMicroCounter( );
    pulseHigh(MAIN_CLOCK);
    sendCommand(NEXT | FLAG_OPTION);
    if (expectedReset && digitalRead(ZERO_MICRO_COUNTER) == LOW) {
        sprintf(text, "*** Branching on %s failed: expected micro counter reset", desc);
        Serial.println(text);
        return false;
    } else if (!expectedReset && digitalRead(ZERO_MICRO_COUNTER) == HIGH) {
        sprintf(text, "*** Branching on %s failed: expected no micro counter reset", desc);
        Serial.println(text);
        return false;
    } else {
        return true;
    }
}

bool testBranch( ) {
    bool passed = true;
    Serial.println("Testing branching");

    sendCommandWithValue(TRANSFER | FROM_INPUT | TO_A, 1);
    sendCommand(CALCULATION | TST_OP | TO_FLAGS);

    passed &= testBranchFlag(ZERO_FLAG_CLR, true, "zero flag cleared for 1");
    passed &= testBranchFlag(ZERO_FLAG_SET, false, "zero flag set for 1");
    passed &= testBranchFlag(NEG_FLAG_CLR, true, "negative flag cleared for 1");
    passed &= testBranchFlag(NEG_FLAG_SET, false, "negative flag set for 1");

    sendCommandWithValue(TRANSFER | FROM_INPUT | TO_A, 0);
    sendCommand(CALCULATION | TST_OP | TO_FLAGS);

    passed &= testBranchFlag(ZERO_FLAG_CLR, false, "zero flag cleared for 0");
    passed &= testBranchFlag(ZERO_FLAG_SET, true, "zero flag set for 0");
    passed &= testBranchFlag(NEG_FLAG_CLR, true, "negative flag cleared for 0");
    passed &= testBranchFlag(NEG_FLAG_SET, false, "negative flag set for 0");

    sendCommandWithValue(TRANSFER | FROM_INPUT | TO_A, -1);
    sendCommand(CALCULATION | TST_OP | TO_FLAGS);

    passed &= testBranchFlag(ZERO_FLAG_CLR, true, "zero flag cleared for -1");
    passed &= testBranchFlag(ZERO_FLAG_SET, false, "zero flag set for -1");
    passed &= testBranchFlag(NEG_FLAG_CLR, false, "negative flag cleared for -1");
    passed &= testBranchFlag(NEG_FLAG_SET, true, "negative flag set for -1");

    sendCommandWithValue(TRANSFER | FROM_INPUT | TO_A, 127);
    sendCommandWithValue(TRANSFER | FROM_INPUT | TO_B, 127);
    sendCommand(CALCULATION | ADD_OP | TO_FLAGS);

    passed &= testBranchFlag(CARRY_FLAG_CLR, true, "carry flag cleared for 127+127");
    passed &= testBranchFlag(CARRY_FLAG_SET, false, "carry flag set for 127+127");

    sendCommandWithValue(TRANSFER | FROM_INPUT | TO_A, 128);
    sendCommandWithValue(TRANSFER | FROM_INPUT | TO_B, 128);
    sendCommand(CALCULATION | ADD_OP | TO_FLAGS);

    passed &= testBranchFlag(CARRY_FLAG_CLR, false, "carry flag cleared for 128+128");
    passed &= testBranchFlag(CARRY_FLAG_SET, true, "carry flag set for 128+128");

    if (passed) {
        Serial.println("Branching passed");
    }
    return passed;
}

void setup( ) {
    pinMode(RESET, OUTPUT);
    pinMode(MAIN_CLOCK, OUTPUT);
    pinMode(COMMAND_CLOCK, OUTPUT);
    pinMode(BUS_OUT_CLOCK, OUTPUT);
    pinMode(BUS_IN_CLOCK, OUTPUT);
    pinMode(SHIFT_CLEAR, OUTPUT);
    pinMode(BUS_IN_LOAD, OUTPUT);
    pinMode(BUS_OUT_ENABLE, OUTPUT);
    pinMode(ZERO_MICRO_COUNTER, INPUT);
    pinMode(ZERO_FLAG, INPUT_PULLUP);
    pinMode(NEG_FLAG, INPUT_PULLUP);
    pinMode(CARRY_FLAG, INPUT_PULLUP);
    pinMode(PROG_MEM, OUTPUT);

    digitalWrite(SHIFT_CLEAR, HIGH);
    digitalWrite(BUS_IN_LOAD, HIGH);
    digitalWrite(PROG_MEM, HIGH);

    Serial.begin(9600);

    pulseHigh(RESET);
    randomSeed(0);
    bool passed = true;
    passed &= testMicroCounter( );
    passed &= testNext( );
    passed &= testProgramCounter( );
    passed &= testRAM( );
    passed &= testROM( );
    passed &= testRegisters( );
    passed &= testALU( );
    passed &= testFlags( );
    passed &= testBranch( );
    if (passed) {
        Serial.println("All tests passed");
    }
}

void loop( ) {}
