#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>

// for lex
#define MAXLEN 256

// Token types
typedef enum {
    UNKNOWN, END, ENDFILE,
    INT, ID,
    ADDSUB, MULDIV,
    ASSIGN,
    LPAREN, RPAREN,
    AND, OR, XOR,
    INCDEC,
    ADDSUB_ASSIGN
} TokenSet;

TokenSet getToken(void);
TokenSet curToken = UNKNOWN;
char lexeme[MAXLEN];
int rnum;
// Test if a token matches the current token
int match(TokenSet token);
// Get the next token
void advance(void);
// Get the lexeme of the current token
char *getLexeme(void);


// for parser
#define TBLSIZE 64
// Set PRINTERR to 1 to print error message while calling error()
// Make sure you set PRINTERR to 0 before you submit your code
#define PRINTERR 0

// Call this macro to print error message and exit the program
// This will also print where you called it in your program
#define error(errorNum) { \
    if (PRINTERR) \
        fprintf(stderr, "error() called at %s:%d: ", __FILE__, __LINE__); \
    err(errorNum); \
}

// Error types
typedef enum {
    UNDEFINED, MISPAREN, NOTNUMID, NOTFOUND, RUNOUT, NOTLVAL, DIVZERO, SYNTAXERR
} ErrorType;

// Structure of the symbol table
typedef struct {
    int val;
    char name[MAXLEN];
} Symbol;

// Structure of a tree node
typedef struct _Node {
    TokenSet data;
    int val;
    char lexeme[MAXLEN];
    struct _Node *left;
    struct _Node *right;
} BTNode;

int sbcount = 0;
Symbol table[TBLSIZE];

// Initialize the symbol table with builtin variables
void initTable(void);
// Get the value of a variable
int getval(char *str);
// Set the value of a variable
int setval(char *str, int val);
// Make a new node according to token type and lexeme
BTNode *makeNode(TokenSet tok, const char *lexe);
// Free the syntax tree
void freeTree(BTNode *root);
BTNode *factor(void);
BTNode *unary_expr(void);
BTNode *muldiv_expr(void);
BTNode *muldiv_expr_tail(BTNode *left);
BTNode *addsub_expr(void);
BTNode *addsub_expr_tail(BTNode *left);
BTNode *and_expr(void);
BTNode *and_expr_tail(BTNode *left);
BTNode *xor_expr(void);
BTNode *xor_expr_tail(BTNode *left);
BTNode *or_expr(void);
BTNode *or_expr_tail(BTNode *left);
BTNode *assign_expr(void);
void statement(void);
// Print error message and exit the program
void err(ErrorType errorNum);


// for codeGen
// Evaluate the syntax tree
int evaluateTree(BTNode *root);
// Print the syntax tree in prefix
void printPrefix(BTNode *root);


/*============================================================================================
lex implementation
============================================================================================*/

