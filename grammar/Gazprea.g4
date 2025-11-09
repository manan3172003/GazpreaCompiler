grammar Gazprea;

file: global_stat* EOF;


global_stat
    : dec_stat
    | typealias_stat
    | function_stat
    | procedure_stat
    ;

typealias_stat: TYPEALIAS type ID SC;

stat
    : dec_stat
    | assign_stat
    | block_stat
    | if_stat
    | loop_stat
    | BREAK SC
    | CONTINUE SC
    | return_stat
    | output_stat
    | input_stat
    | procedure_call_stat
    ;

procedure_stat
    : PROCEDURE ID LPAREN procedure_params? RPAREN (RETURNS type)? SC
    | PROCEDURE ID LPAREN procedure_params? RPAREN (RETURNS type)? block_stat
    ;

procedure_params: procedure_param (COMMA procedure_param)*;

procedure_param: qualifier? type ID?;

procedure_call_stat: CALL ID LPAREN args? RPAREN SC;

function_stat
    : FUNCTION ID LPAREN function_params? RPAREN RETURNS type SC // Forward declaration
    | FUNCTION ID LPAREN function_params? RPAREN RETURNS type EQUAL expr SC
    | FUNCTION ID LPAREN function_params? RPAREN RETURNS type block_stat
    ;

function_params: function_param (COMMA function_param)*;

function_param: type ID?;

args: expr (COMMA expr)*;

output_stat: expr '->' STD_OUTPUT SC;

input_stat: assign_left '<-' STD_INPUT SC;

return_stat: RETURN expr? SC;

if_stat: IF LPAREN expr RPAREN stat else_stat?;
else_stat: ELSE stat;

loop_stat
    : LOOP WHILE LPAREN expr RPAREN stat #prePredicatedLoop
    | LOOP ID IN expr stat #iterativeLoop
    | LOOP stat WHILE LPAREN expr RPAREN SC #postPredicatedLoop
    | LOOP stat #infiniteLoop
    ;

block_stat: LBRACE stat* RBRACE;

assign_stat
    : assign_left EQUAL expr SC #singularAssign
    | assign_left (COMMA assign_left)+ EQUAL expr SC #tupleUnpackAssign
    ;

// TODO: For part 2, when new assignments and inputs are introduced, insert them here
assign_left
    : ID #idLVal
    | TUPLE_ACCESS #tupleElementLVal
    ;

dec_stat
    : qualifier? type  ID (EQUAL expr)? SC
    | qualifier type? ID EQUAL expr SC
    ;


tuple_type: TUPLE LPAREN type_list RPAREN;

type_list: type (COMMA type)+;

type
    : BOOLEAN #booleanType
    | CHARACTER #charType
    | INTEGER #intType
    | REAL #realType
    | tuple_type #tupType
    | type LBRACKET (expr | MULT) RBRACKET (LBRACKET (expr | MULT) RBRACKET) #twoDimArray
    | type LBRACKET (expr | MULT) RBRACKET #oneDimArray
    | ID #aliasType
    ;

qualifier
    : CONST
    | VAR
    ;

expr
    : LPAREN expr RPAREN #parenExpr
    | TUPLE_ACCESS #tupleAccessExpr
    | expr LBRACKET array_access_expr RBRACKET #arrayAccessExpr
    | expr DDOT expr #generatorExpr
    | <assoc=right> op=(PLUS | MINUS | NOT) expr #unaryExpr
    | <assoc=right> expr POWER expr #powerExpr
    | expr op=(MULT | DIV | REM) expr #mulDivRemExpr
    | expr DSTAR expr #dstarExpr
    | expr op=(PLUS | MINUS) expr #addSubExpr
    | expr BY expr #byExpr
    | expr op=(LT | LTE | GT | GTE) expr #relationalExpr
    | expr op=(EQ | NEQ) expr #equalityExpr
    | expr AND expr #andExpr
    | expr op=(OR | XOR) expr #logicalExpr
    | expr APPEND expr #appendExpr
    | AS '<' type '>' LPAREN expr RPAREN #castExpr
    | tuple_lit #tupleLiteral
    | array_lit #arrayLiteral
    | matrix_lit #matrixLiteral
    | float_parse #scientificFloatLiteral
    | STRING_LIT #stringLiteral
    | CHAR_LIT #charLiteral
    | INT_LIT #intLiteral
    | (TRUE | FALSE) #boolLiteral
    | ID #identifier
    | ID LPAREN args? RPAREN #funcProcExpr
    ;

