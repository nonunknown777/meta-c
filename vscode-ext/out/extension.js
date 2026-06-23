"use strict";
var __createBinding = (this && this.__createBinding) || (Object.create ? (function(o, m, k, k2) {
    if (k2 === undefined) k2 = k;
    var desc = Object.getOwnPropertyDescriptor(m, k);
    if (!desc || ("get" in desc ? !m.__esModule : desc.writable || desc.configurable)) {
      desc = { enumerable: true, get: function() { return m[k]; } };
    }
    Object.defineProperty(o, k2, desc);
}) : (function(o, m, k, k2) {
    if (k2 === undefined) k2 = k;
    o[k2] = m[k];
}));
var __setModuleDefault = (this && this.__setModuleDefault) || (Object.create ? (function(o, v) {
    Object.defineProperty(o, "default", { enumerable: true, value: v });
}) : function(o, v) {
    o["default"] = v;
});
var __importStar = (this && this.__importStar) || (function () {
    var ownKeys = function(o) {
        ownKeys = Object.getOwnPropertyNames || function (o) {
            var ar = [];
            for (var k in o) if (Object.prototype.hasOwnProperty.call(o, k)) ar[ar.length] = k;
            return ar;
        };
        return ownKeys(o);
    };
    return function (mod) {
        if (mod && mod.__esModule) return mod;
        var result = {};
        if (mod != null) for (var k = ownKeys(mod), i = 0; i < k.length; i++) if (k[i] !== "default") __createBinding(result, mod, k[i]);
        __setModuleDefault(result, mod);
        return result;
    };
})();
Object.defineProperty(exports, "__esModule", { value: true });
exports.activate = activate;
exports.deactivate = deactivate;
const vscode = __importStar(require("vscode"));
const fs = __importStar(require("fs"));
const path = __importStar(require("path"));
const node_1 = require("vscode-languageclient/node");
const memoryWebview_1 = require("./memoryWebview");
let client;
const LAUNCH_TEMPLATE = {
    version: '0.3.0',
    configurations: [
        {
            name: 'Debug Compiler (current file)',
            type: 'cppdbg',
            request: 'launch',
            program: '${workspaceFolder}/build/brick',
            args: ['${file}', '-o', '${workspaceFolder}/build/${fileBasenameNoExtension}.c'],
            cwd: '${workspaceFolder}',
            MIMode: 'gdb',
            setupCommands: [
                { description: 'Enable pretty-printing for gdb', text: '-enable-pretty-printing', ignoreFailures: true }
            ],
            preLaunchTask: 'Build Brick',
        },
        {
            name: 'Debug Compiled Program',
            type: 'cppdbg',
            request: 'launch',
            program: '${workspaceFolder}/build/${fileBasenameNoExtension}',
            args: [],
            cwd: '${workspaceFolder}',
            MIMode: 'gdb',
            setupCommands: [
                { description: 'Enable pretty-printing', text: '-enable-pretty-printing', ignoreFailures: true },
                { description: 'Load Brick printers', text: 'source ${workspaceFolder}/debugger/.gdbinit', ignoreFailures: true },
            ],
            preLaunchTask: 'Compile this .brc file',
        },
        {
            name: 'Run Compiled Program',
            type: 'cppdbg',
            request: 'launch',
            program: '${workspaceFolder}/build/${fileBasenameNoExtension}',
            args: [],
            cwd: '${workspaceFolder}',
            MIMode: 'gdb',
            preLaunchTask: 'Compile this .brc file (release)',
        },
    ],
};
const TASKS_TEMPLATE = {
    version: '2.0.0',
    tasks: [
        {
            label: 'Build Brick',
            type: 'shell',
            command: 'scons',
            group: { kind: 'build', isDefault: true },
            problemMatcher: ['$gcc'],
        },
        {
            label: 'Build (debug)',
            type: 'shell',
            command: 'scons profile=debug',
            group: 'build',
            problemMatcher: ['$gcc'],
        },
        {
            label: 'Compile this .brc file',
            type: 'shell',
            command: './build/brick "${file}" -o "build/${fileBasenameNoExtension}.c" && gcc -g -Iruntime "build/${fileBasenameNoExtension}.c" runtime/block_memory.c runtime/io.c runtime/hot_reload.c -o "build/${fileBasenameNoExtension}" -ldl',
            problemMatcher: ['$gcc'],
        },
        {
            label: 'Compile this .brc file (release)',
            type: 'shell',
            command: './build/brick "${file}" -o "build/${fileBasenameNoExtension}.c" && gcc -O3 -Iruntime "build/${fileBasenameNoExtension}.c" runtime/block_memory.c runtime/io.c -o "build/${fileBasenameNoExtension}"',
            problemMatcher: ['$gcc'],
        },
        {
            label: 'Run this .brc file',
            type: 'shell',
            command: './build/brick "${file}" -o "${fileBasenameNoExtension}.c" && gcc -O3 "build/${fileBasenameNoExtension}.c" runtime/block_memory.c runtime/io.c -o "build/${fileBasenameNoExtension}" && "build/${fileBasenameNoExtension}"',
            problemMatcher: [],
        },
    ],
};
function activate(context) {
    startLanguageClient(context);
    const provider = new memoryWebview_1.MemoryViewProvider(context.extensionUri);
    context.subscriptions.push(vscode.window.registerWebviewViewProvider(memoryWebview_1.MemoryViewProvider.viewType, provider, { webviewOptions: { retainContextWhenHidden: true } }));
    context.subscriptions.push(vscode.debug.onDidChangeActiveDebugSession(session => {
        console.log('[Brick] onDidChangeActiveDebugSession:', session ? session.id : 'null');
        provider.update();
    }));
    // Real-time updates on pause / step / breakpoint
    context.subscriptions.push(vscode.debug.onDidChangeActiveStackItem(() => {
        console.log('[Brick] onDidChangeActiveStackItem');
        if (vscode.debug.activeDebugSession) {
            provider.update();
        }
    }));
    // Auto-update when debug session starts
    context.subscriptions.push(vscode.debug.onDidStartDebugSession(() => {
        console.log('[Brick] onDidStartDebugSession');
        provider.update();
    }));
    // Also update when debug session terminates
    context.subscriptions.push(vscode.debug.onDidTerminateDebugSession(() => {
        console.log('[Brick] onDidTerminateDebugSession');
        provider.update();
    }));
    context.subscriptions.push(vscode.commands.registerCommand('brick.initWorkspace', initWorkspace));
    context.subscriptions.push(vscode.commands.registerCommand('brick.debugProgram', () => {
        const editor = vscode.window.activeTextEditor;
        if (editor && editor.document.languageId === 'brick') {
            const filePath = editor.document.uri.fsPath;
            const fileName = path.basename(filePath, '.brc');
            vscode.debug.startDebugging(undefined, {
                type: 'cppdbg',
                name: 'Debug Brick Program',
                request: 'launch',
                program: `${vscode.workspace.workspaceFolders?.[0]?.uri.fsPath}/build/${fileName}`,
                args: [],
                cwd: '${workspaceFolder}',
                MIMode: 'gdb',
                setupCommands: [
                    { description: 'Enable pretty-printing', text: '-enable-pretty-printing', ignoreFailures: true },
                    { description: 'Load Brick printers', text: 'source ${workspaceFolder}/debugger/.gdbinit', ignoreFailures: true },
                ],
                preLaunchTask: 'Compile Brick (debug)',
            });
        }
        else {
            vscode.window.showErrorMessage('Open a .brc file first');
        }
    }));
    // Suggest workspace setup when opening .brc files without config
    context.subscriptions.push(vscode.workspace.onDidOpenTextDocument(doc => {
        if (doc.languageId === 'brick' && vscode.workspace.workspaceFolders) {
            const wsPath = vscode.workspace.workspaceFolders[0].uri.fsPath;
            const vscodeDir = path.join(wsPath, '.vscode');
            const launchPath = path.join(vscodeDir, 'launch.json');
            if (!fs.existsSync(launchPath)) {
                setTimeout(() => {
                    vscode.window.showInformationMessage('Brick workspace not configured. Add build & debug configs?', 'Set up workspace').then(selection => {
                        if (selection === 'Set up workspace') {
                            vscode.commands.executeCommand('brick.initWorkspace');
                        }
                    });
                }, 1000);
            }
        }
    }));
}
function initWorkspace() {
    const wsFolders = vscode.workspace.workspaceFolders;
    if (!wsFolders) {
        vscode.window.showErrorMessage('Open a folder first');
        return;
    }
    const wsPath = wsFolders[0].uri.fsPath;
    const vscodeDir = path.join(wsPath, '.vscode');
    if (!fs.existsSync(vscodeDir)) {
        fs.mkdirSync(vscodeDir, { recursive: true });
    }
    const launchPath = path.join(vscodeDir, 'launch.json');
    const tasksPath = path.join(vscodeDir, 'tasks.json');
    if (!fs.existsSync(launchPath)) {
        fs.writeFileSync(launchPath, JSON.stringify(LAUNCH_TEMPLATE, null, 4));
    }
    if (!fs.existsSync(tasksPath)) {
        fs.writeFileSync(tasksPath, JSON.stringify(TASKS_TEMPLATE, null, 4));
    }
    // Reload window to pick up the new files
    vscode.window.showInformationMessage('Brick workspace created! Reload window to activate.', 'Reload Now').then(selection => {
        if (selection === 'Reload Now') {
            vscode.commands.executeCommand('workbench.action.reloadWindow');
        }
    });
}
function startLanguageClient(context) {
    const serverModule = context.asAbsolutePath('server/out/server.js');
    const serverOptions = {
        run: { module: serverModule, transport: node_1.TransportKind.ipc },
        debug: {
            module: serverModule,
            transport: node_1.TransportKind.ipc,
            options: { execArgv: ['--nolazy', '--inspect=6009'] },
        },
    };
    const clientOptions = {
        documentSelector: [{ scheme: 'file', language: 'brick' }],
        synchronize: {
            fileEvents: vscode.workspace.createFileSystemWatcher('**/*.brc'),
        },
    };
    client = new node_1.LanguageClient('brick-language-server', 'Brick Language Server', serverOptions, clientOptions);
    client.start();
}
function deactivate() {
    return client?.stop();
}
//# sourceMappingURL=extension.js.map