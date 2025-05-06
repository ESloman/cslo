/**
 * @file compiler.h
 */

#ifndef cslo_compiler_h
#define cslo_compiler_h

#include "object.h"
#include "scanner.h"
#include "vm.h"

/**
 * @struct Parser
 *
 * Parser struct keeps track of current and
 * previous tokens.
 */
typedef struct Parser {
    Token current;
    Token previous;
    bool hadError;
    bool panicMode;
} Parser;

/**
 * @enum Precedence
 */
typedef enum Precedence {
    PREC_NONE,
    PREC_ASSIGNMENT,  // =
    PREC_OR,          // or
    PREC_AND,         // and
    PREC_EQUALITY,    // == !=
    PREC_COMPARISON,  // < > <= >=
    PREC_TERM,        // + - ++ --
    PREC_FACTOR,      // * / % **
    PREC_UNARY,       // ! -
    PREC_CALL,        // . ()
    PREC_POSTFIX,     // ++, --
    PREC_PRIMARY
} Precedence;

/**
 *
 */
typedef void (*ParseFn)(bool canAssign);

/**
 * @struct ParseRule
 */
typedef struct ParseRule {
    ParseFn prefix;
    ParseFn infix;
    ParseFn postfix;
    Precedence precedence;
} ParseRule;

/**
 * @struct Local
 */
typedef struct Local {
    Token name;
    int depth;
    bool isCaptured;
} Local;

/**
 * @struct Upvalue
 */
typedef struct Upvalue {
    uint8_t index;
    bool isLocal;
} Upvalue;

/**
 * @enum FunctionType
 */
typedef enum FunctionType {
    TYPE_SCRIPT,
    TYPE_FUNCTION,
    TYPE_METHOD,
    TYPE_INITIALISER,
} FunctionType;

/**
 * @struct Compiler
 */
typedef struct Compiler {
    struct Compiler* enclosing;
    ObjFunction* function;
    FunctionType type;

    Local locals[UINT8_COUNT];
    int localCount;
    Upvalue upvalues[UINT8_COUNT];
    int scopeDepth;
} Compiler;

/**
 * @struct ClassCompiler
 */
typedef struct ClassCompiler {
    struct ClassCompiler* enclosing;
    bool hasSuperClass;
} ClassCompiler;

extern Compiler* current;

/**
 * Method for compiling slo code into bytecode.
 */
ObjFunction* compile(const char* source);

#endif
