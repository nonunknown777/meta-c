#include "macro_expander.h"
#include <sstream>
#include <set>
#include <algorithm>
#include <stack>

namespace brick {

// ─── Deep clone ───

static std::unique_ptr<ASTNode> clone_decl(const ASTNode* node);
static std::unique_ptr<ASTNode> clone_stmt(const ASTNode* node);
static std::unique_ptr<ASTNode> clone_expr(const ASTNode* node);

std::unique_ptr<ASTNode> clone_ast(const ASTNode* node) {
    if (!node) return nullptr;
    switch (node->type) {
        case ASTNodeType::PROGRAM: {
            auto* pn = static_cast<const ProgramNode*>(node);
            auto c = std::make_unique<ProgramNode>(pn->location);
            for (auto& d : pn->declarations)
                c->declarations.push_back(clone_decl(d.get()));
            return c;
        }
        case ASTNodeType::PACKAGE_DECL: {
            auto* pd = static_cast<const PackageDecl*>(node);
            return std::make_unique<PackageDecl>(pd->name_parts, pd->location);
        }
        case ASTNodeType::USING_DECL: {
            auto* ud = static_cast<const UsingDecl*>(node);
            return std::make_unique<UsingDecl>(ud->package_parts, ud->location);
        }
        case ASTNodeType::STRUCT_DECL: {
            auto* sd = static_cast<const StructDecl*>(node);
            auto c = std::make_unique<StructDecl>(sd->name, sd->location);
            c->extends = sd->extends;
            c->interfaces = sd->interfaces;
            c->is_private = sd->is_private;
            for (auto& f : sd->fields) c->fields.push_back(clone_decl(f.get()));
            for (auto& m : sd->methods) c->methods.push_back(clone_decl(m.get()));
            return c;
        }
        case ASTNodeType::INTERFACE_DECL: {
            auto* id = static_cast<const InterfaceDecl*>(node);
            auto c = std::make_unique<InterfaceDecl>(id->name, id->location);
            for (auto& m : id->methods) c->methods.push_back(clone_decl(m.get()));
            return c;
        }
        case ASTNodeType::FIELD_DECL: {
            auto* fd = static_cast<const FieldDecl*>(node);
            auto c = std::make_unique<FieldDecl>(fd->type_name, fd->name, fd->location);
            c->is_private = fd->is_private;
            return c;
        }
        case ASTNodeType::FUNC_DECL: {
            auto* fd = static_cast<const FuncDecl*>(node);
            auto c = std::make_unique<FuncDecl>(fd->name, fd->location);
            c->return_type = fd->return_type;
            c->is_private = fd->is_private;
            c->is_constructor = fd->is_constructor;
            c->is_extern = fd->is_extern;
            for (auto& p : fd->params) c->params.push_back(clone_decl(p.get()));
            c->body = clone_stmt(fd->body.get());
            return c;
        }
        case ASTNodeType::PARAM_DECL: {
            auto* pd = static_cast<const ParamDecl*>(node);
            return std::make_unique<ParamDecl>(pd->type_name, pd->name, pd->location);
        }
        case ASTNodeType::BLOCK_DECL: {
            auto* bd = static_cast<const BlockDecl*>(node);
            return std::make_unique<BlockDecl>(bd->name, bd->size, bd->unit, bd->location);
        }
        case ASTNodeType::BLOCK_SCOPE: {
            auto* bs = static_cast<const BlockScope*>(node);
            auto c = std::make_unique<BlockScope>(bs->block_name, bs->location);
            for (auto& s : bs->body) c->body.push_back(clone_stmt(s.get()));
            return c;
        }
        case ASTNodeType::ALLOC_INLINE: {
            auto* ai = static_cast<const AllocInline*>(node);
            return std::make_unique<AllocInline>(clone_expr(ai->expr.get()), ai->block_name, ai->location);
        }
        case ASTNodeType::RESET_EXPR: {
            auto* re = static_cast<const ResetExpr*>(node);
            return std::make_unique<ResetExpr>(re->block_name, re->location);
        }
        case ASTNodeType::INCLUDE_DECL: {
            auto* inc = static_cast<const IncludeDecl*>(node);
            auto c = std::make_unique<IncludeDecl>(inc->header, inc->location);
            c->link_lib = inc->link_lib;
            return c;
        }
        case ASTNodeType::LINK_DECL: {
            auto* ld = static_cast<const LinkDecl*>(node);
            return std::make_unique<LinkDecl>(ld->lib, ld->location);
        }
        case ASTNodeType::BLOCK_STMT: {
            auto* bs = static_cast<const BlockStmt*>(node);
            auto c = std::make_unique<BlockStmt>(bs->location);
            for (auto& s : bs->statements) c->statements.push_back(clone_stmt(s.get()));
            return c;
        }
        case ASTNodeType::IF_STMT: {
            auto* is = static_cast<const IfStmt*>(node);
            auto c = std::make_unique<IfStmt>(is->location);
            c->condition = clone_expr(is->condition.get());
            c->then_branch = clone_stmt(is->then_branch.get());
            c->else_branch = clone_stmt(is->else_branch.get());
            return c;
        }
        case ASTNodeType::WHILE_STMT: {
            auto* ws = static_cast<const WhileStmt*>(node);
            auto c = std::make_unique<WhileStmt>(ws->location);
            c->condition = clone_expr(ws->condition.get());
            c->body = clone_stmt(ws->body.get());
            return c;
        }
        case ASTNodeType::FOR_STMT: {
            auto* fs = static_cast<const ForStmt*>(node);
            auto c = std::make_unique<ForStmt>(fs->location);
            c->init = clone_stmt(fs->init.get());
            c->condition = clone_expr(fs->condition.get());
            c->increment = clone_expr(fs->increment.get());
            c->body = clone_stmt(fs->body.get());
            return c;
        }
        case ASTNodeType::RETURN_STMT: {
            auto* rs = static_cast<const ReturnStmt*>(node);
            auto c = std::make_unique<ReturnStmt>(rs->location);
            c->value = clone_expr(rs->value.get());
            return c;
        }
        case ASTNodeType::EXPR_STMT: {
            auto* es = static_cast<const ExprStmt*>(node);
            return std::make_unique<ExprStmt>(clone_expr(es->expr.get()), es->location);
        }
        case ASTNodeType::INT_LITERAL: {
            auto* il = static_cast<const IntLiteral*>(node);
            auto c = std::make_unique<IntLiteral>(il->value, il->location);
            c->literal_type = il->literal_type;
            return c;
        }
        case ASTNodeType::FLOAT_LITERAL: {
            auto* fl = static_cast<const FloatLiteral*>(node);
            auto c = std::make_unique<FloatLiteral>(fl->value, fl->location);
            c->literal_type = fl->literal_type;
            return c;
        }
        case ASTNodeType::STRING_LITERAL:
            return std::make_unique<StringLiteral>(
                static_cast<const StringLiteral*>(node)->value, node->location);
        case ASTNodeType::BOOL_LITERAL:
            return std::make_unique<BoolLiteral>(
                static_cast<const BoolLiteral*>(node)->value, node->location);
        case ASTNodeType::CHAR_LITERAL:
            return std::make_unique<CharLiteral>(
                static_cast<const CharLiteral*>(node)->value, node->location);
        case ASTNodeType::NULL_LITERAL:
            return std::make_unique<NullLiteral>(node->location);
        case ASTNodeType::IDENT_EXPR: {
            auto* ie = static_cast<const IdentExpr*>(node);
            auto c = std::make_unique<IdentExpr>(ie->name, ie->location);
            c->declared_type = ie->declared_type;
            return c;
        }
        case ASTNodeType::CALL_EXPR: {
            auto* ce = static_cast<const CallExpr*>(node);
            auto c = std::make_unique<CallExpr>(ce->location);
            c->callee = clone_expr(ce->callee.get());
            for (auto& a : ce->arguments) c->arguments.push_back(clone_expr(a.get()));
            return c;
        }
        case ASTNodeType::MEMBER_EXPR: {
            auto* me = static_cast<const MemberExpr*>(node);
            return std::make_unique<MemberExpr>(clone_expr(me->object.get()), me->member, me->location);
        }
        case ASTNodeType::INDEX_EXPR: {
            auto* ie = static_cast<const IndexExpr*>(node);
            auto c = std::make_unique<IndexExpr>(ie->location);
            c->array = clone_expr(ie->array.get());
            c->index = clone_expr(ie->index.get());
            return c;
        }
        case ASTNodeType::BINARY_OP: {
            auto* bo = static_cast<const BinaryOp*>(node);
            return std::make_unique<BinaryOp>(
                clone_expr(bo->left.get()), bo->op, clone_expr(bo->right.get()), bo->location);
        }
        case ASTNodeType::UNARY_OP: {
            auto* uo = static_cast<const UnaryOp*>(node);
            return std::make_unique<UnaryOp>(uo->op, clone_expr(uo->operand.get()), uo->location);
        }
        case ASTNodeType::ASSIGNMENT: {
            auto* a = static_cast<const Assignment*>(node);
            auto c = std::make_unique<Assignment>(a->op, a->location);
            c->target = clone_expr(a->target.get());
            c->value = clone_expr(a->value.get());
            return c;
        }
        case ASTNodeType::MACRO_DECL: {
            auto* md = static_cast<const MacroDecl*>(node);
            auto c = std::make_unique<MacroDecl>(md->name, md->location);
            c->params = md->params;
            c->has_varargs = md->has_varargs;
            for (auto& s : md->body) c->body.push_back(clone_stmt(s.get()));
            return c;
        }
        case ASTNodeType::MACRO_CALL: {
            auto* mc = static_cast<const MacroCall*>(node);
            auto c = std::make_unique<MacroCall>(mc->name, mc->location);
            for (auto& a : mc->args) c->args.push_back(clone_expr(a.get()));
            return c;
        }
        case ASTNodeType::BUILD_BLOCK: {
            auto* bb = static_cast<const BuildBlock*>(node);
            auto c = std::make_unique<BuildBlock>(bb->location);
            for (auto& s : bb->body) c->body.push_back(clone_stmt(s.get()));
            return c;
        }
        case ASTNodeType::EMIT_STMT: {
            auto* es = static_cast<const EmitStmt*>(node);
            return std::make_unique<EmitStmt>(clone_stmt(es->content.get()), es->location);
        }
        case ASTNodeType::INTERPOLATE: {
            auto* ip = static_cast<const Interpolate*>(node);
            if (!ip->name.empty())
                return std::make_unique<Interpolate>(ip->name, ip->location);
            return std::make_unique<Interpolate>(clone_expr(ip->expr.get()), ip->location);
        }
        case ASTNodeType::VALUE_PLACEHOLDER: {
            auto* vp = static_cast<const ValuePlaceholder*>(node);
            return std::make_unique<ValuePlaceholder>(vp->param_name, vp->location);
        }
        default:
            return nullptr;
    }
}

static std::unique_ptr<ASTNode> clone_decl(const ASTNode* node) { return clone_ast(node); }
static std::unique_ptr<ASTNode> clone_stmt(const ASTNode* node) { return clone_ast(node); }
static std::unique_ptr<ASTNode> clone_expr(const ASTNode* node) { return clone_ast(node); }

// ─── Gensym ───

static int gensym_counter = 0;

static std::string gensym(const std::string& base) {
    return base + "__" + std::to_string(++gensym_counter);
}

// ─── Substitution: replace Interpolate nodes with cloned args ───

using ArgMap = std::unordered_map<std::string, std::unique_ptr<ASTNode>>;

static std::unique_ptr<ASTNode> subst_expr(const ASTNode* node, const ArgMap& args) {
    if (!node) return nullptr;

    if (node->type == ASTNodeType::INTERPOLATE) {
        auto* ip = static_cast<const Interpolate*>(node);
        if (!ip->name.empty()) {
            auto it = args.find(ip->name);
            if (it != args.end()) {
                return clone_expr(it->second.get());
            }
        }
        if (ip->expr) {
            return std::make_unique<Interpolate>(subst_expr(ip->expr.get(), args), ip->location);
        }
        return clone_ast(node);
    }

    switch (node->type) {
        case ASTNodeType::BINARY_OP: {
            auto* bo = static_cast<const BinaryOp*>(node);
            return std::make_unique<BinaryOp>(
                subst_expr(bo->left.get(), args), bo->op,
                subst_expr(bo->right.get(), args), bo->location);
        }
        case ASTNodeType::UNARY_OP: {
            auto* uo = static_cast<const UnaryOp*>(node);
            return std::make_unique<UnaryOp>(uo->op, subst_expr(uo->operand.get(), args), uo->location);
        }
        case ASTNodeType::ASSIGNMENT: {
            auto* a = static_cast<const Assignment*>(node);
            auto c = std::make_unique<Assignment>(a->op, a->location);
            c->target = subst_expr(a->target.get(), args);
            c->value = subst_expr(a->value.get(), args);
            return c;
        }
        case ASTNodeType::CALL_EXPR: {
            auto* ce = static_cast<const CallExpr*>(node);
            auto c = std::make_unique<CallExpr>(ce->location);
            c->callee = subst_expr(ce->callee.get(), args);
            for (auto& a : ce->arguments)
                c->arguments.push_back(subst_expr(a.get(), args));
            return c;
        }
        case ASTNodeType::MEMBER_EXPR: {
            auto* me = static_cast<const MemberExpr*>(node);
            return std::make_unique<MemberExpr>(
                subst_expr(me->object.get(), args), me->member, me->location);
        }
        case ASTNodeType::INDEX_EXPR: {
            auto* ie = static_cast<const IndexExpr*>(node);
            auto c = std::make_unique<IndexExpr>(ie->location);
            c->array = subst_expr(ie->array.get(), args);
            c->index = subst_expr(ie->index.get(), args);
            return c;
        }
        case ASTNodeType::ALLOC_INLINE: {
            auto* ai = static_cast<const AllocInline*>(node);
            return std::make_unique<AllocInline>(
                subst_expr(ai->expr.get(), args), ai->block_name, ai->location);
        }
        default:
            return clone_ast(node);
    }
}

static std::unique_ptr<ASTNode> subst_stmt(const ASTNode* node, const ArgMap& args) {
    if (!node) return nullptr;

    switch (node->type) {
        case ASTNodeType::BLOCK_STMT: {
            auto* bs = static_cast<const BlockStmt*>(node);
            auto c = std::make_unique<BlockStmt>(bs->location);
            for (auto& s : bs->statements)
                c->statements.push_back(subst_stmt(s.get(), args));
            return c;
        }
        case ASTNodeType::IF_STMT: {
            auto* is = static_cast<const IfStmt*>(node);
            auto c = std::make_unique<IfStmt>(is->location);
            c->condition = subst_expr(is->condition.get(), args);
            c->then_branch = subst_stmt(is->then_branch.get(), args);
            c->else_branch = subst_stmt(is->else_branch.get(), args);
            return c;
        }
        case ASTNodeType::WHILE_STMT: {
            auto* ws = static_cast<const WhileStmt*>(node);
            auto c = std::make_unique<WhileStmt>(ws->location);
            c->condition = subst_expr(ws->condition.get(), args);
            c->body = subst_stmt(ws->body.get(), args);
            return c;
        }
        case ASTNodeType::FOR_STMT: {
            auto* fs = static_cast<const ForStmt*>(node);
            auto c = std::make_unique<ForStmt>(fs->location);
            c->init = subst_stmt(fs->init.get(), args);
            c->condition = subst_expr(fs->condition.get(), args);
            c->increment = subst_expr(fs->increment.get(), args);
            c->body = subst_stmt(fs->body.get(), args);
            return c;
        }
        case ASTNodeType::RETURN_STMT: {
            auto* rs = static_cast<const ReturnStmt*>(node);
            auto c = std::make_unique<ReturnStmt>(rs->location);
            c->value = subst_expr(rs->value.get(), args);
            return c;
        }
        case ASTNodeType::EXPR_STMT: {
            auto* es = static_cast<const ExprStmt*>(node);
            return std::make_unique<ExprStmt>(subst_expr(es->expr.get(), args), es->location);
        }
        case ASTNodeType::EMIT_STMT: {
            auto* es = static_cast<const EmitStmt*>(node);
            return std::make_unique<EmitStmt>(subst_stmt(es->content.get(), args), es->location);
        }
        case ASTNodeType::BLOCK_SCOPE: {
            auto* bs = static_cast<const BlockScope*>(node);
            auto c = std::make_unique<BlockScope>(bs->block_name, bs->location);
            for (auto& s : bs->body)
                c->body.push_back(subst_stmt(s.get(), args));
            return c;
        }
        default:
            return clone_ast(node);
    }
}

// ─── Apply gensym to __-prefixed identifiers ───

static void apply_gensym(ASTNode* node) {
    if (!node) return;

    switch (node->type) {
        case ASTNodeType::IDENT_EXPR: {
            auto* ie = static_cast<IdentExpr*>(node);
            if (ie->name.size() >= 2 && ie->name[0] == '_' && ie->name[1] == '_')
                ie->name = gensym(ie->name);
            return;
        }
        case ASTNodeType::INTERPOLATE: {
            auto* ip = static_cast<Interpolate*>(node);
            if (!ip->name.empty() && ip->name.size() >= 2 && ip->name[0] == '_' && ip->name[1] == '_')
                ip->name = gensym(ip->name);
            if (ip->expr) apply_gensym(ip->expr.get());
            return;
        }
        default: break;
    }

    if (auto* prog = dynamic_cast<ProgramNode*>(node)) {
        for (auto& d : prog->declarations) apply_gensym(d.get()); return;
    }
    if (auto* bs = dynamic_cast<BlockStmt*>(node)) {
        for (auto& s : bs->statements) apply_gensym(s.get()); return;
    }
    if (auto* ifs = dynamic_cast<IfStmt*>(node)) {
        apply_gensym(ifs->condition.get()); apply_gensym(ifs->then_branch.get()); apply_gensym(ifs->else_branch.get()); return;
    }
    if (auto* whs = dynamic_cast<WhileStmt*>(node)) {
        apply_gensym(whs->condition.get()); apply_gensym(whs->body.get()); return;
    }
    if (auto* fs = dynamic_cast<ForStmt*>(node)) {
        apply_gensym(fs->init.get()); apply_gensym(fs->condition.get()); apply_gensym(fs->increment.get()); apply_gensym(fs->body.get()); return;
    }
    if (auto* rs = dynamic_cast<ReturnStmt*>(node)) {
        apply_gensym(rs->value.get()); return;
    }
    if (auto* es = dynamic_cast<ExprStmt*>(node)) {
        apply_gensym(es->expr.get()); return;
    }
    if (auto* ass = dynamic_cast<Assignment*>(node)) {
        apply_gensym(ass->target.get()); apply_gensym(ass->value.get()); return;
    }
    if (auto* bin = dynamic_cast<BinaryOp*>(node)) {
        apply_gensym(bin->left.get()); apply_gensym(bin->right.get()); return;
    }
    if (auto* call = dynamic_cast<CallExpr*>(node)) {
        apply_gensym(call->callee.get());
        for (auto& a : call->arguments) apply_gensym(a.get()); return;
    }
    if (auto* sd = dynamic_cast<StructDecl*>(node)) {
        for (auto& f : sd->fields) apply_gensym(f.get());
        for (auto& m : sd->methods) apply_gensym(m.get()); return;
    }
    if (auto* fd = dynamic_cast<FuncDecl*>(node)) {
        apply_gensym(fd->body.get()); return;
    }
    if (auto* bs = dynamic_cast<BlockScope*>(node)) {
        for (auto& s : bs->body) apply_gensym(s.get()); return;
    }
}

// ─── Instantiate a macro ───

static std::unique_ptr<ASTNode> instantiate_macro(
    MacroDecl* macro,
    const std::vector<std::unique_ptr<ASTNode>>& call_args,
    std::vector<std::string>& errors,
    int depth = 0)
{
    if (depth >= 64) {
        errors.push_back("macro recursion too deep at '" + macro->name + "'");
        return nullptr;
    }

    size_t expected = macro->params.size();
    size_t got = call_args.size();
    if (!macro->has_varargs && got != expected) {
        errors.push_back("macro '" + macro->name + "' expects " +
            std::to_string(expected) + " args, got " + std::to_string(got));
        return nullptr;
    }
    if (macro->has_varargs && got < expected - 1) {
        errors.push_back("macro '" + macro->name + "' expects at least " +
            std::to_string(expected - 1) + " args, got " + std::to_string(got));
        return nullptr;
    }

    ArgMap args;
    for (size_t i = 0; i < macro->params.size() && i < call_args.size(); i++) {
        args[macro->params[i]] = clone_expr(call_args[i].get());
    }

    auto body_block = std::make_unique<BlockStmt>(macro->location);
    for (auto& s : macro->body) {
        auto substituted = subst_stmt(s.get(), args);
        apply_gensym(substituted.get());
        body_block->statements.push_back(std::move(substituted));
    }

    return body_block;
}

// ─── Collect macros ───

void collect_macros(
    std::vector<std::unique_ptr<ProgramNode>>& asts,
    MacroTable& macro_table)
{
    for (auto& ast : asts) {
        for (auto& decl : ast->declarations) {
            if (decl->type == ASTNodeType::MACRO_DECL) {
                auto* md = static_cast<MacroDecl*>(decl.get());
                macro_table[md->name] = md;
            }
        }
    }
}

// ─── Walk AST and expand all macro calls ───

static bool is_macro_callee(const ASTNode* node, const MacroTable& table) {
    // Support direct CallExpr or ExprStmt wrapping a call
    if (node->type == ASTNodeType::EXPR_STMT) {
        auto* es = static_cast<const ExprStmt*>(node);
        return is_macro_callee(es->expr.get(), table);
    }
    if (node->type != ASTNodeType::CALL_EXPR) return false;
    auto* ce = static_cast<const CallExpr*>(node);
    if (ce->callee->type != ASTNodeType::IDENT_EXPR) return false;
    auto* ident = static_cast<const IdentExpr*>(ce->callee.get());
    return table.count(ident->name) > 0;
}

static std::vector<std::unique_ptr<ASTNode>> expand_call(
    const ASTNode* node,
    MacroTable& macro_table,
    std::vector<std::string>& errors,
    int depth)
{
    std::vector<std::unique_ptr<ASTNode>> result;
    if (node->type != ASTNodeType::CALL_EXPR) return result;
    auto* ce = static_cast<const CallExpr*>(node);
    if (ce->callee->type != ASTNodeType::IDENT_EXPR) return result;
    auto* ident = static_cast<const IdentExpr*>(ce->callee.get());
    auto it = macro_table.find(ident->name);
    if (it == macro_table.end()) return result;

    auto expanded = instantiate_macro(it->second, ce->arguments, errors, depth + 1);
    if (expanded) {
        if (expanded->type == ASTNodeType::BLOCK_STMT) {
            auto* bs = static_cast<BlockStmt*>(expanded.get());
            for (auto& s : bs->statements)
                result.push_back(std::move(s));
        } else {
            result.push_back(std::move(expanded));
        }
    }
    return result;
}

static void expand_in_decl(ASTNode* decl, MacroTable& macro_table,
                           std::vector<std::string>& errors, int depth);
static void expand_in_block(BlockStmt* block, MacroTable& macro_table,
                            std::vector<std::string>& errors, int depth);

// Flatten emit stmt: extract its inner block statements into the target vector
static void flatten_emit(std::unique_ptr<ASTNode>& node,
                         std::vector<std::unique_ptr<ASTNode>>& target) {
    if (node->type == ASTNodeType::EMIT_STMT) {
        auto* es = static_cast<EmitStmt*>(node.get());
        if (es->content && es->content->type == ASTNodeType::BLOCK_STMT) {
            auto* bs = static_cast<BlockStmt*>(es->content.get());
            for (auto& inner : bs->statements)
                target.push_back(std::move(inner));
            return;
        }
    }
    target.push_back(std::move(node));
}

static void expand_in_prog(
    ProgramNode* prog,
    MacroTable& macro_table,
    std::vector<std::string>& errors,
    int depth)
{
    if (!prog || depth > 100) return;
    std::vector<std::unique_ptr<ASTNode>> new_decls;
    for (auto& d : prog->declarations) {
        // Unwrap ExprStmt for top-level macro calls
        ASTNode* callee_node = d.get();
        bool was_expr_stmt = false;
        if (callee_node->type == ASTNodeType::EXPR_STMT) {
            auto* es = static_cast<ExprStmt*>(callee_node);
            callee_node = es->expr.get();
            was_expr_stmt = true;
        }
        if (is_macro_callee(callee_node, macro_table)) {
            auto expanded = expand_call(callee_node, macro_table, errors, depth + 1);
            if (expanded.empty()) {
                new_decls.push_back(std::move(d));
            } else {
                for (auto& s : expanded) {
                    // Flatten emit stmt content into top-level decls
                    if (s->type == ASTNodeType::EMIT_STMT) {
                        auto* es = static_cast<EmitStmt*>(s.get());
                        if (es->content && es->content->type == ASTNodeType::BLOCK_STMT) {
                            auto* bs = static_cast<BlockStmt*>(es->content.get());
                            for (auto& st : bs->statements)
                                new_decls.push_back(std::move(st));
                        }
                    } else {
                        new_decls.push_back(std::move(s));
                    }
                }
            }
        } else {
            new_decls.push_back(std::move(d));
        }
    }
    prog->declarations = std::move(new_decls);
    for (auto& d : prog->declarations)
        expand_in_decl(d.get(), macro_table, errors, depth + 1);
}

static void expand_in_block(
    BlockStmt* block,
    MacroTable& macro_table,
    std::vector<std::string>& errors,
    int depth)
{
    if (!block || depth > 100) return;

    // Phase 1: repeatedly expand macro calls at this level (supports nesting)
    int macro_recursion = 0;
    for (;;) {
        bool expanded_this_pass = false;
        std::vector<std::unique_ptr<ASTNode>> new_stmts;
        for (auto& s : block->statements) {
            if (s->type == ASTNodeType::EXPR_STMT) {
                auto* es = static_cast<ExprStmt*>(s.get());
                if (is_macro_callee(es->expr.get(), macro_table)) {
                    auto result = expand_call(es->expr.get(), macro_table, errors, macro_recursion);
                    if (!result.empty()) {
                        expanded_this_pass = true;
                        macro_recursion++;
                        if (macro_recursion >= 64) {
                            errors.push_back("macro recursion too deep");
                            // push unexpanded to avoid infinite loop
                            for (auto& st : result) { flatten_emit(st, new_stmts); }
                            continue;
                        }
                        for (auto& st : result) { flatten_emit(st, new_stmts); }
                        continue;
                    }
                }
            }
            new_stmts.push_back(std::move(s));
        }
        if (!expanded_this_pass) {
            block->statements = std::move(new_stmts);
            break;
        }
        block->statements = std::move(new_stmts);
    }

    // Phase 2: traverse nested blocks (if/while/for/etc)
    for (auto& s : block->statements) {
        if (s->type == ASTNodeType::IF_STMT) {
            auto* is = static_cast<IfStmt*>(s.get());
            if (is->then_branch && is->then_branch->type == ASTNodeType::BLOCK_STMT)
                expand_in_block(static_cast<BlockStmt*>(is->then_branch.get()), macro_table, errors, depth + 1);
            if (is->else_branch && is->else_branch->type == ASTNodeType::BLOCK_STMT)
                expand_in_block(static_cast<BlockStmt*>(is->else_branch.get()), macro_table, errors, depth + 1);
        } else if (s->type == ASTNodeType::WHILE_STMT) {
            auto* ws = static_cast<WhileStmt*>(s.get());
            if (ws->body && ws->body->type == ASTNodeType::BLOCK_STMT)
                expand_in_block(static_cast<BlockStmt*>(ws->body.get()), macro_table, errors, depth + 1);
        } else if (s->type == ASTNodeType::FOR_STMT) {
            auto* fs = static_cast<ForStmt*>(s.get());
            if (fs->body && fs->body->type == ASTNodeType::BLOCK_STMT)
                expand_in_block(static_cast<BlockStmt*>(fs->body.get()), macro_table, errors, depth + 1);
        } else if (s->type == ASTNodeType::BLOCK_STMT) {
            expand_in_block(static_cast<BlockStmt*>(s.get()), macro_table, errors, depth + 1);
        } else if (s->type == ASTNodeType::BLOCK_SCOPE) {
            auto* bs = static_cast<BlockScope*>(s.get());
            for (auto& st : bs->body)
                if (st->type == ASTNodeType::BLOCK_STMT)
                    expand_in_block(static_cast<BlockStmt*>(st.get()), macro_table, errors, depth + 1);
        }
    }
}

static void expand_in_decl(ASTNode* decl, MacroTable& macro_table,
                           std::vector<std::string>& errors, int depth) {
    if (!decl) return;
    if (decl->type == ASTNodeType::STRUCT_DECL) {
        auto* sd = static_cast<StructDecl*>(decl);
        for (auto& f : sd->fields) expand_in_decl(f.get(), macro_table, errors, depth);
        for (auto& m : sd->methods) expand_in_decl(m.get(), macro_table, errors, depth);
    } else if (decl->type == ASTNodeType::FUNC_DECL) {
        auto* fd = static_cast<FuncDecl*>(decl);
        if (fd->body && fd->body->type == ASTNodeType::BLOCK_STMT)
            expand_in_block(static_cast<BlockStmt*>(fd->body.get()), macro_table, errors, depth);
    } else if (decl->type == ASTNodeType::BLOCK_SCOPE) {
        auto* bs = static_cast<BlockScope*>(decl);
        for (auto& s : bs->body)
            if (s->type == ASTNodeType::BLOCK_STMT)
                expand_in_block(static_cast<BlockStmt*>(s.get()), macro_table, errors, depth);
    }
}

ExpandResult expand_macros(
    std::vector<std::unique_ptr<ProgramNode>>& asts,
    MacroTable& macro_table)
{
    ExpandResult result;
    gensym_counter = 0;

    for (auto& ast : asts) {
        expand_in_prog(ast.get(), macro_table, result.errors, 0);
    }

    if (!result.errors.empty()) result.success = false;

    for (auto& ast : asts) {
        std::vector<std::unique_ptr<ASTNode>> cleaned;
        for (auto& d : ast->declarations) {
            if (d->type != ASTNodeType::MACRO_DECL &&
                d->type != ASTNodeType::BUILD_BLOCK &&
                d->type != ASTNodeType::EMIT_STMT &&
                d->type != ASTNodeType::INTERPOLATE &&
                d->type != ASTNodeType::VALUE_PLACEHOLDER) {
                cleaned.push_back(std::move(d));
            }
        }
        ast->declarations = std::move(cleaned);
    }

    return result;
}

} // namespace brick
