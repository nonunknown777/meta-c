#include "type_checker.h"
#include <sstream>
#include <unordered_set>
#include <cstdint>
#include <cfloat>
#include <algorithm>

namespace brick {

// ─── Type helper utilities ───
// ─── Utilitarios de tipo ───

static std::string normalize_type(const std::string& t) {
    if (t == "byte" || t == "char") return "u8";
    if (t == "short") return "i16";
    if (t == "int") return "i32";
    if (t == "long") return "i64";
    if (t == "float") return "f32";
    if (t == "double") return "f64";
    return t;
}

static bool is_signed_int(const std::string& t) {
    std::string n = normalize_type(t);
    return n == "i8" || n == "i16" || n == "i32" || n == "i64" || n == "isize";
}

static bool is_unsigned_int(const std::string& t) {
    std::string n = normalize_type(t);
    return n == "u8" || n == "u16" || n == "u32" || n == "u64" || n == "usize" || n == "bool";
}

static bool is_float_type(const std::string& t) {
    std::string n = normalize_type(t);
    return n == "f32" || n == "f64";
}

static int type_rank(const std::string& t) {
    std::string n = normalize_type(t);
    if (n == "u8" || n == "i8" || n == "bool") return 1;
    if (n == "u16" || n == "i16") return 2;
    if (n == "u32" || n == "i32") return 3;
    if (n == "u64" || n == "i64" || n == "usize" || n == "isize") return 4;
    if (n == "f32") return 5;
    if (n == "f64") return 6;
    return 0;
}

static std::string signed_type_for_rank(int r) {
    if (r <= 1) return "i8";
    if (r == 2) return "i16";
    if (r == 3) return "i32";
    return "i64";
}

static bool int_fits_in_type(int64_t value, const std::string& type) {
    std::string n = normalize_type(type);
    if (n == "u8")  return value >= 0 && value <= UINT8_MAX;
    if (n == "u16") return value >= 0 && value <= UINT16_MAX;
    if (n == "u32") return value >= 0 && value <= UINT32_MAX;
    if (n == "u64") return value >= 0;
    if (n == "i8")  return value >= INT8_MIN  && value <= INT8_MAX;
    if (n == "i16") return value >= INT16_MIN && value <= INT16_MAX;
    if (n == "i32") return value >= INT32_MIN && value <= INT32_MAX;
    if (n == "i64") return true;
    if (n == "usize") return value >= 0;
    if (n == "isize") return true;
    return true;
}

static bool float_fits_in_type(double value, const std::string& type) {
    std::string n = normalize_type(type);
    if (n == "f32") return value >= -FLT_MAX && value <= FLT_MAX;
    if (n == "f64") return true;
    return true;
}

TypeChecker::TypeChecker(const PackageTable& packages)
    : packages(packages)
{
    declare_builtins();
}

void TypeChecker::declare_builtins() {
    // Built-in types are always known
    // Tipos embutidos sao sempre conhecidos
    // Push global scope
    // Empilha escopo global
    push_scope();
}

void TypeChecker::push_scope() {
    scopes.emplace_back();
}

void TypeChecker::pop_scope() {
    if (!scopes.empty()) scopes.pop_back();
}

void TypeChecker::declare(const std::string& name, const std::string& type,
                          bool is_private, bool is_param) {
    if (scopes.empty()) push_scope();
    auto& scope = scopes.back();
    scope[name] = SymbolInfo{type, is_private, is_param};
}

SymbolInfo* TypeChecker::lookup(const std::string& name) {
    for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
        auto found = it->find(name);
        if (found != it->end()) return &found->second;
    }
    return nullptr;
}

bool TypeChecker::is_type_known(const std::string& type_name) {
    static const std::unordered_set<std::string> builtins = {
        "int", "float", "bool", "char", "String", "void", "block", "null",
        "u8", "u16", "u32", "u64",
        "i8", "i16", "i32", "i64",
        "f32", "f64",
        "usize", "isize",
        "byte", "short", "long",
    };
    if (builtins.count(type_name)) return true;
    if (struct_defs.count(type_name)) return true;
    if (interface_defs.count(type_name)) return true;

    // Check if it's an array type like "int[10]"
    // Verifica se e um tipo array como "int[10]"
    auto bracket = type_name.find('[');
    if (bracket != std::string::npos) {
        std::string base = type_name.substr(0, bracket);
        return is_type_known(base);
    }

    return false;
}

