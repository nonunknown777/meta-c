import {
    createConnection,
    TextDocuments,
    InitializeParams,
    InitializeResult,
    CompletionItem,
    CompletionItemKind,
    TextDocumentPositionParams,
    Hover,
    HoverParams,
    DefinitionParams,
    Location,
    Range,
    Diagnostic,
    DiagnosticSeverity,
    TextDocumentChangeEvent,
    SignatureHelpParams,
    SignatureHelp,
    SignatureInformation,
    ParameterInformation,
    DocumentSymbolParams,
    DocumentSymbol,
    SymbolKind,
    SemanticTokensParams,
    SemanticTokens,
    SemanticTokensLegend,
    CompletionParams,
} from 'vscode-languageserver/node';
import { TextDocument } from 'vscode-languageserver-textdocument';
import { runCompiler, CompilerResult } from './compilerService';
import { existsSync } from 'fs';
import {
    scanDocument,
    ScanResult,
    getCompletionContext,
    getKeywordDoc,
    getWordAtPosition,
    getCurrentLine,
} from './languageService';

const connection = createConnection();
const documents = new TextDocuments(TextDocument);

const compilerResults = new Map<string, CompilerResult>();
const scanCache = new Map<string, ScanResult>();

const tokenTypes = [
    'keyword', 'type', 'class', 'interface', 'function', 'method',
    'property', 'variable', 'string', 'number', 'comment', 'operator',
    'parameter', 'struct', 'constant',
];
const tokenModifiers = ['declaration', 'definition', 'readonly', 'static', 'deprecated', 'abstract'];

const legend: SemanticTokensLegend = {
    tokenTypes: tokenTypes,
    tokenModifiers: tokenModifiers,
};

const typeKeywordSet = new Set(['int', 'float', 'bool', 'char', 'String', 'void', 'PoolAllocator', 'u8', 'u16', 'u32', 'u64', 'i8', 'i16', 'i32', 'i64', 'f32', 'f64', 'usize', 'isize', 'byte']);
const keywordSet = new Set([
    'package', 'using', 'private', 'public',
    'struct', 'extends', 'interface', 'fn', 'return',
    'block', 'reset',
    'if', 'else', 'while', 'for', 'error',
    'null', 'true', 'false',
    'include', 'link', 'extern', 'and',
]);

