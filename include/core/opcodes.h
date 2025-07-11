/**
 * @file opcodes.h
 * @brief Header file for opcodes.
 */

#ifndef cslo_opcodes_h
#define cslo_opcodes_h

/** @enum OpCode
*  This defines all of the valid OpCode for our program.
*
*/
typedef enum OpCode {
    OP_CONSTANT,
    OP_NIL,
    OP_TRUE,
    OP_FALSE,
    OP_POP,
    OP_DEFINE_GLOBAL,
    OP_DEFINE_FINAL_GLOBAL,
    OP_GET_GLOBAL,
    OP_SET_GLOBAL,
    OP_GET_LOCAL,
    OP_SET_LOCAL,
    OP_GET_UPVALUE,
    OP_SET_UPVALUE,
    OP_CLOSE_UPVALUE,
    OP_EQUAL,
    OP_NOT_EQUAL,
    OP_GREATER,
    OP_GREATER_EQUAL,
    OP_LESS,
    OP_LESS_EQUAL,
    OP_NEGATE,
    OP_ADD,
    OP_SUBTRACT,
    OP_MULTIPLY,
    OP_DIVIDE,
    OP_MODULO,
    OP_POW,
    OP_NOT,
    OP_JUMP,
    OP_JUMP_IF_FALSE,
    OP_JUMP_IF_TRUE,
    OP_LOOP,
    OP_CALL,
    OP_INVOKE,
    OP_SUPER_INVOKE,
    OP_CLOSURE,
    OP_RETURN,
    OP_CLASS,
    OP_METHOD,
    OP_INHERIT,
    OP_GET_SUPER,
    OP_SET_PROPERTY,
    OP_GET_PROPERTY,
    OP_DUP,
    OP_DUP2,
    OP_LIST,
    OP_GET_INDEX,
    OP_SET_INDEX,
    OP_SLICE,
    OP_HAS,
    OP_HAS_NOT,
    OP_LEN,
    OP_DICT,
    OP_ENUM,
    OP_IMPORT,
    OP_IMPORT_AS,
    OP_INTERPOLATE,
    OP_ASSERT,
} OpCode;

#endif
