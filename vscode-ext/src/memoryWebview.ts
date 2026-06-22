import * as vscode from 'vscode';

interface BlockInfo {
    name: string;
    size: number;
    used: number;
    unit: string;
    allocs: number;
    peak: number;
}

// GDB convenience function $block_names provides dynamic block discovery

export class MemoryViewProvider implements vscode.WebviewViewProvider {
    public static readonly viewType = 'brick.memoryView';
    private _view?: vscode.WebviewView;

    constructor(private readonly _extensionUri: vscode.Uri) { }

    resolveWebviewView(
        webviewView: vscode.WebviewView,
        _context: vscode.WebviewViewResolveContext,
        _token: vscode.CancellationToken
    ) {
        this._view = webviewView;
        webviewView.webview.options = {
            enableScripts: true,
            localResourceRoots: [this._extensionUri]
        };
        webviewView.webview.html = this._getHtmlContent();
    }

    public async update() {
        console.log('[Brick Mem] update() called');
        if (this._view) {
            const session = vscode.debug.activeDebugSession;
            console.log('[Brick Mem] session:', session ? session.id : 'none');
            if (session) {
                try {
                    const blocks = await this._readBlocksFromSession(session);
                    console.log('[Brick Mem] blocks count:', blocks.length);
                    if (blocks.length > 0) {
                        this._view.webview.postMessage({ command: 'update', blocks });
                        return;
                    }
                } catch (e) {
                    console.log('[Brick Mem] update error:', e);
                }
            }
            console.log('[Brick Mem] posting clear');
            this._view.webview.postMessage({ command: 'clear', msg: session ? 'No memory blocks found' : 'No debug session active' });
        } else {
            console.log('[Brick Mem] no view yet');
        }
    }

    private _cleanReplResult(raw: any): string {
        if (!raw) return '';
        let t = String(raw);
        // Strip $N = prefix from GDB output: "$1 = \"value\"" -> "\"value\""
        const dm = t.match(/^\$\d+\s*=\s*(.*)/);
        if (dm) t = dm[1];
        // Strip outer quotes that wrap the entire value
        if (t.startsWith('"') && t.endsWith('"') && t.length >= 2) t = t.slice(1, -1);
        return t.trim();
    }

