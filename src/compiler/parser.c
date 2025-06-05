/**
 * @file parser.c
 * This file contains the parser for the CSLO language.
 */

#include "stdio.h"
#include "stdlib.h"

#include "compiler/codegen.h"
#include "compiler/scanner.h"
#include "compiler/parser.h"
#include "core/chunk.h"

/**
 * Method for advancing the parser's token.
 *
 * Sets the previous token to the current and retrieves the next
 * token. Will report any errors if it encounters an error token.
 */
void parserAdvance() {
    parser.previous = parser.current;

    for (;;) {
        parser.current = scanToken();
        if (parser.current.type != TOKEN_ERROR) {
            break;
        }

        errorAtCurrent(parser.current.start);
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
void expression() {
    parsePrecedence(PREC_ASSIGNMENT);
}

/**
 * Method for compiling a statement.
 */
void statement() {
    if (matchToken(TOKEN_FOR)) {
        forStatement();
    } else if (matchToken(TOKEN_IF)) {
        ifStatement();
    } else if (matchToken(TOKEN_RETURN)) {
        returnStatement();
    } else if (matchToken(TOKEN_WHILE)) {
        whileStatement();
    } else if (matchToken(TOKEN_LEFT_BRACE)) {
        beginScope();
        block();
        endScope();
    } else {
        expressionStatement();
    }
}

/**
 * Method for compiling expression statements.
 */
void expressionStatement() {
    expression();
    consumeToken(TOKEN_SEMICOLON, "Expect ';' after expression.");
    emitByte(OP_POP, parser.previous.line);
}

/**
 * Method for compiling a declaration.
 */
void declaration() {
    if (matchToken(TOKEN_CLASS)) {
        classDeclaration();
    } else if (matchToken(TOKEN_FUN)) {
        funDeclaration();
    } else if (matchToken(TOKEN_VAR)) {
        varDeclaration();
    } else {
        statement();
    }

    if (parser.panicMode) {
        synchronize();
    }
}

/**
 * Method for compiling blocks.
 */
void block() {
    while (!checkToken(TOKEN_RIGHT_BRACE) && !checkToken(TOKEN_EOF)) {
        declaration();
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
void grouping(bool canAssign) {
    expression();
    consumeToken(TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
}

/**
 * @brief Compiles a numeric literal.
 *
 * Emits bytecode to load the number onto the stack.
 *
 * @param canAssign Indicates if assignment is allowed (unused).
 */
void number(bool canAssign) {
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
void string(bool canAssign) {
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
    expression();

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
            expression();
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
        expression(); // parse the index value
    }
    if (matchToken(TOKEN_COLON)) {
        // handle slicers
        if (!checkToken(TOKEN_RIGHT_BRACKET)) {
            expression(); // parse the end index
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
            expression();
            switch (op) {
                case TOKEN_PLUS_EQUAL:    emitByte(OP_ADD, parser.previous.line); break;
                case TOKEN_MINUS_EQUAL:   emitByte(OP_SUBTRACT, parser.previous.line); break;
                case TOKEN_STAR_EQUAL:    emitByte(OP_MULTIPLY, parser.previous.line); break;
                case TOKEN_SLASH_EQUAL:   emitByte(OP_DIVIDE, parser.previous.line); break;
                default: break;
            }
            emitByte(OP_SET_INDEX, parser.previous.line);
    } else if (canAssign && matchToken(TOKEN_EQUAL)) {
        expression();
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
    expression();
    emitByte(OP_HAS, parser.previous.line);
}

/**
 * Method for compiling has statements.
 */
void hasNot(bool canAssign) {
    expression();
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
    emitBytes(OP_CALL, argCount);
}

/**
 * Method for compiling . expressions.
*/
void dot(bool canAssign) {
    consumeToken(TOKEN_IDENTIFIER, "Expect property name after '.'.");
    uint8_t name = identifierConstant(&parser.previous);

    if (canAssign && matchToken(TOKEN_EQUAL)) {
        expression();
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
    // it's plausible that expression() for compiling the operand
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


/**
 * Table for defining parse rules and precedence.
 *
 * The keys to the table are the token types.
 *
 * The columns are:
 *   - the function for prefix expressions for that token type
 *   - the function for infix expressions for that token type
 *   - the precedence of an infix expression for that token type
 *
 * AKA:
 *   - an expression starting with a '(' will call the 'grouping' expression function
 *   - a '-' token will call 'unary' if it's a prefix \
 *      and 'binary' if it's an infix expression with it's precendence set to TERM
 *
 * The precendence tells us how much of remaining code to consume before returning.
 * AKA, how much code belongs to this expression we're compiling.
 */
ParseRule rules[] = {
    [TOKEN_LEFT_PAREN]      = { grouping,     parseCall,      PREC_CALL       },
    [TOKEN_RIGHT_PAREN]     = { NULL,         NULL,           PREC_NONE       },
    [TOKEN_LEFT_BRACE]      = { NULL,         NULL,           PREC_NONE       },
    [TOKEN_RIGHT_BRACE]     = { NULL,         NULL,           PREC_NONE       },
    [TOKEN_COMMA]           = { NULL,         NULL,           PREC_NONE       },
    [TOKEN_DOT]             = { NULL,         dot,            PREC_CALL       },
    [TOKEN_MINUS]           = { unary,        binary,         PREC_TERM       },
    [TOKEN_PLUS]            = { NULL,         binary,         PREC_TERM       },
    [TOKEN_SEMICOLON]       = { NULL,         NULL,           PREC_NONE       },
    [TOKEN_SLASH]           = { NULL,         binary,         PREC_FACTOR     },
    [TOKEN_STAR]            = { NULL,         binary,         PREC_FACTOR     },
    [TOKEN_MODULO]          = { NULL,         binary,         PREC_FACTOR     },
    [TOKEN_EXPO]            = { NULL,         binary,         PREC_FACTOR     },
    [TOKEN_BANG]            = { unary,        NULL,           PREC_NONE       },
    [TOKEN_BANG_EQUAL]      = { NULL,         binary,         PREC_EQUALITY   },
    [TOKEN_EQUAL]           = { NULL,         NULL,           PREC_NONE       },
    [TOKEN_EQUAL_EQUAL]     = { NULL,         binary,         PREC_EQUALITY   },
    [TOKEN_GREATER]         = { NULL,         binary,         PREC_EQUALITY   },
    [TOKEN_GREATER_EQUAL]   = { NULL,         binary,         PREC_EQUALITY   },
    [TOKEN_LESS]            = { NULL,         binary,         PREC_EQUALITY   },
    [TOKEN_LESS_EQUAL]      = { NULL,         binary,         PREC_EQUALITY   },
    [TOKEN_IDENTIFIER]      = { variable,     NULL,           PREC_NONE       },
    [TOKEN_STRING]          = { string,       NULL,           PREC_NONE       },
    [TOKEN_NUMBER]          = { number,       NULL,           PREC_NONE       },
    [TOKEN_AND]             = { NULL,         and_,           PREC_AND        },
    [TOKEN_CLASS]           = { NULL,         NULL,           PREC_NONE       },
    [TOKEN_ELSE]            = { NULL,         NULL,           PREC_NONE       },
    [TOKEN_FOR]             = { NULL,         NULL,           PREC_NONE       },
    [TOKEN_FUN]             = { NULL,         NULL,           PREC_NONE       },
    [TOKEN_ELIF]            = { NULL,         NULL,           PREC_NONE       },
    [TOKEN_IF]              = { NULL,         NULL,           PREC_NONE       },
    [TOKEN_NIL]             = { literal,      NULL,           PREC_NONE       },
    [TOKEN_OR]              = { NULL,         or_,            PREC_OR         },
    [TOKEN_RETURN]          = { NULL,         NULL,           PREC_NONE       },
    [TOKEN_SUPER]           = { super_,       NULL,           PREC_NONE       },
    [TOKEN_SELF]            = { self,         NULL,           PREC_NONE       },
    [TOKEN_VAR]             = { NULL,         NULL,           PREC_NONE       },
    [TOKEN_WHILE]           = { NULL,         NULL,           PREC_NONE       },
    [TOKEN_ERROR]           = { NULL,         NULL,           PREC_NONE       },
    [TOKEN_EOF]             = { NULL,         NULL,           PREC_NONE       },
    [TOKEN_PLUS_PLUS]       = { prefixIncDec, postfixIncDec,  PREC_POSTFIX    },
    [TOKEN_MINUS_MINUS]     = { prefixIncDec, postfixIncDec,  PREC_POSTFIX    },
    [TOKEN_PLUS_EQUAL]      = { NULL,         compoundAssign, PREC_ASSIGNMENT },
    [TOKEN_MINUS_EQUAL]     = { NULL,         compoundAssign, PREC_ASSIGNMENT },
    [TOKEN_STAR_EQUAL]      = { NULL,         compoundAssign, PREC_ASSIGNMENT },
    [TOKEN_SLASH_EQUAL]     = { NULL,         compoundAssign, PREC_ASSIGNMENT },
    [TOKEN_LEFT_BRACKET]    = { list,         index,          PREC_CALL       },
    [TOKEN_HAS]             = { NULL,         has,            PREC_EQUALITY   },
    [TOKEN_HAS_NOT]         = { NULL,         hasNot,         PREC_EQUALITY   },
};

Parser parser;
