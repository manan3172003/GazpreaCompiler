grammar Gazprea;

file: expr* EOF;

expr
    : LPAREN expr RPAREN #parenExpr
    | TUPLE_ACCESS #tupleAccessExpr
    | expr DDOT expr #rangeExpr
    | op=(PLUS | MINUS | NOT) expr #unaryExpr
    | <assoc=right> expr POWER expr #powerExpr
    | expr op=(MULT | DIV | REM | DSTAR) expr #mulDivRemExpr
    | expr op=(PLUS | MINUS) expr #addSubExpr
    | expr BY expr #byExpr
    | expr op=(LT | LTE | GT | GTE) expr #relationalExpr
    | expr op=(EQ | NEQ) expr #equalityExpr
    | expr AND expr #andExpr
    | expr op=(OR | XOR) expr #logicalExpr
    | expr APPEND expr #appendExpr
    | INT_LIT #intLiteral
    | SCIENTIFIC_FLOAT #scientificFloatLiteral
    | DOT_FLOAT #dotFloatLiteral
    | FLOAT_DOT #floatDotLiteral
    | FLOAT_LIT #floatLiteral
    | STRING_LIT #stringLiteral
    | CHAR_LIT #charLiteral
    | (TRUE | FALSE) #boolLiteral
    | ID #identifier
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
TUPLE_ACCESS: ID DOT INT_LIT;
SCIENTIFIC_FLOAT
    : (DIGIT+ '.' DIGIT* | '.' DIGIT+) EXPONENT
    | DIGIT+ EXPONENT
    ;
DOT_FLOAT
    : '.' DIGIT+ // .14
    ;
FLOAT_DOT
    : DIGIT+ '.' // 3. (requires digits before dot)
    ;
FLOAT_LIT
    : DIGIT+ '.' DIGIT+ // 3.14
    ;
INT_LIT: DIGIT+;
CHAR_LIT: '\'' CHAR '\'';
STRING_LIT: '"' SCHAR_SEQ? '"';

// Identifiers
ID: (LETTER | '_' ) (LETTER | DIGIT | '_')*;
fragment CHAR: ~['\\\r\n] | SimpleEscapeSequence;
fragment LETTER: [a-zA-Z];
fragment DIGIT: [0-9];
fragment EXPONENT: [eE] [+-]? DIGIT+;
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