const keywordCompletions: CompletionItem[] = [
    { label: 'package', kind: CompletionItemKind.Keyword, detail: 'package declaration', insertText: 'package ', insertTextFormat: 1 },
    { label: 'using', kind: CompletionItemKind.Keyword, detail: 'import package', insertText: 'using ', insertTextFormat: 1 },
    { label: 'private', kind: CompletionItemKind.Keyword, detail: 'private visibility' },
    { label: 'public', kind: CompletionItemKind.Keyword, detail: 'public visibility' },
    { label: 'struct', kind: CompletionItemKind.Keyword, detail: 'struct declaration' },
    { label: 'extends', kind: CompletionItemKind.Keyword, detail: 'inherit from struct' },
    { label: 'interface', kind: CompletionItemKind.Keyword, detail: 'interface declaration' },
    { label: 'fn', kind: CompletionItemKind.Keyword, detail: 'function declaration' },
    { label: 'return', kind: CompletionItemKind.Keyword, detail: 'return from function' },
    { label: 'block', kind: CompletionItemKind.Keyword, detail: 'memory block declaration' },
    { label: 'reset', kind: CompletionItemKind.Keyword, detail: 'reset memory block' },
    { label: 'if', kind: CompletionItemKind.Keyword, detail: 'if condition { }' },
    { label: 'else', kind: CompletionItemKind.Keyword, detail: 'else { }' },
    { label: 'while', kind: CompletionItemKind.Keyword, detail: 'while condition { }' },
    { label: 'for', kind: CompletionItemKind.Keyword, detail: 'for init; cond; inc { }' },
    { label: 'error', kind: CompletionItemKind.Keyword, detail: 'panic with message' },
    { label: 'int', kind: CompletionItemKind.Keyword, detail: '64-bit integer type' },
    { label: 'float', kind: CompletionItemKind.Keyword, detail: '64-bit float type' },
    { label: 'bool', kind: CompletionItemKind.Keyword, detail: 'boolean type' },
    { label: 'char', kind: CompletionItemKind.Keyword, detail: 'character type' },
    { label: 'String', kind: CompletionItemKind.Keyword, detail: 'dynamic string type' },
    { label: 'void', kind: CompletionItemKind.Keyword, detail: 'void type' },
    { label: 'u8', kind: CompletionItemKind.Keyword, detail: 'unsigned 8-bit integer' },
    { label: 'u16', kind: CompletionItemKind.Keyword, detail: 'unsigned 16-bit integer' },
    { label: 'u32', kind: CompletionItemKind.Keyword, detail: 'unsigned 32-bit integer' },
    { label: 'u64', kind: CompletionItemKind.Keyword, detail: 'unsigned 64-bit integer' },
    { label: 'i8', kind: CompletionItemKind.Keyword, detail: 'signed 8-bit integer' },
    { label: 'i16', kind: CompletionItemKind.Keyword, detail: 'signed 16-bit integer' },
    { label: 'i32', kind: CompletionItemKind.Keyword, detail: 'signed 32-bit integer' },
    { label: 'i64', kind: CompletionItemKind.Keyword, detail: 'signed 64-bit integer' },
    { label: 'f32', kind: CompletionItemKind.Keyword, detail: '32-bit float' },
    { label: 'f64', kind: CompletionItemKind.Keyword, detail: '64-bit float' },
    { label: 'usize', kind: CompletionItemKind.Keyword, detail: 'pointer-sized unsigned integer' },
    { label: 'isize', kind: CompletionItemKind.Keyword, detail: 'pointer-sized signed integer' },
    { label: 'byte', kind: CompletionItemKind.Keyword, detail: '8-bit byte' },
    { label: 'null', kind: CompletionItemKind.Constant, detail: 'null literal' },
    { label: 'true', kind: CompletionItemKind.Constant, detail: 'boolean true' },
    { label: 'false', kind: CompletionItemKind.Constant, detail: 'boolean false' },
    { label: 'include', kind: CompletionItemKind.Keyword, detail: 'include C header', insertText: 'include "${1:header.h}"', insertTextFormat: 2 },
    { label: 'link', kind: CompletionItemKind.Keyword, detail: 'link C library', insertText: 'link ${1:libname}', insertTextFormat: 2 },
    { label: 'extern', kind: CompletionItemKind.Keyword, detail: 'declare external C function', insertText: 'extern fn ${1:name}(${2:params}) -> ${3:ret}', insertTextFormat: 2 },
    { label: 'and', kind: CompletionItemKind.Keyword, detail: 'connects include and link' },
];

const runtimeFunctionCompletions: CompletionItem[] = [
    { label: 'block_set_tls', kind: CompletionItemKind.Function, detail: '(BlockCtx* ctx) → void', insertText: 'block_set_tls(${1:ctx})', insertTextFormat: 2 },
    { label: 'block_get_tls', kind: CompletionItemKind.Function, detail: '() → BlockCtx*', insertText: 'block_get_tls()', insertTextFormat: 1 },
    { label: 'block_alloc_tls', kind: CompletionItemKind.Function, detail: '(usize size) → void*', insertText: 'block_alloc_tls(${1:size})', insertTextFormat: 2 },
    { label: 'pool_create', kind: CompletionItemKind.Function, detail: '(usize size, usize slot_size) → PoolAllocator*', insertText: 'pool_create(${1:size}, ${2:slot_size})', insertTextFormat: 2 },
    { label: 'pool_add_slot', kind: CompletionItemKind.Function, detail: '(PoolAllocator* pool, void* data) → void', insertText: 'pool_add_slot(${1:pool}, ${2:data})', insertTextFormat: 2 },
    { label: 'pool_alloc', kind: CompletionItemKind.Function, detail: '(PoolAllocator* pool) → void*', insertText: 'pool_alloc(${1:pool})', insertTextFormat: 2 },
    { label: 'pool_free', kind: CompletionItemKind.Function, detail: '(PoolAllocator* pool, void* ptr) → void', insertText: 'pool_free(${1:pool}, ${2:ptr})', insertTextFormat: 2 },
    { label: 'pool_destroy', kind: CompletionItemKind.Function, detail: '(PoolAllocator* pool) → void', insertText: 'pool_destroy(${1:pool})', insertTextFormat: 2 },
    { label: 'block_enable_double_buffer', kind: CompletionItemKind.Function, detail: '(BlockCtx* ctx) → void', insertText: 'block_enable_double_buffer(${1:ctx})', insertTextFormat: 2 },
    { label: 'block_swap_buffers', kind: CompletionItemKind.Function, detail: '(BlockCtx* ctx) → void', insertText: 'block_swap_buffers(${1:ctx})', insertTextFormat: 2 },
    { label: 'block_alloc_db', kind: CompletionItemKind.Function, detail: '(BlockCtx* ctx, usize size) → void*', insertText: 'block_alloc_db(${1:ctx}, ${2:size})', insertTextFormat: 2 },
];

