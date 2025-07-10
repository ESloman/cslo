/**
 * @file compiler.h
 */

#ifndef cslo_compiler_h
#define cslo_compiler_h

#include "parser/parser.h"
#include "core/object.h"
#include "scanner.h"
#include "core/vm.h"

/**
 * @struct Local
 */
typedef struct Local {
    Token name;
    int depth;
    bool isCaptured;
    bool isFinal;
} Local;

/**
 * @struct Upvalue
 */
typedef struct Upvalue {
    uint8_t index;
    bool isLocal;
    bool isFinal;
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
    int innermostLoopStart;
    int innermostLoopScopeDepth;
    int continueJumps[256];
    int continueCount;
    int breakJumps[256];
    int breakCount;
} Compiler;

/**
 * @struct ClassCompiler
 */
typedef struct ClassCompiler {
    struct ClassCompiler* enclosing;
    bool hasSuperClass;
} ClassCompiler;

extern Compiler* current;
extern ClassCompiler* currentClass;
extern Token lastVariableToken;

/**
 * Method for compiling slo code into bytecode.
 */
ObjFunction* compile(const char* source, const char* file);

Token syntheticToken(const char* text);
void namedVariable(Token name, bool canAssign);


/**
 * Method for reporting an error.
 */
void error(const char* message);

/**
 * Method for reporting an error at the current token.
 */
void errorAtCurrent(const char* message);

/**
 * Method for patching the previous jump instruction.
 */
void patchJump(int offset);

/**
 * Method for patching a previous jump instruction to somewhere specific.
 */
void patchJumpTo(int offset, int target);

/**
 * Method for compiling an argument list.
 */
uint8_t argumentList();

/**
 * Method for parsing a variable.
 */
uint8_t parseVariable(bool isFinal, const char* errorMessage);

/**
 * Method for marking a local variable initialised.
 */
void markInitialized();

/**
 * Method for defining a variale.
 */
void defineVariable(uint8_t global, bool isFinal);

/**
 * Method for resolving an upvalue variable.
 */
int resolveUpvalue(Compiler* compiler, Token* name);

/**
 * Method for adding a local.
 */
void addLocal(Token name, bool isFinal);

/**
 * Method for declaring a variable.
 */
void declareVariable(bool isFinal);

/**
 * Method for identifying a constant.
 */
uint8_t identifierConstant(Token* name);

/**
 * Method for comparing two identifiers.
 */
bool identifiersEqual(Token* a, Token* b);

/**
 * Method for resolving a local variable.
 */
int resolveLocal(Compiler* compiler, Token* name);

/**
 * Method for adding an upvalue to the current compiler.
 */
int addUpvalue(Compiler* compiler, uint8_t index, bool isLocal, bool isFinal);

/**
 * Method for beginning a block's scope.
 */
void beginScope();

/**
 * Method for ending a block's scope.
 */
void endScope();

/**
 * Method for compiling a variable declaration.
 */
void varDeclaration(bool isFinal);

/**
 * Method for compiling classes.
 */
void classDeclaration();

/**
 * Method for compiling functions.
 */
void funDeclaration();

/**
 * Method for compiling an enum declaration.
 */
void enumDeclaration();

/**
 * Method for synchronizing after an error.
 */
void synchronize();

#endif
