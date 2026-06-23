#include "build_eval.h"
#include <sstream>
#include <algorithm>
#include <cstdlib>
#include <set>

namespace brick {

// ─── Comptime Value ───

enum class CValType { INT, FLOAT, STRING, BOOL, VOID, EMITTED };

struct CValue {
    CValType type = CValType::VOID;
    int64_t int_val = 0;
    double float_val = 0.0;
    std::string str_val;

    // Emitted code — stored as raw ptrs, freed by the AST that adopts them
    std::vector<CValue> emitted_values;
    std::vector<std::unique_ptr<ASTNode>> emitted_stmts;

    CValue() = default;
    CValue(const CValue&) = delete;
    CValue& operator=(const CValue&) = delete;
    CValue(CValue&&) = default;
    CValue& operator=(CValue&&) = default;

    CValue(int v) : type(CValType::INT), int_val(v) {}
    CValue(int64_t v) : type(CValType::INT), int_val(v) {}
    CValue(double v) : type(CValType::FLOAT), float_val(v) {}
    CValue(std::string v) : type(CValType::STRING), str_val(std::move(v)) {}
    CValue(bool v) : type(CValType::BOOL), int_val(v ? 1 : 0) {}

    CValue clone() const {
        CValue c;
        c.type = type;
        c.int_val = int_val;
        c.float_val = float_val;
        c.str_val = str_val;
        return c;
    }
};

// ─── Symbol table for build blocks ───

struct BuildScope {
    std::unordered_map<std::string, CValue> vars;
    BuildScope* parent = nullptr;
};

static CValue* build_lookup(BuildScope* scope, const std::string& name) {
    while (scope) {
        auto it = scope->vars.find(name);
        if (it != scope->vars.end()) return &it->second;
        scope = scope->parent;
    }
    return nullptr;
}

// ─── Build Eval Engine ───

class BuildEval {
public:
    BuildEval(std::vector<std::unique_ptr<ProgramNode>>& asts,
              MacroTable& macro_table,
              std::vector<std::string>& errors)
        : asts(asts), macro_table(macro_table), errors(errors), scope(nullptr) {}

    bool eval_all() {
        collect_struct_defs();
        for (auto& ast : asts) {
            replace_build_blocks(ast.get());
        }
        return errors.empty();
    }

private:
    std::vector<std::unique_ptr<ProgramNode>>& asts;
    MacroTable& macro_table;
    std::vector<std::string>& errors;

    std::unordered_map<std::string, StructDecl*> struct_defs;
    BuildScope* scope;

    void collect_struct_defs() {
        for (auto& ast : asts) {
            for (auto& d : ast->declarations) {
                if (d->type == ASTNodeType::STRUCT_DECL) {
                    auto* sd = static_cast<StructDecl*>(d.get());
                    struct_defs[sd->name] = sd;
                }
            }
        }
    }

    void replace_build_blocks(ProgramNode* prog) {
        std::vector<std::unique_ptr<ASTNode>> new_decls;
        for (auto& d : prog->declarations) {
            if (d->type == ASTNodeType::BUILD_BLOCK) {
                auto* bb = static_cast<BuildBlock*>(d.get());
                // Push scope
                BuildScope bs;
                bs.parent = scope;
                scope = &bs;
                // Execute each statement
                CValue result;
                for (auto& s : bb->body) {
                    result = exec_stmt(s.get());
                }
                scope = bs.parent;
                // If emitted stmts, add them
                if (result.type == CValType::EMITTED) {
                    for (auto& stmt : result.emitted_stmts) {
                        new_decls.push_back(std::move(stmt));
                    }
                }
            } else {
                new_decls.push_back(std::move(d));
            }
        }
        prog->declarations = std::move(new_decls);
    }

