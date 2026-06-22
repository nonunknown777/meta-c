# Brick GDB Custom Commands
# Comandos Personalizados GDB do Brick
#
# Provides:
# Fornece:
#   (gdb) info blocks     - list all memory blocks
#   (gdb) info blocks     - lista todos os blocos de memoria
#   (gdb) block <name>   - show block details
#   (gdb) block <nome>   - mostra detalhes do bloco
#   (gdb) block-watch    - break on block allocation
#   (gdb) block-watch    - breakpoint na alocacao de bloco
#   (gdb) blocks-list    - list block names (one per line, for DAP)
#   (gdb) blocks-list    - lista nomes de blocos (um por linha, para DAP)

import gdb


def find_blocks_in_frame(frame):
    """Find BlockCtx* variables by scanning frame + all program symbols."""
    """Encontra variaveis BlockCtx* escaneando frame + todos os simbolos do programa."""
    blocks = []
    found_names = set()

    # 1. Scan frame blocks (locals + args)
    # 1. Escaneia blocos do frame (locais + argumentos)
    try:
        block = frame.block()
        _scan_block(block, blocks, found_names, frame)
    except Exception:
        pass

    # 2. Scan global block from frame (covers globals/statics in all objfiles)
    # 2. Escaneia bloco global do frame (cobre globais/estaticos em todos os objfiles)
    try:
        gb = frame.block().global_block
        if gb:
            for sym in gb:
                if not sym.is_function:
                    name = sym.name
                    if name in found_names:
                        continue
                    sym_type = str(sym.type)
                    if sym_type and 'BlockCtx' in sym_type:
                        try:
                            val = sym.value()
                            if val.type.code == gdb.TYPE_CODE_PTR:
                                blocks.append((name, val))
                                found_names.add(name)
                        except Exception:
                            pass
    except Exception:
        pass

    return blocks


def _scan_block(gdb_block, blocks, found_names, frame):
    """Recursively scan GDB blocks for BlockCtx variables."""
    """Escaneia recursivamente blocos GDB em busca de variaveis BlockCtx."""
    for symbol in gdb_block:
        try:
            if symbol.is_argument or symbol.is_variable:
                name = symbol.name
                if name in found_names:
                    continue
                sym_type = str(symbol.type)
                if 'BlockCtx' in sym_type:
                    val = symbol.value(frame)
                    if val.type.code == gdb.TYPE_CODE_PTR:
                        blocks.append((name, val))
                        found_names.add(name)
                    elif val.type.code == gdb.TYPE_CODE_STRUCT:
                        blocks.append((name, val.address))
                        found_names.add(name)
        except Exception:
            pass
    for child in gdb_block:
        _scan_block(child, blocks, found_names, frame)


class BlocksList(gdb.Command):
    """Print names of all BlockCtx variables, one per line.
    Usage: blocks-list"""
    """Imprime nomes de todas as variaveis BlockCtx, um por linha.
    Uso: blocks-list"""

    def __init__(self):
        super().__init__("blocks-list", gdb.COMMAND_DATA)

    def invoke(self, arg, from_tty):
        frame = gdb.selected_frame()
        if not frame:
            return
        blocks = find_blocks_in_frame(frame)
        for name, _ in blocks:
            gdb.write(name + "\n")


class InfoBlocks(gdb.Command):
    """List all memory blocks with usage statistics.
    Usage: info blocks"""
    """Lista todos os blocos de memoria com estatisticas de uso.
    Uso: info blocks"""

    def __init__(self):
        super().__init__("info blocks", gdb.COMMAND_DATA)

    def invoke(self, arg, from_tty):
        frame = gdb.selected_frame()
        if not frame:
            print("No frame selected")
            print("Nenhum frame selecionado")
            return

        blocks = find_blocks_in_frame(frame)

        if not blocks:
            print("No memory blocks found in current scope")
            print("Nenhum bloco de memoria encontrado no escopo atual")
            print("(Hint: blocks are BlockCtx pointers)")
            print("(Dica: blocos sao ponteiros BlockCtx)")
            return

        print(f"{'Block':12} {'Size':10} {'Usage':22} {'Status':8}")
        print("-" * 55)

        for name, ptr in blocks:
            try:
                block = ptr.dereference()
                total = int(block['capacity'])
                used = int(block['used'])
                pct = (used / total * 100) if total > 0 else 0
                total_mb = total / (1024 * 1024)
                used_mb = used / (1024 * 1024)

                bar_len = 20
                filled = int(pct / 100 * bar_len)
                bar = '\u2588' * filled + '\u2591' * (bar_len - filled)

                warning = ' \u26a0' if pct > 80 else ''
                print(f"{name:12} {total_mb:>6.0f}MB  {bar} {pct:3.0f}% {used_mb:>6.0f}MB {warning}")
            except Exception:
                print(f"{name:12} <error reading block>")

        print("\nTip: 'block <name>' for details")
        print("Dica: 'block <nome>' para detalhes")
        print("     'bt' for backtrace on alloc")
        print("     'bt' para backtrace na alocacao")


