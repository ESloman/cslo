/**
 * @file parser.c
 * This file contains the parser for the CSLO language.
 */

#include "stdio.h"
#include "stdlib.h"
#include <string.h>

#include "compiler/codegen.h"
#include "compiler/scanner.h"
#include "compiler/parser.h"
#include "core/chunk.h"
#include "parser/statements.h"
#include "parser/rules.h"

/**
 * Method for initialising the parser.
 */
void initParser() {
    parser.hadError = false;
    parser.panicMode = false;
    parser.previous = (Token){0};
    parser.current = (Token){0};
    // Initialize lookahead buffer if you add one
    for (int i = 0; i < MAX_LOOKAHEAD; i++) {
        parser.lookahead[i] = scanToken();
    }
}

/**
 * Method for advancing the parser's token.
 *
 * Sets the previous token to the current and retrieves the next
 * token. Will report any errors if it encounters an error token.
 */
void parserAdvance() {
    parser.previous = parser.current;
    parser.current = parser.lookahead[0];

    // Shift lookahead tokens left and fill the last slot
    for (int i = 0; i < MAX_LOOKAHEAD - 1; i++) {
        parser.lookahead[i] = parser.lookahead[i + 1];
    }

    for (;;) {
        parser.lookahead[MAX_LOOKAHEAD - 1] = scanToken();
        if (parser.lookahead[MAX_LOOKAHEAD - 1].type != TOKEN_ERROR) {
            break;
        }

        errorAtCurrent(parser.lookahead[MAX_LOOKAHEAD - 1].start);
    }
}

/**
 * Method for parsing an expression with a given precedence.
 *
 * Will parse the expression according to the precedence level
 * and advance the parser's token.
 */
void parsePrecedence(Precedence precedence) {
    parserAdvance();
    ParseFn prefixRule = getRule(parser.previous.type)->prefix;
    if (prefixRule == NULL) {
        error("Expect expression.");
        return;
    }

    bool canAssign = precedence <= PREC_ASSIGNMENT;
    prefixRule(canAssign);

    while (precedence <= getRule(parser.current.type)->precedence) {
        parserAdvance();
        ParseFn infixRule = getRule(parser.previous.type)->infix;
        infixRule(canAssign);
    }

    if (canAssign && matchToken(TOKEN_EQUAL)) {
        error("Invalid assignment target.");
    }
}

/**
 * Method for returning the rules for a given token.
 *
 * Will return the ParseRule for the given token type.
 */
ParseRule* getRule(TokenType type) {
    return &rules[type];
}


/**
 * Method for peeking at the nth token without consuming it.
 */
Token peekToken(int n) {
    if (n == 0) {
        return parser.current;
    }
    if (n == -1 ) {
        return parser.previous;
    }
    if (n < -1 || n > MAX_LOOKAHEAD) {
        // Return an error token or handle as needed
        Token error;
        error.type = TOKEN_ERROR;
        error.start = "Lookahead out of range";
        error.length = strlen(error.start);
        error.line = parser.current.line;
        return error;
    }
    return parser.lookahead[n - 1];
}

/**
 * Method for consuming a given token type.
 *
 * Will advance over the current token if it matches,
 * otherwise will throw an error with the given message.
 */
 void consumeToken(TokenType type, const char* message) {
    if (parser.current.type == type) {
        parserAdvance();
        return;
    }

    errorAtCurrent(message);
}

/**
 * Method for checking if the current token type type.
 */
bool checkToken(TokenType type) {
    return parser.current.type == type;
}

/**
 * Method for matching and consuming the given token type.
 */
bool matchToken(TokenType type) {
    if (!checkToken(type)) {
        return false;
    }
    parserAdvance();
    return true;
}

// Methods for actually parsing various expressions

/**
 * Method for compiling an expression.
 */
void parseExpression() {
    parsePrecedence(PREC_ASSIGNMENT);
}

/**
 * Method for compiling a declaration.
 */
