#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cstdlib>
#include <cstdio>
#include <unistd.h>
#include <sys/stat.h>
#include <cctype>
#include <cstring>
#include <algorithm>
#include <sys/wait.h>
#include <signal.h>
#include "lexer/lexer.h"
#include "parser/parser.h"
#include "parser/package.h"
#include "parser/macro_expander.h"
#include "parser/build_eval.h"
#include "codegen/codegen.h"
#include "shared/lsp.h"
#include "shared/version.h"
#include "embedded_runtime.h"
#ifdef BRICK_TRACK_BLOCKS
#include "memvis.h"
#endif

void print_usage() {
    std::cerr << "Brick Compiler v" << BRICK_VERSION_STRING << "\n";
    std::cerr << "Usage:\n";
    std::cerr << "  brick <input.brc> [-o output.c] [--lsp]\n";
    std::cerr << "  brick build <input.brc> [-o output] [--release]\n";
    std::cerr << "  brick run   <input.brc> [--release]\n";
#ifdef BRICK_TRACK_BLOCKS
    std::cerr << "  brick --visualize <file>   compile, run and visualize memory blocks\n";
    std::cerr << "  brick --attach <pid>       attach visualizer to running process\n";
#endif
    std::cerr << "  brick bind <c_header.h>   generate .brc bindings from C header\n";
    std::cerr << "  brick --help\n";
    std::cerr << "  brick --version\n";
    std::cerr << "\n";
    std::cerr << "  --release   omit tracking overhead (max performance, no visualizer)\n";
}

