#include <iostream>
#include <sstream>
#include "alert.h"
#include "BuiltinFunc.h"
#include "Evaluator.h"
#include "GC.h"
#include "Utils.h"
#include "Error.h"

namespace metro {

using namespace objects;

Evaluator::Evaluator(AST::Scope* rootScope)
  : rootScope(rootScope),
    _loopScope(nullptr),
    _funcScope(nullptr),
    _scope(nullptr)
{
}

//
// === eval ===
//
Object* Evaluator::eval(AST::Base* ast) {

  switch( ast->kind ) {
    case ASTKind::Function:
      break;

    case ASTKind::Value: {
      return ast->as<AST::Value>()->object;
    }

    case ASTKind::Variable: {
      auto var = ast->as<AST::Variable>();
      auto pvar = this->getCurrentStorage()[var->getName()];

      if( this->inFunction() && !pvar )
        pvar = this->globalStorage[var->getName()];

      if( !pvar )
        Error(ast)
          .setMessage("variable '" + std::string(ast->token->str) +"' is not defined")
          .emit()
          .exit();

      return pvar->clone();
    }

    case ASTKind::Array: {
      auto obj = new Vector();

      for( auto&& e : ast->as<AST::Array>()->elements )
        obj->append(this->eval(e));

      return obj;
    }

    case ASTKind::Tuple: {
      auto obj = new Tuple({ });

      for( auto&& e : ast->as<AST::Array>()->elements )
        obj->elements.emplace_back(this->eval(e));

      return obj;
    }

    //
    // call function
    //
    case ASTKind::CallFunc: {
      auto cf = ast->as<AST::CallFunc>();
      std::vector<Object*> args;

      for( auto&& arg : cf->arguments )
        GC::bind(args.emplace_back(this->eval(arg)));

      auto result = this->evalCallFunc(cf, nullptr, args);

      for( auto&& arg : args )
        GC::unbind(arg);

      return result;
    }

    //
    // index ref
    //
    case ASTKind::IndexRef: {
      auto x = ast->as<AST::Expr>();

      return this->evalIndexRef(x, this->eval(x->left), this->eval(x->right));
    }

    //
    // member access
    //
    case ASTKind::MemberAccess: {
      auto x = ast->as<AST::Expr>();

      auto obj = this->eval(x->left);
      std::string name;

      if( x->right->kind == ASTKind::Variable ) {
        auto member = x->right->as<AST::Variable>();
        name = member->getName();

        switch( obj->type.kind ) {
          case Type::Int: {
            if( name == "abs" )
              return new Int(std::abs(obj->as<Int>()->value));

            break;
          }

          case Type::String: {
            if( name == "count" )
              return new USize(obj->as<String>()->value.size());

            break;
          }
        }
      }
      else if( x->right->kind == ASTKind::CallFunc ) {
        alert;

      }
      else {
        throw;
      }

      Error(x->right)
        .setMessage("object of type '" + obj->type.toString() + "' don't have a member '" + name + "'")
        .emit()
        .exit();
    }

    //
    // not
    //
    case ASTKind::Not: {
      auto x = ast->as<AST::Expr>();

      auto obj = this->eval(x->left);

      if( !obj->type.equals(Type::Bool) ) {
        Error(x->left)
          .setMessage("expected boolean expression")
          .emit()
          .exit();
      }

      obj->as<Bool>()->value ^= 1;
      return obj;
    }

    //
    // equal
    //
    case ASTKind::Equal: {
      auto x = ast->as<AST::Expr>();

      return new Bool(this->eval(x->left)->equals(this->eval(x->right)));
    }

    //
    // logical
    //
    case ASTKind::LogAND:
    case ASTKind::LogOR: {
      auto x = ast->as<AST::Expr>();

      auto lhs = this->eval(x->left);
      auto rhs = this->eval(x->right);

      if( !lhs->type.equals(Type::Bool) ) {
        Error(x->left)
          .setMessage("expected boolean expression")
          .emit()
          .exit();
      }

      if( !rhs->type.equals(Type::Bool) ) {
        Error(x->right)
          .setMessage("expected boolean expression")
          .emit()
          .exit();
      }

      if( ast->kind == ASTKind::LogAND )
        return new Bool(lhs->as<Bool>()->value && rhs->as<Bool>()->value);

      return new Bool(lhs->as<Bool>()->value || rhs->as<Bool>()->value);
    }

    //
    // range
    //
    case ASTKind::Range: {
      auto x = ast->as<AST::Expr>();

      auto begin = this->eval(x->left);
      auto end = this->eval(x->right);

      if( !begin->type.equals(Type::Int) )
        Error(x->left)
          .setMessage("expected 'int' object")
          .emit()
          .exit();

      if( !end->type.equals(Type::Int) )
        Error(x->right)
          .setMessage("expected 'int' object")
          .emit()
          .exit();

      if( begin->as<Int>()->value >= end->as<Int>()->value ) {
        Error(x)
          .setMessage("start value must less than end").emit().exit();
      }

      return new Range(begin->as<Int>()->value, end->as<Int>()->value);
    }
 
    //
    // assign
    //
    case ASTKind::Assignment: {
      auto assign = ast->as<AST::Expr>();

      auto value = this->eval(assign->right);

      if( assign->left->kind == ASTKind::Variable ) {
        auto& storage = this->getCurrentStorage();
        auto name = assign->left->as<AST::Variable>()->getName();

        if( !storage.contains(name) )
          GC::bind(value);
        else
          GC::unbind(storage[name]);
      }

      return this->evalAsLeft(assign->left) = value;
    }

    //
    // Statements
    //
    case ASTKind::Scope:
    case ASTKind::If:
    case ASTKind::Switch:
    case ASTKind::Return:
    case ASTKind::Break:
    case ASTKind::Continue:
    case ASTKind::Loop:
    case ASTKind::While:
    case ASTKind::DoWhile:
    case ASTKind::For:
      this->evalStatements(ast);
      break;

    default:
      return this->evalOperator(ast->as<AST::Expr>());
  }

  return None::getNone();
}

//
// === evalAsLeft ===
//
Object*& Evaluator::evalAsLeft(AST::Base* ast) {
  switch( ast->kind ) {
    case ASTKind::Variable: {
      return *this->findVariable(ast->as<AST::Variable>()->getName());
    }

    case ASTKind::IndexRef: {
      auto x = ast->as<AST::Expr>();

      return this->evalIndexRef(x, this->evalAsLeft(x->left), this->eval(x->right));
    }

    case ASTKind::MemberAccess:
      return this->evalAsLeft(ast->as<AST::Expr>()->left);
  }

  Error(ast)
    .setMessage("expected writable expression")
    .emit()
    .exit();
}

//
// === evalIndexRef ===
//
Object*& Evaluator::evalIndexRef(AST::Expr* ast, Object* obj, Object* objIndex) {

  size_t index = 0;

  switch( objIndex->type.kind ) {
    case Type::Int:
      index = objIndex->as<Int>()->value;
      break;

    case Type::USize:
      index = objIndex->as<USize>()->value;
      break;

    default:
      Error(ast->right)
        .setMessage("expected 'int' or 'usize' object")
        .emit()
        .exit();
  }

  switch( obj->type.kind ) {
    case Type::String: {
      return (Object*&)obj->as<String>()->value[index];
    }

    case Type::Vector:
      return obj->as<Vector>()->elements[index];
  }

  Error(ast->right)
    .setMessage("object of type '" + obj->type.toString() + "' is not subscriptable")
    .emit()
    .exit();
}

Object* Evaluator::evalCallFunc(AST::CallFunc* ast, Object* self, std::vector<Object*>& args) {

  auto [userdef, builtin] = this->findFunction(ast->getName(), self);
  auto name = std::string(ast->getName());

  if( self ) {
    args.insert(args.begin(), self);
  }

  if( builtin ) {
    return builtin->call(ast, args);
  }

  if( !userdef ) {
    auto error = Error(ast->token);

    if( self )
      error.setMessage("object of type '" + self->type.toString() + "' is not have member function '" + name + "'");
    else
      error.setMessage("undefined function name '" + name + "'");

    error
      .emit()
      .exit();
  }

  if( userdef->arguments.size() != args.size() ) {
    auto err = Error(ast->token);

    if( userdef->arguments.size() < args.size() ) {
      err.setMessage("too many arguments");
    }
    else {
      err.setMessage("too few arguments");
    }

    err.emit().exit();
  }

  auto& stack = this->push_stack(userdef);

  for( auto it = args.begin(); auto&& arg : userdef->arguments )
    stack.storage[arg->str] = *it++;

  this->eval(userdef->scope);

  auto result = stack.result;

  this->pop_stack();

  return result ? result : None::getNone();
}

Evaluator::CallStack& Evaluator::push_stack(AST::Function const* func) {
  return this->callStacks.emplace_back(func);
}

void Evaluator::pop_stack() {
  auto& stack = this->getCurrentCallStack();

  for( auto&& [_, lvar] : stack.storage ) {
    GC::unbind(lvar);
  }

  this->callStacks.pop_back();
}

Evaluator::ScopeEvaluationFlags& Evaluator::getCurrentScope() {
  return *this->_scope;
}

bool Evaluator::inFunction() const {
  return !this->callStacks.empty();
}

Evaluator::CallStack& Evaluator::getCurrentCallStack() {
  return *callStacks.rbegin();
}

Evaluator::Storage& Evaluator::getCurrentStorage() {
  if( this->inFunction() )
    return this->getCurrentCallStack().storage;

  return this->globalStorage;
}

Object** Evaluator::findVariable(std::string_view name, bool allowCreate) {
  auto& storage = this->getCurrentStorage();

  if( !storage.contains(name) && !allowCreate )
    return nullptr;

  return &storage[name];
}


//
// -- findFunction() --
//
std::tuple<AST::Function const*, builtin::BuiltinFunc const*> Evaluator::findFunction(std::string_view name, Object* self) {

  for( auto&& bf : builtin::BuiltinFunc::getAllFunctions() ) {
    if( !bf.have_self != !self )
      continue;

    if( self && !self->type.equals(bf.self_type) )
      continue;

    if( bf.name == name ) {
      return { nullptr, &bf };
    }
  }

  for( auto&& ast : this->rootScope->list ) {
    if( ast->kind != ASTKind::Function )
      continue;

    auto func = ast->as<AST::Function>();

    if( func->getName() == name )
      return { func, nullptr };
  }

  return { nullptr, nullptr };
}

} // namespace metro