    CValue exec_stmt(ASTNode* stmt) {
        switch (stmt->type) {
            case ASTNodeType::EXPR_STMT: {
                auto* es = static_cast<ExprStmt*>(stmt);
                return exec_expr(es->expr.get());
            }
            case ASTNodeType::IF_STMT: {
                auto* is = static_cast<IfStmt*>(stmt);
                CValue cond = exec_expr(is->condition.get());
                bool b = to_bool(cond);
                if (b) {
                    return exec_block(is->then_branch.get());
                } else if (is->else_branch) {
                    return exec_stmt(is->else_branch.get());
                }
                return CValue();
            }
            case ASTNodeType::WHILE_STMT: {
                auto* ws = static_cast<WhileStmt*>(stmt);
                CValue result;
                while (true) {
                    CValue cond = exec_expr(ws->condition.get());
                    if (!to_bool(cond)) break;
                    result = exec_block(ws->body.get());
                }
                return result;
            }
            case ASTNodeType::FOR_STMT: {
                auto* fs = static_cast<ForStmt*>(stmt);
                if (fs->init) exec_stmt(fs->init.get());
                CValue result;
                while (true) {
                    if (fs->condition) {
                        CValue cond = exec_expr(fs->condition.get());
                        if (!to_bool(cond)) break;
                    }
                    result = exec_block(fs->body.get());
                    if (fs->increment) exec_expr(fs->increment.get());
                }
                return result;
            }
            case ASTNodeType::BLOCK_STMT: {
                return exec_block(stmt);
            }
            case ASTNodeType::EMIT_STMT: {
                auto* es = static_cast<EmitStmt*>(stmt);
                // Emit clones the content AST and marks it as emitted
                CValue result;
                result.type = CValType::EMITTED;
                // Clone the content
                auto cloned = clone_ast(es->content.get());
                if (cloned) {
                    if (cloned->type == ASTNodeType::BLOCK_STMT) {
                        auto* bs = static_cast<BlockStmt*>(cloned.get());
                        for (auto& s : bs->statements) {
                            result.emitted_stmts.push_back(std::move(s));
                        }
                    } else {
                        result.emitted_stmts.push_back(std::move(cloned));
                    }
                }
                return result;
            }
            default:
                return CValue();
        }
    }

    CValue exec_block(ASTNode* node) {
        if (node->type == ASTNodeType::BLOCK_STMT) {
            auto* bs = static_cast<BlockStmt*>(node);
            BuildScope local_scope;
            local_scope.parent = scope;
            scope = &local_scope;
            CValue result;
            for (auto& s : bs->statements) {
                result = exec_stmt(s.get());
            }
            scope = local_scope.parent;
            return result;
        }
        return exec_stmt(node);
    }

