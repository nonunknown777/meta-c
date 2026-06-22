# Brick GDB Pretty-Printers
# Pretty-Printers GDB do Brick
# Auto-loaded via .gdbinit
# Carregado automaticamente via .gdbinit
#
# Provides:
# Fornece:
#   - BlockCtx*: block name, size, usage, allocations
#   - BlockCtx*: nome do bloco, tamanho, uso, alocacoes
#   - BrickString: string content
#   - BrickString: conteudo da string
#   - Block-allocated ptrs: offset within block
#   - Ponteiros alocados em bloco: offset dentro do bloco

import gdb
import re

class BlockCtxPrinter:
    """Pretty-printer for Brick BlockCtx"""
    """Pretty-printer para Brick BlockCtx"""

    def __init__(self, val):
        self.val = val

    def to_string(self):
        total = int(self.val['capacity'])
        used = int(self.val['used'])
        allocs = int(self.val['allocation_count'])

        if total >= 1024 * 1024:
            total_s = f"{total / (1024*1024):.0f}MB"
            used_s = f"{used / (1024*1024):.0f}MB"
        elif total >= 1024:
            total_s = f"{total / 1024:.0f}KB"
            used_s = f"{used / 1024:.0f}KB"
        else:
            total_s = f"{total}B"
            used_s = f"{used}B"

        pct = (used / total * 100) if total > 0 else 0
        bar_len = 20
        filled = int(pct / 100 * bar_len)
        bar = '\u2588' * filled + '\u2591' * (bar_len - filled)

        return f"'{self.val.type.name}' {{ {total_s}, used: {used_s} ({pct:.0f}%), allocs: {allocs} }}\n  [{bar}]"

    def display_hint(self):
        return 'string'


class BrickStringPrinter:
    """Pretty-printer for Brick String (struct { char* data; int len; })"""
    """Pretty-printer para Brick String (struct { char* data; int len; })"""

    def __init__(self, val):
        self.val = val

    def to_string(self):
        try:
            data_ptr = self.val['data']
            length = int(self.val['len'])
            if data_ptr == 0 or length == 0:
                return '""'
            s = data_ptr.string(length=length, errors='replace')
            return f'"{s}" (len={length})'
        except Exception:
            return '<invalid string>'
            # <string invalida>


class BlockAllocPrinter:
    """Shows offset within block for block-allocated pointers"""
    """Mostra offset dentro do bloco para ponteiros alocados em bloco"""

    def __init__(self, val, block_name, offset, block_ctx):
        self.val = val
        self.block_name = block_name
        self.offset = offset
        self.block_ctx = block_ctx

    def to_string(self):
        try:
            total = int(self.block_ctx['capacity'])
            used = int(self.block_ctx['used'])
            pct = (used / total * 100) if total > 0 else 0
            return (f"@{self.block_name}+{self.offset}B "
                    f"(block: {pct:.0f}% full)")
                    # f"(bloco: {pct:.0f}% cheio)")
        except Exception:
            return f"@{self.block_name}+{self.offset}B"

    def display_hint(self):
        return 'string'


def _find_block_ranges(frame):
    """Find all BlockCtx* variables and their data ranges in all program symbols."""
    """Encontra todas as variaveis BlockCtx* e suas faixas de dados em todos os simbolos do programa."""
    blocks = []
    found_names = set()

    # 1. Scan frame blocks (locals + args)
    # 1. Escaneia blocos do frame (locais + argumentos)
    try:
        block = frame.block()
        _scan_for_blocks(block, blocks, found_names)
    except Exception:
        pass

    # 2. Scan global block from frame (covers globals/statics in all objfiles)
    # 2. Escaneia bloco global do frame (cobre globais/estaticos em todos os objfiles)
    try:
        gb = frame.block().global_block
        if gb:
            for sym in gb:
                if sym.is_function:
                    continue
                name = sym.name
                if name in found_names:
                    continue
                sym_type = str(sym.type)
                if not sym_type or 'BlockCtx' not in sym_type:
                    continue
                try:
                    val = sym.value()
                    if val.type.code == gdb.TYPE_CODE_PTR:
                        ptr = val
                    elif val.type.code == gdb.TYPE_CODE_STRUCT:
                        ptr = val.address
                    else:
                        continue
                    block_ctx = ptr.dereference()
                    data_ptr = int(block_ctx['data'])
                    capacity = int(block_ctx['capacity'])
                    blocks.append((name, data_ptr, data_ptr + capacity, ptr))
                    found_names.add(name)
                except Exception:
                    pass
    except Exception:
        pass

    return blocks


def _scan_for_blocks(gdb_block, blocks, found_names):
    """Recursively scan GDB blocks for BlockCtx variables and their ranges."""
    """Escaneia recursivamente blocos GDB em busca de variaveis BlockCtx e suas faixas."""
    for symbol in gdb_block:
        try:
            if symbol.is_argument or symbol.is_variable:
                name = symbol.name
                if name in found_names:
                    continue
                sym_type = str(symbol.type)
                if 'BlockCtx' in sym_type:
                    val = symbol.value(gdb.selected_frame())
                    if val.type.code == gdb.TYPE_CODE_PTR:
                        ptr = val
                    elif val.type.code == gdb.TYPE_CODE_STRUCT:
                        ptr = val.address
                    else:
                        continue
                    try:
                        block_ctx = ptr.dereference()
                        data_ptr = int(block_ctx['data'])
                        capacity = int(block_ctx['capacity'])
                        blocks.append((name, data_ptr, data_ptr + capacity, ptr))
                        found_names.add(name)
                    except Exception:
                        pass
        except Exception:
            pass
    for child in gdb_block:
        _scan_for_blocks(child, blocks, found_names)


def block_alloc_lookup(val):
    """Lookup function for block-allocated pointers.
    Checks if a pointer falls within any known BlockCtx's data range."""
    """Funcao de busca para ponteiros alocados em bloco.
    Verifica se um ponteiro cai dentro da faixa de dados de algum BlockCtx conhecido."""
    if val.type.code != gdb.TYPE_CODE_PTR:
        return None
    try:
        addr = int(val)
    except Exception:
        return None
    if addr == 0:
        return None
    try:
        frame = gdb.selected_frame()
        blocks = _find_block_ranges(frame)
        for name, start, end, ctx_ptr in blocks:
            if start <= addr < end:
                offset = addr - start
                return BlockAllocPrinter(val, name, offset, ctx_ptr.dereference())
    except Exception:
        pass
    return None


def register_printers():
    """Register all Brick pretty-printers with GDB"""
    """Registra todos os pretty-printers Brick no GDB"""

    def block_ctx_lookup(val):
        if val.type.name in ('BlockCtx', 'struct BlockCtx'):
            return BlockCtxPrinter(val)
        return None

    def brick_string_lookup(val):
        if val.type.name in ('BrickString', 'struct BrickString'):
            return BrickStringPrinter(val)
        return None

    gdb.pretty_printers.append(block_ctx_lookup)
    gdb.pretty_printers.append(brick_string_lookup)
    gdb.pretty_printers.append(block_alloc_lookup)


register_printers()