bool TypeChecker::is_struct_type(const std::string& type_name) {
    return struct_defs.count(type_name) > 0;
}

bool TypeChecker::is_interface_type(const std::string& type_name) {
    return interface_defs.count(type_name) > 0;
}

bool TypeChecker::can_assign(const std::string& from, const std::string& to) {
    if (from == to) return true;
    if (from == "null" || to == "null") return true;

    std::string f = normalize_type(from);
    std::string t = normalize_type(to);

    if (f == t) return true;

    // One of them is not a numeric type — allow only if same
    // Um deles nao e numerico — permite apenas se igual
    if (!is_signed_int(f) && !is_unsigned_int(f) && !is_float_type(f)) return false;
    if (!is_signed_int(t) && !is_unsigned_int(t) && !is_float_type(t)) return false;

    int f_rank = type_rank(f);
    int t_rank = type_rank(t);

    bool f_signed = is_signed_int(f);
    bool f_unsigned = is_unsigned_int(f);
    bool f_float = is_float_type(f);
    bool t_signed = is_signed_int(t);
    bool t_unsigned = is_unsigned_int(t);
    bool t_float = is_float_type(t);

    // Float → Int narrowing: always error
    // Float → Int estreitamento: sempre erro
    if (f_float && (t_signed || t_unsigned)) return false;

    // Int → Float widening: allow
    // Int → Float alargamento: permite
    if ((f_signed || f_unsigned) && t_float) return f_rank <= t_rank;

    // Float → Float: widening only
    if (f_float && t_float) return f_rank <= t_rank;

    // Signed ↔ Unsigned same rank: error
    // Signed ↔ Unsigned mesmo rank: erro
    if (f_signed != t_signed && f_rank == t_rank) return false;

    // Both int: widening only
    // Ambos int: apenas alargamento
    return f_rank <= t_rank;
}

std::string TypeChecker::promote_types(const std::string& a, const std::string& b) {
    std::string t1 = normalize_type(a);
    std::string t2 = normalize_type(b);

    if (t1 == t2) return a;

    bool f1 = is_float_type(t1);
    bool f2 = is_float_type(t2);

    // Exception: int + float → float (promote integer to float)
    // Excecao: int + float → float (promove inteiro para float)
    if (f1 && !f2) return t1;
    if (!f1 && f2) return t2;

    int r1 = type_rank(t1);
    int r2 = type_rank(t2);

    // Both float → larger
    // Ambos float → maior
    if (f1 && f2) return r1 >= r2 ? a : b;

    bool s1 = is_signed_int(t1);
    bool s2 = is_signed_int(t2);

    // Same sign → larger rank
    // Mesmo sinal → maior rank
    if (s1 == s2) return r1 >= r2 ? a : b;

    // Mixed signed/unsigned
    // Misto signed/unsigned
    std::string signed_t  = s1 ? t1 : t2;
    std::string unsigned_t = s1 ? t2 : t1;
    int r_s = s1 ? r1 : r2;
    int r_u = s1 ? r2 : r1;

    if (r_s >= r_u) return signed_t;
    return signed_type_for_rank(r_u + 1);
}

void TypeChecker::add_error(const std::string& msg) {
    errors.push_back(msg);
}