std::string read_file(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "error: could not open " << path << "\n";
        exit(1);
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

static bool write_file(const std::string& path, const char* data, size_t len) {
    FILE* f = fopen(path.c_str(), "wb");
    if (!f) return false;
    fwrite(data, 1, len, f);
    fclose(f);
    return true;
}

static void ensure_parent_dirs(const std::string& path) {
    size_t pos = 0;
    while ((pos = path.find('/', pos)) != std::string::npos) {
        std::string sub = path.substr(0, pos);
        mkdir(sub.c_str(), 0755);
        pos++;
    }
}

static int cmd_build(const std::string& input, const std::string& output,
                     bool run_mode, bool release_mode) {
    // 1. Temp dir for build artifacts
    // 1. Diretorio temporario para artefatos de build
    char tmpdir[] = "/tmp/brick-build-XXXXXX";
    if (!mkdtemp(tmpdir)) {
        std::cerr << "error: could not create temp directory\n";
        return 1;
    }
    std::string tmp = tmpdir;

    // 2. Extract embedded runtime files
    // 2. Extrai arquivos de runtime embutidos
    auto& info = brick::embedded::runtime_info;
    for (int i = 0; i < info.num_sources; i++) {
        auto& f = info.sources[i];
        std::string fp = tmp + "/" + f.name;
        ensure_parent_dirs(fp);
        if (!write_file(fp, f.content, f.length)) {
            std::cerr << "error: could not write " << fp << "\n";
            return 1;
        }
    }

    // 3. Compile .brc -> .c
    // 3. Compila .brc -> .c
    std::string source = read_file(input);
    auto tokens = brick::tokenize(source, input);
    auto parse_result = brick::parse(tokens);
    if (!parse_result.errors.empty()) {
        for (const auto& e : parse_result.errors)
            std::cerr << "error: " << e << "\n";
        return 1;
    }
    if (!parse_result.ast) return 1;

    auto packages = brick::resolve_packages(parse_result.ast, input);
    std::vector<std::unique_ptr<brick::ProgramNode>> asts;
    asts.push_back(std::move(parse_result.ast));

    brick::MacroTable macro_table;
    brick::collect_macros(asts, macro_table);
    auto build_result = brick::eval_build_blocks(asts, macro_table);
    for (const auto& e : build_result.errors)
        std::cerr << "error: " << e << "\n";
    if (!build_result.success) return 1;

    auto expand_result = brick::expand_macros(asts, macro_table);
    for (const auto& e : expand_result.errors)
        std::cerr << "error: " << e << "\n";
    if (!expand_result.success) return 1;

    auto codegen_result = brick::generate_c(asts, packages);
    for (const auto& e : codegen_result.errors)
        std::cerr << "error: " << e << "\n";
    if (!codegen_result.success) return 1;

    std::string c_path = tmp + "/_gen.c";
    {
        std::ofstream out(c_path);
        if (!out.is_open()) {
            std::cerr << "error: could not write " << c_path << "\n";
            return 1;
        }
        out << codegen_result.c_code;
    }

    // 4. Collect .c sources
    // 4. Coleta fontes .c
    std::string srcs;
    for (int i = 0; i < info.num_sources; i++) {
        std::string name = info.sources[i].name;
        size_t len = name.size();
        if (len >= 2 && name.substr(len - 2) == ".c")
            srcs += " " + tmp + "/" + name;
    }

    // 5. Collect link flags
    // 5. Coleta flags de linkagem
    std::string flags;
    for (int i = 0; i < info.num_flags; i++)
        flags += " " + std::string(info.flags[i]);
    for (const auto& lf : codegen_result.link_flags)
        flags += " -l" + lf;

    // 6. Invoke C compiler
    // 6. Invoca compilador C
    std::string bin = run_mode ? (tmp + "/_run") : output;
    std::string track = release_mode ? "" : " -DBRICK_TRACK_BLOCKS";
    std::string cmd = std::string(info.cc) + " -O3" + track + " -I" + tmp + " "
        + c_path + srcs + " -o " + bin + flags;

    std::cerr << "[build] " << info.cc << " -O3 ... -o " << bin << "\n";
    int ret = system(cmd.c_str());
    ret = WEXITSTATUS(ret);

    if (ret != 0) {
        std::cerr << "[build] failed (exit " << ret << ")\n";
        // Keep temp dir for debugging
        // Mantem diretorio temp para debug
        std::cerr << "[build] artifacts left at " << tmp << "\n";
        return ret;
    }

    // 7. Move binary to final location if built to temp
    // 7. Move binario para local final se construido em temp
    if (run_mode) {
        std::string mv = "mv " + bin + " " + output;
        system(mv.c_str());
    }

    // 8. Cleanup temp
    // 8. Limpa diretorio temporario
    std::string rm = "rm -rf " + tmp;
    system(rm.c_str());

    std::cerr << "[build] " << output << " ready\n";
    return 0;
}

static std::string map_c_type(const std::string& c_type) {
    if (c_type == "int") return "i32";
    if (c_type == "long") return "i64";
    if (c_type == "short") return "i16";
    if (c_type == "char") return "i8";
    if (c_type == "float") return "f32";
    if (c_type == "double") return "f64";
    if (c_type == "size_t") return "usize";
    if (c_type == "ssize_t") return "isize";
    if (c_type == "void") return "void";
    if (c_type == "unsigned char" || c_type == "uchar") return "u8";
    if (c_type == "unsigned short") return "u16";
    if (c_type == "unsigned int") return "u32";
    if (c_type == "unsigned long") return "u64";
    if (c_type.find("struct ") == 0) return c_type.substr(7);
    if (c_type.find("enum ") == 0) return c_type.substr(5);
    return c_type; // pass through for unknown types
}

static int cmd_bind(const std::string& header_path) {
    std::string source = read_file(header_path);
    std::string output = header_path;
    auto pos = output.rfind('.');
    if (pos != std::string::npos) output = output.substr(0, pos);
    output += "_bindings.brc";

    // Simple regex-like extraction: find function declarations
    // Extracao simples tipo regex: encontra declaracoes de funcao
    std::string result;
    result += "// Auto-generated bindings for " + header_path + "\n";
    result += "// Generated by `brick bind " + header_path + "`\n\n";

    // Extract include path
    result += "include \"" + header_path + "\"\n\n";

    // Simple heuristic: find lines matching "type name(type param, ...);"
    // Heuristica simples: encontra linhas tipo "tipo nome(tipo param, ...);"
    size_t i = 0;
    while (i < source.size()) {
        // Skip preprocessor lines, comments, etc.
        // Pula linhas de preprocessador, comentarios, etc.
        while (i < source.size() && (source[i] == '#' || source[i] == '\n' || source[i] == '\r')) {
            while (i < source.size() && source[i] != '\n') i++;
            if (i < source.size()) i++;
        }

        // Find a line that starts with a return type keyword
        // Encontra linha que comeca com keyword de tipo de retorno
        static const char* type_keywords[] = {
            "int", "void", "char", "long", "short", "float", "double",
            "unsigned", "signed", "size_t", "ssize_t", "int8_t", "uint8_t",
            "int16_t", "uint16_t", "int32_t", "uint32_t", "int64_t", "uint64_t",
            "FILE", "pid_t", "off_t", "struct ", "enum "
        };

        // Skip whitespace
        // Pula espacos
        while (i < source.size() && (source[i] == ' ' || source[i] == '\t')) i++;

        bool matched = false;
        for (const char* kw : type_keywords) {
            size_t klen = strlen(kw);
            if (i + klen <= source.size() &&
                source.compare(i, klen, kw) == 0 &&
                (i + klen >= source.size() || !std::isalnum(source[i + klen]))) {
                // Found potential return type
                // Encontrou tipo de retorno potencial

                // Find end of return type
                // Encontra fim do tipo de retorno
                size_t start = i;
                while (i < source.size() && source[i] != ';' && source[i] != '{' && source[i] != '\n') {
                    if (source[i] == '(') break;
                    if (source[i] == '/' && i + 1 < source.size() && source[i + 1] == '*')
                        break;
                    i++;
                }

                if (i < source.size() && source[i] == '(') {
                    size_t paren_start = i;
                    // Find matching closing paren
                    // Encontra parentese de fechamento correspondente
                    int depth = 0;
                    size_t paren_end = paren_start;
                    while (paren_end < source.size()) {
                        if (source[paren_end] == '(') depth++;
                        else if (source[paren_end] == ')') {
                            depth--;
                            if (depth == 0) break;
                        }
                        else if (source[paren_end] == '/' && paren_end + 1 < source.size()
                                 && source[paren_end + 1] == '/')
                            break;
                        paren_end++;
                    }

                    if (depth == 0 && paren_end < source.size()) {
                        // Extract function name (last token before '(')
                        // Extrai nome da funcao (ultimo token antes de '(')
                        std::string ret_type_str = source.substr(start, paren_start - start);
                        // Trim trailing spaces/tabs
                        // Remove espacos/tabs finais
                        while (!ret_type_str.empty() &&
                               (ret_type_str.back() == ' ' || ret_type_str.back() == '\t'))
                            ret_type_str.pop_back();

                        // Function name is after return type
                        // Nome da funcao esta depois do tipo de retorno
                        std::string func_name;
                        size_t name_start = ret_type_str.rfind(' ');
                        if (name_start != std::string::npos) {
                            func_name = ret_type_str.substr(name_start + 1);
                            ret_type_str = ret_type_str.substr(0, name_start);
                            // Trim trailing for ret type
                            // Remove espacos finais do tipo de retorno
                            while (!ret_type_str.empty() &&
                                   (ret_type_str.back() == ' ' || ret_type_str.back() == '\t' ||
                                    ret_type_str.back() == '*'))
                                ret_type_str.pop_back();
                        }

                        // Only emit if we have a reasonable function name and type
                        // So emite se temos um nome de funcao e tipo razoaveis
                        if (!func_name.empty() && !ret_type_str.empty() &&
                            isalpha(func_name[0])) {
                            result += "extern fn " + func_name + "(";

                            // Parse parameters
                            // Analisa parametros
                            std::string params_str = source.substr(paren_start + 1,
                                paren_end - paren_start - 1);
                            // Remove void parameter
                            // Remove parametro void
                            if (params_str.find("void") != std::string::npos) {
                                // no params
                            } else if (!params_str.empty()) {
                                // Split by comma
                                // Divide por virgula
                                // Just count params with generic names
                                // So conta params com nomes genericos
                                result += "...";
                            }
                            result += ")";

                            // Map return type
                            // Mapeia tipo de retorno
                            result += " -> " + map_c_type(ret_type_str);
                            result += "\n";
                            matched = true;
                        }
                    }
                }
                break;
            }
        }

        if (!matched) {
            if (i < source.size()) i++;
        }
    }

    // Write output
    // Escreve saida
    std::ofstream out(output);
    if (!out.is_open()) {
        std::cerr << "error: could not write " << output << "\n";
        return 1;
    }
    out << result;
    out.close();
    std::cerr << "[bind] wrote " << output << " (" << std::count(result.begin(), result.end(), '\n')
              << " lines)\n";
    return 0;
}

static int cmd_run(const std::string& input) {
    // Build to a temp binary
    // Compila para binario temporario
    char tmpdir[] = "/tmp/brick-run-XXXXXX";
    if (!mkdtemp(tmpdir)) {
        std::cerr << "error: could not create temp directory\n";
        return 1;
    }
    std::string out = std::string(tmpdir) + "/prog";

    int ret = cmd_build(input, out, false, false);
    if (ret != 0) {
        std::string rm = "rm -rf " + std::string(tmpdir);
        system(rm.c_str());
        return ret;
    }

    // Execute
    // Executa
    std::cerr << "[run] " << out << "\n";
    ret = system(out.c_str());
    ret = WEXITSTATUS(ret);

    // Cleanup
    // Limpeza
    std::string rm = "rm -rf " + std::string(tmpdir);
    system(rm.c_str());

    return ret;
}

int main(int argc, char** argv) {
    if (argc < 2) {
        print_usage();
        return 1;
    }

    std::string subcommand;
    std::string input_file;
    std::string output_file;
    bool lsp_mode = false;
    int arg_idx = 1;

    // Check for subcommand (build / run)
    // Verifica subcomando (build / run)
    std::string first = argv[1];
    if (first == "bind") {
        if (argc < 3) {
            std::cerr << "error: missing C header file\n";
            return 1;
        }
        return cmd_bind(argv[2]);
    }
    if (first == "build" || first == "run") {
        subcommand = first;
        arg_idx = 2;
        if (argc < 3) {
            std::cerr << "error: missing input file\n";
            return 1;
        }
    }

    // ─── Visualizer flags ──────────────────────────────────────
    // ─── Flags do Visualizador ──────────────────────────────────────
    bool visualize_mode = false;
    int attach_pid = 0;
#ifdef BRICK_TRACK_BLOCKS
    for (int i = arg_idx; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--visualize") {
            visualize_mode = true;
        }
        if (arg == "--attach" && i + 1 < argc) {
            attach_pid = std::stoi(argv[++i]);
        }
    }
#endif

    bool build_release = false;

    for (int i = arg_idx; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--help") {
            print_usage();
            return 0;
        } else if (arg == "--version") {
            std::cout << "Brick Compiler v" << BRICK_VERSION_STRING << "\n";
            return 0;
        } else if (arg == "--lsp") {
            lsp_mode = true;
        } else if (arg == "--release") {
            build_release = true;
        } else if (arg == "--visualize") {
            // handled above
        } else if (arg == "--attach") {
            i++; // skip PID arg
        } else if (arg == "-o" && i + 1 < argc) {
            output_file = argv[++i];
        } else {
            input_file = arg;
        }
    }

    // ─── Visualize mode: compile + run + attach TUI ────────────
    // ─── Modo visualizar: compila + executa + anexa TUI ────────