    CValue exec_expr(ASTNode* expr) {
        if (!expr) return CValue();

        switch (expr->type) {
            case ASTNodeType::INT_LITERAL: {
                auto* il = static_cast<IntLiteral*>(expr);
                return CValue(il->value);
            }
            case ASTNodeType::FLOAT_LITERAL: {
                auto* fl = static_cast<FloatLiteral*>(expr);
                return CValue(fl->value);
            }
            case ASTNodeType::STRING_LITERAL: {
                auto* sl = static_cast<StringLiteral*>(expr);
                return CValue(sl->value);
            }
            case ASTNodeType::BOOL_LITERAL: {
                auto* bl = static_cast<BoolLiteral*>(expr);
                return CValue(bl->value);
            }
            case ASTNodeType::IDENT_EXPR: {
                auto* ie = static_cast<IdentExpr*>(expr);
                auto* val = build_lookup(scope, ie->name);
                if (!val) {
                    errors.push_back("build: undefined variable '" + ie->name + "'");
                    return CValue();
                }
                return val->clone();
            }
            case ASTNodeType::BINARY_OP: {
                auto* bo = static_cast<BinaryOp*>(expr);
                CValue left = exec_expr(bo->left.get());
                CValue right = exec_expr(bo->right.get());

                if (left.type == CValType::INT && right.type == CValType::INT) {
                    switch (bo->op) {
                        case TokenType::PLUS:  return CValue(left.int_val + right.int_val);
                        case TokenType::MINUS: return CValue(left.int_val - right.int_val);
                        case TokenType::STAR:  return CValue(left.int_val * right.int_val);
                        case TokenType::SLASH:
                            if (right.int_val == 0) {
                                errors.push_back("build: division by zero");
                                return CValue();
                            }
                            return CValue(left.int_val / right.int_val);
                        case TokenType::EQ:  return CValue(left.int_val == right.int_val);
                        case TokenType::NEQ: return CValue(left.int_val != right.int_val);
                        case TokenType::LT:  return CValue(left.int_val < right.int_val);
                        case TokenType::GT:  return CValue(left.int_val > right.int_val);
                        case TokenType::LEQ: return CValue(left.int_val <= right.int_val);
                        case TokenType::GEQ: return CValue(left.int_val >= right.int_val);
                        default: return CValue();
                    }
                }
                if (left.type == CValType::STRING && right.type == CValType::STRING) {
                    if (bo->op == TokenType::PLUS) {
                        return CValue(left.str_val + right.str_val);
                    }
                    if (bo->op == TokenType::EQ) return CValue(left.str_val == right.str_val);
                    if (bo->op == TokenType::NEQ) return CValue(left.str_val != right.str_val);
                    return CValue();
                }
                if (left.type == CValType::BOOL && right.type == CValType::BOOL) {
                    if (bo->op == TokenType::AND) return CValue(left.int_val && right.int_val);
                    if (bo->op == TokenType::OR)  return CValue(left.int_val || right.int_val);
                    return CValue();
                }
                // Mixed int/float
                if ((left.type == CValType::INT || left.type == CValType::FLOAT) &&
                    (right.type == CValType::INT || right.type == CValType::FLOAT)) {
                    double lv = (left.type == CValType::INT) ? (double)left.int_val : left.float_val;
                    double rv = (right.type == CValType::INT) ? (double)right.int_val : right.float_val;
                    switch (bo->op) {
                        case TokenType::PLUS:  return CValue(lv + rv);
                        case TokenType::MINUS: return CValue(lv - rv);
                        case TokenType::STAR:  return CValue(lv * rv);
                        case TokenType::SLASH: return CValue(lv / rv);
                        case TokenType::EQ:  return CValue(lv == rv);
                        case TokenType::NEQ: return CValue(lv != rv);
                        case TokenType::LT:  return CValue(lv < rv);
                        case TokenType::GT:  return CValue(lv > rv);
                        case TokenType::LEQ: return CValue(lv <= rv);
                        case TokenType::GEQ: return CValue(lv >= rv);
                        default: return CValue();
                    }
                }
                return CValue();
            }
            case ASTNodeType::CALL_EXPR: {
                auto* ce = static_cast<CallExpr*>(expr);
                if (ce->callee->type == ASTNodeType::MEMBER_EXPR) {
                    // T.fields, T.name, T.size reflection
                    auto* me = static_cast<MemberExpr*>(ce->callee.get());
                    if (me->object->type == ASTNodeType::IDENT_EXPR) {
                        auto* obj_ident = static_cast<IdentExpr*>(me->object.get());
                        auto it = struct_defs.find(obj_ident->name);
                        if (it != struct_defs.end()) {
                            if (me->member == "fields" || me->member == "name" || me->member == "size") {
                                return handle_reflection(obj_ident->name, me->member);
                            }
                        }
                    }
                }
                if (ce->callee->type == ASTNodeType::IDENT_EXPR) {
                    auto* ident = static_cast<IdentExpr*>(ce->callee.get());
                    // Build-time pure functions
                    if (ident->name == "to_string") {
                        if (ce->arguments.size() >= 1) {
                            CValue arg = exec_expr(ce->arguments[0].get());
                            if (arg.type == CValType::INT) return CValue(std::to_string(arg.int_val));
                            if (arg.type == CValType::FLOAT) return CValue(std::to_string(arg.float_val));
                            if (arg.type == CValType::BOOL) return CValue(arg.int_val ? "true" : "false");
                            return arg;
                        }
                    }
                    if (ident->name == "len") {
                        if (ce->arguments.size() >= 1) {
                            CValue arg = exec_expr(ce->arguments[0].get());
                            if (arg.type == CValType::STRING) {
                                return CValue((int64_t)arg.str_val.size());
                            }
                        }
                    }
                    if (ident->name == "int_of") {
                        if (ce->arguments.size() >= 1) {
                            CValue arg = exec_expr(ce->arguments[0].get());
                            if (arg.type == CValType::STRING) return CValue((int64_t)std::atoll(arg.str_val.c_str()));
                            if (arg.type == CValType::FLOAT) return CValue((int64_t)arg.float_val);
                            if (arg.type == CValType::BOOL) return CValue(arg.int_val ? 1 : 0);
                            return arg;
                        }
                    }
                    // User function call inside build — not supported for now
                    errors.push_back("build: function calls inside build not yet supported (called '" + ident->name + "')");
                    return CValue();
                }
                return CValue();
            }
            case ASTNodeType::ASSIGNMENT: {
                auto* as = static_cast<Assignment*>(expr);
                CValue val = exec_expr(as->value.get());
                if (as->target->type == ASTNodeType::IDENT_EXPR && scope) {
                    auto* ident = static_cast<IdentExpr*>(as->target.get());
                    // Use emplace to avoid copy
                    scope->vars.emplace(ident->name, std::move(val));
                }
                return CValue();
            }
            case ASTNodeType::EMIT_STMT: {
                // Emit as expression returns the emitted block as a CValue
                auto* es = static_cast<EmitStmt*>(expr);
                CValue result;
                result.type = CValType::EMITTED;
                auto cloned = clone_ast(es->content.get());
                if (cloned) {
                    if (cloned->type == ASTNodeType::BLOCK_STMT) {
                        auto* bs = static_cast<BlockStmt*>(cloned.get());
                        for (auto& s : bs->statements)
                            result.emitted_stmts.push_back(std::move(s));
                    } else {
                        result.emitted_stmts.push_back(std::move(cloned));
                    }
                }
                return result;
            }
            default: {
                // Handle MEMBER_EXPR standalone (not in call)
                if (expr->type == ASTNodeType::MEMBER_EXPR) {
                    auto* me = static_cast<MemberExpr*>(expr);
                    if (me->object->type == ASTNodeType::IDENT_EXPR) {
                        auto* obj_ident = static_cast<IdentExpr*>(me->object.get());
                        auto it = struct_defs.find(obj_ident->name);
                        if (it != struct_defs.end()) {
                            return handle_reflection(obj_ident->name, me->member);
                        }
                    }
                }
                return CValue();
            }
        }
    }