void parseDeclaration() {
    if (matchToken(TOKEN_IMPORT)) {
        parseImportStatement();
    }
    else if (matchToken(TOKEN_CLASS)) {
        classDeclaration();
    } else if (matchToken(TOKEN_FUN)) {
        funDeclaration();
    } else if (matchToken(TOKEN_VAR)) {
        varDeclaration();
    } else if (matchToken(TOKEN_ENUM)) {
        enumDeclaration();
    } else {
        parseStatement();
    }

    if (parser.panicMode) {
        synchronize();
    }
}

/**
 * Method for compiling blocks.
 */
void parseBlock() {
    while (!checkToken(TOKEN_RIGHT_BRACE) && !checkToken(TOKEN_EOF)) {
        parseDeclaration();
    }

    consumeToken(TOKEN_RIGHT_BRACE, "Expect '}' after block.");
}

/**
 * @brief Compiles a grouping expression (parentheses).
 *
 * Parses the inner expression and expects a closing parenthesis.
 *
 * @param canAssign Indicates if assignment is allowed (unused).
 */
void parseGrouping(bool canAssign) {
    parseExpression();
    consumeToken(TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
}

/**
 * @brief Compiles a numeric literal.
 *
 * Emits bytecode to load the number onto the stack.
 *
 * @param canAssign Indicates if assignment is allowed (unused).
 */
void parseNumber(bool canAssign) {
    double value = strtod(parser.previous.start, NULL);
    emitConstant(NUMBER_VAL(value));
}

/**
 * @brief Compiles a string literal.
 *
 * Emits bytecode to load the string onto the stack.
 *
 * @param canAssign Indicates if assignment is allowed (unused).
 */
void parseString(bool canAssign) {
    emitConstant(OBJ_VAL(copyString(parser.previous.start +1, parser.previous.length - 2)));
}

/**
 * Method for compiling a variable.
 */
void variable(bool canAssign) {
    lastVariableToken = parser.previous;
    namedVariable(parser.previous, canAssign);
}

/**
 * @brief Compiles a postfix increment or decrement expression (x++ or x--).
 *
 * This function assumes the variable's value is already on the stack.
 * It duplicates the value (so the original is left as the result of the expression),
 * increments or decrements the value, stores the new value back into the variable,
 * and then pops the new value, leaving the original value on the stack.
 *
 * @param canAssign Indicates if assignment is allowed (unused for postfix).
 */
void postfixIncDec(bool canAssign) {
    TokenType op = parser.previous.type;
    // The variable is already on the stack (from the prefix rule for identifiers)
    // Duplicate the value for the result
    emitByte(OP_DUP, parser.previous.line);
    emitConstant(NUMBER_VAL(1));
    emitByte(op == TOKEN_PLUS_PLUS ? OP_ADD : OP_SUBTRACT, parser.previous.line);


    uint8_t setOp;
    int arg = resolveLocal(current, &lastVariableToken);

    if (arg != -1) {
        setOp = OP_SET_LOCAL;
    } else if ((arg = resolveUpvalue(current, &lastVariableToken)) != -1) {
        setOp = OP_SET_UPVALUE;
    } else {
        arg = identifierConstant(&lastVariableToken);
        setOp = OP_SET_GLOBAL;
    }

    emitBytes(setOp, (uint8_t)arg);
    emitByte(OP_POP, parser.previous.line);
}

/**
 * @brief Compiles a prefix increment or decrement expression (++x or --x).
 *
 * Advances to the variable token, loads its value, increments or decrements it,
 * stores the new value back into the variable, and leaves the new value on the stack
 * as the result of the expression.
 *
 * @param canAssign Indicates if assignment is allowed (unused for prefix).
 */
void prefixIncDec(bool canAssign) {
    TokenType op = parser.previous.type;
    parserAdvance();
    variable(false);
    emitConstant(NUMBER_VAL(1));
    emitByte(op == TOKEN_PLUS_PLUS ? OP_ADD : OP_SUBTRACT, parser.previous.line);
    uint8_t setOp;
    int arg = resolveLocal(current, &lastVariableToken);

    if (arg != -1) {
        setOp = OP_SET_LOCAL;
    } else if ((arg = resolveUpvalue(current, &lastVariableToken)) != -1) {
        setOp = OP_SET_UPVALUE;
    } else {
        arg = identifierConstant(&lastVariableToken);
        setOp = OP_SET_GLOBAL;
    }
    emitBytes(setOp, (uint8_t)arg);
}

/**
 * @brief Compiles a compound assignment expression (e.g., x += y, x -= y, x *= y, x /= y).
 *
 * This function assumes the left-hand side is a variable and is already on the stack.
 * It parses the right-hand side expression, emits the appropriate bytecode for the operation,
 * and stores the result back into the variable.
 *
 * @param canAssign Indicates if assignment is allowed (unused).
 */
void compoundAssign(bool canAssign) {
    if (lastVariableToken.start == NULL || lastVariableToken.length == 0) {
        error("Invalid assignment target for compound assignment.");
        return;
    }

    TokenType op = parser.previous.type;
    // The left-hand side (variable) is already parsed and on the stack
    // Save the variable token for assignment
    Token varToken = lastVariableToken;

    // Parse the right-hand side expression
    parseExpression();

    // Perform the operation
    if (op == TOKEN_PLUS_EQUAL) {
        emitByte(OP_ADD, parser.previous.line);
    } else if (op == TOKEN_MINUS_EQUAL) {
        emitByte(OP_SUBTRACT, parser.previous.line);
    } else if (op == TOKEN_STAR_EQUAL) {
        emitByte(OP_MULTIPLY, parser.previous.line);
    } else if (op == TOKEN_SLASH_EQUAL) {
        emitByte(OP_DIVIDE, parser.previous.line);
    }

    // Assign the result back to the variable
    uint8_t setOp;
    int arg = resolveLocal(current, &varToken);
    if (arg != -1) {
        setOp = OP_SET_LOCAL;
    } else if ((arg = resolveUpvalue(current, &varToken)) != -1) {
        setOp = OP_SET_UPVALUE;
    } else {
        arg = identifierConstant(&varToken);
        setOp = OP_SET_GLOBAL;
    }
    emitBytes(setOp, (uint8_t)arg);
}

/**
 * Method for compiling a list.
 */
void list(bool canAssign) {
    int32_t argCount = 0;

    if (!checkToken(TOKEN_RIGHT_BRACKET)) {
        do {
            parseExpression();
            if (argCount >= INT32_MAX) {
                error("Can't have more elements in a list that 65k.");
            }
            argCount++;
        } while(matchToken(TOKEN_COMMA));
    }

    consumeToken(TOKEN_RIGHT_BRACKET, "Expect ']' after list literal.");
    emitByte(OP_LIST, parser.previous.line);
    emitByte((argCount >> 8) & 0xff, parser.previous.line);
    emitByte(argCount & 0xff, parser.previous.line);
}

/**
 * Method for compiling list indexing.
 */
void index(bool canAssign) {
    if (checkToken(TOKEN_COLON)) {
        emitByte(OP_NIL, parser.previous.line); // default to nil if no start index
    } else {
        parseExpression(); // parse the index value
    }
    if (matchToken(TOKEN_COLON)) {
        // handle slicers
        if (!checkToken(TOKEN_RIGHT_BRACKET)) {
            parseExpression(); // parse the end index
        } else {
            emitByte(OP_NIL, parser.previous.line); // default to nil if no end index
        }
        consumeToken(TOKEN_RIGHT_BRACKET, "Expect ']' after index.");
        emitByte(OP_SLICE, parser.previous.line);
        return;
    }

    // handle standard indexing
    consumeToken(TOKEN_RIGHT_BRACKET, "Expect ']' after index.");
    if (matchToken(TOKEN_PLUS_EQUAL) || matchToken(TOKEN_MINUS_EQUAL) ||
        matchToken(TOKEN_STAR_EQUAL) || matchToken(TOKEN_SLASH_EQUAL)) {

            TokenType op = parser.previous.type;

            emitByte(OP_DUP2, parser.previous.line);
            emitByte(OP_GET_INDEX, parser.previous.line);

            // If we have a compound assignment, we need to compile the right-hand side
            parseExpression();
            switch (op) {
                case TOKEN_PLUS_EQUAL:    emitByte(OP_ADD, parser.previous.line); break;
                case TOKEN_MINUS_EQUAL:   emitByte(OP_SUBTRACT, parser.previous.line); break;
                case TOKEN_STAR_EQUAL:    emitByte(OP_MULTIPLY, parser.previous.line); break;
                case TOKEN_SLASH_EQUAL:   emitByte(OP_DIVIDE, parser.previous.line); break;
                default: break;
            }
            emitByte(OP_SET_INDEX, parser.previous.line);
    } else if (canAssign && matchToken(TOKEN_EQUAL)) {
        parseExpression();
        emitByte(OP_SET_INDEX, parser.previous.line);
    } else {
        emitByte(OP_GET_INDEX, parser.previous.line);
    }
}

/**
 * Method for compiling AND statements.
 * Effectively, works like control flow.
 * By this point, the left-hand side has been compiled
 * already so if it evaluates to 'False', we want to skip
 * the right-hand statement.
 */
void and_(bool canAssign) {
    int endJump = emitJump(OP_JUMP_IF_FALSE);

    emitByte(OP_POP, parser.previous.line);
    parsePrecedence(PREC_AND);

    patchJump(endJump);
}

/**
 * Method for compiling OR statements.
 * Effectively, works like control flow.
 * By this point, the left-hand side has been compiled
 * already so if it evaluates to 'True', we want to skip
 * the right-hand statement.
 */
void or_(bool canAssign) {
    int elseJump = emitJump(OP_JUMP_IF_FALSE);
    int endJump = emitJump(OP_JUMP);

    patchJump(elseJump);
    emitByte(OP_POP, parser.previous.line);

    parsePrecedence(PREC_OR);
    patchJump(endJump);
}

/**
 * Method for compiling has statements.
 */
void has(bool canAssign) {
    parseExpression();
    emitByte(OP_HAS, parser.previous.line);
}

/**
 * Method for compiling has statements.
 */
void hasNot(bool canAssign) {
    parseExpression();
    emitByte(OP_HAS_NOT, parser.previous.line);
}

/**
 *
 */
void binary(bool canAssign) {
    TokenType operatorType = parser.previous.type;

    int tLine = parser.previous.line;
    ParseRule* rule = getRule(operatorType);
    parsePrecedence((Precedence)(rule->precedence + 1));

    switch (operatorType) {
        case TOKEN_BANG_EQUAL:
            emitByte(OP_NOT_EQUAL, tLine);
            break;
        case TOKEN_EQUAL_EQUAL:
            emitByte(OP_EQUAL, tLine);
            break;
        case TOKEN_GREATER:
            emitByte(OP_GREATER, tLine);
            break;
        case TOKEN_GREATER_EQUAL:
            emitByte(OP_GREATER_EQUAL, tLine);
            break;
        case TOKEN_LESS:
            emitByte(OP_LESS, tLine);
            break;
        case TOKEN_LESS_EQUAL:
            emitByte(OP_LESS_EQUAL, tLine);
            break;
        case TOKEN_PLUS:
            emitByte(OP_ADD, tLine);
            break;
        case TOKEN_MODULO:
            emitByte(OP_MODULO, tLine);
            break;
        case TOKEN_EXPO:
            emitByte(OP_POW, tLine);
            break;
        case TOKEN_MINUS:
            emitByte(OP_SUBTRACT, tLine);
            break;
        case TOKEN_STAR:
            emitByte(OP_MULTIPLY, tLine);
            break;
        case TOKEN_SLASH:
            emitByte(OP_DIVIDE, tLine);
            break;
        default:
            return;
        }
}

/**
 * Method for compiling function calls.
 */
void parseCall(bool canAssign) {
    uint8_t argCount = argumentList();
    #ifdef DEBUG_LOGGING
    printf("Emitting OP_CALL with %d args. Locals: %d\n", argCount, current->localCount);
    #endif
    emitBytes(OP_CALL, argCount);
}

/**
 * Method for compiling . expressions.
*/
void dot(bool canAssign) {
    consumeToken(TOKEN_IDENTIFIER, "Expect property name after '.'.");
    uint8_t name = identifierConstant(&parser.previous);

    if (canAssign && matchToken(TOKEN_EQUAL)) {
        parseExpression();
        emitBytes(OP_SET_PROPERTY, name);
    } else if (matchToken(TOKEN_LEFT_PAREN)) {
        uint8_t argCount = argumentList();
        emitBytes(OP_INVOKE, name);
        emitByte(argCount, parser.previous.line);
    } else {
        emitBytes(OP_GET_PROPERTY, name);
    }
}

/**
 * @brief Compiles a literal value (nil, true, false).
 *
 * Emits bytecode to load the literal onto the stack.
 *
 * @param canAssign Indicates if assignment is allowed (unused).
 */
void literal(bool canAssign) {

    int tLine = parser.previous.line;
    switch (parser.previous.type) {
        case TOKEN_FALSE:
            emitByte(OP_FALSE, tLine);
            break;
        case TOKEN_NIL:
            emitByte(OP_NIL, tLine);
            break;
        case TOKEN_TRUE:
            emitByte(OP_TRUE, tLine);
            break;
        default:
            return;
    }
}

/**
 * @brief Handles the 'super' keyword for superclass method access.
 *
 * Emits bytecode to access or invoke a method from the superclass.
 *
 * @param canAssign Indicates if assignment is allowed (unused).
 */
void super_(bool canAssign) {
    if (currentClass == NULL) {
        error("Can't use 'super' outside of a class.");
    } else if (!currentClass->hasSuperClass) {
        error("Can't use 'super' in a class with no superclass.");
    }

    consumeToken(TOKEN_DOT, "Expected '.' after 'super'.");
    consumeToken(TOKEN_IDENTIFIER, "Expect superclass method name.");
    uint8_t name = identifierConstant(&parser.previous);

    namedVariable(syntheticToken("self"), false);
    if (matchToken(TOKEN_LEFT_PAREN)) {
        uint8_t argCount = argumentList();
        namedVariable(syntheticToken("super"), false);
        emitBytes(OP_SUPER_INVOKE, name);
        emitByte(argCount, parser.previous.line);
    } else {
        namedVariable(syntheticToken("super"), false);
        emitBytes(OP_GET_SUPER, name);
    }
}

/**
 * @brief Handles the 'self' keyword for method calls within a class.
 *
 * Emits bytecode to push the current instance onto the stack.
 *
 * @param canAssign Indicates if assignment is allowed (unused).
 */
void self(bool canAssign) {
    if (currentClass == NULL) {
        error("Can't use 'self' outside of a class.");
        return;
    }
    variable(false);
}

/**
 * @brief Compiles a unary expression (e.g., -x or !x).
 *
 * Parses the operand and emits the appropriate bytecode for the unary operator.
 *
 * @param canAssign Indicates if assignment is allowed (unused).
 */
void unary(bool canAssign) {
    TokenType operatorType = parser.previous.type;

    // store the line this unary operator is one for 'emitByte'
    // it's plausible that parseExpression for compiling the operand
    // will take us onto a different line so our error reporting
    // could be wrong
    int tLine = parser.previous.line;

    parsePrecedence(PREC_UNARY);

    switch (operatorType) {
        case TOKEN_MINUS:
            emitByte(OP_NEGATE, tLine);
            break;
        case TOKEN_BANG:
            emitByte(OP_NOT, tLine);
            break;
        default:
            return;
    }
}

Parser parser;