const snippetCompletions: CompletionItem[] = [
    { label: 'struct', kind: CompletionItemKind.Snippet, detail: 'struct declaration', insertText: 'struct $1 {\n\t$0\n}', insertTextFormat: 2 },
    { label: 'fn', kind: CompletionItemKind.Snippet, detail: 'function declaration', insertText: 'fn $1($2)$3 {\n\t$0\n}', insertTextFormat: 2 },
    { label: 'if', kind: CompletionItemKind.Snippet, detail: 'if statement', insertText: 'if $1 {\n\t$0\n}', insertTextFormat: 2 },
    { label: 'while', kind: CompletionItemKind.Snippet, detail: 'while loop', insertText: 'while $1 {\n\t$0\n}', insertTextFormat: 2 },
    { label: 'for', kind: CompletionItemKind.Snippet, detail: 'for loop', insertText: 'for $1 $2 = $3; $4; $5 {\n\t$0\n}', insertTextFormat: 2 },
    { label: 'block', kind: CompletionItemKind.Snippet, detail: 'block declaration', insertText: 'block $1 = ${2:64}MB', insertTextFormat: 2 },
    { label: 'block-scope', kind: CompletionItemKind.Snippet, detail: 'block scope', insertText: 'block $1 {\n\t$0\n}', insertTextFormat: 2 },
    { label: 'interface', kind: CompletionItemKind.Snippet, detail: 'interface declaration', insertText: 'interface $1 {\n\t$0\n}', insertTextFormat: 2 },
    { label: 'package', kind: CompletionItemKind.Snippet, detail: 'package declaration', insertText: 'package $1', insertTextFormat: 2 },
    { label: 'using', kind: CompletionItemKind.Snippet, detail: 'using declaration', insertText: 'using $1', insertTextFormat: 2 },
    { label: 'main', kind: CompletionItemKind.Snippet, detail: 'main function', insertText: 'fn main() {\n\t$0\n}', insertTextFormat: 2 },
];