#ifdef BRICK_TRACK_BLOCKS
    if (visualize_mode) {
        if (input_file.empty()) {
            std::cerr << "error: --visualize requires a .brc file (or try --attach <pid>)\n";
            return 1;
        }

        // Build to temp binary
        // Compila para binario temporario
        char tmpdir[] = "/tmp/brick-viz-XXXXXX";
        if (!mkdtemp(tmpdir)) {
            std::cerr << "error: could not create temp directory\n";
            return 1;
        }
        std::string bin = std::string(tmpdir) + "/prog";
        int ret = cmd_build(input_file, bin, false, false);
        if (ret != 0) {
            std::string rm = "rm -rf " + std::string(tmpdir);
            system(rm.c_str());
            return ret;
        }

        // Fork: child runs binary, parent attaches visualizer
        // Fork: filho executa binario, pai anexa visualizador
        pid_t pid = fork();
        if (pid == 0) {
            // Child: exec binary
            // Filho: executa binario
            execl(bin.c_str(), bin.c_str(), nullptr);
            _exit(1);
        }

        // Parent: wait briefly for shm to be created, then attach
        // Pai: espera brevemente o shm ser criado, entao anexa
        usleep(150000); // 150ms

        MemVisConfig cfg = MEMVIS_DEFAULT_CONFIG;
        std::cerr << "[viz] attached to PID " << pid << " ...\n";
        memvis_attach(pid, cfg);

        // When visualizer exits, kill child and cleanup
        // Quando visualizador sair, mata filho e limpa
        kill(pid, SIGTERM);
        waitpid(pid, nullptr, 0);
        std::string rm = "rm -rf " + std::string(tmpdir);
        system(rm.c_str());
        return 0;
    }

    if (attach_pid > 0) {
        MemVisConfig cfg = MEMVIS_DEFAULT_CONFIG;
        memvis_attach(attach_pid, cfg);
        return 0;
    }
