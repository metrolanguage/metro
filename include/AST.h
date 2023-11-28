#pragma once

#include <vector>
#include "Token.h"
#include "Object.h"

namespace metro {

namespace builtin {
  struct BuiltinFunc;
}

enum class ASTKind {
  // factor
  Value,
  Variable,
  CallFunc,
  
  Array,
  Tuple,

  // member access
  MemberAccess,

  IndexRef,

  Not,

  // expr
  Add,
  Sub,
  Mul,
  Div,
  Mod,

  LShift,
  RShift,

  Range,

  Bigger,         // a >  b
  BiggerOrEqual,  // a >= b

  Equal,

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
  // Let,

  // control-statements
  If,
  Switch,
  Return,
  Break,
  Continue,

  // loop-statements
  Loop,
  While,
  DoWhile,
  For,

  // function
  Function,

  // user-types
  Enum,
  Struct,

  // namespace
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

struct WithName : Base {
  std::string name;

  std::string const& getName() const {
    return this->name;
  }

  WithName(ASTKind kind, std::string const& name)
    : Base(kind, nullptr),
      name(name)
  {
  }

protected:
  WithName(ASTKind k, Token* t = nullptr)
    : Base(k, t),
      name(t ? t->str : "")
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

  ~Value()
  {
  }
};

struct Variable : WithName {
  using WithName::WithName;
  
  Variable(Token* token)
    : WithName(ASTKind::Variable, token)
  {
  }
};

struct Array : Base {
  std::vector<Base*> elements;

  Array(Token* token, std::vector<Base*> elems = { })
    : Base(ASTKind::Array, token),
      elements(std::move(elems))
  {
  }

  ~Array()
  {
    for( auto&& e : this->elements )
      delete e;
  }
};

struct Function;
struct CallFunc : Base {
  std::vector<Base*> arguments;

  Function const* userdef;              // if user-defined
  builtin::BuiltinFunc const* builtin;  // if builtin

  std::string_view getName() const {
    return this->token->str;
  }

  CallFunc(Token* token, std::vector<Base*> arguments = { })
    : Base(ASTKind::CallFunc, token),
      arguments(std::move(arguments)),
      userdef(nullptr),
      builtin(nullptr)
  {
  }

  ~CallFunc()
  {
    for( auto&& arg : this->arguments )
      delete arg;
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

  ~Expr()
  {
    delete this->left;

    if( this->right )
      delete this->right;
  }
};

struct Scope : Base {
  std::vector<Base*> list;

  Scope(Token* token)
    : Base(ASTKind::Scope, token)
  {
  }

  ~Scope()
  {
    for( auto&& x : this->list )
      delete x;
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

  ~If()
  {
    delete this->cond;
    delete this->case_true;
    delete this->case_false;
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

    ~Case()
    {
      delete this->to_compare;
      delete this->scope;
    }
  };

  Base* expr;
  std::vector<Case> cases;

  Case& append(Base* to_compare, Base* scope) {
    return this->cases.emplace_back(to_compare, scope);
  }

  Switch(Token* token)
    : Base(ASTKind::Switch, token),
      expr(nullptr)
  {
  }

  ~Switch()
  {
    delete this->expr;
  }
};

struct While : Base {
  Base* cond;
  Base* code;

  While(Token* token)
    : Base(ASTKind::While, token),
      cond(nullptr),
      code(nullptr)
  {
  }

  ~While()
  {
    delete this->cond;
    delete this->code;
  }
};

struct For : Base {
  Base* iter;
  Base* content;
  Base* code;

  For(Token* token)
    : Base(ASTKind::For, token),
      iter(nullptr),
      content(nullptr),
      code(nullptr)
  {
  }

  ~For()
  {
    delete this->iter;
    delete this->content;
    delete this->code;
  }
};

struct Function : Base {
  Token* name_token;
  std::vector<Token*> arguments;
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

  ~Function()
  {
    delete this->scope;
  }
};

struct Enum : Base {
  struct Enumerator {

  };

  Token* name_token;

};



} // namespace AST

} // namespace metro