TokenSet getToken(void)
{
    int i = 0;
    char c = '\0';

    while ((c = fgetc(stdin)) == ' ' || c == '\t');

    if (isdigit(c)) {
        lexeme[0] = c;
        c = fgetc(stdin);
        i = 1;
        while (isdigit(c) && i < MAXLEN) {
            lexeme[i] = c;
            ++i;
            c = fgetc(stdin);
        }
        ungetc(c, stdin);
        lexeme[i] = '\0';
        return INT;
    } else if (c == '+' || c == '-') {
        lexeme[0] = c;
        if(c == '+'){
            c = fgetc(stdin);
            if(c == '+'){
                lexeme[1] = c;
                lexeme[2] = '\0';
                return INCDEC;
            }
            else if(c == '='){
                lexeme[1] = c;
                lexeme[2] = '\0';
                return ADDSUB_ASSIGN;
            }
            else {
                lexeme[1] = '\0';
                ungetc(c, stdin);
                return ADDSUB;
            }
        } else {
            c = fgetc(stdin);
            if(c == '-'){
                lexeme[1] = c;
                lexeme[2] = '\0';
                return INCDEC;
            }
            else if(c == '='){
                lexeme[1] = c;
                lexeme[2] = '\0';
                return ADDSUB_ASSIGN;
            }
            else {
                lexeme[1] = '\0';
                ungetc(c, stdin);
                return ADDSUB;
            }         
        }        
    } else if (c == '*' || c == '/') {
        lexeme[0] = c;
        lexeme[1] = '\0';
        return MULDIV;
    } else if (c == '\n') {
        lexeme[0] = '\0';
        return END;
    } else if (c == '=') {
        strcpy(lexeme, "=");
        return ASSIGN;
    } else if (c == '(') {
        strcpy(lexeme, "(");
        return LPAREN;
    } else if (c == ')') {
        strcpy(lexeme, ")");
        return RPAREN;
    } else if (c == '&') {
        lexeme[0] = '&';
        lexeme[1] = '\0';
        return AND;
    } else if (c == '|') {
        lexeme[0] = '|';
        lexeme[1] = '\0';
        return OR;
    } else if (c == '^') {
        lexeme[0] = '^';
        lexeme[1] = '\0';
        return XOR;
    } else if (isalpha(c) || c == '_') {
        lexeme[0] = c;
        c = fgetc(stdin);
        i = 1;
        while ((isalpha(c) || isdigit(c) || c == '_') && (i < MAXLEN)) {
            lexeme[i] = c;
            ++i;
            c = fgetc(stdin);
        }
        ungetc(c, stdin);
        lexeme[i] = '\0';
        return ID;
    } else if (c == EOF) {
        return ENDFILE;
    } else {
        return UNKNOWN;
    }
}

void advance(void) {
    curToken = getToken();
}

int match(TokenSet token) {
    if (curToken == UNKNOWN)
        advance();
    return token == curToken;
}

char *getLexeme(void) {
    return lexeme;
}


/*============================================================================================
parser implementation
============================================================================================*/

void initTable(void) {
    strcpy(table[0].name, "x");
    table[0].val = 0;
    strcpy(table[1].name, "y");
    table[1].val = 0;
    strcpy(table[2].name, "z");
    table[2].val = 0;
    sbcount = 3;
}

int getval(char *str) {
    int i = 0;

    for (i = 0; i < sbcount; i++)
        if (strcmp(str, table[i].name) == 0)
            return table[i].val;

    if (sbcount >= TBLSIZE)
        error(RUNOUT);
    err(NOTFOUND);
    // create new variable(symbol) with value = 0//
    strcpy(table[sbcount].name, str);
    table[sbcount].val = 0;
    sbcount++;
    return 0;
}

int setval(char *str, int val) {
    int i = 0;

    for (i = 0; i < sbcount; i++) {
        if (strcmp(str, table[i].name) == 0) {
            table[i].val = val;
            return val;
        }
    }

    if (sbcount >= TBLSIZE)
        error(RUNOUT);
    // create new variable(symbol) with value you set//
    strcpy(table[sbcount].name, str);
    table[sbcount].val = val;
    sbcount++;
    return val;
}

int getVaribleIndex(char *str) {
    int i = 0;
    for (i = 0; i < sbcount; i++) {
        if (strcmp(str, table[i].name) == 0) {
            return i;
        }
    }
    err(RUNOUT);
    return -1;
}

BTNode *makeNode(TokenSet tok, const char *lexe) {
    BTNode *node = (BTNode*)malloc(sizeof(BTNode));
    strcpy(node->lexeme, lexe);
    node->data = tok;
    node->val = 0;
    node->left = NULL;
    node->right = NULL;
    return node;
}

void freeTree(BTNode *root) {
    if (root != NULL) {
        freeTree(root->left);
        freeTree(root->right);
        free(root);
    }
}