std::vector<std::string> TypeChecker::check(
    const std::vector<std::unique_ptr<ProgramNode>>& asts)
{
    errors.clear();

    // First pass: collect all struct and interface definitions
    // Primeira passada: coleta todas as definicoes de struct e interface
    for (const auto& ast : asts) {
        for (const auto& decl : ast->declarations) {
            switch (decl->type) {
                case ASTNodeType::PACKAGE_DECL: {
                    auto* pd = static_cast<PackageDecl*>(decl.get());
                    for (size_t i = 0; i < pd->name_parts.size(); i++) {
                        if (i > 0) current_package += ".";
                        current_package += pd->name_parts[i];
                    }
                    break;
                }
                case ASTNodeType::STRUCT_DECL:
                    struct_defs[static_cast<StructDecl*>(decl.get())->name]
                        = static_cast<StructDecl*>(decl.get());
                    break;
                case ASTNodeType::INTERFACE_DECL:
                    interface_defs[static_cast<InterfaceDecl*>(decl.get())->name]
                        = static_cast<InterfaceDecl*>(decl.get());
                    break;
                default:
                    break;
            }
        }
    }

    // Second pass: check all declarations
    // Segunda passada: verifica todas as declaracoes
    for (const auto& ast : asts) {
        for (const auto& decl : ast->declarations) {
            switch (decl->type) {
                case ASTNodeType::STRUCT_DECL:
                    check_struct(static_cast<StructDecl*>(decl.get()));
                    break;
                case ASTNodeType::INTERFACE_DECL:
                    check_interface(static_cast<InterfaceDecl*>(decl.get()));
                    break;
                case ASTNodeType::FUNC_DECL:
                    check_function(static_cast<FuncDecl*>(decl.get()), "");
                    break;
                case ASTNodeType::USING_DECL: {
                    auto* ud = static_cast<UsingDecl*>(decl.get());
                    if (ud->package_parts.size() == 1 && ud->package_parts[0] == "IO") {
                        using_io = true;
                    }
                    break;
                }
                case ASTNodeType::BLOCK_DECL: {
                    auto* bd = static_cast<BlockDecl*>(decl.get());
                    if (lookup(bd->name)) {
                        add_error(decl->location.file + ":" +
                                  std::to_string(decl->location.line) +
                                  ": duplicate block name '" + bd->name + "'");
                    }
                    declare(bd->name, "block", false);
                    break;
                }
                default:
                    break;
            }
        }
    }

    return errors;
}

void TypeChecker::check_struct(StructDecl* sd) {
    current_struct = sd->name;

    // Check extends
    // Verifica extends
    if (!sd->extends.empty()) {
        if (!struct_defs.count(sd->extends)) {
            add_error(sd->location.file + ":" + std::to_string(sd->location.line) +
                      ": struct '" + sd->name + "' extends unknown struct '" +
                      sd->extends + "'");
        }
        // TODO: check cyclic inheritance
        // TODO: verificar heranca ciclica
    }

    // Check interfaces
    // Verifica interfaces
    for (const auto& iface : sd->interfaces) {
        if (!interface_defs.count(iface)) {
            add_error(sd->location.file + ":" + std::to_string(sd->location.line) +
                      ": struct '" + sd->name + "' implements unknown interface '" +
                      iface + "'");
        }
    }

    // Check fields (including inherited)
    // Verifica campos (incluindo herdados)
    push_scope();
    declare_inherited_fields(sd);
    for (const auto& field : sd->fields) {
        if (field->type == ASTNodeType::FIELD_DECL) {
            auto* fd = static_cast<FieldDecl*>(field.get());
            if (!is_type_known(fd->type_name)) {
                add_error(sd->location.file + ":" + std::to_string(fd->location.line) +
                          ": unknown type '" + fd->type_name + "' in field '" +
                          fd->name + "'");
            }
            declare(fd->name, fd->type_name, fd->is_private);
        }
    }

    // Check methods
    // Verifica metodos
    for (const auto& method : sd->methods) {
        if (method->type == ASTNodeType::FUNC_DECL) {
            check_function(static_cast<FuncDecl*>(method.get()), sd->name);
        }
    }

    pop_scope();
    current_struct = "";
}

void TypeChecker::check_interface(InterfaceDecl* id) {
    // Interfaces only declare method signatures — just validate no conflicts
    // Interfaces so declaram assinaturas de metodo — so valida sem conflitos
    push_scope();
    for (const auto& method : id->methods) {
        if (method->type == ASTNodeType::FUNC_DECL) {
            auto* fd = static_cast<FuncDecl*>(method.get());
            if (lookup(fd->name)) {
                add_error(id->location.file + ":" + std::to_string(fd->location.line) +
                          ": duplicate method '" + fd->name + "' in interface '" +
                          id->name + "'");
            }
            declare(fd->name, "fn", false);
        }
    }
    pop_scope();
}

void TypeChecker::declare_inherited_fields(StructDecl* sd) {
    if (sd->extends.empty()) return;
    if (!struct_defs.count(sd->extends)) return;
    auto* base = struct_defs[sd->extends];
    // First recurse to declare grandparent fields
    // Primeiro recorre para declarar campos de avos
    declare_inherited_fields(base);
    for (const auto& f : base->fields) {
        if (f->type == ASTNodeType::FIELD_DECL) {
            auto* fd = static_cast<FieldDecl*>(f.get());
            if (!lookup(fd->name)) {
                declare(fd->name, fd->type_name, fd->is_private);
            }
        }
    }
}

