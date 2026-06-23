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
    """Find BlockCtx* and PoolAllocator* variables by scanning frame + all symbols."""
    """Encontra variaveis BlockCtx* e PoolAllocator* escaneando frame + todos os simbolos."""
    blocks = []
    pools = []
    found_blocks = set()
    found_pools = set()

    # 1. Scan frame blocks (locals + args)
    # 1. Escaneia blocos do frame (locais + argumentos)
    try:
        block = frame.block()
        _scan_block(block, blocks, found_blocks, frame)
        _scan_pools(block, pools, found_pools, frame)
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
                    sym_type = str(sym.type)
                    # BlockCtx variables
                    if sym_type and 'BlockCtx' in sym_type and name not in found_blocks:
                        try:
                            val = sym.value()
                            if val.type.code == gdb.TYPE_CODE_PTR:
                                blocks.append((name, val))
                                found_blocks.add(name)
                        except Exception:
                            pass
                    # PoolAllocator variables (__pool_*)
                    if sym_type and 'PoolAllocator' in sym_type and name not in found_pools:
                        try:
                            val = sym.value()
                            if val.type.code == gdb.TYPE_CODE_PTR:
                                pools.append((name, val))
                                found_pools.add(name)
                        except Exception:
                            pass
    except Exception:
        pass

    # 3. Find _tls_current_block if present
    # 3. Encontra _tls_current_block se presente
    if '_tls_current_block' not in found_blocks:
        try:
            tls_sym = gdb.lookup_global_symbol('_tls_current_block')
            if tls_sym:
                val = tls_sym.value()
                if val.type.code == gdb.TYPE_CODE_PTR and int(val) != 0:
                    blocks.append(('_tls_current_block', val))
                    found_blocks.add('_tls_current_block')
        except Exception:
            pass

    # Map pool names: __pool_global -> global
    pool_map = {}
    for name, val in pools:
        block_name = name.replace('__pool_', '', 1)
        pool_map[block_name] = val

    return blocks, pool_map


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


def _scan_pools(gdb_block, pools, found_names, frame):
    """Recursively scan GDB blocks for PoolAllocator* variables (__pool_*)."""
    """Escaneia recursivamente blocos GDB em busca de variaveis PoolAllocator* (__pool_*)."""
    for symbol in gdb_block:
        try:
            if symbol.is_argument or symbol.is_variable:
                name = symbol.name
                if name in found_names:
                    continue
                sym_type = str(symbol.type)
                if 'PoolAllocator' in sym_type:
                    val = symbol.value(frame)
                    if val.type.code == gdb.TYPE_CODE_PTR:
                        pools.append((name, val))
                        found_names.add(name)
        except Exception:
            pass
    for child in gdb_block:
        _scan_pools(child, pools, found_names, frame)


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
        blocks, _ = find_blocks_in_frame(frame)
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

        blocks, pool_map = find_blocks_in_frame(frame)

        if not blocks:
            print("No memory blocks found in current scope")
            print("Nenhum bloco de memoria encontrado no escopo atual")
            print("(Hint: blocks are BlockCtx pointers)")
            print("(Dica: blocos sao ponteiros BlockCtx)")
            return

        show_pool = '--pool' in arg or '-p' in arg
        if show_pool:
            print(f"{'Block':12} {'Size':10} {'Usage':22} {'Pool':7} {'Status':8}")
            print("-" * 62)
        else:
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

                flags = ''
                if '_tls_current_block' in name:
                    flags += ' T'
                block_name = name.replace('_tls_current_block', 'TLS')

                if show_pool:
                    pool_str = 'yes' if pool_map.get(block_name) or pool_map.get(name) else '--'
                    warning = ' \u26a0' if pct > 80 else ''
                    print(f"{block_name:12} {total_mb:>6.0f}MB  {bar} {pct:3.0f}% {used_mb:>6.0f}MB {pool_str:5} {warning}{flags}")
                else:
                    warning = ' \u26a0' if pct > 80 else ''
                    print(f"{block_name:12} {total_mb:>6.0f}MB  {bar} {pct:3.0f}% {used_mb:>6.0f}MB {warning}{flags}")
            except Exception:
                print(f"{name:12} <error reading block>")

        print("\nTip: 'block <name>' for details  |  'info blocks -p' for pool column")
        print("Dica: 'block <nome>' para detalhes  |  'info blocks -p' para coluna pool")
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

        name = arg.strip()
        try:
            val = frame.read_var(name)
        except Exception:
            print(f"Block '{name}' not found in current scope")
            print(f"Bloco '{name}' nao encontrado no escopo atual")
            # Also try with PoolAllocator for __pool_<name>
            try:
                pool_val = frame.read_var('__pool_' + name)
                _print_pool_info(name, pool_val)
            except Exception:
                pass
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

            print(f"Block '{name}'")
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

            # Double-buffer check
            try:
                db_addr = int(val)
                db_result = gdb.parse_and_eval(f"(int)block_has_double_buffer({db_addr})")
                if int(db_result):
                    print(f"  DoubleBuf:  active")
                    print(f"  DoubleBuf:  ativo")
            except Exception:
                pass

            if pct > 80:
                print("\n  \u26a0 WARNING: Block is more than 80% full!")
                print("\n  \u26a0 AVISO: Bloco esta mais de 80% cheio!")

            # Pool allocator info
            _print_pool_info(name, None)

        except Exception as e:
            print(f"Error reading block: {e}")
            print(f"Erro ao ler bloco: {e}")