// factor := INT | ID | INCDEC ID | LPAREN assign_expr RPAREN
BTNode *factor(void) {
    BTNode *retp = NULL, *left = NULL;

    if (match(INT)) {
        retp = makeNode(INT, getLexeme());
        advance();
    } else if (match(ID)) {
        retp = makeNode(ID, getLexeme());
        advance();
    } else if (match(INCDEC)) {
        retp = makeNode(INCDEC, getLexeme());
        advance();
        if (match(ID)) {
            retp->right = makeNode(ID, getLexeme());
            advance();
        }
        else err(SYNTAXERR);
    } else if (match(LPAREN)) {
        advance();
        retp = assign_expr();
        if (match(RPAREN))
            advance();
        else
            error(MISPAREN);
    } else {
        error(NOTNUMID);
    }
    return retp;
}

// unary_expr := ADDSUB unary_expr | factor
BTNode *unary_expr(void) {
    BTNode *node = NULL;

    if (match(ADDSUB)) {
        node = makeNode(ADDSUB, getLexeme());
        node->left = makeNode(INT, "0");
        advance();
        node->right = unary_expr();
        return node;
    }

    else return factor();
}

// muldiv_expr_tail := MULDIV unary_expr muldiv_expr_tail | NiL
BTNode *muldiv_expr_tail(BTNode *left) {
    BTNode *node = NULL;

    if (match(MULDIV)) {
        node = makeNode(MULDIV, getLexeme());
        advance();
        
        node->left = left;
        node->right = unary_expr();
        return muldiv_expr_tail(node);
    } else {
        return left;
    }
}

// muldiv_expr := unary_expr muldiv_expr_tail
BTNode *muldiv_expr(void) {
    BTNode *node = unary_expr();
    return muldiv_expr_tail(node);
}

// addsub_expr_tail := ADDSUB muldiv_expr addsub_expr_tail | NiL
BTNode *addsub_expr_tail(BTNode *left) {
    BTNode *node = NULL;

    if (match(ADDSUB)) {
        node = makeNode(ADDSUB, getLexeme());
        advance();
        node->left = left;
        node->right = muldiv_expr();
        return addsub_expr_tail(node);
    } else {
        return left;
    }
}

// addsub_expr := muldiv_expr addsub_expr_tail
BTNode *addsub_expr(void) {
    BTNode *node = muldiv_expr();
    return addsub_expr_tail(node);
}

// and_expr_tail := AND addsub_expr and_expr_tail | NiL
BTNode *and_expr_tail(BTNode *left) {
    BTNode *node = NULL;

    if (match(AND)) {
        node = makeNode(AND, getLexeme());
        advance();
        node->left = left;
        node->right = addsub_expr();
        return and_expr_tail(node);
    } else {
        return left;
    }
}

// and_expr := addsub_expr and_expr_tail
BTNode *and_expr(void) {
    BTNode *node = addsub_expr();
    return and_expr_tail(node);
}

// xor_expr_tail := XOR and_expr xor_expr_tail | NiL
BTNode *xor_expr_tail(BTNode *left) {
    BTNode *node = NULL;

    if (match(XOR)) {
        node = makeNode(XOR, getLexeme());
        advance();
        node->left = left;
        node->right = and_expr();
        return xor_expr_tail(node);
    } else {
        return left;
    }
}

// xor_expr := and_expr xor_expr_tail
BTNode *xor_expr(void) {
    BTNode *node = and_expr();
    return xor_expr_tail(node);
}

// or_expr_tail := OR xor_expr or_expr_tail | NiL
BTNode *or_expr_tail(BTNode *left) {
    BTNode *node = NULL;

    if (match(OR)) {
        node = makeNode(OR, getLexeme());
        advance();
        node->left = left;
        node->right = xor_expr();
        return or_expr_tail(node);
    } else {
        return left;
    }
}

// or_expr := xor_expr or_expr_tail
BTNode *or_expr(void) {
    BTNode *node = xor_expr();
    return or_expr_tail(node);
}

// assign_expr := ID ASSIGN assign_expr | ID ADDSUB_ASSIGN assign_expr | or_expr 
BTNode *assign_expr(void) {
    BTNode *node = NULL;
    BTNode *left = NULL;
    left = or_expr();
    if(left->data == ID){
        if(match(ASSIGN)){
            node = makeNode(ASSIGN, getLexeme());
            advance();
            node->left = left;
            node->right = assign_expr();
        }
        else if(match(ADDSUB_ASSIGN)){
            node = makeNode(ADDSUB_ASSIGN, getLexeme());
            advance();
            node->left = left;
            node->right = assign_expr();
        }
        else node = left;
    }
    else node = left;
    return node;
}

