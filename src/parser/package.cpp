#include "package.h"
#include <stdexcept>

namespace brick {

PackageTable resolve_packages(
    std::unique_ptr<ProgramNode>& ast,
    const std::string& filename)
{
    PackageTable table;
    table.parsed_files.push_back(filename);

    for (auto& decl : ast->declarations) {
        if (decl->type == ASTNodeType::PACKAGE_DECL) {
            auto* pkg = static_cast<PackageDecl*>(decl.get());
            std::string full_name;
            for (const auto& part : pkg->name_parts) {
                if (!full_name.empty()) full_name += ".";
                full_name += part;
            }
            table.current_package = full_name;
        }
    }

    if (table.current_package.empty()) {
        table.current_package = "__main__";
    }

    auto ensure_package = [&](const std::string& name) -> PackageInfo* {
        auto it = table.packages.find(name);
        if (it == table.packages.end()) {
            auto info = std::make_unique<PackageInfo>();
            info->full_name = name;
            PackageInfo* ptr = info.get();
            table.packages[name] = ptr;
            table._owned.push_back(std::move(info));
            return ptr;
        }
        return it->second;
    };

    PackageInfo* current = ensure_package(table.current_package);
    current->visited = true;

    for (auto& decl : ast->declarations) {
        if (decl->type == ASTNodeType::STRUCT_DECL) {
            auto* sd = static_cast<StructDecl*>(decl.get());
            current->declarations.push_back(sd);
            if (!sd->is_private) {
                current->exported_structs.insert(sd->name);
            }
        } else if (decl->type == ASTNodeType::FUNC_DECL) {
            auto* fd = static_cast<FuncDecl*>(decl.get());
            current->declarations.push_back(fd);
            if (!fd->is_private) {
                current->exported_functions.insert(fd->name);
            }
        } else {
            current->declarations.push_back(decl.get());
        }
    }

    return table;
}

void merge_package_tables(PackageTable& global, PackageTable&& local) {
    auto ensure_package = [&](const std::string& name) -> PackageInfo* {
        auto it = global.packages.find(name);
        if (it == global.packages.end()) {
            auto info = std::make_unique<PackageInfo>();
            info->full_name = name;
            PackageInfo* ptr = info.get();
            global.packages[name] = ptr;
            global._owned.push_back(std::move(info));
            return ptr;
        }
        return it->second;
    };

    for (auto& [name, info] : local.packages) {
        PackageInfo* global_info = ensure_package(name);
        for (const auto& s : info->exported_structs) {
            global_info->exported_structs.insert(s);
        }
        for (const auto& f : info->exported_functions) {
            global_info->exported_functions.insert(f);
        }
        for (auto* decl : info->declarations) {
            global_info->declarations.push_back(decl);
        }
        global_info->visited = global_info->visited || info->visited;
    }

    for (const auto& f : local.parsed_files) {
        global.parsed_files.push_back(f);
    }
}

bool is_accessible(const PackageTable& table,
                   const std::string& from_package,
                   const std::string& symbol_name)
{
    for (auto& [name, info] : table.packages) {
        if (info->exported_structs.count(symbol_name) ||
            info->exported_functions.count(symbol_name)) {
            return true;
        }
        if (name == from_package) {
            for (auto* decl : info->declarations) {
                if (decl->type == ASTNodeType::STRUCT_DECL) {
                    auto* sd = static_cast<StructDecl*>(decl);
                    if (sd->name == symbol_name) return true;
                }
                if (decl->type == ASTNodeType::FUNC_DECL) {
                    auto* fd = static_cast<FuncDecl*>(decl);
                    if (fd->name == symbol_name) return true;
                }
            }
        }
    }
    return false;
}

} // namespace brick
