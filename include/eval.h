#pragma once

#include "ast.h"
#include "object.h"

namespace metro {

class Evaluator {
public:

  Evaluator() { }

  objects::Object* eval(AST::Base* ast);


private:


};

} // namespace metro