// statement := ENDFILE | END | expr END
void statement(void) {
    BTNode *retp = NULL;

    if (match(ENDFILE)) {
        printf("MOV r0 [0]\n");
        printf("MOV r1 [4]\n");
        printf("MOV r2 [8]\n");
        printf("EXIT 0\n");
        exit(0);
    } else if (match(END)) {
        //printf(">> ");
        advance();
    } else {
        retp = assign_expr();
        if (match(END)) {
            rnum = 0;
            int ans = evaluateTree(retp);
            printf("%d\n", evaluateTree(retp));
            /*
            printf("Prefix traversal: ");
            printPrefix(retp);
            printf("\n");
            printf(">> ");
            */
            freeTree(retp);
            advance();
        } else {
            err(SYNTAXERR);
        }
    }
}

void err(ErrorType errorNum) {
    printf("EXIT 1\n");
    if (PRINTERR) {
        fprintf(stderr, "error: ");
        switch (errorNum) {
            case MISPAREN:
                fprintf(stderr, "mismatched parenthesis\n");
                break;
            case NOTNUMID:
                fprintf(stderr, "number or identifier expected\n");
                break;
            case NOTFOUND:
                fprintf(stderr, "variable not defined\n");
                break;
            case RUNOUT:
                fprintf(stderr, "out of memory\n");
                break;
            case NOTLVAL:
                fprintf(stderr, "lvalue required as an operand\n");
                break;
            case DIVZERO:
                fprintf(stderr, "divide by constant zero\n");
                break;
            case SYNTAXERR:
                fprintf(stderr, "syntax error\n");
                break;
            default:
                fprintf(stderr, "undefined error\n");
                break;
        }
    }
    exit(0);
}


/*============================================================================================
codeGen implementation
============================================================================================*/

int checkifid(BTNode *root) {
    if(!root) return 0; 
    if(root->data == ID) return 1;
    else return checkifid(root->left) + checkifid(root->right);
}