void TypeChecker::check_function(FuncDecl* fd, const std::string& struct_name) {
    // Check constructor naming
    // Verifica nomenclatura do construtor
    if (!struct_name.empty()) {
        fd->is_constructor = (fd->name == struct_name);
    }

    // Check return type
    // Verifica tipo de retorno
    if (!fd->return_type.empty() && !is_type_known(fd->return_type)) {
        add_error(fd->location.file + ":" + std::to_string(fd->location.line) +
                  ": unknown return type '" + fd->return_type + "' in function '" +
                  fd->name + "'");
    }

    push_scope();

    // Declare 'this' for methods
    // Declara "this" para metodos
    if (!struct_name.empty()) {
        declare("this", struct_name + "*", false, true);
    }

    // Declare parameters
    // Declara parametros
    for (const auto& param : fd->params) {
        if (param->type == ASTNodeType::PARAM_DECL) {
            auto* pd = static_cast<ParamDecl*>(param.get());
            if (!is_type_known(pd->type_name)) {
                add_error(fd->location.file + ":" + std::to_string(pd->location.line) +
                          ": unknown parameter type '" + pd->type_name + "' in function '" +
                          fd->name + "'");
            }
            if (lookup(pd->name)) {
                add_error(fd->location.file + ":" + std::to_string(pd->location.line) +
                          ": duplicate parameter '" + pd->name + "' in function '" +
                          fd->name + "'");
            }
            declare(pd->name, pd->type_name, false, true);
        }
    }

    // Check body
    // Verifica corpo
    if (fd->body && fd->body->type == ASTNodeType::BLOCK_STMT) {
        check_block(static_cast<BlockStmt*>(fd->body.get()), fd->return_type);
    }

    pop_scope();
}

void TypeChecker::check_block(BlockStmt* block, const std::string& return_type) {
    push_scope();
    for (const auto& stmt : block->statements) {
        check_statement(stmt.get(), return_type);
    }
    pop_scope();
}