function getFilePath(uri: string): string {
    return uri.replace(/^file:\/\//, '');
}

function isBrickFile(uri: string): boolean {
    return uri.endsWith('.brc') || uri.includes('file:');
}

function runFastScanner(doc: TextDocument): ScanResult {
    const cached = scanCache.get(doc.uri);
    if (cached) return cached;
    const result = scanDocument(doc.getText());
    scanCache.set(doc.uri, result);
    return result;
}

function invalidateCache(uri: string) {
    scanCache.delete(uri);
    compilerResults.delete(uri);
}

function runCompilerOnDoc(doc: TextDocument): CompilerResult {
    const filePath = getFilePath(doc.uri);
    if (!existsSync(filePath)) {
        const result: CompilerResult = {
            success: false,
            output: { tokens: [], symbols: [], errors: [{ message: `File not found: ${filePath}`, line: 0, col: 0, file: filePath, severity: 1 }] },
            raw: '',
        };
        compilerResults.set(doc.uri, result);
        return result;
    }
    const result = runCompiler(filePath);
    compilerResults.set(doc.uri, result);
    return result;
}

function computeDiagnostics(doc: TextDocument): Diagnostic[] {
    const diagnostics: Diagnostic[] = [];
    const fastResult = runFastScanner(doc);

    // Quick syntax-level errors from fast scanner
    for (const err of fastResult.errors) {
        diagnostics.push({
            severity: DiagnosticSeverity.Error,
            range: Range.create(
                Math.max(0, err.line - 1),
                Math.max(0, err.col - 1),
                Math.max(0, err.line - 1),
                err.col
            ),
            message: err.message,
            source: 'brick',
        });
    }

    // Merge compiler errors (more accurate but slower)
    const compilerResult = compilerResults.get(doc.uri);
    if (compilerResult) {
        for (const err of compilerResult.output.errors) {
            const severity = err.severity === 1 ? DiagnosticSeverity.Error
                : err.severity === 2 ? DiagnosticSeverity.Warning
                    : DiagnosticSeverity.Information;
            diagnostics.push({
                severity,
                range: Range.create(
                    Math.max(0, err.line - 1),
                    Math.max(0, err.col - 1),
                    Math.max(0, err.line - 1),
                    err.col
                ),
                message: err.message,
                source: 'brick',
            });
        }
    }

    return diagnostics;
}

function validateDocument(doc: TextDocument, useCompiler: boolean = false) {
    invalidateCache(doc.uri);
    runFastScanner(doc);

    if (useCompiler) {
        runCompilerOnDoc(doc);
    }

    const diagnostics = computeDiagnostics(doc);
    connection.sendDiagnostics({ uri: doc.uri, diagnostics });
}

// Debounce compiler calls
let compileTimer: ReturnType<typeof setTimeout> | null = null;
const DEBOUNCE_MS = 800;

function debouncedCompilerRun(doc: TextDocument) {
    if (compileTimer) clearTimeout(compileTimer);
    compileTimer = setTimeout(() => {
        compileTimer = null;
        runCompilerOnDoc(doc);
        const diagnostics = computeDiagnostics(doc);
        connection.sendDiagnostics({ uri: doc.uri, diagnostics });
    }, DEBOUNCE_MS);
}

connection.onInitialize((params: InitializeParams): InitializeResult => {
    return {
        capabilities: {
            textDocumentSync: {
                openClose: true,
                change: 2,
            },
            completionProvider: {
                triggerCharacters: ['.', '@', ' ', ':'],
                completionItem: {
                    labelDetailsSupport: true,
                },
            },
            hoverProvider: true,
            definitionProvider: true,
            signatureHelpProvider: {
                triggerCharacters: ['(', ','],
            },
            documentSymbolProvider: true,
            semanticTokensProvider: {
                legend,
                full: true,
                range: false,
            },
        },
    };
});

// ─── Document Sync ────────────────────────────────────────

documents.onDidOpen((event: TextDocumentChangeEvent<TextDocument>) => {
    validateDocument(event.document, true);
});

documents.onDidChangeContent((event: TextDocumentChangeEvent<TextDocument>) => {
    invalidateCache(event.document.uri);
    runFastScanner(event.document);
    debouncedCompilerRun(event.document);
});

documents.onDidClose((event: TextDocumentChangeEvent<TextDocument>) => {
    scanCache.delete(event.document.uri);
    compilerResults.delete(event.document.uri);
});

// ─── Completions ──────────────────────────────────────────

connection.onCompletion((params: CompletionParams): CompletionItem[] => {
    const doc = documents.get(params.textDocument.uri);
    if (!doc) return [];

    const text = doc.getText();
    const line = params.position.line;
    const col = params.position.character;
    const scan = runFastScanner(doc);
    const context = getCompletionContext(text, line, col, params.context?.triggerCharacter);

    const items: CompletionItem[] = [];

    // Context-aware completions
    if (context.isAfterDot) {
        const linePrefix = context.linePrefix;
        const objMatch = linePrefix.match(/\.\s*([a-zA-Z_][a-zA-Z0-9_]*)?$/);
        const objWord = linePrefix.match(/([a-zA-Z_][a-zA-Z0-9_]*)\.\s*[a-zA-Z0-9_]*$/);
        if (objWord) {
            const objName = objWord[1];
            // Check if it's a block (for .reset())
            if (scan.blocks.find(b => b.name === objName)) {
                items.push({
                    label: 'reset',
                    kind: CompletionItemKind.Method,
                    detail: '() → void',
                    insertText: 'reset()',
                    insertTextFormat: 1,
                });
            }
            // Check if it's a struct name or variable of struct type
            const structType = scan.structs.get(objName);
            if (structType) {
                for (const f of structType.fields) {
                    items.push({ label: f.name, kind: CompletionItemKind.Field, detail: `${f.type}` });
                }
                for (const m of structType.methods) {
                    const params = m.params.join(', ');
                    items.push({ label: m.name, kind: CompletionItemKind.Method, detail: `(${params}) → ${m.return_type}` });
                }
            } else {
                // Variable of struct type
                const varSym = scan.symbols.find(s => s.name === objName && s.type_name && scan.structs.has(s.type_name));
                if (varSym) {
                    const st = scan.structs.get(varSym.type_name);
                    if (st) {
                        for (const f of st.fields) {
                            items.push({ label: f.name, kind: CompletionItemKind.Field, detail: `${f.type}` });
                        }
                        for (const m of st.methods) {
                            const params = m.params.join(', ');
                            items.push({ label: m.name, kind: CompletionItemKind.Method, detail: `(${params}) → ${m.return_type}` });
                        }
                    }
                }
            }
        }
        return items;
    }

    if (context.isAfterAt) {
        for (const b of scan.blocks) {
            items.push({ label: b.name, kind: CompletionItemKind.Struct, detail: `memory block` });
        }
        return items;
    }

    if (context.isAfterKeyword) {
        const kw = context.isAfterKeyword;
        if (kw === 'struct' || kw === 'interface') {
            items.push({ label: 'MyName', kind: CompletionItemKind.Class, detail: 'PascalCase name' });
            return items;
        }
        if (kw === 'extends') {
            for (const [, s] of scan.structs) {
                items.push({ label: s.name, kind: CompletionItemKind.Class, detail: 'struct' });
            }
            return items;
        }
        if (kw === 'using') {
            items.push({ label: 'IO', kind: CompletionItemKind.Module, detail: 'Standard I/O package (print)' });
            return items;
        }
        if (kw === 'block') {
            items.push({ label: 'name', kind: CompletionItemKind.Variable, detail: 'block name = SIZE UNIT (e.g. 64MB)' });
            return items;
        }
        if (kw === 'private' || kw === 'public') {
            for (const t of ['fn', 'int', 'float', 'bool', 'char', 'String', 'void', 'u8', 'u16', 'u32', 'u64', 'i8', 'i16', 'i32', 'i64', 'f32', 'f64', 'usize', 'isize', 'byte']) {
                items.push({ label: t, kind: CompletionItemKind.Keyword, detail: `${kw} ${t}` });
            }
            return items;
        }
        if (kw === 'package') {
            items.push({ label: 'PACKAGE_NAME', kind: CompletionItemKind.Module, detail: 'package name in UPPER case' });
            return items;
        }
        if (kw === 'extern') {
            items.push({ label: 'fn', kind: CompletionItemKind.Keyword, detail: 'extern fn declaration', insertText: 'fn ${1:name}(${2:params}) -> ${3:ret}', insertTextFormat: 2 });
            return items;
        }
        if (kw === 'include') {
            items.push({ label: '"header.h"', kind: CompletionItemKind.Text, detail: 'header file path', insertText: '"${1:header.h}"', insertTextFormat: 2 });
            return items;
        }
        if (kw === 'link') {
            items.push({ label: 'libname', kind: CompletionItemKind.Text, detail: 'library name (e.g. m, SDL2, pthread)', insertText: '${1:libname}', insertTextFormat: 2 });
            return items;
        }
    }

    // Default: keywords + runtime functions + snippets + document symbols
    for (const kw of keywordCompletions) {
        if (!context.currentWord || kw.label.startsWith(context.currentWord) || context.currentWord.startsWith(kw.label)) {
            items.push(kw);
        }
    }

    for (const fn of runtimeFunctionCompletions) {
        if (!context.currentWord || fn.label.startsWith(context.currentWord) || context.currentWord.startsWith(fn.label)) {
            items.push(fn);
        }
    }

    for (const snip of snippetCompletions) {
        if (!context.currentWord || snip.label.startsWith(context.currentWord) || context.currentWord.startsWith(snip.label)) {
            items.push(snip);
        }
    }

    // Add user-defined symbols
    for (const sym of scan.symbols) {
        let kind: CompletionItemKind = CompletionItemKind.Reference;
        switch (sym.kind) {
            case 'struct': kind = CompletionItemKind.Class; break;
            case 'interface': kind = CompletionItemKind.Interface; break;
            case 'function':
            case 'constructor': kind = CompletionItemKind.Function; break;
            case 'field': kind = CompletionItemKind.Field; break;
            case 'variable': kind = CompletionItemKind.Variable; break;
            case 'param': kind = CompletionItemKind.Variable; break;
            case 'block': kind = CompletionItemKind.Struct; break;
            case 'package': kind = CompletionItemKind.Module; break;
        }
        const exists = items.find(i => i.label === sym.name);
        if (!exists) {
            items.push({
                label: sym.name,
                kind,
                detail: `${sym.kind}${sym.type_name ? `: ${sym.type_name}` : ''}`,
            });
        }
    }

    return items;
});

connection.onCompletionResolve((item: CompletionItem): CompletionItem => {
    return item;
});

// ─── Hover ────────────────────────────────────────────────

connection.onHover((params: HoverParams): Hover | null => {
    const doc = documents.get(params.textDocument.uri);
    if (!doc) return null;

    const text = doc.getText();
    const word = getWordAtPosition(text, params.position.line, params.position.character);
    if (!word) return null;

    // Check keyword docs
    const kwDoc = getKeywordDoc(word);
    if (kwDoc) {
        return {
            contents: {
                kind: 'markdown',
                value: `**\`${word}\`**\n\n${kwDoc}`,
            },
        };
    }

    // Check scanned symbols
    const scan = runFastScanner(doc);
    const sym = scan.symbols.find(s => s.name === word);
    if (sym) {
        const parentInfo = sym.parent ? ` — member of \`${sym.parent}\`` : '';
        return {
            contents: {
                kind: 'markdown',
                value: `**\`${sym.name}\`**\n\n_${sym.kind}_${sym.type_name ? ` — \`${sym.type_name}\`` : ''}${parentInfo}\n\nLine ${sym.line}:${sym.col}`,
            },
        };
    }

    // Check structs
    const structObj = scan.structs.get(word);
    if (structObj) {
        let md = `**\`${structObj.name}\`** _(struct)_\n\n`;
        if (structObj.fields.length > 0) {
            md += '**Fields:**\n';
            for (const f of structObj.fields) {
                md += `- \`${f.name}: ${f.type}\`\n`;
            }
            md += '\n';
        }
        if (structObj.methods.length > 0) {
            md += '**Methods:**\n';
            for (const m of structObj.methods) {
                md += `- \`${m.name}(${m.params.join(', ')}) → ${m.return_type}\`\n`;
            }
        }
        return { contents: { kind: 'markdown', value: md } };
    }

    return null;
});

// ─── Go to Definition ─────────────────────────────────────

connection.onDefinition((params: DefinitionParams): Location | null => {
    const doc = documents.get(params.textDocument.uri);
    if (!doc) return null;

    const text = doc.getText();
    const word = getWordAtPosition(text, params.position.line, params.position.character);
    if (!word) return null;

    const scan = runFastScanner(doc);
    const sym = scan.symbols.find(s => s.name === word);
    if (sym && sym.line > 0) {
        return {
            uri: params.textDocument.uri,
            range: Range.create(
                Math.max(0, sym.line - 1),
                Math.max(0, sym.col - 1),
                Math.max(0, sym.line - 1),
                sym.col + word.length - 1
            ),
        };
    }

    return null;
});

// ─── Signature Help ───────────────────────────────────────

connection.onSignatureHelp((sigParams: SignatureHelpParams): SignatureHelp | null => {
    const doc = documents.get(sigParams.textDocument.uri);
    if (!doc) return null;

    const text = doc.getText();
    const lines = text.split('\n');
    const line = sigParams.position.line;
    const col = sigParams.position.character;
    if (line >= lines.length) return null;

    const currentLine = lines[line];
    const prefix = currentLine.substring(0, col);

    // Find the function name before '('
    const callMatch = prefix.match(/([a-zA-Z_][a-zA-Z0-9_]*)\s*\([^)]*$/);
    if (!callMatch) return null;

    const funcName = callMatch[1];
    const scan = runFastScanner(doc);

    // Look for the function in symbols
    let fnParams: string[] = [];
    let returnType = 'void';
    let found = false;

    // Check struct methods
    for (const [, st] of scan.structs) {
        const method = st.methods.find(m => m.name === funcName);
        if (method) {
            fnParams = method.params;
            returnType = method.return_type;
            found = true;
            break;
        }
    }

    // Check top-level function symbols
    if (!found) {
        const funcSym = scan.symbols.find(s => s.name === funcName && (s.kind === 'function' || s.kind === 'constructor'));
        if (funcSym) {
            const funcLine = getCurrentLine(text, funcSym.line - 1);
            const paramMatch = funcLine.match(/\(([^)]*)\)/);
            if (paramMatch) {
                const raw = paramMatch[1];
                if (raw.trim()) {
                    fnParams = raw.split(',').map(p => {
                        const trimmed = p.trim();
                        const parts = trimmed.split(/\s+/);
                        return parts.length >= 2 ? parts[parts.length - 1] : parts[0];
                    });
                }
            }
            found = true;
        }
    }

    if (!found) {
        if (funcName === 'print') {
            return {
                signatures: [{
                    label: 'print(...values)',
                    documentation: 'Prints values to stdout with newline. Supports format strings: `print("{0} = {1}", x, y)`',
                    parameters: [
                        ParameterInformation.create('values', 'Values or format string with positional args'),
                    ],
                }],
                activeSignature: 0,
                activeParameter: 0,
            };
        }
        if (funcName === 'error') {
            return {
                signatures: [{
                    label: 'error(String message)',
                    documentation: 'Prints message and aborts execution (panic).',
                    parameters: [
                        ParameterInformation.create('message', 'Error message string'),
                    ],
                }],
                activeSignature: 0,
                activeParameter: 0,
            };
        }
        if (funcName === 'reset') {
            return {
                signatures: [{
                    label: 'block.reset()',
                    documentation: 'Resets (frees all allocations in) a memory block.',
                    parameters: [],
                }],
                activeSignature: 0,
                activeParameter: 0,
            };
        }
        return null;
    }

    // Count commas in current call to determine active parameter
    const callText = prefix.substring(prefix.lastIndexOf('('));
    let commaCount = 0;
    let parenDepth = 0;
    for (let i = 0; i < callText.length; i++) {
        if (callText[i] === '(') parenDepth++;
        else if (callText[i] === ')') parenDepth--;
        else if (callText[i] === ',' && parenDepth === 1) commaCount++;
    }

    const sigInfo: SignatureInformation = {
        label: `${funcName}(${fnParams.join(', ')}) → ${returnType}`,
        parameters: fnParams.map(p => ParameterInformation.create(p)),
    };

    return {
        signatures: [sigInfo],
        activeSignature: 0,
        activeParameter: Math.min(commaCount, Math.max(0, fnParams.length - 1)),
    };
});

// ─── Document Symbols ─────────────────────────────────────

connection.onDocumentSymbol((params: DocumentSymbolParams): DocumentSymbol[] => {
    const doc = documents.get(params.textDocument.uri);
    if (!doc) return [];

    const scan = runFastScanner(doc);
    const symbols: DocumentSymbol[] = [];

    // Add package
    if (scan.packageName) {
        symbols.push({
            name: scan.packageName,
            kind: SymbolKind.Module,
            range: Range.create(0, 0, 0, scan.packageName.length),
            selectionRange: Range.create(0, 0, 0, scan.packageName.length),
            detail: 'package',
        });
    }

    // Add blocks
    for (const b of scan.blocks) {
        symbols.push({
            name: b.name,
            kind: SymbolKind.Struct,
            range: Range.create(b.line - 1, 0, b.line - 1, b.name.length),
            selectionRange: Range.create(b.line - 1, 0, b.line - 1, b.name.length),
            detail: 'memory block',
        });
    }

    // Add structs with children
    for (const [, st] of scan.structs) {
        const children: DocumentSymbol[] = [];
        for (const f of st.fields) {
            children.push({
                name: f.name,
                kind: SymbolKind.Field,
                range: Range.create(f.line - 1, 0, f.line - 1, f.name.length),
                selectionRange: Range.create(f.line - 1, 0, f.line - 1, f.name.length),
                detail: f.type,
            });
        }
        for (const m of st.methods) {
            const isCtor = m.name === st.name;
            children.push({
                name: m.name,
                kind: isCtor ? SymbolKind.Constructor : SymbolKind.Method,
                range: Range.create(m.line - 1, 0, m.line - 1, m.name.length),
                selectionRange: Range.create(m.line - 1, 0, m.line - 1, m.name.length),
                detail: `${m.return_type} (${m.params.join(', ')})`,
            });
        }
        symbols.push({
            name: st.name,
            kind: SymbolKind.Class,
            range: Range.create(st.line - 1, 0, st.line - 1, st.name.length),
            selectionRange: Range.create(st.line - 1, 0, st.line - 1, st.name.length),
            children,
        });
    }

    // Add top-level functions
    const topFunctions = scan.symbols.filter(s =>
        (s.kind === 'function' || s.kind === 'constructor') && s.line > 0 && !s.parent
    );
    for (const f of topFunctions) {
        symbols.push({
            name: f.name,
            kind: f.kind === 'constructor' ? SymbolKind.Constructor : SymbolKind.Function,
            range: Range.create(f.line - 1, 0, f.line - 1, f.name.length),
            selectionRange: Range.create(f.line - 1, 0, f.line - 1, f.name.length),
            detail: f.type_name,
        });
    }

    return symbols;
});

// ─── Semantic Tokens ──────────────────────────────────────

connection.languages.semanticTokens.on((params: SemanticTokensParams): SemanticTokens => {
    const doc = documents.get(params.textDocument.uri);
    if (!doc) return { data: [] };

    const text = doc.getText();
    const scan = runFastScanner(doc);
    const data: number[] = [];

    // Build a set of symbol names for quick lookup
    const symbolNames = new Set(scan.symbols.map(s => s.name));
    const structNames = new Set(scan.structs.keys());
    const blockNames = new Set(scan.blocks.map(b => b.name));
    const typeNames = new Set([...structNames, ...typeKeywordSet]);

    let prevLine = 0;
    let prevCol = 0;

    function pushToken(line: number, col: number, length: number, typeIdx: number, modIdx: number = 0) {
        const deltaLine = line - prevLine;
        const deltaCol = deltaLine === 0 ? col - prevCol : col;
        data.push(deltaLine, deltaCol, length, typeIdx, modIdx);
        prevLine = line;
        prevCol = deltaLine === 0 ? col : col;
    }

    for (const tok of scan.tokens) {
        const line = tok.line - 1;
        const col = tok.col - 1;
        const len = tok.lexeme.length;

        switch (tok.type) {
            case 'COMMENT':
                pushToken(line, col, len, 10);
                break;
            case 'STRING_LITERAL':
            case 'CHAR_LITERAL':
                pushToken(line, col, len, 8);
                break;
            case 'INT_LITERAL':
            case 'FLOAT_LITERAL':
                pushToken(line, col, len, 9);
                break;
            case 'PACKAGE':
            case 'USING':
            case 'PRIVATE':
            case 'PUBLIC':
            case 'STRUCT':
            case 'EXTENDS':
            case 'INTERFACE':
            case 'FN':
            case 'RETURN':
            case 'IF':
            case 'ELSE':
            case 'WHILE':
            case 'FOR':
            case 'BLOCK':
            case 'RESET':
            case 'ERROR':
            case 'INCLUDE':
            case 'LINK':
            case 'EXTERN':
            case 'AND':
                pushToken(line, col, len, 0);
                break;
            case 'TRUE':
            case 'FALSE':
            case 'NULL':
                pushToken(line, col, len, 14);
                break;
            case 'IDENTIFIER': {
                if (typeNames.has(tok.lexeme)) {
                    pushToken(line, col, len, 1, 1);
                } else if (blockNames.has(tok.lexeme)) {
                    pushToken(line, col, len, 13);
                } else if (symbolNames.has(tok.lexeme)) {
                    const sym = scan.symbols.find(s => s.name === tok.lexeme);
                    if (sym) {
                        switch (sym.kind) {
                            case 'function':
                            case 'constructor':
                                pushToken(line, col, len, 4, 1);
                                break;
                            case 'field':
                                pushToken(line, col, len, 6);
                                break;
                            case 'param':
                                pushToken(line, col, len, 12);
                                break;
                            case 'variable':
                                pushToken(line, col, len, 7);
                                break;
                            default:
                                pushToken(line, col, len, 7);
                        }
                    } else {
                        pushToken(line, col, len, 7);
                    }
                } else if (/^[A-Z]/.test(tok.lexeme) && tok.lexeme !== 'IO') {
                    pushToken(line, col, len, 2, 1);
                } else {
                    pushToken(line, col, len, 7);
                }
                break;
            }
            case 'AT':
                pushToken(line, col, len, 11);
                break;
            case 'ARROW':
            case 'ASSIGN':
            case 'PLUS_ASSIGN':
            case 'MINUS_ASSIGN':
            case 'EQ':
            case 'NEQ':
            case 'LT':
            case 'GT':
            case 'LE':
            case 'GE':
            case 'AND':
            case 'OR':
            case 'NOT':
            case 'PLUS':
            case 'MINUS':
            case 'STAR':
            case 'SLASH':
            case 'PLUS_ASSIGN':
            case 'MINUS_ASSIGN':
            case 'STAR_ASSIGN':
            case 'SLASH_ASSIGN':
            case 'LSHIFT':
            case 'RSHIFT':
            case 'BIT_AND':
            case 'BIT_OR':
            case 'BIT_XOR':
            case 'BIT_NOT':
                pushToken(line, col, len, 11);
                break;
            default:
                break;
        }
    }

    return { data };
});

// ─── Startup ──────────────────────────────────────────────

documents.listen(connection);
connection.listen();
