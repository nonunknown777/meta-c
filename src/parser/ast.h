#ifndef BRICK_AST_H
#define BRICK_AST_H

#include <string>
#include <vector>
#include <memory>
#include "../shared/types.h"

namespace brick {

enum class ASTNodeType {
    PROGRAM,
    PACKAGE_DECL, USING_DECL,
    STRUCT_DECL, INTERFACE_DECL,
    FIELD_DECL, FUNC_DECL, PARAM_DECL,
    BLOCK_DECL, BLOCK_SCOPE,
    ALLOC_INLINE, RESET_EXPR,
    INCLUDE_DECL, LINK_DECL,

    // Statements
    BLOCK_STMT, IF_STMT, WHILE_STMT, FOR_STMT, RETURN_STMT, EXPR_STMT,

    // Expressions
    INT_LITERAL, FLOAT_LITERAL, STRING_LITERAL, BOOL_LITERAL, CHAR_LITERAL, NULL_LITERAL,
    IDENT_EXPR, CALL_EXPR, MEMBER_EXPR, INDEX_EXPR,
    BINARY_OP, UNARY_OP, ASSIGNMENT,

    // Macro nodes
    MACRO_DECL, MACRO_CALL, BUILD_BLOCK, EMIT_STMT,
    INTERPOLATE, VALUE_PLACEHOLDER,
};

struct ASTNode {
    ASTNodeType type;
    SourceLocation location;
    std::string resolved_type;
    virtual ~ASTNode() = default;
    ASTNode(ASTNodeType t, SourceLocation loc) : type(t), location(loc) {}
};

// ─── Declarations ───

struct ProgramNode : ASTNode {
    std::vector<std::unique_ptr<ASTNode>> declarations;
    ProgramNode(SourceLocation loc) : ASTNode(ASTNodeType::PROGRAM, loc) {}
};

struct PackageDecl : ASTNode {
    std::vector<std::string> name_parts;
    PackageDecl(std::vector<std::string> parts, SourceLocation loc)
        : ASTNode(ASTNodeType::PACKAGE_DECL, loc), name_parts(std::move(parts)) {}
};

struct UsingDecl : ASTNode {
    std::vector<std::string> package_parts;
    UsingDecl(std::vector<std::string> parts, SourceLocation loc)
        : ASTNode(ASTNodeType::USING_DECL, loc), package_parts(std::move(parts)) {}
};

struct IncludeDecl : ASTNode {
    std::string header;
    std::string link_lib;

    IncludeDecl(std::string h, SourceLocation loc)
        : ASTNode(ASTNodeType::INCLUDE_DECL, loc), header(std::move(h)) {}
};

struct LinkDecl : ASTNode {
    std::string lib;

    LinkDecl(std::string l, SourceLocation loc)
        : ASTNode(ASTNodeType::LINK_DECL, loc), lib(std::move(l)) {}
};

struct StructDecl : ASTNode {
    std::string name;
    std::string extends;
    std::vector<std::string> interfaces;
    std::vector<std::unique_ptr<ASTNode>> fields;
    std::vector<std::unique_ptr<ASTNode>> methods;
    bool is_private = false;

    StructDecl(std::string n, SourceLocation loc)
        : ASTNode(ASTNodeType::STRUCT_DECL, loc), name(std::move(n)) {}
};

struct InterfaceDecl : ASTNode {
    std::string name;
    std::vector<std::unique_ptr<ASTNode>> methods;

    InterfaceDecl(std::string n, SourceLocation loc)
        : ASTNode(ASTNodeType::INTERFACE_DECL, loc), name(std::move(n)) {}
};

struct FieldDecl : ASTNode {
    std::string type_name;
    std::string name;
    bool is_private = false;

    FieldDecl(std::string t, std::string n, SourceLocation loc)
        : ASTNode(ASTNodeType::FIELD_DECL, loc), type_name(std::move(t)), name(std::move(n)) {}
};

struct ParamDecl : ASTNode {
    std::string type_name;
    std::string name;

    ParamDecl(std::string t, std::string n, SourceLocation loc)
        : ASTNode(ASTNodeType::PARAM_DECL, loc), type_name(std::move(t)), name(std::move(n)) {}
};

struct FuncDecl : ASTNode {
    std::string return_type;
    std::string name;
    std::vector<std::unique_ptr<ASTNode>> params;
    std::unique_ptr<ASTNode> body;
    bool is_private = false;
    bool is_constructor = false;
    bool is_extern = false;