int evaluateTree(BTNode *root) {
    int retval = 0, lv = 0, rv = 0, address = 0, check = 0;

    if (root != NULL) {
        switch (root->data) {
            case ID:
                retval = getval(root->lexeme);
                address = getVaribleIndex(root->lexeme) * 4;
                printf("MOV r%d [%d]\n", rnum, address);
                rnum++;
                break;
            case INT:
                retval = atoi(root->lexeme);
                printf("MOV r%d %d\n", rnum, retval);
                rnum++;
                break;
            case ASSIGN:
                rv = evaluateTree(root->right);
                retval = setval(root->left->lexeme, rv);
                address = getVaribleIndex(root->left->lexeme) * 4;
                printf("MOV [%d] r%d\n", address, rnum - 1);
                break;
            case ADDSUB:
            case MULDIV:
            case AND:
            case XOR:
            case OR:
                lv = evaluateTree(root->left);
                rv = evaluateTree(root->right);
                rnum--;
                if (strcmp(root->lexeme, "+") == 0) {
                    retval = lv + rv;
                    printf("ADD r%d r%d\n", rnum - 1, rnum);
                } else if (strcmp(root->lexeme, "-") == 0) {
                    retval = lv - rv;
                    printf("SUB r%d r%d\n", rnum - 1, rnum);
                } else if (strcmp(root->lexeme, "*") == 0) {
                    retval = lv * rv;
                    printf("MUL r%d r%d\n", rnum - 1, rnum);
                } else if (strcmp(root->lexeme, "/") == 0) {
                    if ((rv == 0) && (checkifid(root->right) == 0)) {
                        error(DIVZERO);
                    }
                    else if (rv != 0) retval = lv / rv;
                    printf("DIV r%d r%d\n", rnum - 1, rnum);
                } else if (strcmp(root->lexeme, "&") == 0) {
                    retval = lv & rv;
                    printf("AND r%d r%d\n", rnum - 1, rnum);
                } else if (strcmp(root->lexeme, "^") == 0) {
                    retval = lv ^ rv;
                    printf("XOR r%d r%d\n", rnum - 1, rnum);
                } else if (strcmp(root->lexeme, "|") == 0) {
                    retval = lv | rv;
                    printf("OR r%d r%d\n", rnum - 1, rnum);
                }
                break;
            case INCDEC:
                check = getval(root->right->lexeme);             
                address = getVaribleIndex(root->right->lexeme) * 4;
                if (strcmp(root->lexeme, "++") == 0) {
                    retval = setval(root->right->lexeme, check + 1);
                    /*
                    */
                    printf("MOV r%d [%d]\n", rnum, address);
                    rnum++;
                    printf("MOV r%d 1\n", rnum);
                    printf("ADD r%d r%d\n", rnum - 1, rnum);
                    printf("MOV [%d] r%d\n", address, rnum - 1);
                } else if (strcmp(root->lexeme, "--") == 0) {                   
                    retval = setval(root->right->lexeme, check - 1);
                    /*
                    */
                    printf("MOV r%d [%d]\n", rnum, address);
                    rnum++;
                    printf("MOV r%d 1\n", rnum);
                    printf("SUB r%d r%d\n", rnum - 1, rnum);
                    printf("MOV [%d] r%d\n", address, rnum - 1);
                }
                break;
            case ADDSUB_ASSIGN:
                check = getval(root->left->lexeme);
                address = getVaribleIndex(root->left->lexeme) * 4;
                rv = evaluateTree(root->right);
                if (strcmp(root->lexeme, "+=") == 0) {
                    retval = setval(root->left->lexeme, check + rv);
                    /*
                    */
                    printf("MOV r%d [%d]\n", rnum, address);
                    printf("ADD r%d r%d\n", rnum, rnum - 1);
                    printf("MOV [%d] r%d\n", address, rnum);
                    printf("MOV r%d r%d\n", rnum - 1, rnum);
                } else if (strcmp(root->lexeme, "-=") == 0) {
                    retval = setval(root->left->lexeme, check - rv);
                    /*
                    */
                    printf("MOV r%d [%d]\n", rnum, address);
                    printf("SUB r%d r%d\n", rnum, rnum - 1);
                    printf("MOV [%d] r%d\n", address, rnum);
                    printf("MOV r%d r%d\n", rnum - 1, rnum);
                }
                break;
            default:
                retval = 0;
        }
    }
    return retval;
}

void printPrefix(BTNode *root) {
    if (root != NULL) {
        printf("%s ", root->lexeme);
        printPrefix(root->left);
        printPrefix(root->right);
    }
}


/*============================================================================================
main
============================================================================================*/

// This package is a calculator
// It works like a Python interpretor
// Example:
// >> y = 2
// >> z = 2
// >> x = 3 * y + 4 / (2 * z)
// It will print the answer of every line
// You should turn it into an expression compiler
// And print the assembly code according to the input

// This is the grammar used in this package
// You can modify it according to the spec and the slide
// statement  :=  ENDFILE | END | expr END
// expr    	  :=  term expr_tail
// expr_tail  :=  ADDSUB term expr_tail | NiL
// term 	  :=  factor term_tail
// term_tail  :=  MULDIV factor term_tail| NiL
// factor	  :=  INT | ADDSUB INT |
//		   	      ID  | ADDSUB ID  |
//		   	      ID ASSIGN expr |
//		   	      LPAREN expr RPAREN |
//		   	      ADDSUB LPAREN expr RPAREN

int main() {
    initTable();
    /*
    freopen("input.txt", "w", stdout);
    printf(">> ");
    */
    while (1) {
        statement();
    }
    return 0;
}
/*
x = 100+10*y
y = z+100*10/50*10
z = 10*x/100
xx = x
yy = y
zz = z
x = xx ^x
y=yy|y
z = zz&z
z += 8
x -= (1 + 2 * xx / z + 123 + --y)
*/