    private _cleanMiLine(line: string): string {
        let s = line.trim();
        s = s.replace(/^~\s*"?/, '');
        s = s.replace(/"\s*$/, '');
        s = s.replace(/\\\\t/g, '\t').replace(/\\\\n/g, '\n').replace(/\\t/g, '\t').replace(/\\n/g, '\n');
        return s.trim();
    }

    private async _getDAPFrameId(session: vscode.DebugSession): Promise<number | undefined> {
        try {
            const tResp = await session.customRequest('threads', {});
            const tid = tResp?.threads?.[0]?.id;
            if (tid === undefined) return undefined;
            const sResp = await session.customRequest('stackTrace', { threadId: tid });
            return sResp?.stackFrames?.[0]?.id;
        } catch (e) {
            console.log('[Brick Mem] _getDAPFrameId err:', e);
            return undefined;
        }
    }

    private async _evalExpr(session: vscode.DebugSession, expr: string, frameId: number): Promise<any> {
        try {
            return await session.customRequest('evaluate', {
                expression: expr, context: 'repl', frameId
            });
        } catch {
            return null;
        }
    }

    private async _evalCommand(session: vscode.DebugSession, cmd: string): Promise<any> {
        try {
            return await session.customRequest('evaluate', {
                expression: cmd, context: 'repl'
            });
        } catch (e: any) {
            console.log('[Brick Mem] _evalCommand err:', e?.message || String(e));
            return null;
        }
    }

    private async _getBlockNames(session: vscode.DebugSession, frameId: number): Promise<string[]> {
        // Diagnostic: check if .gdbinit was loaded
        {
            const r = await this._evalExpr(session, '$_brick_loaded', frameId);
            console.log('[Brick Mem] DIAG $_brick_loaded:', JSON.stringify(r));
        }

        // Approach 1: $_block_names via expression evaluate (needs frameId)
        {
            const r = await this._evalExpr(session, '$_block_names', frameId);
            console.log('[Brick Mem] A1 raw:', JSON.stringify(r));
            if (r && r.result) {
                const t = this._cleanReplResult(r.result);
                if (t.length > 0) {
                    const n = t.split(',').map(s => s.trim()).filter(s => s.length > 0);
                    if (n.length > 0) { console.log('[Brick Mem] A1 ok:', n); return n; }
                }
            }
        }

        // Approach 2: info variables -t BlockCtx (CLI command, no frameId)
        {
            const r = await this._evalCommand(session, 'info variables -t BlockCtx');
            console.log('[Brick Mem] A2 raw:', JSON.stringify(r));
            if (r && r.result) {
                const names: string[] = [];
                for (const rawLine of String(r.result).split('\n')) {
                    const m = this._cleanMiLine(rawLine).match(/BlockCtx\s*\*?\s*(\w+);/);
                    if (m) names.push(m[1]);
                }
                if (names.length > 0) { console.log('[Brick Mem] A2 ok:', [...new Set(names)]); return [...new Set(names)]; }
            }
        }

        // Approach 3: info variables BlockCtx (CLI command, no frameId)
        {
            const r = await this._evalCommand(session, 'info variables BlockCtx');
            console.log('[Brick Mem] A3 raw:', JSON.stringify(r));
            if (r && r.result) {
                const names: string[] = [];
                for (const rawLine of String(r.result).split('\n')) {
                    const m = this._cleanMiLine(rawLine).match(/BlockCtx\s*\*?\s*(\w+);/);
                    if (m) names.push(m[1]);
                }
                if (names.length > 0) { console.log('[Brick Mem] A3 ok:', [...new Set(names)]); return [...new Set(names)]; }
            }
        }

        // Approach 4: blocks-list command (CLI command, no frameId)
        {
            const r = await this._evalCommand(session, 'blocks-list');
            console.log('[Brick Mem] A4 raw:', JSON.stringify(r));
            if (r && r.result) {
                const names = String(r.result).split('\n')
                    .map(s => this._cleanMiLine(s))
                    .filter(s => s.length > 0 && !s.startsWith('~'));
                if (names.length > 0) { console.log('[Brick Mem] A4 ok:', names); return names; }
            }
        }

        return [];
    }

    private _extractNum(raw: any): number {
        if (raw === undefined || raw === null) return NaN;
        let s = String(raw).trim();
        // Strip $N = prefix
        const dm = s.match(/^\$\d+\s*=\s*(.*)/);
        if (dm) s = dm[1].trim();
        // Parse integer (strip any trailing non-numeric)
        const nm = s.match(/^(-?\d+)/);
        return nm ? parseInt(nm[1], 10) : NaN;
    }

    private async _readBlocksFromSession(session: vscode.DebugSession): Promise<BlockInfo[]> {
        const frameId = await this._getDAPFrameId(session);
        if (frameId === undefined) {
            console.log('[Brick Mem] no frameId (process running?), skip');
            return [];
        }
        console.log('[Brick Mem] frameId:', frameId);

        const names = await this._getBlockNames(session, frameId);
        console.log('[Brick Mem] Blocks found:', names);
        if (names.length === 0) return [];

        const blocks: BlockInfo[] = [];

        for (const name of names) {
            try {
                const capResult = await this._evalExpr(session, `${name}->capacity`, frameId);
                console.log(`[Brick Mem] ${name}->capacity:`, JSON.stringify(capResult));
                if (!capResult || capResult.result === undefined) continue;

                const capacity = this._extractNum(capResult.result);
                if (isNaN(capacity) || capacity === 0) {
                    console.log(`[Brick Mem] ${name}: capacity=${capacity} (skip)`);
                    continue;
                }

                const usedR = await this._evalExpr(session, `${name}->used`, frameId);
                const peakR = await this._evalExpr(session, `${name}->peak_used`, frameId);
                const allocR = await this._evalExpr(session, `${name}->allocation_count`, frameId);

                const used = usedR ? this._extractNum(usedR.result) : 0;
                const peak = peakR ? this._extractNum(peakR.result) : 0;
                const allocs = allocR ? this._extractNum(allocR.result) : 0;

                const unit = capacity >= 1024 * 1024 ? 'MB' : capacity >= 1024 ? 'KB' : 'B';
                const divisor = unit === 'MB' ? 1024 * 1024 : unit === 'KB' ? 1024 : 1;

                console.log(`[Brick Mem] ${name}: cap=${capacity} used=${used} peak=${peak} allocs=${allocs}`);

                blocks.push({
                    name,
                    size: Math.round(capacity / divisor),
                    used: Math.round(used / divisor),
                    peak: Math.round(peak / divisor),
                    unit,
                    allocs
                });
            } catch (e) {
                console.log(`[Brick Mem] ${name} error:`, e);
            }
        }

        console.log('[Brick Mem] Blocks data:', blocks);
        return blocks;
    }

    private _getHtmlContent(): string {
        return `<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <style>
        :root {
            --bg: #1e1e1e;
            --card: #2d2d2d;
            --border: #3c3c3c;
            --text: #cccccc;
            --accent: #569cd6;
            --warn: #d16969;
            --green: #6a9955;
        }
        body {
            font-family: 'Consolas', 'Courier New', monospace;
            font-size: 12px;
            background: var(--bg);
            color: var(--text);
            padding: 8px;
            margin: 0;
        }
        .block {
            background: var(--card);
            border: 1px solid var(--border);
            border-radius: 4px;
            padding: 8px;
            margin-bottom: 6px;
        }
        .block-header {
            display: flex;
            justify-content: space-between;
            align-items: center;
            cursor: pointer;
        }
        .block-name {
            color: var(--accent);
            font-weight: bold;
        }
        .block-size {
            color: #888;
        }
        .bar-container {
            height: 16px;
            background: var(--bg);
            border-radius: 8px;
            margin: 4px 0;
            overflow: hidden;
        }
        .bar-fill {
            height: 100%;
            background: linear-gradient(90deg, var(--green), var(--accent));
            border-radius: 8px;
            transition: width 0.2s;
        }
        .bar-fill.warning {
            background: linear-gradient(90deg, #dcdcaa, var(--warn));
        }
        .bar-fill.critical {
            background: linear-gradient(90deg, var(--warn), #cc0000);
        }
        .block-details {
            font-size: 11px;
            color: #999;
            margin-top: 4px;
        }
        .block-details span {
            margin-right: 12px;
        }
        .empty {
            color: #666;
            text-align: center;
            margin-top: 20px;
        }
        .header {
            border-bottom: 1px solid var(--border);
            padding-bottom: 6px;
            margin-bottom: 8px;
            color: var(--accent);
            font-weight: bold;
        }
    </style>
</head>
<body>
    <div class="header">BRICK MEMORY</div>
    <div id="blocks">
        <div class="empty" id="empty-msg">No debug session active</div>
    </div>

    <script>
        const vscode = acquireVsCodeApi();

        function renderBlocks(blocks, msg) {
            const container = document.getElementById('blocks');

            if (!blocks || blocks.length === 0) {
                container.innerHTML = '<div class="empty">' + (msg || 'No memory blocks found') + '</div>';
                return;
            }

            container.innerHTML = blocks.map(block => {
                const pct = (block.used / block.size) * 100;
                const barClass = pct > 90 ? 'critical' : pct > 75 ? 'warning' : '';
                const warning = pct > 80 ? '\\u26A0' : '';

                return \`
                    <div class="block">
                        <div class="block-header">
                            <span class="block-name">\${block.name}</span>
                            <span class="block-size">\${block.size}\${block.unit}</span>
                        </div>
                        <div class="bar-container">
                            <div class="bar-fill \${barClass}" style="width: \${pct}%"></div>
                        </div>
                        <div class="block-details">
                            <span>\${block.used}\${block.unit} used (\${pct.toFixed(0)}%)</span>
                            <span>\${block.allocs} allocs</span>
                            \${warning ? '<span style="color: var(--warn)">' + warning + ' near capacity!</span>' : ''}
                        </div>
                    </div>
                \`;
            }).join('');
        }

        // Listen for updates from extension
        window.addEventListener('message', event => {
            const message = event.data;
            switch (message.command) {
                case 'update':
                    renderBlocks(message.blocks);
                    break;
                case 'clear':
                    renderBlocks(null, message.msg || 'No memory blocks found');
                    break;
            }
        });
    </script>
</body>
</html>`;
    }
}