    FuncDecl(std::string n, SourceLocation loc)
        : ASTNode(ASTNodeType::FUNC_DECL, loc), name(std::move(n)) {}
};

// ─── Block Memory ───

struct BlockDecl : ASTNode {
    std::string name;
    int64_t size;
    std::string unit;

    BlockDecl(std::string n, int64_t s, std::string u, SourceLocation loc)
        : ASTNode(ASTNodeType::BLOCK_DECL, loc), name(std::move(n)), size(s), unit(std::move(u)) {}
};

struct BlockScope : ASTNode {
    std::string block_name;
    std::vector<std::unique_ptr<ASTNode>> body;

    BlockScope(std::string bn, SourceLocation loc)
        : ASTNode(ASTNodeType::BLOCK_SCOPE, loc), block_name(std::move(bn)) {}
};

struct AllocInline : ASTNode {
    std::unique_ptr<ASTNode> expr;
    std::string block_name;

    AllocInline(std::unique_ptr<ASTNode> e, std::string bn, SourceLocation loc)
        : ASTNode(ASTNodeType::ALLOC_INLINE, loc), expr(std::move(e)), block_name(std::move(bn)) {}
};

struct ResetExpr : ASTNode {
    std::string block_name;

    ResetExpr(std::string bn, SourceLocation loc)
        : ASTNode(ASTNodeType::RESET_EXPR, loc), block_name(std::move(bn)) {}
};

// ─── Statements ───

struct BlockStmt : ASTNode {
    std::vector<std::unique_ptr<ASTNode>> statements;
    BlockStmt(SourceLocation loc) : ASTNode(ASTNodeType::BLOCK_STMT, loc) {}
};

struct IfStmt : ASTNode {
    std::unique_ptr<ASTNode> condition;
    std::unique_ptr<ASTNode> then_branch;
    std::unique_ptr<ASTNode> else_branch;

    IfStmt(SourceLocation loc) : ASTNode(ASTNodeType::IF_STMT, loc) {}
};

struct WhileStmt : ASTNode {
    std::unique_ptr<ASTNode> condition;
    std::unique_ptr<ASTNode> body;

    WhileStmt(SourceLocation loc) : ASTNode(ASTNodeType::WHILE_STMT, loc) {}
};

struct ForStmt : ASTNode {
    std::unique_ptr<ASTNode> init;
    std::unique_ptr<ASTNode> condition;
    std::unique_ptr<ASTNode> increment;
    std::unique_ptr<ASTNode> body;

    ForStmt(SourceLocation loc) : ASTNode(ASTNodeType::FOR_STMT, loc) {}
};

struct ReturnStmt : ASTNode {
    std::unique_ptr<ASTNode> value;
    ReturnStmt(SourceLocation loc) : ASTNode(ASTNodeType::RETURN_STMT, loc) {}
};

struct ExprStmt : ASTNode {
    std::unique_ptr<ASTNode> expr;
    ExprStmt(std::unique_ptr<ASTNode> e, SourceLocation loc)
        : ASTNode(ASTNodeType::EXPR_STMT, loc), expr(std::move(e)) {}
};

// ─── Expressions ───

struct IntLiteral : ASTNode {
    int64_t value;
    std::string literal_type;
    IntLiteral(int64_t v, SourceLocation loc)
        : ASTNode(ASTNodeType::INT_LITERAL, loc), value(v) {}
};

struct FloatLiteral : ASTNode {
    double value;
    std::string literal_type;
    FloatLiteral(double v, SourceLocation loc)
        : ASTNode(ASTNodeType::FLOAT_LITERAL, loc), value(v) {}
};

struct StringLiteral : ASTNode {
    std::string value;
    StringLiteral(std::string v, SourceLocation loc)
        : ASTNode(ASTNodeType::STRING_LITERAL, loc), value(std::move(v)) {}
};

struct BoolLiteral : ASTNode {
    bool value;
    BoolLiteral(bool v, SourceLocation loc)
        : ASTNode(ASTNodeType::BOOL_LITERAL, loc), value(v) {}
};

struct CharLiteral : ASTNode {
    char value;
    CharLiteral(char v, SourceLocation loc)
        : ASTNode(ASTNodeType::CHAR_LITERAL, loc), value(v) {}
};

struct NullLiteral : ASTNode {
    NullLiteral(SourceLocation loc) : ASTNode(ASTNodeType::NULL_LITERAL, loc) {}
};

struct IdentExpr : ASTNode {
    std::string name;
    std::string declared_type;
    IdentExpr(std::string n, SourceLocation loc)
        : ASTNode(ASTNodeType::IDENT_EXPR, loc), name(std::move(n)) {}
};

struct CallExpr : ASTNode {
    std::unique_ptr<ASTNode> callee;
    std::vector<std::unique_ptr<ASTNode>> arguments;