void TypeChecker::check_statement(ASTNode* stmt, const std::string& return_type) {
    switch (stmt->type) {
        case ASTNodeType::BLOCK_STMT:
            check_block(static_cast<BlockStmt*>(stmt), return_type);
            break;

        case ASTNodeType::IF_STMT: {
            auto* is = static_cast<IfStmt*>(stmt);
            std::string cond_type = check_expression(is->condition.get());
            if (!can_assign(cond_type, "bool")) {
                add_error(stmt->location.file + ":" + std::to_string(stmt->location.line) +
                          ": if condition must be bool, got '" + cond_type + "'");
            }
            check_statement(is->then_branch.get(), return_type);
            if (is->else_branch) {
                check_statement(is->else_branch.get(), return_type);
            }
            break;
        }

        case ASTNodeType::WHILE_STMT: {
            auto* ws = static_cast<WhileStmt*>(stmt);
            std::string cond_type = check_expression(ws->condition.get());
            if (!can_assign(cond_type, "bool")) {
                add_error(stmt->location.file + ":" + std::to_string(stmt->location.line) +
                          ": while condition must be bool, got '" + cond_type + "'");
            }
            check_statement(ws->body.get(), return_type);
            break;
        }

        case ASTNodeType::FOR_STMT: {
            auto* fs = static_cast<ForStmt*>(stmt);
            if (fs->init) check_statement(fs->init.get(), return_type);
            if (fs->condition) {
                std::string cond_type = check_expression(fs->condition.get());
                if (!can_assign(cond_type, "bool")) {
                    add_error(stmt->location.file + ":" + std::to_string(stmt->location.line) +
                              ": for condition must be bool, got '" + cond_type + "'");
                }
            }
            if (fs->increment) check_expression(fs->increment.get());
            check_statement(fs->body.get(), return_type);
            break;
        }

        case ASTNodeType::RETURN_STMT: {
            auto* rs = static_cast<ReturnStmt*>(stmt);
            if (rs->value) {
                std::string val_type = check_expression(rs->value.get());
                if (return_type.empty() || return_type == "void") {
                    add_error(stmt->location.file + ":" + std::to_string(stmt->location.line) +
                              ": cannot return value from void function");
                } else if (!can_assign(val_type, return_type)) {
                    add_error(stmt->location.file + ":" + std::to_string(stmt->location.line) +
                              ": cannot return '" + val_type + "' from function returning '" +
                              return_type + "'");
                }
            } else {
                if (!return_type.empty() && return_type != "void") {
                    add_error(stmt->location.file + ":" + std::to_string(stmt->location.line) +
                              ": must return a value of type '" + return_type + "'");
                }
            }
            break;
        }

        case ASTNodeType::EXPR_STMT: {
            auto* es = static_cast<ExprStmt*>(stmt);
            // Check if this is a variable declaration: ExprStmt(Assignment(IdentExpr, value))
            // Verifica se e declaracao de variavel: ExprStmt(Assignment(IdentExpr, value))
            // where the target is not yet declared
            // onde o alvo ainda nao foi declarado
                if (es->expr->type == ASTNodeType::ASSIGNMENT) {
                    auto* assign = static_cast<Assignment*>(es->expr.get());
                    if (assign->target->type == ASTNodeType::IDENT_EXPR) {
                        auto* ident = static_cast<IdentExpr*>(assign->target.get());
                        if (!lookup(ident->name)) {
                            // This is a variable declaration - infer type from value
                            // Isto e uma declaracao de variavel — infere tipo do valor
                            std::string val_type = check_expression(assign->value.get());
                            // Check if unsuffixed literal fits in declared type
                            // Verifica se literal sem sufixo cabe no tipo declarado
                            if (!ident->declared_type.empty()) {
                                if (assign->value->type == ASTNodeType::INT_LITERAL) {
                                    auto* il = static_cast<IntLiteral*>(assign->value.get());
                                    if (il->literal_type.empty() && !int_fits_in_type(il->value, ident->declared_type)) {
                                        add_error(assign->location.file + ":" +
                                                  std::to_string(assign->location.line) +
                                                  ": value " + std::to_string(il->value) +
                                                  " does not fit in type '" + ident->declared_type + "'");
                                    }
                                }
                                val_type = ident->declared_type;
                            }
                            declare(ident->name, val_type, false);
                            assign->target->resolved_type = val_type;
                            return;
                        }
                    }
                }
            check_expression(es->expr.get());
            break;
        }

        case ASTNodeType::BLOCK_DECL: {
            // Block declarations are statements too (block game = 64MB)
            // Declaracoes de bloco tambem sao statements (block game = 64MB)
            auto* bd = static_cast<BlockDecl*>(stmt);
            if (lookup(bd->name)) {
                add_error(stmt->location.file + ":" + std::to_string(stmt->location.line) +
                          ": duplicate block name '" + bd->name + "'");
            }
            declare(bd->name, "block", false);
            break;
        }

        case ASTNodeType::BLOCK_SCOPE: {
            auto* bs = static_cast<BlockScope*>(stmt);
            push_scope();
            for (const auto& s : bs->body) {
                check_statement(s.get(), return_type);
            }
            pop_scope();
            break;
        }

        default: {
            std::string t = check_expression(stmt);
            break;
        }
    }
}

