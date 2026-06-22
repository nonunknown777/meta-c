#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cstdlib>
#include <cstdio>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <signal.h>
#include "lexer/lexer.h"
#include "parser/parser.h"
#include "parser/package.h"
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