def _print_pool_info(block_name, pool_val=None):
    """Print PoolAllocator info for a given block name."""
    """Imprime informacao do PoolAllocator para um dado nome de bloco."""
    try:
        if pool_val is None:
            frame = gdb.selected_frame()
            if not frame:
                return
            pool_val = frame.read_var('__pool_' + block_name)
        if pool_val.type.code == gdb.TYPE_CODE_PTR and int(pool_val) != 0:
            pool = pool_val.dereference()
            slot_count = int(pool['slot_count'])
            slots = pool['slots']
            total_free = 0
            total_cap = 0
            for i in range(slot_count):
                slot = slots[i]
                bs = int(slot['block_size'])
                cap = int(slot['capacity'])
                free = int(slot['count'])
                used = cap - free
                total_free += free
                total_cap += cap
                pct = (used / cap * 100) if cap > 0 else 0
                print(f"  Pool slot[{i}]:  {bs}B x {cap}")
                print(f"    Used:  {used}  Free:  {free}  ({pct:.0f}% full)")
                print(f"    Usado: {used}  Livre: {free}  ({pct:.0f}% cheio)")
            if slot_count > 0:
                total_used = total_cap - total_free
                print(f"  Pool total:     {total_cap} blocks, {total_used} used, {total_free} free")
    except Exception:
        pass


class BlockWatchCommand(gdb.Command):
    """Set breakpoints on block allocation for a specific block.
    Usage: block-watch <block_name>
    Watches block_alloc, block_alloc_aligned, and block_alloc_db.
    Uses architecture-aware register names (x86-64: $rdi, ARM64: $x0)."""
    """Define breakpoints na alocacao de bloco para um bloco especifico.
    Uso: block-watch <nome_do_bloco>
    Monitora block_alloc, block_alloc_aligned e block_alloc_db.
    Usa nomes de registradores conscientes da arquitetura."""

    def __init__(self):
        super().__init__("block-watch", gdb.COMMAND_BREAKPOINTS)

    def invoke(self, arg, from_tty):
        if not arg:
            print("Usage: block-watch <block_name>")
            print("Uso: block-watch <nome_do_bloco>")
            return

        name = arg.strip()

        # Architecture-aware register name for first argument
        # Nome do registrador consciente da arquitetura para o primeiro argumento
        reg = '$rdi'
        try:
            frame = gdb.selected_frame()
            if frame:
                arch = frame.architecture()
                arch_name = arch.name()
                if 'aarch64' in arch_name or 'arm' in arch_name:
                    reg = '$x0'
        except Exception:
            pass

        funcs = ['block_alloc', 'block_alloc_aligned', 'block_alloc_db']
        count = 0
        for func in funcs:
            try:
                cond = f"{reg} == (void*){name}"
                gdb.execute(f"break {func} if {cond}")
                print(f"  Breakpoint at {func} when block == '{name}'")
                print(f"  Breakpoint em {func} quando bloco == '{name}'")
                count += 1
            except Exception:
                pass

        if count == 0:
            print(f"Could not set any breakpoints for '{name}'")
            print(f"Nao foi possivel definir breakpoints para '{name}'")
        else:
            print(f"Set {count} breakpoint(s) for block '{name}'")
            print(f"{count} breakpoint(s) definido(s) para o bloco '{name}'")


class BrickDebugAdvice(gdb.Command):
    """Print debugging advice for Brick programs.
    Usage: brick debug"""
    """Imprime conselhos de debug para programas Brick.
    Uso: brick debug"""

    def __init__(self):
        super().__init__("brick", gdb.COMMAND_USER)

    def invoke(self, arg, from_tty):
        if not arg or 'debug' not in arg:
            print("Usage: brick debug")
            print("Uso: brick debug")
            return
        print("Brick Debug Advice / Conselhos de Debug Brick")
        print("=" * 50)
        print()
        print("1. Compile with debug symbols:")
        print("   Compile com simbolos de debug:")
        print("   $ scons debug=1")
        print("   (Disables inlining, enables -O0)")
        print("   (Desativa inlinacao, ativa -O0)")
        print()
        print("2. To step into functions, use -O0 (debug=1):")
        print("   Para step into funcoes, use -O0 (debug=1):")
        print("   Inline hints (always_inline) hide function bodies")
        print("   Dicas de inline (always_inline) escondem corpos de funcao")
        print()
        print("3. Available commands / Comandos disponiveis:")
        print("   info blocks          - list all memory blocks")
        print("   info blocks -p       - list with pool column")
        print("   block <name>         - show block details")
        print("   block-watch <name>   - break on block alloc")
        print("   blocks-list          - list block names (for DAP)")
        print()
        print("4. Memory layout / Layout de memoria:")
        print("   BlockCtx* variables -> BlockCtxPrinter")
        print("   PoolAllocator*      -> PoolAllocatorPrinter")
        print("   BrickString         -> BrickStringPrinter")
        print("   Block ptrs          -> BlockAllocPrinter")


# Register commands
# Registra comandos
BlocksList()
InfoBlocks()
BlockCommand()
BlockWatchCommand()
BrickDebugAdvice()

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
            blocks, _ = gdb._brick_find_blocks(frame)
            names = ",".join(name for name, _ in blocks)
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