std::string TypeChecker::check_expression(ASTNode* expr) {
    if (!expr) return "void";

    switch (expr->type) {
        case ASTNodeType::INT_LITERAL: {
            auto* il = static_cast<IntLiteral*>(expr);
            if (!il->literal_type.empty()) {
                if (!int_fits_in_type(il->value, il->literal_type)) {
                    add_error(expr->location.file + ":" +
                              std::to_string(expr->location.line) +
                              ": value " + std::to_string(il->value) +
                              " does not fit in type '" + il->literal_type + "'");
                }
                expr->resolved_type = il->literal_type;
                return il->literal_type;
            }
            expr->resolved_type = "int";
            return "int";
        }

        case ASTNodeType::FLOAT_LITERAL: {
            auto* fl = static_cast<FloatLiteral*>(expr);
            if (!fl->literal_type.empty()) {
                if (!float_fits_in_type(fl->value, fl->literal_type)) {
                    add_error(expr->location.file + ":" +
                              std::to_string(expr->location.line) +
                              ": value does not fit in type '" + fl->literal_type + "'");
                }
                expr->resolved_type = fl->literal_type;
                return fl->literal_type;
            }
            expr->resolved_type = "float";
            return "float";
        }

        case ASTNodeType::STRING_LITERAL:
            expr->resolved_type = "String";
            return "String";

        case ASTNodeType::BOOL_LITERAL:
            expr->resolved_type = "bool";
            return "bool";

        case ASTNodeType::CHAR_LITERAL:
            expr->resolved_type = "char";
            return "char";

        case ASTNodeType::NULL_LITERAL:
            expr->resolved_type = "null";
            return "null";

        case ASTNodeType::IDENT_EXPR: {
            auto* ident = static_cast<IdentExpr*>(expr);
            auto* sym = lookup(ident->name);
            if (!sym) {
                add_error(expr->location.file + ":" + std::to_string(expr->location.line) +
                          ": undefined symbol '" + ident->name + "'");
                expr->resolved_type = "unknown";
                return "unknown";
            }
            expr->resolved_type = sym->type;
            return sym->type;
        }

        case ASTNodeType::BINARY_OP: {
            auto* bin = static_cast<BinaryOp*>(expr);
            std::string left_type = check_expression(bin->left.get());
            std::string right_type = check_expression(bin->right.get());

            // Arithmetic operators
            // Operadores aritmeticos
            if (bin->op == TokenType::PLUS || bin->op == TokenType::MINUS ||
                bin->op == TokenType::STAR || bin->op == TokenType::SLASH) {
                if (!can_assign(left_type, "int") && !can_assign(left_type, "float") &&
                    !can_assign(left_type, "i32") && !can_assign(left_type, "f64")) {
                    add_error(expr->location.file + ":" + std::to_string(expr->location.line) +
                              ": arithmetic op requires numeric types, got '" +
                              left_type + "' and '" + right_type + "'");
                }
                std::string result = promote_types(left_type, right_type);
                expr->resolved_type = result;
                return result;
            }

            // Comparison operators
            // Operadores de comparacao
            if (bin->op == TokenType::EQ || bin->op == TokenType::NEQ ||
                bin->op == TokenType::LT || bin->op == TokenType::GT ||
                bin->op == TokenType::LEQ || bin->op == TokenType::GEQ) {
                if (!can_assign(left_type, right_type) && !can_assign(right_type, left_type)) {
                    add_error(expr->location.file + ":" + std::to_string(expr->location.line) +
                              ": cannot compare '" + left_type + "' and '" + right_type + "'");
                }
                expr->resolved_type = "bool";
                return "bool";
            }

            // Logical operators
            // Operadores logicos
            if (bin->op == TokenType::AND || bin->op == TokenType::OR) {
                if (!can_assign(left_type, "bool")) {
                    add_error(expr->location.file + ":" + std::to_string(expr->location.line) +
                              ": logical op requires bool, got '" + left_type + "'");
                }
                expr->resolved_type = "bool";
                return "bool";
            }

            expr->resolved_type = left_type;
            return left_type;
        }

        case ASTNodeType::UNARY_OP: {
            auto* un = static_cast<UnaryOp*>(expr);
            std::string op_type = check_expression(un->operand.get());
            if (un->op == TokenType::NOT) {
                if (!can_assign(op_type, "bool")) {
                    add_error(expr->location.file + ":" + std::to_string(expr->location.line) +
                              ": not requires bool, got '" + op_type + "'");
                }
                expr->resolved_type = "bool";
                return "bool";
            }
            expr->resolved_type = op_type;
            return op_type;
        }

        case ASTNodeType::CALL_EXPR: {
            auto* call = static_cast<CallExpr*>(expr);

            // Resolve arguments
            // Resolve argumentos
            for (const auto& arg : call->arguments) {
                check_expression(arg.get());
            }

            // If callee is an IdentExpr, look up the function
            // Se callee e um IdentExpr, busca a funcao
            if (call->callee->type == ASTNodeType::IDENT_EXPR) {
                auto* ident = static_cast<IdentExpr*>(call->callee.get());

                // Check for built-in print() function
                // Verifica funcao print() embutida
                if (ident->name == "print") {
                    if (!using_io) {
                        add_error(expr->location.file + ":" +
                                  std::to_string(expr->location.line) +
                                  ": use 'using IO;' before calling print()");
                    }
                    for (const auto& arg : call->arguments) {
                        std::string t = check_expression(arg.get());
                        std::string tn = normalize_type(t);
                        bool is_valid_print_type =
                            tn == "i8" || tn == "i16" || tn == "i32" || tn == "i64" ||
                            tn == "u8" || tn == "u16" || tn == "u32" || tn == "u64" ||
                            tn == "f32" || tn == "f64" ||
                            tn == "bool" ||
                            tn == "String" ||
                            tn == "isize" || tn == "usize";
                        if (!is_valid_print_type) {
                            add_error(expr->location.file + ":" +
                                      std::to_string(expr->location.line) +
                                      ": unsupported type '" + t + "' for print()");
                        }
                    }
                    expr->resolved_type = "void";
                    return "void";
                }

                // Check for constructor call: struct name matches
                // Verifica chamada de construtor: nome da struct corresponde
                if (is_struct_type(ident->name)) {
                    check_constructor_call(ident->name, call);
                    expr->resolved_type = ident->name;
                    return ident->name;
                }

                // Check for built-in error() function
                // Verifica funcao error() embutida
                if (ident->name == "error") {
                    expr->resolved_type = "void";
                    return "void";
                }

                // Regular function call
                // Chamada de funcao normal
                expr->resolved_type = "int"; // default fallback
                                             // fallback padrao
                return "int";
            }

            // Method call: obj.method()
            // Chamada de metodo: obj.method()
            if (call->callee->type == ASTNodeType::MEMBER_EXPR) {
                auto* mem = static_cast<MemberExpr*>(call->callee.get());
                std::string obj_type = check_expression(mem->object.get());

                // Handle block.reset() built-in
                // Trata block.reset() embutido
                if (obj_type == "block") {
                    if (mem->member == "reset") {
                        expr->resolved_type = "void";
                        return "void";
                    }
                    add_error(expr->location.file + ":" + std::to_string(expr->location.line) +
                              ": block has no method '" + mem->member + "'");
                    expr->resolved_type = "void";
                    return "void";
                }

                // Strip pointer
                // Remove ponteiro
                std::string base_type = obj_type;
                if (!base_type.empty() && base_type.back() == '*') {
                    base_type.pop_back();
                }

                if (is_struct_type(base_type)) {
                    // Check if method exists
                    // Verifica se metodo existe
                    auto* sd = struct_defs[base_type];
                    bool found = false;
                    for (const auto& method : sd->methods) {
                        if (method->type == ASTNodeType::FUNC_DECL) {
                            auto* fd = static_cast<FuncDecl*>(method.get());
                            if (fd->name == mem->member) {
                                expr->resolved_type = fd->return_type.empty() ? "void" : fd->return_type;
                                found = true;
                                break;
                            }
                        }
                    }
                    if (!found) {
                        add_error(expr->location.file + ":" + std::to_string(expr->location.line) +
                                  ": struct '" + base_type + "' has no method '" +
                                  mem->member + "'");
                    }
                } else {
                    add_error(expr->location.file + ":" + std::to_string(expr->location.line) +
                              ": cannot call method on non-struct type '" + obj_type + "'");
                }
                return expr->resolved_type;
            }

            expr->resolved_type = "int";
            return "int";
        }

        case ASTNodeType::MEMBER_EXPR: {
            auto* mem = static_cast<MemberExpr*>(expr);
            std::string obj_type = check_expression(mem->object.get());

            std::string base_type = obj_type;
            if (!base_type.empty() && base_type.back() == '*') {
                base_type.pop_back();
            }

            if (is_struct_type(base_type)) {
                auto* sd = struct_defs[base_type];
                for (const auto& field : sd->fields) {
                    if (field->type == ASTNodeType::FIELD_DECL) {
                        auto* fd = static_cast<FieldDecl*>(field.get());
                        if (fd->name == mem->member) {
                            expr->resolved_type = fd->type_name;
                            return fd->type_name;
                        }
                    }
                }

                // Check if it's a method reference
                // Verifica se e uma referencia de metodo
                for (const auto& method : sd->methods) {
                    if (method->type == ASTNodeType::FUNC_DECL) {
                        auto* func = static_cast<FuncDecl*>(method.get());
                        if (func->name == mem->member) {
                            expr->resolved_type = "fn";
                            return "fn";
                        }
                    }
                }

                add_error(expr->location.file + ":" + std::to_string(expr->location.line) +
                          ": struct '" + base_type + "' has no member '" +
                          mem->member + "'");
            } else {
                add_error(expr->location.file + ":" + std::to_string(expr->location.line) +
                          ": cannot access member of non-struct type '" + obj_type + "'");
            }

            expr->resolved_type = "unknown";
            return "unknown";
        }

        case ASTNodeType::INDEX_EXPR: {
            auto* ie = static_cast<IndexExpr*>(expr);
            std::string arr_type = check_expression(ie->array.get());
            std::string idx_type = check_expression(ie->index.get());
            if (!can_assign(idx_type, "int")) {
                add_error(expr->location.file + ":" + std::to_string(expr->location.line) +
                          ": array index must be int, got '" + idx_type + "'");
            }

            // Extract element type from array type "int[10]" -> "int"
            // Extrai tipo do elemento do tipo array "int[10]" -> "int"
            auto bracket = arr_type.find('[');
            if (bracket != std::string::npos) {
                std::string elem_type = arr_type.substr(0, bracket);
                expr->resolved_type = elem_type;
                return elem_type;
            }

            expr->resolved_type = arr_type;
            return arr_type;
        }

        case ASTNodeType::ASSIGNMENT: {
            auto* assign = static_cast<Assignment*>(expr);
            std::string target_type = check_expression(assign->target.get());
            std::string value_type = check_expression(assign->value.get());

            // Check if unsuffixed literal fits in target type
            // Verifica se literal sem sufixo cabe no tipo destino
            if (assign->value->type == ASTNodeType::INT_LITERAL) {
                auto* il = static_cast<IntLiteral*>(assign->value.get());
                if (il->literal_type.empty() && target_type != value_type) {
                    if (!int_fits_in_type(il->value, target_type)) {
                        add_error(expr->location.file + ":" +
                                  std::to_string(expr->location.line) +
                                  ": value " + std::to_string(il->value) +
                                  " does not fit in type '" + target_type + "'");
                    }
                    // Infer the literal's type from the target
                    // Infere o tipo do literal a partir do destino
                    assign->value->resolved_type = target_type;
                    value_type = target_type;
                }
            }

            if (!can_assign(value_type, target_type)) {
                add_error(expr->location.file + ":" + std::to_string(expr->location.line) +
                          ": cannot assign '" + value_type + "' to '" + target_type + "'");
            }
            expr->resolved_type = target_type;
            return target_type;
        }

        case ASTNodeType::ALLOC_INLINE: {
            auto* ai = static_cast<AllocInline*>(expr);
            std::string inner_type = check_expression(ai->expr.get());
            expr->resolved_type = inner_type;
            return inner_type;
        }

        case ASTNodeType::RESET_EXPR: {
            expr->resolved_type = "void";
            return "void";
        }

        default:
            expr->resolved_type = "unknown";
            return "unknown";
    }
}