#endif

    if (input_file.empty()) {
        std::cerr << "error: no input file\n";
        return 1;
    }

    // ─── build / run ───────────────────────────────────────────
    // ─── build / run ───────────────────────────────────────────
    if (subcommand == "build") {
        if (output_file.empty()) {
            auto pos = input_file.rfind('.');
            output_file = (pos != std::string::npos)
                ? input_file.substr(0, pos) : input_file;
        }
        if (lsp_mode)
            std::cerr << "warning: --lsp has no effect in build mode\n";
        return cmd_build(input_file, output_file, false, build_release);
    }

    if (subcommand == "run") {
        if (lsp_mode)
            std::cerr << "warning: --lsp has no effect in run mode\n";
        if (build_release)
            std::cerr << "warning: --release has no effect in run mode (use with build)\n";
        return cmd_run(input_file);
    }

    // ─── Compile only (backward compatible) ────────────────────
    // ─── Compile only (backward compatible) ────────────────────
    if (output_file.empty()) {
        auto pos = input_file.rfind('.');
        output_file = (pos != std::string::npos
            ? input_file.substr(0, pos) : input_file) + ".c";
    }

    std::string source = read_file(input_file);

    brick::LspOutput lsp_out;

    auto tokens = brick::tokenize(source, input_file);
    if (lsp_mode) {
        lsp_out.tokens = tokens;
    } else {
        std::cout << "[lexer] " << tokens.size() << " tokens\n";
    }

    auto parse_result = brick::parse(tokens);
    if (!parse_result.errors.empty()) {
        for (const auto& err : parse_result.errors) {
            if (lsp_mode) {
                lsp_out.errors.push_back(
                    brick::parse_error_string(err, input_file));
            } else {
                std::cerr << "error: " << err << "\n";
            }
        }
        if (!lsp_mode) return 1;
    }

    if (!parse_result.ast) {
        if (!lsp_mode) return 1;
        std::cout << brick::emit_lsp_json(lsp_out);
        return 1;
    }
    if (!lsp_mode) std::cout << "[parser] AST built\n";

    auto packages = brick::resolve_packages(parse_result.ast, input_file);
    if (!lsp_mode) std::cout << "[package] resolved\n";

    std::vector<std::unique_ptr<brick::ProgramNode>> asts;
    asts.push_back(std::move(parse_result.ast));

    // Macro pipeline: collect -> eval build -> expand
    // Pipeline de macros: coleta -> avalia build -> expande
    brick::MacroTable macro_table;
    brick::collect_macros(asts, macro_table);
    if (!macro_table.empty() && !lsp_mode)
        std::cout << "[macro] " << macro_table.size() << " macros collected\n";

    auto build_result = brick::eval_build_blocks(asts, macro_table);
    for (const auto& e : build_result.errors) {
        if (lsp_mode) {
            lsp_out.errors.push_back(brick::parse_error_string(e, input_file));
        } else {
            std::cerr << "error: " << e << "\n";
        }
    }
    if (!build_result.success && !lsp_mode) return 1;

    auto expand_result = brick::expand_macros(asts, macro_table);
    for (const auto& e : expand_result.errors) {
        if (lsp_mode) {
            lsp_out.errors.push_back(brick::parse_error_string(e, input_file));
        } else {
            std::cerr << "error: " << e << "\n";
        }
    }
    if (!expand_result.success && !lsp_mode) return 1;
    if (!lsp_mode) std::cout << "[macro] expansion done\n";

    auto codegen_result = brick::generate_c(asts, packages);

    for (const auto& err : codegen_result.errors) {
        if (lsp_mode) {
            lsp_out.errors.push_back(
                brick::parse_error_string(err, input_file));
        } else {
            std::cerr << "error: " << err << "\n";
        }
    }
    if (!codegen_result.success && !lsp_mode) return 1;
    if (!lsp_mode) std::cout << "[codegen] C code generated\n";

    if (!asts.empty() && asts[0]) {
        brick::collect_symbols(asts[0].get(), lsp_out.symbols);
    }

    if (lsp_mode) {
        std::cout << brick::emit_lsp_json(lsp_out);
        return lsp_out.errors.empty() && codegen_result.success ? 0 : 1;
    }

    std::ofstream out(output_file);
    if (!out.is_open()) {
        std::cerr << "error: could not write " << output_file << "\n";
        return 1;
    }
    out << codegen_result.c_code;
    out.close();

    std::cout << "[output] " << output_file << "\n";
    return 0;
}
