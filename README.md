# asm_trans

A two-pass x86 assembler translator for a subset of the x86 instruction set. Reads source code from `code.txt`, builds a symbol table, generates machine code, and produces a listing.

## Build

```bash
g++ -o asm_trans main.cpp
```

## Usage

```bash
./asm_trans
```

Input file: `code.txt`. An example input file is included in the repository. The following files are created on success:
- `listing.txt` — listing with addresses and machine code
- `object_code.txt` — object code in `H/T/E` format

## Program Structure

The source file must follow this structure:

```asm
NAME SEGMENT
    ORG 100H         ; or ORG 0
    ; instructions
    INT 21H
    VAR DB value     ; variable declarations
    VAR DW value
NAME ENDS
END
```

## Supported Instructions

| Instruction      | Description                       |
|------------------|-----------------------------------|
| `MOV dst, src`   | Move data                         |
| `XCHG op1, op2`  | Exchange operands                 |
| `DEC op`         | Decrement                         |
| `LOOP label`     | Loop while CX ≠ 0                 |

## Operand Formats

**16-bit registers:** `AX CX DX BX SP BP SI DI`  
**8-bit registers:** `AL CL DL BL AH CH DH BH`

**Immediate values:**

| Format          | Example   |
|-----------------|-----------|
| Decimal         | `100`     |
| Hexadecimal     | `0FFH`    |
| Binary          | `101B`    |
| Character       | `'A'`     |
| String (≤2 B)   | `"AB"`    |
| Negative        | `-10`     |

**Index addressing:** `[SI]`, `[DI+offset]`, `[SI-offset]`

## Object Code Format

```
H <segment_name> <size_hex>
T <address_hex> <length_hex> <bytes_hex>
E <start_address_hex>
```

## Error Codes

| Code | Description                                                               |
|-----:|---------------------------------------------------------------------------|
|    0 | Invalid label name                                                        |
|    1 | Incompatible operand sizes                                                |
|    2 | First operand cannot be an immediate value                                |
|    3 | Invalid number format                                                     |
|    4 | Number exceeds the allowed bit width                                      |
|    5 | Invalid operation format (both operands are memory)                       |
|    6 | Missing second operand                                                    |
|    7 | Missing comma in binary instruction                                       |
|    8 | Missing first operand                                                     |
|    9 | Second operand cannot be an immediate value                               |
|   10 | Invalid index register                                                    |
|   11 | Missing closing bracket in index expression                               |
|   12 | Missing displacement operand                                              |
|   13 | Undeclared variable                                                       |
|   14 | Index displacement size does not match the other operand                  |
|   15 | Unary instruction has two operands                                        |
|   16 | LOOP operand must be a label                                              |
|   17 | Name already in use                                                       |
|   18 | Name conflicts with an instruction mnemonic                               |
|   19 | Name conflicts with a register name                                       |
|   20 | More than one directive on a line                                         |
|   21 | Invalid directive context                                                 |
|   22 | ENDS directive must be followed by END                                    |
|   23 | Invalid segment name                                                      |
|   24 | INT 21H must be followed by DB, DW, or ENDS                               |
|   25 | Interrupt instruction only accepts the value 21H                          |
|   26 | ORG block must end with INT 21H                                           |
|   27 | ORG expression must be 0 or 256                                           |
|   28 | ORG directive is required                                                 |
|   29 | Code must begin with a SEGMENT directive                                  |
|   30 | Unrecognized instruction                                                  |
|   31 | Program structure does not meet requirements                              |
|   32 | Memory allocation error                                                   |
