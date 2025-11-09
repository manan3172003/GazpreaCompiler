grammar Gazprea;

file: global_stat* EOF;


global_stat
    : dec_stat // TODO: Do global decs require const keyword or we can infer?
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

// TODO: Can procedure only be defined in a global scope?
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

// TODO: Calrify if this is analogous
// if (i == 1) loop {i -> std_output;}
// if (i == 1) while (i < 10){print (10);}
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
    | tuple_access #tupleElementLVal
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
    | tuple_access #tupleAccessExpr
    | expr LBRACKET expr RBRACKET #arrayAccessExpr
    | expr DDOT expr #sliceRangeExpr
//    | DDOT expr #sliceEndExpr
//    | expr DDOT #sliceStartExpr
//    | SLICE_INT #intSliceEndExpr
//    | expr SLICE_INT #exprIntSliceEndExpr
//    | INT_SLICE expr? #intSliceStartExpr
//    | INT_SLICE_INT #intSliceRangeExpr
//    | SLICE_ID #idSliceEndExpr
//    | expr SLICE_ID #exprIdSliceEndExpr
//    | ID_SLICE expr? #idSliceStartExpr
//    | ID_SLICE_ID #idSliceRangeExpr
//    | DDOT #sliceAllExpr
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
    | scientific_float #scientificFloatLiteral
//    | dot_float #dotFloatLiteral
//    | float_dot #floatDotLiteral
//    | float_lit #floatLiteral
    | STRING_LIT #stringLiteral
    | CHAR_LIT #charLiteral
    | int_lit #intLiteral
    | (TRUE | FALSE) #boolLiteral
    | ID #identifier
    | ID LPAREN args? RPAREN #funcProcExpr // TODO: Type check on procedure assign & decl & unary
    ;

tuple_lit: LPAREN tuple_elements RPAREN;
array_lit: LBRACKET array_elements? RBRACKET;
matrix_lit: LBRACKET (array_lit (COMMA array_lit)*)? RBRACKET;
// matrix_lit: LBRACKET ((LBRACKET elements (COMMA elements )* RBRACKET) (COMMA (LBRACKET elements (COMMA elements )* RBRACKET))*)? RBRACKET;
array_elements: expr (COMMA expr)*;
tuple_elements: expr (COMMA expr)+;

tuple_access: ID DOT int_lit;

scientific_float
    : float_lit EXPONENT int_lit
    | int_lit EXPONENT int_lit
    | dot_float
    | float_dot
    | float_lit
    ;
dot_float
    : DOT int_lit // .14
    ;
float_dot
    : int_lit DOT // 3. (requires digits before dot)
    ;
float_lit
    : int_lit DOT int_lit // 3.14
    ;

int_lit: DIGIT+;

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
EXPONENT: ('e'|'E') (PLUS|MINUS)?;
CHAR_LIT: '\'' CHAR '\'';
STRING_LIT: '"' SCHAR_SEQ? '"';

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
