"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.scanDocument = scanDocument;
exports.getLinePrefix = getLinePrefix;
exports.getWordAtPosition = getWordAtPosition;
exports.getCurrentLine = getCurrentLine;
exports.getCompletionContext = getCompletionContext;
exports.getKeywordDoc = getKeywordDoc;
exports.getBlockNames = getBlockNames;
const KEYWORDS = new Set([
    'package', 'using', 'private', 'public',
    'struct', 'extends', 'interface', 'fn', 'return',
    'block', 'reset',
    'if', 'else', 'while', 'for', 'error',
    'int', 'float', 'bool', 'char', 'String', 'void',
    'u8', 'u16', 'u32', 'u64',
    'i8', 'i16', 'i32', 'i64',
    'f32', 'f64',
    'usize', 'isize', 'byte',
    'null', 'true', 'false',
    'include', 'link', 'extern', 'and',
]);
const KEYWORD_DOCS = {
    package: 'Declares a package namespace. Example: `package SPRITES`',
    using: 'Imports a package. Example: `using IO`',
    private: 'Makes the following declaration visible only within the package.',
    public: 'Makes the following declaration visible to all packages.',
    struct: 'Declares a struct (class-like type with fields and methods).',
    extends: 'Inherits from another struct. Example: `struct Player extends Entity`',
    interface: 'Declares an interface with method signatures.',
    fn: 'Declares a function or method.',
    return: 'Returns a value from a function.',
    block: 'Declares or scopes a memory block. Example: `block game = 64MB`',
    reset: 'Frees all allocations in a memory block. Example: `game.reset()`',
    if: 'Conditional branch. Parentheses are optional.',
    else: 'Alternative branch for if statements.',
    while: 'Loop while condition is true. Parentheses are optional.',
    for: 'C-style for loop: `for int i = 0; i < 10; i++ { }`',
    error: 'Panics with a message and aborts execution.',
    int: '64-bit signed integer type.',
    float: '64-bit floating point type.',
    bool: 'Boolean type (`true` or `false`).',
    char: '8-bit character type.',
    String: 'Dynamic string type (heap-allocated in a block).',
    void: 'No return type.',
    u8: 'Unsigned 8-bit integer type.',
    u16: 'Unsigned 16-bit integer type.',
    u32: 'Unsigned 32-bit integer type.',
    u64: 'Unsigned 64-bit integer type.',
    i8: 'Signed 8-bit integer type.',
    i16: 'Signed 16-bit integer type.',
    i32: 'Signed 32-bit integer type.',
    i64: 'Signed 64-bit integer type.',
    f32: '32-bit floating point type.',
    f64: '64-bit floating point type (same as `float`).',
    usize: 'Unsigned pointer-sized integer type.',
    isize: 'Signed pointer-sized integer type.',
    byte: '8-bit byte type (alias for `u8`).',
    null: 'Null pointer literal, assignable to any type.',
    true: 'Boolean true literal.',
    false: 'Boolean false literal.',
    include: 'Include a C header. Example: `include "math.h"`',
    link: 'Link a C library. Example: `link m` or `link SDL2`',
    extern: 'Declare an external C function. Example: `extern fn sqrt(f64 x) -> f64`',
    and: 'Connects include and link: `include "math.h" and link m`',
    PoolAllocator: 'Pool allocator type for fixed-size slot allocation. Created via `pool_create()`.',
    block_set_tls: 'Sets the thread-local storage block context. Used for per-thread memory blocks.',
    block_get_tls: 'Returns the thread-local storage block context.',
    block_alloc_tls: 'Allocates memory from the thread-local storage block.',
    pool_create: 'Creates a new pool allocator with the given total size and slot size.',
    pool_add_slot: 'Adds a pre-allocated data slot to the pool.',
    pool_alloc: 'Allocates a slot from the pool. Returns NULL if full.',
    pool_free: 'Frees a slot back to the pool for reuse.',
    pool_destroy: 'Destroys the pool allocator and frees all memory.',
    block_enable_double_buffer: 'Enables double-buffering on a block, keeping active and shadow buffers.',
    block_swap_buffers: 'Swaps the active and shadow buffers of a double-buffered block.',
    block_alloc_db: 'Allocates memory in a double-buffered block, returning a pointer valid in both buffers.',
};
const FIXED_WIDTH_TYPES = new Set(['u8', 'u16', 'u32', 'u64', 'i8', 'i16', 'i32', 'i64', 'f32', 'f64', 'usize', 'isize', 'byte']);
const BUILTIN_TYPES = new Set(['int', 'float', 'bool', 'char', 'String', 'void', 'PoolAllocator', ...FIXED_WIDTH_TYPES]);
function scanDocument(text) {
    const tokens = [];
    const symbols = [];
    const blocks = [];
    const structs = new Map();
    const errors = [];
    const usings = [];
    let packageName = '';
    const lines = text.split('\n');
    let inBlockComment = false;
    // First pass: collect structs with their bodies for context
    const structBodies = new Map();
    // State tracking
    let braceDepth = 0;
    let structBraceDepth = 0;
    let inStructBody = false;
    for (let lineIdx = 0; lineIdx < lines.length; lineIdx++) {
        let line = lines[lineIdx];
        let col = 0;
        const len = line.length;
        while (col < len) {
            const ch = line[col];
            // Skip whitespace
            if (ch === ' ' || ch === '\t') {
                col++;
                continue;
            }
            // Line comment
            if (ch === '/' && col + 1 < len && line[col + 1] === '/') {
                tokens.push({ type: 'COMMENT', lexeme: line.slice(col), line: lineIdx + 1, col: col + 1 });
                break;
            }
            // Strings
            if (ch === '"') {
                const start = col;
                col++;
                while (col < len && line[col] !== '"') {
                    if (line[col] === '\\')
                        col++;
                    col++;
                }
                const hasClosing = col < len && line[col] === '"';
                if (hasClosing)
                    col++;
                const lexeme = line.slice(start, col);
                tokens.push({ type: 'STRING_LITERAL', lexeme, line: lineIdx + 1, col: start + 1 });
                if (!hasClosing) {
                    errors.push({ message: 'unterminated string literal', line: lineIdx + 1, col: start + 1 });
                }
                continue;
            }
            // Char literals
            if (ch === '\'') {
                const start = col;
                col++;
                if (col < len && line[col] === '\\')
                    col++;
                col++;
                const hasClosing = col < len && line[col] === '\'';
                if (hasClosing)
                    col++;
                const lexeme = line.slice(start, col);
                tokens.push({ type: 'CHAR_LITERAL', lexeme, line: lineIdx + 1, col: start + 1 });
                if (!hasClosing) {
                    errors.push({ message: 'unterminated char literal', line: lineIdx + 1, col: start + 1 });
                }
                continue;
            }
            // Numbers (int & float)
            if (ch >= '0' && ch <= '9') {
                const start = col;
                let isFloat = false;
                while (col < len && (line[col] >= '0' && line[col] <= '9'))
                    col++;
                if (col < len && line[col] === '.') {
                    isFloat = true;
                    col++;
                    while (col < len && (line[col] >= '0' && line[col] <= '9'))
                        col++;
                }
                const lexeme = line.slice(start, col);
                tokens.push({ type: isFloat ? 'FLOAT_LITERAL' : 'INT_LITERAL', lexeme, line: lineIdx + 1, col: start + 1 });
                continue;
            }
            // Identifiers & Keywords
            if ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || ch === '_') {
                const start = col;
                while (col < len && ((line[col] >= 'a' && line[col] <= 'z') || (line[col] >= 'A' && line[col] <= 'Z') || (line[col] >= '0' && line[col] <= '9') || line[col] === '_'))
                    col++;
                const lexeme = line.slice(start, col);
                if (KEYWORDS.has(lexeme)) {
                    tokens.push({ type: lexeme.toUpperCase(), lexeme, line: lineIdx + 1, col: start + 1 });
                }
                else {
                    tokens.push({ type: 'IDENTIFIER', lexeme, line: lineIdx + 1, col: start + 1 });
                }
                continue;
            }
            // Multi-char operators
            if (ch === '=' && col + 1 < len && line[col + 1] === '=') {
                tokens.push({ type: 'EQ', lexeme: '==', line: lineIdx + 1, col: col + 1 });
                col += 2;
                continue;
            }
            if (ch === '!' && col + 1 < len && line[col + 1] === '=') {
                tokens.push({ type: 'NEQ', lexeme: '!=', line: lineIdx + 1, col: col + 1 });
                col += 2;
                continue;
            }
            if (ch === '<' && col + 1 < len && line[col + 1] === '=') {
                tokens.push({ type: 'LE', lexeme: '<=', line: lineIdx + 1, col: col + 1 });
                col += 2;
                continue;
            }
            if (ch === '>' && col + 1 < len && line[col + 1] === '=') {
                tokens.push({ type: 'GE', lexeme: '>=', line: lineIdx + 1, col: col + 1 });
                col += 2;
                continue;
            }
            if (ch === '&' && col + 1 < len && line[col + 1] === '&') {
                tokens.push({ type: 'AND', lexeme: '&&', line: lineIdx + 1, col: col + 1 });
                col += 2;
                continue;
            }
            if (ch === '|' && col + 1 < len && line[col + 1] === '|') {
                tokens.push({ type: 'OR', lexeme: '||', line: lineIdx + 1, col: col + 1 });
                col += 2;
                continue;
            }
            if (ch === '+' && col + 1 < len && line[col + 1] === '=') {
                tokens.push({ type: 'PLUS_ASSIGN', lexeme: '+=', line: lineIdx + 1, col: col + 1 });
                col += 2;
                continue;
            }
            if (ch === '-' && col + 1 < len && line[col + 1] === '=') {
                tokens.push({ type: 'MINUS_ASSIGN', lexeme: '-=', line: lineIdx + 1, col: col + 1 });
                col += 2;
                continue;
            }
            if (ch === '-' && col + 1 < len && line[col + 1] === '>') {
                tokens.push({ type: 'ARROW', lexeme: '->', line: lineIdx + 1, col: col + 1 });
                col += 2;
                continue;
            }
            if (ch === '+' && col + 1 < len && line[col + 1] === '+') {
                tokens.push({ type: 'PLUSPLUS', lexeme: '++', line: lineIdx + 1, col: col + 1 });
                col += 2;
                continue;
            }
            if (ch === '-' && col + 1 < len && line[col + 1] === '-') {
                tokens.push({ type: 'MINUSMINUS', lexeme: '--', line: lineIdx + 1, col: col + 1 });
                col += 2;
                continue;
            }
            if (ch === '<' && col + 1 < len && line[col + 1] === '<') {
                tokens.push({ type: 'LSHIFT', lexeme: '<<', line: lineIdx + 1, col: col + 1 });
                col += 2;
                continue;
            }
            if (ch === '>' && col + 1 < len && line[col + 1] === '>') {
                tokens.push({ type: 'RSHIFT', lexeme: '>>', line: lineIdx + 1, col: col + 1 });
                col += 2;
                continue;
            }
            if (ch === '*' && col + 1 < len && line[col + 1] === '=') {
                tokens.push({ type: 'STAR_ASSIGN', lexeme: '*=', line: lineIdx + 1, col: col + 1 });
                col += 2;
                continue;
            }
            if (ch === '/' && col + 1 < len && line[col + 1] === '=') {
                tokens.push({ type: 'SLASH_ASSIGN', lexeme: '/=', line: lineIdx + 1, col: col + 1 });
                col += 2;
                continue;
            }
            // Single char tokens
            if (ch === '{') {
                tokens.push({ type: 'LBRACE', lexeme: '{', line: lineIdx + 1, col: col + 1 });
                col++;
                continue;
            }
            if (ch === '}') {
                tokens.push({ type: 'RBRACE', lexeme: '}', line: lineIdx + 1, col: col + 1 });
                col++;
                continue;
            }
            if (ch === '(') {
                tokens.push({ type: 'LPAREN', lexeme: '(', line: lineIdx + 1, col: col + 1 });
                col++;
                continue;
            }
            if (ch === ')') {
                tokens.push({ type: 'RPAREN', lexeme: ')', line: lineIdx + 1, col: col + 1 });
                col++;
                continue;
            }
            if (ch === '[') {
                tokens.push({ type: 'LBRACKET', lexeme: '[', line: lineIdx + 1, col: col + 1 });
                col++;
                continue;
            }
            if (ch === ']') {
                tokens.push({ type: 'RBRACKET', lexeme: ']', line: lineIdx + 1, col: col + 1 });
                col++;
                continue;
            }
            if (ch === '.') {
                tokens.push({ type: 'DOT', lexeme: '.', line: lineIdx + 1, col: col + 1 });
                col++;
                continue;
            }
            if (ch === ',') {
                tokens.push({ type: 'COMMA', lexeme: ',', line: lineIdx + 1, col: col + 1 });
                col++;
                continue;
            }
            if (ch === ';') {
                tokens.push({ type: 'SEMICOLON', lexeme: ';', line: lineIdx + 1, col: col + 1 });
                col++;
                continue;
            }
            if (ch === '@') {
                tokens.push({ type: 'AT', lexeme: '@', line: lineIdx + 1, col: col + 1 });
                col++;
                continue;
            }
            if (ch === '=') {
                tokens.push({ type: 'ASSIGN', lexeme: '=', line: lineIdx + 1, col: col + 1 });
                col++;
                continue;
            }
            if (ch === '+') {
                tokens.push({ type: 'PLUS', lexeme: '+', line: lineIdx + 1, col: col + 1 });
                col++;
                continue;
            }
            if (ch === '-') {
                tokens.push({ type: 'MINUS', lexeme: '-', line: lineIdx + 1, col: col + 1 });
                col++;
                continue;
            }
            if (ch === '*') {
                tokens.push({ type: 'STAR', lexeme: '*', line: lineIdx + 1, col: col + 1 });
                col++;
                continue;
            }
            if (ch === '/') {
                tokens.push({ type: 'SLASH', lexeme: '/', line: lineIdx + 1, col: col + 1 });
                col++;
                continue;
            }
            if (ch === '<') {
                tokens.push({ type: 'LT', lexeme: '<', line: lineIdx + 1, col: col + 1 });
                col++;
                continue;
            }
            if (ch === '>') {
                tokens.push({ type: 'GT', lexeme: '>', line: lineIdx + 1, col: col + 1 });
                col++;
                continue;
            }
            if (ch === '!') {
                tokens.push({ type: 'NOT', lexeme: '!', line: lineIdx + 1, col: col + 1 });
                col++;
                continue;
            }
            if (ch === ':') {
                tokens.push({ type: 'COLON', lexeme: ':', line: lineIdx + 1, col: col + 1 });
                col++;
                continue;
            }
            if (ch === '&') {
                tokens.push({ type: 'BIT_AND', lexeme: '&', line: lineIdx + 1, col: col + 1 });
                col++;
                continue;
            }
            if (ch === '|') {
                tokens.push({ type: 'BIT_OR', lexeme: '|', line: lineIdx + 1, col: col + 1 });
                col++;
                continue;
            }
            if (ch === '^') {
                tokens.push({ type: 'BIT_XOR', lexeme: '^', line: lineIdx + 1, col: col + 1 });
                col++;
                continue;
            }
            if (ch === '~') {
                tokens.push({ type: 'BIT_NOT', lexeme: '~', line: lineIdx + 1, col: col + 1 });
                col++;
                continue;
            }
            // Unknown char — skip
            col++;
        }
    }
    // Second pass: build symbol table using the tokens
    let i = 0;
    const tok = tokens;
    while (i < tok.length) {
        const t = tok[i];
        // Package declaration
        if (t.type === 'PACKAGE' && i + 1 < tok.length && tok[i + 1].type === 'IDENTIFIER') {
            packageName = tok[i + 1].lexeme;
            symbols.push({ name: packageName, kind: 'package', type_name: 'package', line: t.line, col: t.col });
            i += 2;
            continue;
        }
        // Using declaration
        if (t.type === 'USING' && i + 1 < tok.length) {
            let j = i + 1;
            let usingPath = '';
            while (j < tok.length && (tok[j].type === 'IDENTIFIER' || tok[j].type === 'DOT')) {
                usingPath += tok[j].lexeme;
                j++;
            }
            usings.push(usingPath);
            i = j;
            continue;
        }
        // Include: include "header.h"
        if (t.type === 'INCLUDE' && i + 1 < tok.length && tok[i + 1].type === 'STRING_LITERAL') {
            i += 2;
            // Check for optional "and link libname"
            if (i < tok.length && tok[i].type === 'AND' && i + 1 < tok.length && tok[i + 1].type === 'LINK' && i + 2 < tok.length && tok[i + 2].type === 'IDENTIFIER') {
                i += 3;
            }
            continue;
        }
        // Link: link libname
        if (t.type === 'LINK' && i + 1 < tok.length && tok[i + 1].type === 'IDENTIFIER') {
            i += 2;
            continue;
        }
        // Block declaration: block NAME = NUM UNIT
        if (t.type === 'BLOCK' && i + 1 < tok.length && tok[i + 1].type === 'IDENTIFIER') {
            const blockName = tok[i + 1].lexeme;
            blocks.push({ name: blockName, line: t.line });
            symbols.push({ name: blockName, kind: 'block', type_name: 'block', line: t.line, col: t.col });
            i += 2;
            // skip = SIZE UNIT if present
            if (i < tok.length && tok[i].type === 'ASSIGN') {
                i++;
                if (i < tok.length && (tok[i].type === 'INT_LITERAL' || tok[i].type === 'FLOAT_LITERAL')) {
                    i++;
                    if (i < tok.length && tok[i].type === 'IDENTIFIER')
                        i++;
                }
            }
            continue;
        }
        // Block scope: block NAME { ... }
        if (t.type === 'BLOCK' && i + 3 < tok.length && tok[i + 1].type === 'IDENTIFIER' && tok[i + 2].type === 'LBRACE') {
            blocks.push({ name: tok[i + 1].lexeme, line: t.line });
            symbols.push({ name: tok[i + 1].lexeme, kind: 'block', type_name: 'block', line: t.line, col: t.col });
            i += 3;
            continue;
        }
        // Block scope: block NAME :  (colon syntax to change default block)
        if (t.type === 'BLOCK' && i + 2 < tok.length && tok[i + 1].type === 'IDENTIFIER' && tok[i + 2].type === 'COLON') {
            const blockName = tok[i + 1].lexeme;
            if (!blocks.find(b => b.name === blockName)) {
                blocks.push({ name: blockName, line: t.line });
                symbols.push({ name: blockName, kind: 'block', type_name: 'block', line: t.line, col: t.col });
            }
            i += 3;
            continue;
        }
        // Struct declaration: struct NAME ... { ... }
        if (t.type === 'STRUCT' && i + 1 < tok.length && tok[i + 1].type === 'IDENTIFIER') {
            const structName = tok[i + 1].lexeme;
            const structLine = t.line;
            const structCol = t.col;
            let extendsName = '';
            let j = i + 2;
            // Skip optional "extends Name"
            if (j < tok.length && tok[j].type === 'EXTENDS' && j + 1 < tok.length && tok[j + 1].type === 'IDENTIFIER') {
                extendsName = tok[j + 1].lexeme;
                j += 2;
            }
            // Skip [ and find { to get brace depth
            let structStartBrace = -1;
            while (j < tok.length && tok[j].type !== 'LBRACE')
                j++;
            if (j < tok.length && tok[j].type === 'LBRACE') {
                structStartBrace = j;
                j++;
            }
            // Find the matching RBRACE
            let braceCount = 1;
            let structEndIdx = -1;
            for (let k = j; k < tok.length; k++) {
                if (tok[k].type === 'LBRACE')
                    braceCount++;
                if (tok[k].type === 'RBRACE')
                    braceCount--;
                if (braceCount === 0) {
                    structEndIdx = k;
                    break;
                }
            }
            const structObj = {
                name: structName,
                fields: [],
                methods: [],
                line: structLine,
                col: structCol,
            };
            symbols.push({ name: structName, kind: 'struct', type_name: 'struct', line: structLine, col: structCol });
            // Parse fields and methods inside struct body
            if (structStartBrace >= 0 && structEndIdx > structStartBrace) {
                let k = structStartBrace + 1;
                while (k < structEndIdx) {
                    // Skip private/public
                    let isPrivate = false;
                    if (tok[k].type === 'PRIVATE' || tok[k].type === 'PUBLIC') {
                        isPrivate = tok[k].type === 'PRIVATE';
                        k++;
                    }
                    // Check if it's a field (TYPE NAME) or method (fn NAME)
                    if (tok[k].type === 'FN' && k + 1 < tok.length && tok[k + 1].type === 'IDENTIFIER') {
                        // Method
                        const fnName = tok[k + 1].lexeme;
                        const fnLine = tok[k].line;
                        k += 2;
                        // Parameters: ( param1, param2, ... )
                        const params = [];
                        if (k < tok.length && tok[k].type === 'LPAREN') {
                            k++;
                            let paramDepth = 1;
                            while (k < tok.length && paramDepth > 0) {
                                if (tok[k].type === 'LPAREN')
                                    paramDepth++;
                                if (tok[k].type === 'RPAREN')
                                    paramDepth--;
                                if (paramDepth > 0 && tok[k].type === 'IDENTIFIER' && k > 0 && tok[k - 1].type !== 'DOT') {
                                    // Check if preceded by a type keyword
                                    if (k >= 2 && (tok[k - 1].type === 'IDENTIFIER' || BUILTIN_TYPES.has(tok[k - 1].lexeme.toLowerCase()))) {
                                        params.push(tok[k].lexeme);
                                    }
                                }
                                k++;
                            }
                        }
                        // Return type: -> TYPE
                        let returnType = 'void';
                        if (k < tok.length && tok[k].type === 'ARROW' && k + 1 < tok.length) {
                            k++;
                            if (tok[k].type === 'IDENTIFIER' || BUILTIN_TYPES.has(tok[k].lexeme.toLowerCase())) {
                                returnType = tok[k].lexeme;
                                k++;
                            }
                        }
                        // Constructor detection
                        const isCtor = fnName === structName;
                        symbols.push({
                            name: fnName,
                            kind: isCtor ? 'constructor' : 'function',
                            type_name: returnType,
                            line: fnLine,
                            col: tok[k - 1].col,
                            parent: structName,
                        });
                        structObj.methods.push({ name: fnName, params, return_type: returnType, line: fnLine });
                        // Skip body { ... }
                        if (k < tok.length && tok[k].type === 'LBRACE') {
                            braceCount = 1;
                            k++;
                            while (k < tok.length && braceCount > 0) {
                                if (tok[k].type === 'LBRACE')
                                    braceCount++;
                                if (tok[k].type === 'RBRACE')
                                    braceCount--;
                                if (braceCount > 0)
                                    k++;
                            }
                            k++;
                        }
                    }
                    else {
                        // Field: TYPE NAME or TYPE[N] NAME (possibly with @ or =)
                        // Types can be keyword tokens (INT, U8, FLOAT, etc.) or IDENTIFIER tokens (user-defined structs)
                        // Dynamically build the set of type keyword tokens from BUILTIN_TYPES
                        const typeTokenTypes = new Set([
                            'IDENTIFIER',
                            ...Array.from(BUILTIN_TYPES).map(t => t.toUpperCase())
                        ]);
                        // Check for array type: TYPE [ NUMBER ] NAME
                        if (typeTokenTypes.has(tok[k].type) &&
                            k + 3 < tok.length &&
                            tok[k + 1].type === 'LBRACKET' &&
                            (tok[k + 2].type === 'INT_LITERAL' || tok[k + 2].type === 'IDENTIFIER') &&
                            tok[k + 3].type === 'RBRACKET' &&
                            k + 4 < tok.length &&
                            tok[k + 4].type === 'IDENTIFIER' &&
                            tok[k + 4].lexeme !== '(') {
                            const fieldType = tok[k].lexeme;
                            const fieldName = tok[k + 4].lexeme;
                            const fieldLine = tok[k + 4].line;
                            symbols.push({ name: fieldName, kind: 'field', type_name: `${fieldType}[]`, line: fieldLine, col: tok[k + 4].col, parent: structName });
                            structObj.fields.push({ name: fieldName, type: `${fieldType}[]`, line: fieldLine });
                            k += 5;
                            if (k < tok.length && tok[k].type === 'AT') {
                                k++;
                                if (k < tok.length && tok[k].type === 'IDENTIFIER')
                                    k++;
                            }
                            if (k < tok.length && tok[k].type === 'ASSIGN') {
                                k++;
                                while (k < tok.length && tok[k].type !== 'LBRACE' && tok[k].type !== 'RBRACE' && tok[k].type !== 'FN')
                                    k++;
                            }
                        }
                        // Check for scalar type: TYPE NAME
                        else if (typeTokenTypes.has(tok[k].type) && k + 1 < tok.length && tok[k + 1].type === 'IDENTIFIER' && tok[k + 1].lexeme !== '(') {
                            const fieldType = tok[k].lexeme;
                            k++;
                            const fieldName = tok[k].lexeme;
                            const fieldLine = tok[k].line;
                            symbols.push({ name: fieldName, kind: 'field', type_name: fieldType, line: fieldLine, col: tok[k].col, parent: structName });
                            structObj.fields.push({ name: fieldName, type: fieldType, line: fieldLine });
                            k++;
                            if (k < tok.length && tok[k].type === 'AT') {
                                k++;
                                if (k < tok.length && tok[k].type === 'IDENTIFIER')
                                    k++;
                            }
                            if (k < tok.length && tok[k].type === 'ASSIGN') {
                                k++;
                                while (k < tok.length && tok[k].type !== 'LBRACE' && tok[k].type !== 'RBRACE' && tok[k].type !== 'FN')
                                    k++;
                            }
                        }
                        // Check for pointer type: * TYPE NAME  (e.g. *u8 data, *Player ref)
                        else if (tok[k].type === 'STAR' && k + 2 < tok.length && (typeTokenTypes.has(tok[k + 1].type) || BUILTIN_TYPES.has(tok[k + 1].lexeme.toLowerCase())) && tok[k + 2].type === 'IDENTIFIER' && tok[k + 2].lexeme !== '(') {
                            const ptrType = '*' + tok[k + 1].lexeme;
                            const fieldName = tok[k + 2].lexeme;
                            const fieldLine = tok[k + 2].line;
                            symbols.push({ name: fieldName, kind: 'field', type_name: ptrType, line: fieldLine, col: tok[k + 2].col, parent: structName });
                            structObj.fields.push({ name: fieldName, type: ptrType, line: fieldLine });
                            k += 3;
                            if (k < tok.length && tok[k].type === 'AT') {
                                k++;
                                if (k < tok.length && tok[k].type === 'IDENTIFIER')
                                    k++;
                            }
                            if (k < tok.length && tok[k].type === 'ASSIGN') {
                                k++;
                                while (k < tok.length && tok[k].type !== 'LBRACE' && tok[k].type !== 'RBRACE' && tok[k].type !== 'FN')
                                    k++;
                            }
                        }
                        // Check for pointer array: * TYPE [ NUMBER ] NAME (e.g. *Player[10] team)
                        else if (tok[k].type === 'STAR' && k + 4 < tok.length &&
                            (typeTokenTypes.has(tok[k + 1].type) || BUILTIN_TYPES.has(tok[k + 1].lexeme.toLowerCase())) &&
                            tok[k + 2].type === 'LBRACKET' &&
                            (tok[k + 3].type === 'INT_LITERAL' || tok[k + 3].type === 'IDENTIFIER') &&
                            tok[k + 4].type === 'RBRACKET' &&
                            k + 5 < tok.length &&
                            tok[k + 5].type === 'IDENTIFIER' &&
                            tok[k + 5].lexeme !== '(') {
                            const ptrType = '*' + tok[k + 1].lexeme;
                            const fieldName = tok[k + 5].lexeme;
                            const fieldLine = tok[k + 5].line;
                            symbols.push({ name: fieldName, kind: 'field', type_name: `${ptrType}[]`, line: fieldLine, col: tok[k + 5].col, parent: structName });
                            structObj.fields.push({ name: fieldName, type: `${ptrType}[]`, line: fieldLine });
                            k += 6;
                            if (k < tok.length && tok[k].type === 'AT') {
                                k++;
                                if (k < tok.length && tok[k].type === 'IDENTIFIER')
                                    k++;
                            }
                            if (k < tok.length && tok[k].type === 'ASSIGN') {
                                k++;
                                while (k < tok.length && tok[k].type !== 'LBRACE' && tok[k].type !== 'RBRACE' && tok[k].type !== 'FN')
                                    k++;
                            }
                        }
                        else {
                            k++;
                        }
                    }
                }
            }
            structs.set(structName, structObj);
            i = structEndIdx >= 0 ? structEndIdx + 1 : j + 1;
            continue;
        }
        // Interface declaration: interface NAME { ... }
        if (t.type === 'INTERFACE' && i + 1 < tok.length && tok[i + 1].type === 'IDENTIFIER') {
            const ifaceName = tok[i + 1].lexeme;
            symbols.push({ name: ifaceName, kind: 'interface', type_name: 'interface', line: t.line, col: t.col });
            i += 2;
            while (i < tok.length && tok[i].type !== 'LBRACE')
                i++;
            if (i < tok.length && tok[i].type === 'LBRACE') {
                let braceCount = 1;
                i++;
                while (i < tok.length && braceCount > 0) {
                    if (tok[i].type === 'LBRACE')
                        braceCount++;
                    if (tok[i].type === 'RBRACE')
                        braceCount--;
                    if (braceCount > 0)
                        i++;
                }
                i++;
            }
            continue;
        }
        // Extern function: extern fn NAME ( params ) -> Type  (no body expected for C interop)
        // Also handle: extern fn NAME ( params ) -> Type { ... } (with body for wrapping)
        if (t.type === 'EXTERN' && i + 1 < tok.length && tok[i + 1].type === 'FN' && i + 2 < tok.length && tok[i + 2].type === 'IDENTIFIER') {
            const fnName = tok[i + 2].lexeme;
            const fnLine = t.line;
            let j = i + 3;
            // Parameters
            const params = [];
            if (j < tok.length && tok[j].type === 'LPAREN') {
                j++;
                let paramDepth = 1;
                while (j < tok.length && paramDepth > 0) {
                    if (tok[j].type === 'LPAREN')
                        paramDepth++;
                    if (tok[j].type === 'RPAREN')
                        paramDepth--;
                    if (paramDepth > 0 && tok[j].type === 'IDENTIFIER' && j > 0 && tok[j - 1].type !== 'DOT') {
                        if (j >= 2 && (tok[j - 1].type === 'IDENTIFIER' || BUILTIN_TYPES.has(tok[j - 1].lexeme.toLowerCase()) || tok[j - 1].lexeme === '*')) {
                            params.push(tok[j].lexeme);
                        }
                    }
                    j++;
                }
            }
            let returnType = 'void';
            if (j < tok.length && tok[j].type === 'ARROW' && j + 1 < tok.length) {
                j++;
                if (tok[j].type === 'IDENTIFIER' || BUILTIN_TYPES.has(tok[j].lexeme.toLowerCase()) || tok[j].lexeme === '*') {
                    // Handle *T return type
                    if (tok[j].lexeme === '*' && j + 1 < tok.length) {
                        returnType = '*' + tok[j + 1].lexeme;
                        j += 2;
                    }
                    else {
                        returnType = tok[j].lexeme;
                        j++;
                    }
                }
            }
            symbols.push({
                name: fnName,
                kind: 'function',
                type_name: returnType,
                line: fnLine,
                col: t.col,
            });
            for (const p of params) {
                symbols.push({
                    name: p,
                    kind: 'param',
                    type_name: '',
                    line: fnLine,
                    col: 0,
                    parent: fnName,
                });
            }
            // Skip optional body
            i = j;
            if (i < tok.length && tok[i].type === 'LBRACE') {
                let braceCount = 1;
                i++;
                while (i < tok.length && braceCount > 0) {
                    if (tok[i].type === 'LBRACE')
                        braceCount++;
                    if (tok[i].type === 'RBRACE')
                        braceCount--;
                    if (braceCount > 0)
                        i++;
                }
                i++;
            }
            continue;
        }
        // Top-level function: fn NAME ( params ) -> Type { ... }
        if (t.type === 'FN' && i + 1 < tok.length && tok[i + 1].type === 'IDENTIFIER') {
            const fnName = tok[i + 1].lexeme;
            const fnLine = t.line;
            let j = i + 2;
            // Parameters
            const params = [];
            if (j < tok.length && tok[j].type === 'LPAREN') {
                j++;
                let paramDepth = 1;
                while (j < tok.length && paramDepth > 0) {
                    if (tok[j].type === 'LPAREN')
                        paramDepth++;
                    if (tok[j].type === 'RPAREN')
                        paramDepth--;
                    if (paramDepth > 0 && tok[j].type === 'IDENTIFIER' && j > 0 && tok[j - 1].type !== 'DOT') {
                        if (j >= 2 && (tok[j - 1].type === 'IDENTIFIER' || BUILTIN_TYPES.has(tok[j - 1].lexeme.toLowerCase()))) {
                            params.push(tok[j].lexeme);
                        }
                    }
                    j++;
                }
            }
            let returnType = 'void';
            if (j < tok.length && tok[j].type === 'ARROW' && j + 1 < tok.length) {
                j++;
                if (tok[j].type === 'IDENTIFIER' || BUILTIN_TYPES.has(tok[j].lexeme.toLowerCase())) {
                    returnType = tok[j].lexeme;
                    j++;
                }
            }
            const isCtor = structs.has(fnName);
            symbols.push({
                name: fnName,
                kind: fnName === 'main' ? 'function' : (isCtor ? 'constructor' : 'function'),
                type_name: returnType,
                line: fnLine,
                col: t.col,
            });
            // Add params as symbols
            for (const p of params) {
                const paramToken = tok.find(tk => tk.type === 'IDENTIFIER' && tk.lexeme === p && tk.line >= fnLine && tk.line <= fnLine + 5);
                symbols.push({
                    name: p,
                    kind: 'param',
                    type_name: '',
                    line: paramToken?.line || fnLine,
                    col: paramToken?.col || 0,
                    parent: fnName,
                });
            }
            // Skip body
            i = j;
            if (i < tok.length && tok[i].type === 'LBRACE') {
                let braceCount = 1;
                i++;
                while (i < tok.length && braceCount > 0) {
                    if (tok[i].type === 'LBRACE')
                        braceCount++;
                    if (tok[i].type === 'RBRACE')
                        braceCount--;
                    if (braceCount > 0)
                        i++;
                }
                i++;
            }
            continue;
        }
        i++;
    }
    // Find variables in function bodies
    // Patterns: TYPE NAME, TYPE NAME = ..., TYPE[N] NAME, TYPE NAME @block
    for (let idx = 0; idx < tokens.length; idx++) {
        const t = tokens[idx];
        // Helper to check if a token is a type keyword (built-in or fixed-width)
        const isTypeKeyword = (tok) => {
            return BUILTIN_TYPES.has(tok.lexeme) && KEYWORDS.has(tok.lexeme);
        };
        // Helper to check if a token is a user-defined type (PascalCase identifier)
        const isUserType = (tok) => {
            return tok.type === 'IDENTIFIER' && /^[A-Z]/.test(tok.lexeme) && !KEYWORDS.has(tok.lexeme);
        };
        const isVarNameToken = (tok) => {
            return tok.type === 'IDENTIFIER' && tok.lexeme !== '(';
        };
        // Pattern 1: TYPE NAME  (keyword type, e.g. int x, u8 y, float z)
        if (isTypeKeyword(t) && idx + 1 < tokens.length && isVarNameToken(tokens[idx + 1])) {
            const varName = tokens[idx + 1].lexeme;
            if (!symbols.find(s => s.name === varName && (s.kind === 'field' || s.kind === 'param' || s.kind === 'variable'))) {
                symbols.push({
                    name: varName,
                    kind: 'variable',
                    type_name: t.lexeme,
                    line: t.line,
                    col: t.col,
                });
            }
            continue;
        }
        // Pattern 2: USER_TYPE NAME  (PascalCase user-defined type, e.g. Player p)
        if (isUserType(t) && idx + 1 < tokens.length && isVarNameToken(tokens[idx + 1])) {
            const varName = tokens[idx + 1].lexeme;
            if (!symbols.find(s => s.name === varName && (s.kind === 'field' || s.kind === 'param' || s.kind === 'variable'))) {
                symbols.push({
                    name: varName,
                    kind: 'variable',
                    type_name: t.lexeme,
                    line: t.line,
                    col: t.col,
                });
            }
            continue;
        }
        // Pattern 3: TYPE [ NUMBER ] NAME  (array type, e.g. int[10] arr, u8[256] buf)
        if (isTypeKeyword(t) && idx + 3 < tokens.length &&
            tokens[idx + 1].type === 'LBRACKET' &&
            (tokens[idx + 2].type === 'INT_LITERAL' || tokens[idx + 2].type === 'IDENTIFIER') &&
            tokens[idx + 3].type === 'RBRACKET' &&
            idx + 4 < tokens.length && isVarNameToken(tokens[idx + 4])) {
            const varName = tokens[idx + 4].lexeme;
            if (!symbols.find(s => s.name === varName && (s.kind === 'field' || s.kind === 'param' || s.kind === 'variable'))) {
                symbols.push({
                    name: varName,
                    kind: 'variable',
                    type_name: `${t.lexeme}[]`,
                    line: t.line,
                    col: t.col,
                });
            }
            continue;
        }
        // Pattern 4: USER_TYPE [ NUMBER ] NAME  (e.g. Player[10] team)
        if (isUserType(t) && idx + 3 < tokens.length &&
            tokens[idx + 1].type === 'LBRACKET' &&
            (tokens[idx + 2].type === 'INT_LITERAL' || tokens[idx + 2].type === 'IDENTIFIER') &&
            tokens[idx + 3].type === 'RBRACKET' &&
            idx + 4 < tokens.length && isVarNameToken(tokens[idx + 4])) {
            const varName = tokens[idx + 4].lexeme;
            if (!symbols.find(s => s.name === varName && (s.kind === 'field' || s.kind === 'param' || s.kind === 'variable'))) {
                symbols.push({
                    name: varName,
                    kind: 'variable',
                    type_name: `${t.lexeme}[]`,
                    line: t.line,
                    col: t.col,
                });
            }
            continue;
        }
        // Pattern 5: * TYPE NAME  (pointer type, e.g. *u8 ptr, *void data, *Player ref)
        if (t.type === 'STAR' && idx + 2 < tokens.length &&
            (isTypeKeyword(tokens[idx + 1]) || isUserType(tokens[idx + 1])) &&
            isVarNameToken(tokens[idx + 2])) {
            const ptrType = tokens[idx + 1].lexeme;
            const varName = tokens[idx + 2].lexeme;
            if (!symbols.find(s => s.name === varName && (s.kind === 'field' || s.kind === 'param' || s.kind === 'variable'))) {
                symbols.push({
                    name: varName,
                    kind: 'variable',
                    type_name: `*${ptrType}`,
                    line: t.line,
                    col: t.col,
                });
            }
            continue;
        }
        // Pattern 6: * TYPE [ NUMBER ] NAME  (pointer array, e.g. *Player[10] team)
        if (t.type === 'STAR' && idx + 5 < tokens.length &&
            (isTypeKeyword(tokens[idx + 1]) || isUserType(tokens[idx + 1])) &&
            tokens[idx + 2].type === 'LBRACKET' &&
            (tokens[idx + 3].type === 'INT_LITERAL' || tokens[idx + 3].type === 'IDENTIFIER') &&
            tokens[idx + 4].type === 'RBRACKET' &&
            isVarNameToken(tokens[idx + 5])) {
            const ptrType = tokens[idx + 1].lexeme;
            const varName = tokens[idx + 5].lexeme;
            if (!symbols.find(s => s.name === varName && (s.kind === 'field' || s.kind === 'param' || s.kind === 'variable'))) {
                symbols.push({
                    name: varName,
                    kind: 'variable',
                    type_name: `*${ptrType}[]`,
                    line: t.line,
                    col: t.col,
                });
            }
            continue;
        }
    }
    return { tokens, symbols, blocks, structs, errors, packageName, usings };
}
function getLinePrefix(text, line, col) {
    const lines = text.split('\n');
    if (line >= lines.length)
        return '';
    return lines[line].substring(0, col);
}
function getWordAtPosition(text, line, col) {
    const lines = text.split('\n');
    if (line >= lines.length)
        return '';
    const l = lines[line];
    let start = col;
    let end = col;
    while (start > 0 && /[a-zA-Z0-9_]/.test(l[start - 1]))
        start--;
    while (end < l.length && /[a-zA-Z0-9_]/.test(l[end]))
        end++;
    return l.substring(start, end);
}
function getCurrentLine(text, line) {
    const lines = text.split('\n');
    return line < lines.length ? lines[line] : '';
}
function getCompletionContext(text, line, col, triggerChar) {
    const prefix = getLinePrefix(text, line, col);
    const word = getWordAtPosition(text, line, col);
    // Check for dot before cursor
    const dotMatch = prefix.match(/\.([a-zA-Z0-9_]*)$/);
    const atMatch = prefix.match(/@([a-zA-Z0-9_]*)$/);
    const colonMatch = prefix.match(/:([a-zA-Z0-9_]*)$/);
    // Check if we're after a keyword
    const keywordMatch = prefix.match(/\b(package|using|private|public|struct|extends|interface|fn|block|if|while|for|extern|include|link)\s+([a-zA-Z0-9_]*)$/);
    // Check if we're after a type
    const typeMatch = prefix.match(/\b(u(?:8|16|32|64)|i(?:8|16|32|64)|f(?:32|64)|usize|isize|byte|int|float|bool|char|String|void)\s+([a-zA-Z0-9_]*)$/);
    return {
        prefix,
        triggerKind: triggerChar ? 2 : 1,
        triggerChar,
        isAfterDot: dotMatch !== null,
        isAfterAt: atMatch !== null,
        isAfterColon: colonMatch !== null,
        isAfterKeyword: keywordMatch ? keywordMatch[1] : null,
        isAfterType: typeMatch !== null,
        currentWord: word,
        linePrefix: prefix,
    };
}
function getKeywordDoc(keyword) {
    return KEYWORD_DOCS[keyword];
}
function getBlockNames(scan) {
    return scan.blocks.map(b => b.name);
}
//# sourceMappingURL=languageService.js.map