#ifndef BRICK_PACKAGE_H
#define BRICK_PACKAGE_H

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <set>
#include "ast.h"

namespace brick {

struct PackageInfo {
    std::string full_name;
    std::vector<std::string> parts;
    std::vector<ASTNode*> declarations;
    bool visited = false;

    // Symbols exported by this package
    // Simbolos exportados por este pacote
    std::set<std::string> exported_structs;
    std::set<std::string> exported_functions;
};

struct PackageTable {
    // Map package name → info
    // Mapeia nome do pacote → info
    std::unordered_map<std::string, PackageInfo*> packages;

    // Owned PackageInfo storage
    // Armazenamento proprio de PackageInfo
    std::vector<std::unique_ptr<PackageInfo>> _owned;

    // Current file's package
    // Pacote do arquivo atual
    std::string current_package;

    // Track files parsed
    // Rastreia arquivos analisados
    std::vector<std::string> parsed_files;
};

// Resolve packages from a parsed AST
// Resolve pacotes a partir de uma AST analisada
// Returns a PackageTable with all packages resolved
// Retorna uma PackageTable com todos os pacotes resolvidos
PackageTable resolve_packages(
    std::unique_ptr<ProgramNode>& ast,
    const std::string& filename
);

// Merge multiple package tables from different files
// Mescla multiplas tabelas de pacotes de arquivos diferentes
void merge_package_tables(PackageTable& global, PackageTable&& local);

// Check if a name is accessible from the given package
// Verifica se um nome e acessivel a partir do pacote informado
bool is_accessible(const PackageTable& table,
                   const std::string& from_package,
                   const std::string& symbol_name);

} // namespace brick
  // namespace brick

#endif