void TypeChecker::check_constructor_call(const std::string& struct_name, CallExpr* call) {
    auto* sd = struct_defs[struct_name];
    // Find matching constructor
    // Encontra construtor correspondente
    bool found = false;
    for (const auto& method : sd->methods) {
        if (method->type == ASTNodeType::FUNC_DECL) {
            auto* fd = static_cast<FuncDecl*>(method.get());
            if (fd->is_constructor) {
                // Check param count
                // Verifica contagem de parametros
                if (fd->params.size() == call->arguments.size()) {
                    found = true;
                    for (size_t i = 0; i < fd->params.size(); i++) {
                        auto* pd = static_cast<ParamDecl*>(fd->params[i].get());
                        if (!can_assign(call->arguments[i]->resolved_type, pd->type_name)) {
                            add_error(call->location.file + ":" +
                                      std::to_string(call->location.line) +
                                      ": constructor argument " + std::to_string(i) +
                                      " type mismatch: expected '" + pd->type_name +
                                      "', got '" + call->arguments[i]->resolved_type + "'");
                        }
                    }
                }
            }
        }
    }
    if (!found && !sd->methods.empty()) {
        // If struct has no constructor, that's ok — can still be allocated
        // Se struct nao tem construtor, tudo bem — ainda pode ser alocada
    }
}

} // namespace brick
  // namespace brick