class BlockCommand(gdb.Command):
    """Show details of a specific memory block.
    Usage: block <name>"""
    """Mostra detalhes de um bloco de memoria especifico.
    Uso: block <nome>"""

    def __init__(self):
        super().__init__("block", gdb.COMMAND_DATA)

    def invoke(self, arg, from_tty):
        if not arg:
            print("Usage: block <name>")
            print("Uso: block <nome>")
            print("       'info blocks' to list available blocks")
            print("       'info blocks' para listar blocos disponiveis")
            return

        frame = gdb.selected_frame()
        if not frame:
            print("No frame selected")
            print("Nenhum frame selecionado")
            return

        try:
            val = frame.read_var(arg.strip())
        except Exception:
            print(f"Block '{arg.strip()}' not found in current scope")
            print(f"Bloco '{arg.strip()}' nao encontrado no escopo atual")
            return

        try:
            if val.type.code == gdb.TYPE_CODE_PTR:
                block = val.dereference()
            else:
                block = val

            total = int(block['capacity'])
            used = int(block['used'])
            peak = int(block['peak_used'])
            allocs = int(block['allocation_count'])

            total_mb = total / (1024 * 1024)
            used_mb = used / (1024 * 1024)
            peak_mb = peak / (1024 * 1024)
            pct = (used / total * 100) if total > 0 else 0

            print(f"Block '{arg.strip()}'")
            print(f"  Capacity:   {total_mb:.0f} MB ({total} bytes)")
            print(f"  Capacidade: {total_mb:.0f} MB ({total} bytes)")
            print(f"  Used:       {used_mb:.1f} MB ({used} bytes)  ({pct:.1f}%)")
            print(f"  Usado:      {used_mb:.1f} MB ({used} bytes)  ({pct:.1f}%)")
            print(f"  Free:       {(total - used) / (1024*1024):.1f} MB")
            print(f"  Livre:      {(total - used) / (1024*1024):.1f} MB")
            print(f"  Peak:       {peak_mb:.1f} MB")
            print(f"  Pico:       {peak_mb:.1f} MB")
            print(f"  Allocs:     {allocs}")
            print(f"  Alocacoes:  {allocs}")
            print(f"  Address:    {val}")
            print(f"  Endereco:   {val}")

            if pct > 80:
                print("\n  \u26a0 WARNING: Block is more than 80% full!")
                print("\n  \u26a0 AVISO: Bloco esta mais de 80% cheio!")

        except Exception as e:
            print(f"Error reading block: {e}")
            print(f"Erro ao ler bloco: {e}")


class BlockWatchCommand(gdb.Command):
    """Set a breakpoint on block allocation for a specific block.
    Usage: block-watch <block_name>"""
    """Define um breakpoint na alocacao de bloco para um bloco especifico.
    Uso: block-watch <nome_do_bloco>"""

    def __init__(self):
        super().__init__("block-watch", gdb.COMMAND_BREAKPOINTS)

    def invoke(self, arg, from_tty):
        if not arg:
            print("Usage: block-watch <block_name>")
            print("Uso: block-watch <nome_do_bloco>")
            return

        bp_cmd = f"break block_alloc if $rdi == {arg.strip()}"
        gdb.execute(bp_cmd)
        print(f"Breakpoint set: block_alloc when block == '{arg.strip()}'")
        print(f"Breakpoint definido: block_alloc quando bloco == '{arg.strip()}'")


# Register commands
# Registra comandos
BlocksList()
InfoBlocks()
BlockCommand()
BlockWatchCommand()

# Expose find_blocks_in_frame on gdb module for hook-stop access
# Expoe find_blocks_in_frame no modulo gdb para acesso hook-stop
gdb._brick_find_blocks = find_blocks_in_frame

# Event-based stop handler (GDB 7.9+) — sets $_block_names as backup
# Manipulador de parada baseado em evento (GDB 7.9+) — define $_block_names como backup
def _on_stop(event):
    """Event handler: auto-populate $_block_names on every stop."""
    """Manipulador de evento: preenche automaticamente $_block_names em cada parada."""
    try:
        frame = gdb.selected_frame()
        if frame:
            names = ",".join(name for name, _ in gdb._brick_find_blocks(frame))
            gdb.set_convenience_variable("_block_names", names)
        else:
            gdb.set_convenience_variable("_block_names", "")
    except Exception:
        gdb.set_convenience_variable("_block_names", "")

try:
    gdb.events.stop.connect(_on_stop)
except AttributeError:
    pass  # GDB too old, relies on hook-stop in .gdbinit
         # GDB muito antigo, depende de hook-stop no .gdbinit