    CValue handle_reflection(const std::string& type_name, const std::string& member) {
        if (member == "name") {
            return CValue(type_name);
        }
        auto it = struct_defs.find(type_name);
        if (it == struct_defs.end()) {
            errors.push_back("build: unknown type '" + type_name + "' for reflection");
            return CValue();
        }
        auto* sd = it->second;
        if (member == "fields") {
            // Return a string containing field names joined by ", "
            std::string fields_str;
            for (size_t i = 0; i < sd->fields.size(); i++) {
                auto* fd = static_cast<FieldDecl*>(sd->fields[i].get());
                if (i > 0) fields_str += ", ";
                fields_str += fd->name;
            }
            return CValue(fields_str);
        }
        if (member == "size") {
            // Return a rough byte size (same logic as codegen)
            return CValue((int64_t)estimate_type_size(type_name));
        }
        errors.push_back("build: unknown reflection member '" + member + "' on type '" + type_name + "'");
        return CValue();
    }

    int estimate_type_size(const std::string& type_name) {
        auto bracket = type_name.find('[');
        if (bracket != std::string::npos) {
            std::string base = type_name.substr(0, bracket);
            std::string count_str = type_name.substr(bracket + 1, type_name.find(']') - bracket - 1);
            int count = std::stoi(count_str);
            return estimate_type_size(base) * count;
        }
        std::string n = normalize(type_name);
        if (n == "u8" || n == "i8" || n == "bool" || n == "char") return 1;
        if (n == "u16" || n == "i16") return 2;
        if (n == "u32" || n == "i32" || n == "f32") return 4;
        if (n == "u64" || n == "i64" || n == "f64" || n == "usize" || n == "isize") return 8;
        if (n == "String") return 16;
        if (struct_defs.count(type_name)) {
            auto* sd = struct_defs[type_name];
            int total = 0;
            for (auto& f : sd->fields) {
                if (f->type == ASTNodeType::FIELD_DECL) {
                    total += estimate_type_size(static_cast<FieldDecl*>(f.get())->type_name);
                }
            }
            return total;
        }
        return 8;
    }

    static std::string normalize(const std::string& t) {
        if (t == "byte" || t == "char") return "u8";
        if (t == "short") return "i16";
        if (t == "int") return "i32";
        if (t == "long") return "i64";
        if (t == "float") return "f32";
        if (t == "double") return "f64";
        return t;
    }

    static bool to_bool(const CValue& v) {
        if (v.type == CValType::INT) return v.int_val != 0;
        if (v.type == CValType::FLOAT) return v.float_val != 0.0;
        if (v.type == CValType::BOOL) return v.int_val != 0;
        if (v.type == CValType::STRING) return !v.str_val.empty();
        return false;
    }
};

BuildEvalResult eval_build_blocks(
    std::vector<std::unique_ptr<ProgramNode>>& asts,
    MacroTable& macro_table)
{
    BuildEvalResult result;
    BuildEval eval(asts, macro_table, result.errors);
    result.success = eval.eval_all();
    return result;
}

} // namespace brick