    CallExpr(SourceLocation loc) : ASTNode(ASTNodeType::CALL_EXPR, loc) {}
};

struct MemberExpr : ASTNode {
    std::unique_ptr<ASTNode> object;
    std::string member;

    MemberExpr(std::unique_ptr<ASTNode> obj, std::string m, SourceLocation loc)
        : ASTNode(ASTNodeType::MEMBER_EXPR, loc), object(std::move(obj)), member(std::move(m)) {}
};

struct IndexExpr : ASTNode {
    std::unique_ptr<ASTNode> array;
    std::unique_ptr<ASTNode> index;

    IndexExpr(SourceLocation loc) : ASTNode(ASTNodeType::INDEX_EXPR, loc) {}
};

struct BinaryOp : ASTNode {
    std::unique_ptr<ASTNode> left;
    TokenType op;
    std::unique_ptr<ASTNode> right;

    BinaryOp(std::unique_ptr<ASTNode> l, TokenType o, std::unique_ptr<ASTNode> r, SourceLocation loc)
        : ASTNode(ASTNodeType::BINARY_OP, loc), left(std::move(l)), op(o), right(std::move(r)) {}
};

struct UnaryOp : ASTNode {
    TokenType op;
    std::unique_ptr<ASTNode> operand;

    UnaryOp(TokenType o, std::unique_ptr<ASTNode> opnd, SourceLocation loc)
        : ASTNode(ASTNodeType::UNARY_OP, loc), op(o), operand(std::move(opnd)) {}
};

struct Assignment : ASTNode {
    std::unique_ptr<ASTNode> target;
    TokenType op;
    std::unique_ptr<ASTNode> value;

    Assignment(TokenType o, SourceLocation loc) : ASTNode(ASTNodeType::ASSIGNMENT, loc), op(o) {}
};

// ─── Macro Nodes ───

struct MacroDecl : ASTNode {
    std::string name;
    std::vector<std::string> params;
    bool has_varargs = false;
    std::vector<std::unique_ptr<ASTNode>> body;

    MacroDecl(std::string n, SourceLocation loc)
        : ASTNode(ASTNodeType::MACRO_DECL, loc), name(std::move(n)) {}
};

struct MacroCall : ASTNode {
    std::string name;
    std::vector<std::unique_ptr<ASTNode>> args;

    MacroCall(std::string n, SourceLocation loc)
        : ASTNode(ASTNodeType::MACRO_CALL, loc), name(std::move(n)) {}
};

struct BuildBlock : ASTNode {
    std::vector<std::unique_ptr<ASTNode>> body;

    BuildBlock(SourceLocation loc) : ASTNode(ASTNodeType::BUILD_BLOCK, loc) {}
};

struct EmitStmt : ASTNode {
    std::unique_ptr<ASTNode> content;

    EmitStmt(std::unique_ptr<ASTNode> c, SourceLocation loc)
        : ASTNode(ASTNodeType::EMIT_STMT, loc), content(std::move(c)) {}
};

struct Interpolate : ASTNode {
    std::string name;
    std::unique_ptr<ASTNode> expr;

    Interpolate(std::string n, SourceLocation loc)
        : ASTNode(ASTNodeType::INTERPOLATE, loc), name(std::move(n)) {}
    Interpolate(std::unique_ptr<ASTNode> e, SourceLocation loc)
        : ASTNode(ASTNodeType::INTERPOLATE, loc), expr(std::move(e)) {}
};

struct ValuePlaceholder : ASTNode {
    std::string param_name;

    ValuePlaceholder(std::string pn, SourceLocation loc)
        : ASTNode(ASTNodeType::VALUE_PLACEHOLDER, loc), param_name(std::move(pn)) {}
};

} // namespace brick

#endif
