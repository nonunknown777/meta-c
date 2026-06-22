import { execSync } from 'child_process';
import { existsSync } from 'fs';

export interface LspToken {
    type: string;
    lexeme: string;
    line: number;
    col: number;
    file: string;
}

export interface LspSymbol {
    name: string;
    kind: string;
    type_name: string;
    line: number;
    col: number;
    file: string;
}

export interface LspError {
    message: string;
    line: number;
    col: number;
    file: string;
    severity: number;
}

export interface LspOutput {
    tokens: LspToken[];
    symbols: LspSymbol[];
    errors: LspError[];
}

export interface CompilerResult {
    success: boolean;
    output: LspOutput;
    raw: string;
}

export function findBrick(): string {
    const candidates = [
        'brick',
        './build/brick',
        '../build/brick',
        '../../build/brick',
        '/usr/local/bin/brick',
    ];
    for (const cmd of candidates) {
        try {
            execSync(`${cmd} --help`, { encoding: 'utf8', stdio: 'pipe' });
            return cmd;
        } catch {
            continue;
        }
    }
    return 'brick';
}

export function runCompiler(filePath: string): CompilerResult {
    const cmd = findBrick();

    try {
        const raw = execSync(`"${cmd}" "${filePath}" --lsp`, {
            encoding: 'utf8',
            stdio: ['ignore', 'pipe', 'pipe'],
            timeout: 30000,
        });

        const output: LspOutput = JSON.parse(raw);
        return { success: true, output, raw };
    } catch (err: unknown) {
        if (err instanceof Error && 'stdout' in err) {
            const execErr = err as { stdout?: string; stderr?: string; status?: number };
            if (execErr.stdout) {
                try {
                    const output: LspOutput = JSON.parse(execErr.stdout);
                    return { success: false, output, raw: execErr.stdout };
                } catch {
                    // Not JSON output
                }
            }
            return {
                success: false,
                output: { tokens: [], symbols: [], errors: [{ message: execErr.stderr || err.message, line: 0, col: 0, file: filePath, severity: 1 }] },
                raw: execErr.stderr || '',
            };
        }
        return {
            success: false,
            output: { tokens: [], symbols: [], errors: [{ message: String(err), line: 0, col: 0, file: filePath, severity: 1 }] },
            raw: String(err),
        };
    }
}
