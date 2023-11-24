#pragma once

#include <vector>
#include "token.h"
#include "object.h"

namespace metro {

struct BuiltinFunc;

enum class ASTKind {
  // factor
  Value,
  Variable,
  CallFunc,
  
  // member access
  MemberAccess,

  // expr
  Add,
  Sub,
  Mul,
  Div,
  Mod,

  // bit calc
  BitAND,   // &
  BitXOR,   // ^
  BitOR,    // |

  // logical
  LogAND,     // &&
  LogOR,      // ||

  // assign
  Assignment,

  // scope
  Scope,

  // variable declaration
  Let,

  // control-statements
  If,
  Switch,

  // loop-statements
  While, /* also: used for other kind. */

  // global scope
  Function,
  Namespace,


  /* "import" be processed immediately in parsing. */

};

namespace AST {

struct Base {
  ASTKind kind;
  Token* token;

  template <class T>
  T* as() { return (T*)this; }

protected:
  Base(ASTKind kind, Token* token = nullptr)
    : kind(kind),
      token(token)
  {
  }
};

struct Value : Base {
  objects::Object* object;

  Value(Token* token, objects::Object* object)
    : Base(ASTKind::Value, token),
      object(object)
  {
  }
};

struct Variable : Base {
  std::string_view getName() const {
    return this->token->str;
  }

  Variable(Token* token)
    : Base(ASTKind::Variable, token)
  {
  }
};

struct Function;
struct CallFunc : Base {
  std::vector<Base*> arguments;

  union {
    Function* userdef;      // if user-defined
    BuiltinFunc* builtin;   // if builtin
  };

  std::string_view getName() const {
    return this->token->str;
  }

  CallFunc(Token* token, std::vector<Base*> arguments = { })
    : Base(ASTKind::CallFunc, token),
      arguments(std::move(arguments)),
      userdef(nullptr)
  {
  }
};

struct Expr : Base {
  Base* left;
  Base* right;
  
  Expr(ASTKind kind, Token* op_token, Base* left, Base* right)
    : Base(kind, op_token), // the op of "a op b"
      left(left),
      right(right)
  {
  }
};

struct Scope : Base {
  std::vector<Base*> statements;

  Scope()
    : Base(ASTKind::Scope)
  {
  }
};

struct If : Base {
  Base* cond;
  Base* case_true;
  Base* case_false;

  If(Token* token)
    : Base(ASTKind::If, token),
      cond(nullptr),
      case_true(nullptr),
      case_false(nullptr)
  {
  }
};

struct Switch : Base {
  struct Case {
    Base* to_compare;
    Base* scope;

    Case(Base* to_compare, Base* scope)
      : to_compare(to_compare),
        scope(scope)
    {
    }
  };

  Base* expr;
  std::vector<Case> cases;

  Case& append(Base* toCompare, Base* scope) {
    return this->cases.emplace_back(toCompare, scope);
  }

  Switch(Token* token)
    : Base(ASTKind::Switch, token),
      expr(nullptr)
  {
  }
};

struct While : Base {
  Base* cond;
  Base* scope;

  While(Token* token)
    : Base(ASTKind::While, token),
      cond(nullptr),
      scope(nullptr)
  {
  }
};

struct Function : Base {
  struct Argument {
    Base* type;
    Token* name;

    Argument(Base* type, Token* name)
      : type(type),
        name(name)
    {
    }
  };

  Token* name_token;
  std::vector<Argument> arguments;
  Scope* scope;

  std::string_view getName() const {
    return this->name_token->str;
  }

  Function(Token* token)
    : Base(ASTKind::Function, token),
      name_token(nullptr),
      scope(nullptr)
  {
  }
};

} // namespace AST

} // namespace metro