array_access_expr
    : expr DDOT expr #sliceRangeExpr
    | DDOT expr #sliceEndExpr
    | expr DDOT #sliceStartExpr
    | DDOT #sliceAllExpr
    ;

tuple_lit: LPAREN tuple_elements RPAREN;
array_lit: LBRACKET array_elements? RBRACKET;
matrix_lit: LBRACKET (array_lit (COMMA array_lit)*)? RBRACKET;
array_elements: expr (COMMA expr)*;
tuple_elements: expr (COMMA expr)+;

float_parse
    : (float_lit | dot_float | float_dot) EXPONENT #scientificFloat
    | INT_LIT EXPONENT #scientificFloatNoDecimal
    | dot_float #dotFloat
    | float_dot #floatDot
    | float_lit #floatLiteral
    ;

dot_float
    : DOT INT_LIT // .14
    ;
float_dot
    : INT_LIT DOT // 3. (requires digits before dot)
    ;
float_lit
    : INT_LIT DOT INT_LIT // 3.14
    ;

// Keywords
AND: 'and';
AS: 'as';
BOOLEAN: 'boolean';
BREAK: 'break';
BY: 'by';
CALL: 'call';
CHARACTER: 'character';
COLUMNS: 'columns';
CONST: 'const';
CONTINUE: 'continue';
ELSE: 'else';
FALSE: 'false';
FORMAT: 'format';
FUNCTION: 'function';
IF: 'if';
IN: 'in';
INTEGER: 'integer';
LENGTH: 'length';
LOOP: 'loop';
NOT: 'not';
OR: 'or';
PROCEDURE: 'procedure';
REAL: 'real';
RETURN: 'return';
RETURNS: 'returns';
REVERSE: 'reverse';
ROWS: 'rows';
STD_INPUT: 'std_input';
STD_OUTPUT: 'std_output';
STREAM_STATE: 'stream_state';
STRING: 'string';
TRUE: 'true';
TUPLE: 'tuple';
TYPEALIAS: 'typealias';
VAR: 'var';
VECTOR: 'vector';
WHILE: 'while';
XOR: 'xor';
DDOT: '..';

// Operators
PLUS: '+';
MINUS: '-';
MULT: '*';
DSTAR: '**';
DIV: '/';
REM: '%';
POWER: '^';
EQ: '==';
NEQ: '!=';
LT: '<';
LTE: '<=';
GT: '>';
GTE: '>=';
EQUAL: '=';
LPAREN: '(';
RPAREN: ')';
LBRACE: '{';
RBRACE: '}';
LBRACKET: '[';
RBRACKET: ']';
APPEND: '||';
DOT: '.';
COMMA: ',';
SC: ';';

// Literals
//INT_SLICE_INT: INT_LIT DDOT INT_LIT;
//INT_SLICE: INT_LIT DDOT;
//SLICE_INT: DDOT INT_LIT;
//ID_SLICE_ID: ID DDOT ID;
//ID_SLICE: ID DDOT;
//SLICE_ID: DDOT ID;
TUPLE_ACCESS: ID DOT DIGIT+;
EXPONENT: [eE] (PLUS | MINUS)? DIGIT+;
CHAR_LIT: '\'' CHAR '\'';
STRING_LIT: '"' SCHAR_SEQ? '"';
INT_LIT: DIGIT+;

// Identifiers
ID: (LETTER | '_' ) (LETTER | DIGIT | '_')*;
fragment CHAR: ~['\\\r\n] | SimpleEscapeSequence;
fragment LETTER: [a-zA-Z];
DIGIT: [0-9];
//fragment EXPONENT: [eE] [+-]? DIGIT+;
fragment SimpleEscapeSequence: '\\' [0batnr"'\\];
fragment SCHAR_SEQ: SCHAR+;
fragment SCHAR
    : ~["\\\r\n]
    | SimpleEscapeSequence
    | '\\\n'
    | '\\\r\n'
    ;

// Skip whitespace
WS : [ \t\r\n]+ -> skip ;
// Comments
LINE_COMMENT: '//' ~[\r\n]* -> skip ;
BLOCK_COMMENT: '/*' .*? '*/' -> skip ;
