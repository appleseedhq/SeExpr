/*
 SEEXPR SOFTWARE
 Copyright 2011 Disney Enterprises, Inc. All rights reserved
 
 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are
 met:
 
 * Redistributions of source code must retain the above copyright
 notice, this list of conditions and the following disclaimer.
 
 * Redistributions in binary form must reproduce the above copyright
 notice, this list of conditions and the following disclaimer in
 the documentation and/or other materials provided with the
 distribution.
 
 * The names "Disney", "Walt Disney Pictures", "Walt Disney Animation
 Studios" or the names of its contributors may NOT be used to
 endorse or promote products derived from this software without
 specific prior written permission from Walt Disney Pictures.
 
 Disclaimer: THIS SOFTWARE IS PROVIDED BY WALT DISNEY PICTURES AND
 CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,
 BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
 FOR A PARTICULAR PURPOSE, NONINFRINGEMENT AND TITLE ARE DISCLAIMED.
 IN NO EVENT SHALL WALT DISNEY PICTURES, THE COPYRIGHT HOLDER OR
 CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND BASED ON ANY
 THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.
*/

/* Parse tree nodes - this is where the expression evaluation happens.

   Some implementation notes: 

   1) Vector vs scalar - Any node can accept vector or scalar inputs
   and return a vector or scalar result.  If a node returns a scalar,
   it is only required to set the [0] component and the other
   components must be assumed to be invalid.

   2) SeExprNode::prep - This is called for all nodes during parsing
   once the syntax has been checked.  Anything can be done here, such
   as function binding, variable lookups, etc., but the only thing
   that must be done is to determine whether the result is going to be
   vector or scalar.  This can in some cases depend on whether the
   children are vector or scalar so the parser calls prep on the root
   node and each node is expected to call prep on its children and
   then set its own _isVec variable.  The wantVec param provides
   context from the parent (and ultimately the owning expression) as
   to whether a vector is desired, but nodes are not bound by this and
   may produce a scalar even when a vector is wanted.
   
   The base class method implements the default behavior which is to
   pass down the wantVec flag to all children and set isVec to true if
   any child is a vec.

   If the prep method fails, an error string should be set and false
   should be returned.
*/

#ifndef MAKEDEPEND
#include <math.h>
#include <sstream>
#endif
#include "SeVec3d.h"
#include "SeExprType.h"
#include "SeExpression.h"
#include "SeExprEnv.h"
#include "SeExprNode.h"
#include "SeExprFunc.h"


SeExprNode::SeExprNode(const SeExpression* expr)
    : _expr(expr), _parent(0), _isVec(0)
{
}


SeExprNode::SeExprNode(const SeExpression* expr,
                       const SeExprType & type)
    : _expr(expr), _parent(0), _isVec(0), _type(type)
{
}


SeExprNode::SeExprNode(const SeExpression* expr,
                       SeExprNode* a)
    : _expr(expr), _parent(0), _isVec(0)
{
    _children.reserve(1);
    addChild(a);
}


SeExprNode::SeExprNode(const SeExpression* expr,
                       SeExprNode* a,
                       const SeExprType & type)
    : _expr(expr), _parent(0), _isVec(0), _type(type)
{
    _children.reserve(1);
    addChild(a);
}


SeExprNode::SeExprNode(const SeExpression* expr,
                       SeExprNode* a,
                       SeExprNode* b)
    : _expr(expr), _parent(0), _isVec(0)
{
    _children.reserve(2);
    addChild(a);
    addChild(b);
}


SeExprNode::SeExprNode(const SeExpression* expr,
                       SeExprNode* a,
                       SeExprNode* b,
                       const SeExprType & type)
    : _expr(expr), _parent(0), _isVec(0), _type(type)
{
    _children.reserve(2);
    addChild(a);
    addChild(b);
}


SeExprNode::SeExprNode(const SeExpression* expr,
                       SeExprNode* a,
                       SeExprNode* b,
		       SeExprNode* c)
    : _expr(expr), _parent(0), _isVec(0)
{
    _children.reserve(3);
    addChild(a);
    addChild(b);
    addChild(c);
}


SeExprNode::SeExprNode(const SeExpression* expr,
                       SeExprNode* a,
                       SeExprNode* b,
		       SeExprNode* c,
                       const SeExprType & type)
    : _expr(expr), _parent(0), _isVec(0), _type(type)
{
    _children.reserve(3);
    addChild(a);
    addChild(b);
    addChild(c);
}


SeExprNode::~SeExprNode()
{
    // delete children
    std::vector<SeExprNode*>::iterator iter;
    for (iter = _children.begin(); iter != _children.end(); iter++)
	delete *iter;
}


void
SeExprNode::addChild(SeExprNode* child)
{
    _children.push_back(child);
    child->_parent = this;
}


void
SeExprNode::addChildren(SeExprNode* surrogate)
{
    std::vector<SeExprNode*>::iterator iter;
    for (iter = surrogate->_children.begin();
	 iter != surrogate->_children.end(); 
	 iter++)
    {
	addChild(*iter);
    }
    surrogate->_children.clear();
    delete surrogate;
}


SeExprType
SeExprNode::prep(SeExprType wanted, SeExprVarEnv & env)
{
    /** The default behavior is to call prep on children (giving AnyType as desired type).
     *  If all children return valid types, returns NoneType.
     *  Otherwise,                          returns ErrorType.
     *  *Note:* Ignores wanted type.
     */
    bool error = false;

    _type = SeExprType::NoneType();

    std::vector<SeExprNode*>::iterator       child   = _children.begin();
    std::vector<SeExprNode*>::iterator const e_child = _children.end  ();

    for(; child != e_child; child++)
        error = error || (!((*child)->prep(SeExprType::AnyType(), env)).isValid());

    if(error)
        _type = SeExprType::ErrorType();

    return _type;
}


void
SeExprNode::eval(SeVec3d& result) const
{
    // the default behavior is to just eval all the children for
    // their side-effects (i.e. setting variables)
    // there is no result value
    SeVec3d val;
    for (int i = 0; i < numChildren(); i++)
	child(i)->eval(val);
    result = 0.0;
}


SeExprType
SeExprBlockNode::prep(SeExprType wanted, SeExprVarEnv & env)
{
    bool valid = child(0)->prep(SeExprType::AnyType(), env).isValid();

    _type = child(1)->prep(wanted, env);

    if(!valid)
        _type = SeExprType::ErrorType();

    return _type;
}


void
SeExprBlockNode::eval(SeVec3d& result) const
{
    // eval block, then eval primary expr
    SeVec3d val;
    child(0)->eval(val);
    child(1)->eval(result);
}


SeExprType
SeExprIfThenElseNode::prep(SeExprType wanted, SeExprVarEnv & env)
{
    SeExprVarEnv           thenEnv,  elseEnv;
    SeExprType   condType, thenType, elseType;

    bool error = false;
    _type = SeExprType::NoneType();

    condType = child(0)->prep(SeExprType::FP1Type(),env);

    if(!condType.isValid())
        error = true;
    else if(!condType.isa(SeExprType::FP1Type())) {
        error = true;
        addError("Expected FP1 type in condition expression of if statement but found " + condType.toString());
    }

    thenEnv  = SeExprVarEnv::newScope(env);
    thenType = child(1)->prep(SeExprType::AnyType(), thenEnv);

    elseEnv  = SeExprVarEnv::newScope(env);
    elseType = child(2)->prep(SeExprType::AnyType(), elseEnv);

    if(!thenType.isValid() ||
       !elseType.isValid())
        error = true;

    if(env.changesMatch(thenEnv, elseEnv)) {
        env.add(thenEnv);
    } else {
        error = true;
        addError("Types of variables do not match after if statement");
    }

    if(error)
        _type = SeExprType::ErrorType();

    return _type;
}


void
SeExprIfThenElseNode::eval(SeVec3d& result) const
{
    // eval condition, then choose then/else block
    SeVec3d val;
    child(0)->eval(val);
    if (val[0])
	child(1)->eval(val);
    else
	child(2)->eval(val);
    result = 0.0;
}


SeExprType
SeExprAssignNode::prep(SeExprType wanted, SeExprVarEnv & env)
{
    _type = SeExprType::NoneType();

    _assignedType = child(0)->prep(SeExprType::AnyType(), env);

    if(!_assignedType.isValid())
        _type = SeExprType::ErrorType();

    //This could add errors to the variable environment
    env.add(_name, new SeExprLocalVarRef(_assignedType));

    return _type;
}


void
SeExprAssignNode::eval(SeVec3d& result) const
{
    if (_var) {
	// eval expression and store in variable
	const SeExprNode* node = child(0);
	node->eval(_var->val);
        //assume that eval made the correct assignment
	//if (_var->isVec() && !node->isVec())
        //_var->val[1] = _var->val[2] = _var->val[0];
    }
    else result = 0.0;
}


SeExprType
SeExprVecNode::prep(SeExprType wanted, SeExprVarEnv & env)
{
    bool error = false;
    _type = SeExprType::FPNType(numChildren());

    std::vector<SeExprNode*>::iterator       ic = _children.begin();
    std::vector<SeExprNode*>::iterator const ec = _children.end  ();

    for(int count = 1; ic != ec; ic++, count++) {
        SeExprType childType = (*ic)->prep(SeExprType::FP1Type(), env);
        if(!childType.isValid())
            error = true;
        else if(!childType.isa(SeExprType::FP1Type())) {
            error = true;
            std::stringstream countstream;
            countstream << count;
            addError("Expected FP1 type in vector literal but found " + childType.toString() + " in position " + countstream.str());
        }
    }

    if(error)
        _type = SeExprType::ErrorType();

    return _type;
}


void
SeExprVecNode::eval(SeVec3d& result) const
{
    if (_isVec) {
	SeVec3d v;
	child(0)->eval(v); result[0] = v[0];
	child(1)->eval(v); result[1] = v[0];
	child(2)->eval(v); result[2] = v[0];
    } else {
	child(0)->eval(result);
    }
}


SeVec3d
SeExprVecNode::value() const {
    if(const SeExprNumNode* f = dynamic_cast<const SeExprNumNode*>(child(0))) {
        double first = f->value();
        if(const SeExprNumNode* s = dynamic_cast<const SeExprNumNode*>(child(1))) {
            double second = s->value();
            if(const SeExprNumNode* t = dynamic_cast<const SeExprNumNode*>(child(2))) {
                double third = t->value();
                return SeVec3d(first, second, third);
            };
        };
    };

    return SeVec3d(0.0);
};


SeExprType
SeExprCondNode::prep(SeExprType wanted, SeExprVarEnv & env)
{
    //TODO: determine if extra environments are necessary
    SeExprType condType, thenType, elseType;

    bool error = false;
    _type = SeExprType::ErrorType();

    condType = child(0)->prep(SeExprType::FP1Type(),env);

    if(!condType.isValid())
        error = true;
    else if(!condType.isa(SeExprType::FP1Type())) {
        error = true;
        addError("Expected FP1 type in condition of ternary conditional expression but found " + condType.toString());
    }

    thenType = child(1)->prep(wanted, env);

    elseType = child(2)->prep(wanted, env);

    if(!thenType.isValid() ||
       !elseType.isValid())
        error = true;
    else {
        if(thenType.isa(wanted)) {
            error = true;
            addError("Expected " + wanted.toString() + " type from then branch of ternary conditional expression but found " + thenType.toString());
        }

        if(elseType.isa(wanted)) {
            error = true;
            addError("Expected " + wanted.toString() + " type from else branch of ternary conditional expression but found " + elseType.toString());
        }
    }

    if(!error)
        _type = thenType;

    return _type;
}


void
SeExprCondNode::eval(SeVec3d& result) const
{
    SeVec3d v;
    child(0)->eval(v);
    const SeExprNode* node = v[0] ? child(1) : child(2);
    node->eval(result);
    if (_isVec && !node->isVec())
	result[1] = result[2] = result[0];
}


SeExprType
SeExprAndNode::prep(SeExprType wanted, SeExprVarEnv & env)
{
    //TODO: determine if extra environment is necessary
    SeExprType firstType, secondType;

    bool error = false;
    _type = SeExprType::FP1Type();

    firstType = child(0)->prep(SeExprType::FP1Type(), env);

    if(!firstType.isValid())
        error = true;
    else if(!firstType.isa(SeExprType::FP1Type())) {
        error = true;
        addError("Expected FP1 type from first operand of and expression but found " + firstType.toString());
    }

    secondType = child(1)->prep(SeExprType::FP1Type(), env);

    if(!secondType.isValid())
        error = true;
    else if(!secondType.isa(SeExprType::FP1Type())) {
        error = true;
        addError("Expected FP1 type from second operand of and expression but found " + secondType.toString());
    }

    if(error)
        _type = SeExprType::ErrorType();

    return _type;
}


void
SeExprAndNode::eval(SeVec3d& result) const
{
    // operands and result must be scalar
    SeVec3d a, b;
    child(0)->eval(a);
    if (!a[0]) {
	result[0] = 0;
    } else { 
	child(1)->eval(b);
	result[0] = (b[0] != 0.0); 
    }
}


SeExprType
SeExprOrNode::prep(SeExprType wanted, SeExprVarEnv & env)
{
    //TODO: determine if extra environment is necessary
    SeExprType firstType, secondType;

    bool error = false;
    _type = SeExprType::FP1Type();

    firstType = child(0)->prep(SeExprType::FP1Type(), env);

    if(!firstType.isValid())
        error = true;
    else if(!firstType.isa(SeExprType::FP1Type())) {
        error = true;
        addError("Expected FP1 type from first operand of or expression but found " + firstType.toString());
    }

    secondType = child(1)->prep(SeExprType::FP1Type(), env);

    if(!secondType.isValid())
        error = true;
    else if(!secondType.isa(SeExprType::FP1Type())) {
        error = true;
        addError("Expected FP1 type from second operand of or expression but found " + secondType.toString());
    }

    if(error)
        _type = SeExprType::ErrorType();

    return _type;
}


void
SeExprOrNode::eval(SeVec3d& result) const
{
    // operands and result must be scalar
    SeVec3d a, b;
    child(0)->eval(a);
    if (a[0]) {
	result[0] = 1;
    } else { 
	child(1)->eval(b);
	result[0] = (b[0] != 0.0); 
    }
}


SeExprType
SeExprSubscriptNode::prep(SeExprType wanted, SeExprVarEnv & env)
{
    //TODO: double-check order of evaluation - order MAY effect environment evaluation (probably not, though)
    SeExprType vecType, scriptType;

    bool error = false;
    _type = SeExprType::FP1Type();

    vecType = child(0)->prep(SeExprType::NumericType(), env);

    if(!vecType.isValid())
        error = true;
    else if(!vecType.isa(SeExprType::NumericType())) {
        error = true;
        addError("Expected Numeric type from vector operand of subscript operator but found " + vecType.toString());
    }

    scriptType = child(1)->prep(SeExprType::FP1Type(), env);

    if(!scriptType.isValid())
        error = true;
    else if(!scriptType.isa(SeExprType::FP1Type())) {
        error = true;
        addError("Expected FP1 type from subscript operand of subscript operator but found " + scriptType.toString());
    }

    if(error)
        _type = SeExprType::ErrorType();

    return _type;
}


void
SeExprSubscriptNode::eval(SeVec3d& result) const
{
    const SeExprNode* child0 = child(0);
    const SeExprNode* child1 = child(1);
    SeVec3d a, b;
    child0->eval(a);
    child1->eval(b);
    int index = int(b[0]);

    if (child0->isVec()) {
	switch(index) {
	case 0:  result[0] = a[0]; break;
	case 1:  result[0] = a[1]; break;
	case 2:  result[0] = a[2]; break;
	default: result[0] = 0; break;
	}
    } else {
	switch(index) {
	case 0:
	case 1:
	case 2:  result[0] = a[0]; break;
	default: result[0] = 0; break;
	}
    }
}


SeExprType
SeExprNegNode::prep(SeExprType wanted, SeExprVarEnv & env)
{
    _type = child(0)->prep(wanted, env);

    if(_type.isValid() &&
       !_type.isa(SeExprType::NumericType())) {
        addError("Expected Numeric type from operand to negation operator but found " + _type.toString());
        _type = SeExprType::ErrorType();
    }

    return _type;
}


void
SeExprNegNode::eval(SeVec3d& result) const
{
    SeVec3d a;
    const SeExprNode* child0 = child(0);
    child0->eval(a);
    result[0] = -a[0];
    if (_isVec) {
	result[1] = -a[1];
	result[2] = -a[2];
    }
}


SeExprType
SeExprInvertNode::prep(SeExprType wanted, SeExprVarEnv & env)
{
    _type = child(0)->prep(wanted, env);

    if(_type.isValid() &&
       !_type.isa(SeExprType::NumericType())) {
        addError("Expected Numeric type from operand to inversion operator but found " + _type.toString());
        _type = SeExprType::ErrorType();
    }

    return _type;
}


void
SeExprInvertNode::eval(SeVec3d& result) const
{
    SeVec3d a;
    const SeExprNode* child0 = child(0);
    child0->eval(a);
    result[0] = 1-a[0];
    if (_isVec) {
	result[1] = 1-a[1];
	result[2] = 1-a[2];
    }
}


SeExprType
SeExprNotNode::prep(SeExprType wanted, SeExprVarEnv & env)
{
    _type = child(0)->prep(wanted, env);

    if(_type.isValid() &&
       !_type.isa(SeExprType::NumericType())) {
        addError("Expected Numeric type from operand to not operator but found " + _type.toString());
        _type = SeExprType::ErrorType();
    }

    return _type;
}


void
SeExprNotNode::eval(SeVec3d& result) const
{
    SeVec3d a;
    const SeExprNode* child0 = child(0);
    child0->eval(a);
    result[0] = !a[0];
    if (_isVec) {
	result[1] = !a[1];
	result[2] = !a[2];
    }
}


SeExprType
SeExprEqNode::prep(SeExprType wanted, SeExprVarEnv & env)
{
    //TODO: double-check order of evaluation - order MAY effect environment evaluation (probably not, though)
    SeExprType firstType, secondType;

    bool error = false;
    _type = SeExprType::FP1Type();

    firstType = child(0)->prep(SeExprType::NumericType(), env);

    if(!firstType.isValid())
        error = true;
    else if(!firstType.isa(SeExprType::NumericType())) {
        error = true;
        addError("Expected Numeric type from first operand to == operator but found" + firstType.toString());
    }

    secondType = child(1)->prep(SeExprType::NumericType(), env);

    if(!secondType.isValid())
        error = true;
    else if(!secondType.isa(SeExprType::NumericType())) {
        error = true;
        addError("Expected Numeric type from second operand to == operator but found" + secondType.toString());
    }

    if(firstType.isValid()  &&
       secondType.isValid() &&
       !firstType.compatibleNum(secondType)) {
        error = true;
        addError("Types " + firstType.toString() + " and " + secondType.toString() + " are not compatible types for == operator");
    }

    if(error)
        _type = SeExprType::ErrorType();

    return _type;
}


void
SeExprEqNode::eval(SeVec3d& result) const
{
    SeVec3d a, b;
    const SeExprNode* child0 = child(0);
    const SeExprNode* child1 = child(1);
    child0->eval(a);
    child1->eval(b);

    if (!child0->isVec()) a[1] = a[2] = a[0];
    if (!child1->isVec()) b[1] = b[2] = b[0];
    result[0] = a[0] == b[0] && a[1] == b[1] && a[2] == b[2];
}


SeExprType
SeExprNeNode::prep(SeExprType wanted, SeExprVarEnv & env)
{
    //TODO: double-check order of evaluation - order MAY effect environment evaluation (probably not, though)
    SeExprType firstType, secondType;

    bool error = false;
    _type = SeExprType::FP1Type();

    firstType = child(0)->prep(SeExprType::NumericType(), env);

    if(!firstType.isValid())
        error = true;
    else if(!firstType.isa(SeExprType::NumericType())) {
        error = true;
        addError("Expected Numeric type from first operand to != operator but found" + firstType.toString());
    }

    secondType = child(1)->prep(SeExprType::NumericType(), env);

    if(!secondType.isValid())
        error = true;
    else if(!secondType.isa(SeExprType::NumericType())) {
        error = true;
        addError("Expected Numeric type from second operand to != operator but found" + secondType.toString());
    }

    if(firstType.isValid()  &&
       secondType.isValid() &&
       !firstType.compatibleNum(secondType)) {
        error = true;
        addError("Types " + firstType.toString() + " and " + secondType.toString() + " are not compatible types for != operator");
    }

    if(error)
        _type = SeExprType::ErrorType();

    return _type;
}


void
SeExprNeNode::eval(SeVec3d& result) const
{
    SeVec3d a, b;
    const SeExprNode* child0 = child(0);
    const SeExprNode* child1 = child(1);
    child0->eval(a);
    child1->eval(b);

    if (!child0->isVec()) a[1] = a[2] = a[0];
    if (!child1->isVec()) b[1] = b[2] = b[0];
    result[0] = a[0] != b[0] || a[1] != b[1] || a[2] != b[2];
}


SeExprType
SeExprLtNode::prep(SeExprType wanted, SeExprVarEnv & env)
{
    //TODO: double-check order of evaluation - order MAY effect environment evaluation (probably not, though)
    SeExprType firstType, secondType;

    bool error = false;
    _type = SeExprType::FP1Type();

    firstType = child(0)->prep(SeExprType::NumericType(), env);

    if(!firstType.isValid())
        error = true;
    else if(!firstType.isa(SeExprType::NumericType())) {
        error = true;
        addError("Expected Numeric type from first operand to < operator but found" + firstType.toString());
    }

    secondType = child(1)->prep(SeExprType::NumericType(), env);

    if(!secondType.isValid())
        error = true;
    else if(!secondType.isa(SeExprType::NumericType())) {
        error = true;
        addError("Expected Numeric type from second operand to < operator but found" + secondType.toString());
    }

    if(firstType.isValid()  &&
       secondType.isValid() &&
       !firstType.compatibleNum(secondType)) {
        error = true;
        addError("Types " + firstType.toString() + " and " + secondType.toString() + " are not compatible types for < operator");
    }

    if(error)
        _type = SeExprType::ErrorType();

    return _type;
}


void
SeExprLtNode::eval(SeVec3d& result) const
{
    SeVec3d a, b;
    const SeExprNode* child0 = child(0);
    const SeExprNode* child1 = child(1);
    child0->eval(a);
    child1->eval(b);

    result[0] = a[0] < b[0];
}


SeExprType
SeExprGtNode::prep(SeExprType wanted, SeExprVarEnv & env)
{
    //TODO: double-check order of evaluation - order MAY effect environment evaluation (probably not, though)
    SeExprType firstType, secondType;

    bool error = false;
    _type = SeExprType::FP1Type();

    firstType = child(0)->prep(SeExprType::NumericType(), env);

    if(!firstType.isValid())
        error = true;
    else if(!firstType.isa(SeExprType::NumericType())) {
        error = true;
        addError("Expected Numeric type from first operand to > operator but found" + firstType.toString());
    }

    secondType = child(1)->prep(SeExprType::NumericType(), env);

    if(!secondType.isValid())
        error = true;
    else if(!secondType.isa(SeExprType::NumericType())) {
        error = true;
        addError("Expected Numeric type from second operand to > operator but found" + secondType.toString());
    }

    if(firstType.isValid()  &&
       secondType.isValid() &&
       !firstType.compatibleNum(secondType)) {
        error = true;
        addError("Types " + firstType.toString() + " and " + secondType.toString() + " are not compatible types for > operator");
    }

    if(error)
        _type = SeExprType::ErrorType();

    return _type;
}


void
SeExprGtNode::eval(SeVec3d& result) const
{
    SeVec3d a, b;
    const SeExprNode* child0 = child(0);
    const SeExprNode* child1 = child(1);
    child0->eval(a);
    child1->eval(b);

    result[0] = a[0] > b[0];
}


SeExprType
SeExprLeNode::prep(SeExprType wanted, SeExprVarEnv & env)
{
    //TODO: double-check order of evaluation - order MAY effect environment evaluation (probably not, though)
    SeExprType firstType, secondType;

    bool error = false;
    _type = SeExprType::FP1Type();

    firstType = child(0)->prep(SeExprType::NumericType(), env);

    if(!firstType.isValid())
        error = true;
    else if(!firstType.isa(SeExprType::NumericType())) {
        error = true;
        addError("Expected Numeric type from first operand to <= operator but found" + firstType.toString());
    }

    secondType = child(1)->prep(SeExprType::NumericType(), env);

    if(!secondType.isValid())
        error = true;
    else if(!secondType.isa(SeExprType::NumericType())) {
        error = true;
        addError("Expected Numeric type from second operand to <= operator but found" + secondType.toString());
    }

    if(firstType.isValid()  &&
       secondType.isValid() &&
       !firstType.compatibleNum(secondType)) {
        error = true;
        addError("Types " + firstType.toString() + " and " + secondType.toString() + " are not compatible types for <= operator");
    }

    if(error)
        _type = SeExprType::ErrorType();

    return _type;
}


void
SeExprLeNode::eval(SeVec3d& result) const
{
    SeVec3d a, b;
    const SeExprNode* child0 = child(0);
    const SeExprNode* child1 = child(1);
    child0->eval(a);
    child1->eval(b);

    result[0] = a[0] <= b[0];
}


SeExprType
SeExprGeNode::prep(SeExprType wanted, SeExprVarEnv & env)
{
    //TODO: double-check order of evaluation - order MAY effect environment evaluation (probably not, though)
    SeExprType firstType, secondType;

    bool error = false;
    _type = SeExprType::FP1Type();

    firstType = child(0)->prep(SeExprType::NumericType(), env);

    if(!firstType.isValid())
        error = true;
    else if(!firstType.isa(SeExprType::NumericType())) {
        error = true;
        addError("Expected Numeric type from first operand to >= operator but found" + firstType.toString());
    }

    secondType = child(1)->prep(SeExprType::NumericType(), env);

    if(!secondType.isValid())
        error = true;
    else if(!secondType.isa(SeExprType::NumericType())) {
        error = true;
        addError("Expected Numeric type from second operand to >= operator but found" + secondType.toString());
    }

    if(firstType.isValid()  &&
       secondType.isValid() &&
       !firstType.compatibleNum(secondType)) {
        error = true;
        addError("Types " + firstType.toString() + " and " + secondType.toString() + " are not compatible types for >= operator");
    }

    if(error)
        _type = SeExprType::ErrorType();

    return _type;
}


void
SeExprGeNode::eval(SeVec3d& result) const
{
    SeVec3d a, b;
    const SeExprNode* child0 = child(0);
    const SeExprNode* child1 = child(1);
    child0->eval(a);
    child1->eval(b);

    result[0] = a[0] >= b[0];
}


SeExprType
SeExprAddNode::prep(SeExprType wanted, SeExprVarEnv & env)
{
    //TODO: double-check order of evaluation - order MAY effect environment evaluation (probably not, though)
    SeExprType firstType, secondType;

    bool error = false;
    _type = SeExprType::ErrorType();

    firstType = child(0)->prep(SeExprType::NumericType(), env);

    if(!firstType.isValid())
        error = true;
    else if(!firstType.isa(SeExprType::NumericType())) {
        error = true;
        addError("Expected Numeric type from first operand to + operator but found" + firstType.toString());
    }

    secondType = child(1)->prep(SeExprType::NumericType(), env);

    if(!secondType.isValid())
        error = true;
    else if(!secondType.isa(SeExprType::NumericType())) {
        error = true;
        addError("Expected Numeric type from second operand to + operator but found" + secondType.toString());
    }

    if(firstType.isValid()  &&
       secondType.isValid() &&
       !firstType.compatibleNum(secondType)) {
        error = true;
        addError("Types " + firstType.toString() + " and " + secondType.toString() + " are not compatible types for + operator");
    }

    if(!error)
        _type = (firstType.isFP1() ? secondType : firstType);

    return _type;
}


void
SeExprAddNode::eval(SeVec3d& result) const
{
    const SeExprNode* child0 = child(0);
    const SeExprNode* child1 = child(1);
    SeVec3d a, b;
    child0->eval(a);
    child1->eval(b);

    if (!_isVec) {
	result[0] = a[0] + b[0];
    }
    else {
	// at least one child is a vector and the result is too
	if (!child0->isVec()) a[1] = a[2] = a[0];
	if (!child1->isVec()) b[1] = b[2] = b[0];
	result = a + b;
    }
}


SeExprType
SeExprSubNode::prep(SeExprType wanted, SeExprVarEnv & env)
{
    //TODO: double-check order of evaluation - order MAY effect environment evaluation (probably not, though)
    SeExprType firstType, secondType;

    bool error = false;
    _type = SeExprType::ErrorType();

    firstType = child(0)->prep(SeExprType::NumericType(), env);

    if(!firstType.isValid())
        error = true;
    else if(!firstType.isa(SeExprType::NumericType())) {
        error = true;
        addError("Expected Numeric type from first operand to - operator but found" + firstType.toString());
    }

    secondType = child(1)->prep(SeExprType::NumericType(), env);

    if(!secondType.isValid())
        error = true;
    else if(!secondType.isa(SeExprType::NumericType())) {
        error = true;
        addError("Expected Numeric type from second operand to - operator but found" + secondType.toString());
    }

    if(firstType.isValid()  &&
       secondType.isValid() &&
       !firstType.compatibleNum(secondType)) {
        error = true;
        addError("Types " + firstType.toString() + " and " + secondType.toString() + " are not compatible types for - operator");
    }

    if(!error)
        _type = (firstType.isFP1() ? secondType : firstType);

    return _type;
}


void
SeExprSubNode::eval(SeVec3d& result) const
{
    const SeExprNode* child0 = child(0);
    const SeExprNode* child1 = child(1);
    SeVec3d a, b;
    child0->eval(a);
    child1->eval(b);

    if (!_isVec) {
	result[0] = a[0] - b[0];
    }
    else {
	// at least one child is a vector and the result is too
	if (!child0->isVec()) a[1] = a[2] = a[0];
	if (!child1->isVec()) b[1] = b[2] = b[0];
	result = a - b;
    }
}


SeExprType
SeExprMulNode::prep(SeExprType wanted, SeExprVarEnv & env)
{
    //TODO: double-check order of evaluation - order MAY effect environment evaluation (probably not, though)
    SeExprType firstType, secondType;

    bool error = false;
    _type = SeExprType::ErrorType();

    firstType = child(0)->prep(SeExprType::NumericType(), env);

    if(!firstType.isValid())
        error = true;
    else if(!firstType.isa(SeExprType::NumericType())) {
        error = true;
        addError("Expected Numeric type from first operand to * operator but found" + firstType.toString());
    }

    secondType = child(1)->prep(SeExprType::NumericType(), env);

    if(!secondType.isValid())
        error = true;
    else if(!secondType.isa(SeExprType::NumericType())) {
        error = true;
        addError("Expected Numeric type from second operand to * operator but found" + secondType.toString());
    }

    if(firstType.isValid()  &&
       secondType.isValid() &&
       !firstType.compatibleNum(secondType)) {
        error = true;
        addError("Types " + firstType.toString() + " and " + secondType.toString() + " are not compatible types for * operator");
    }

    if(!error)
        _type = (firstType.isFP1() ? secondType : firstType);

    return _type;
}


void
SeExprMulNode::eval(SeVec3d& result) const
{
    const SeExprNode* child0 = child(0);
    const SeExprNode* child1 = child(1);
    SeVec3d a, b;
    child0->eval(a);
    child1->eval(b);

    if (!_isVec) {
	result[0] = a[0] * b[0];
    }
    else {
	// at least one child is a vector and the result is too
	if (!child0->isVec()) a[1] = a[2] = a[0];
	if (!child1->isVec()) b[1] = b[2] = b[0];
	result = a * b;
    }
}


SeExprType
SeExprDivNode::prep(SeExprType wanted, SeExprVarEnv & env)
{
    //TODO: double-check order of evaluation - order MAY effect environment evaluation (probably not, though)
    SeExprType firstType, secondType;

    bool error = false;
    _type = SeExprType::ErrorType();

    firstType = child(0)->prep(SeExprType::NumericType(), env);

    if(!firstType.isValid())
        error = true;
    else if(!firstType.isa(SeExprType::NumericType())) {
        error = true;
        addError("Expected Numeric type from first operand to / operator but found" + firstType.toString());
    }

    secondType = child(1)->prep(SeExprType::NumericType(), env);

    if(!secondType.isValid())
        error = true;
    else if(!secondType.isa(SeExprType::NumericType())) {
        error = true;
        addError("Expected Numeric type from second operand to / operator but found" + secondType.toString());
    }

    if(firstType.isValid()  &&
       secondType.isValid() &&
       !firstType.compatibleNum(secondType)) {
        error = true;
        addError("Types " + firstType.toString() + " and " + secondType.toString() + " are not compatible types for / operator");
    }

    if(!error)
        _type = (firstType.isFP1() ? secondType : firstType);

    return _type;
}


void
SeExprDivNode::eval(SeVec3d& result) const
{
    const SeExprNode* child0 = child(0);
    const SeExprNode* child1 = child(1);
    SeVec3d a, b;
    child0->eval(a);
    child1->eval(b);

    if (!_isVec) {
	result[0] = a[0] / b[0];
    }
    else {
	// at least one child is a vector and the result is too
	if (!child0->isVec()) a[1] = a[2] = a[0];
	if (!child1->isVec()) b[1] = b[2] = b[0];
	result = a / b;
    }
}


static double niceMod(double a, double b)
{
    if (b == 0) return 0;
    return a - floor(a/b)*b;
}


SeExprType
SeExprModNode::prep(SeExprType wanted, SeExprVarEnv & env)
{
    //TODO: double-check order of evaluation - order MAY effect environment evaluation (probably not, though)
    SeExprType firstType, secondType;

    bool error = false;
    _type = SeExprType::ErrorType();

    firstType = child(0)->prep(SeExprType::NumericType(), env);

    if(!firstType.isValid())
        error = true;
    else if(!firstType.isa(SeExprType::NumericType())) {
        error = true;
        addError("Expected Numeric type from first operand to % operator but found" + firstType.toString());
    }

    secondType = child(1)->prep(SeExprType::NumericType(), env);

    if(!secondType.isValid())
        error = true;
    else if(!secondType.isa(SeExprType::NumericType())) {
        error = true;
        addError("Expected Numeric type from second operand to % operator but found" + secondType.toString());
    }

    if(firstType.isValid()  &&
       secondType.isValid() &&
       !firstType.compatibleNum(secondType)) {
        error = true;
        addError("Types " + firstType.toString() + " and " + secondType.toString() + " are not compatible types for % operator");
    }

    if(!error)
        _type = (firstType.isFP1() ? secondType : firstType);

    return _type;
}


void
SeExprModNode::eval(SeVec3d& result) const
{
    const SeExprNode* child0 = child(0);
    const SeExprNode* child1 = child(1);
    SeVec3d a, b;
    child0->eval(a);
    child1->eval(b);

    if (!_isVec) {
	result[0] = niceMod(a[0], b[0]);
    }
    else {
	// at least one child is a vector and the result is too
	if (!child0->isVec()) a[1] = a[2] = a[0];
	if (!child1->isVec()) b[1] = b[2] = b[0];
	result[0] = niceMod(a[0], b[0]);
	result[1] = niceMod(a[1], b[1]);
	result[2] = niceMod(a[2], b[2]);
    }
}


SeExprType
SeExprExpNode::prep(SeExprType wanted, SeExprVarEnv & env)
{
    //TODO: double-check order of evaluation - order MAY effect environment evaluation (probably not, though)
    SeExprType firstType, secondType;

    bool error = false;
    _type = SeExprType::ErrorType();

    firstType = child(0)->prep(SeExprType::NumericType(), env);

    if(!firstType.isValid())
        error = true;
    else if(!firstType.isa(SeExprType::NumericType())) {
        error = true;
        addError("Expected Numeric type from first operand to ^ operator but found" + firstType.toString());
    }

    secondType = child(1)->prep(SeExprType::NumericType(), env);

    if(!secondType.isValid())
        error = true;
    else if(!secondType.isa(SeExprType::NumericType())) {
        error = true;
        addError("Expected Numeric type from second operand to ^ operator but found" + secondType.toString());
    }

    if(firstType.isValid()  &&
       secondType.isValid() &&
       !firstType.compatibleNum(secondType)) {
        error = true;
        addError("Types " + firstType.toString() + " and " + secondType.toString() + " are not compatible types for ^ operator");
    }

    if(!error)
        _type = (firstType.isFP1() ? secondType : firstType);

    return _type;
}


void
SeExprExpNode::eval(SeVec3d& result) const
{
    const SeExprNode* child0 = child(0);
    const SeExprNode* child1 = child(1);
    SeVec3d a, b;
    child0->eval(a);
    child1->eval(b);

    if (!_isVec) {
	result[0] = pow(a[0], b[0]);
    }
    else {
	// at least one child is a vector and the result is too
	if (!child0->isVec()) a[1] = a[2] = a[0];
	if (!child1->isVec()) b[1] = b[2] = b[0];
	result[0] = pow(a[0], b[0]);
	result[1] = pow(a[1], b[1]);
	result[2] = pow(a[2], b[2]);
    }
}


SeExprType
SeExprVarNode::prep(SeExprType wanted, SeExprVarEnv & env)
{
    _type = SeExprType::ErrorType();

    // ask expression to resolve var
    _var = env.find(name());
    if (!_var) _var = _expr->resolveVar(name());
    if (!_var)
        addError(std::string("No variable named $")+name());
    else
        _type = _var->type();

    return _type;
}


void
SeExprVarNode::eval(SeVec3d& result) const
{
    if (_var) _var->eval(this, result);
    else result = 0.0;
}


SeExprType
SeExprNumNode::prep(SeExprType wanted, SeExprVarEnv & env)
{
    _type = SeExprType::FP1Type();

    return _type;
}


SeExprType
SeExprStrNode::prep(SeExprType wanted, SeExprVarEnv & env)
{
    _type = SeExprType::StringType();

    return _type;
}

bool
SeExprFuncNode::prepArgs(std::string const & name, SeExprType wanted, SeExprVarEnv & env)
{
    bool error = false;
    int count;

    std::vector<SeExprNode*>::iterator       ic = _children.begin();
    std::vector<SeExprNode*>::iterator const ec = _children.end  ();

    for(count = 0; ic != ec; ++ic, ++count) {
        SeExprType childType = (*ic)->prep(wanted, env);
        if(!childType.isValid())
            error = true;
        else if(!childType.isa(wanted)) {
            error = true;
            std::stringstream countstream;
            countstream << count;
            addError("Expected " + wanted.toString() + " type from " + countstream.str() + " operand to " + name + " function but found" + childType.toString());
        }
    }

    return !error; //return convention is true if no errors
}

SeExprType
SeExprFuncNode::prep(SeExprType wanted, SeExprVarEnv & env)
{
    bool error = false;
    //int  _dim  = 0;

    _type = SeExprType::ErrorType();

    // ask expression to resolve func
    _func = _expr->resolveFunc(_name);
    // otherwise, look in global func table
    if (!_func) _func = SeExprFunc::lookup(_name);

    if(!_func) {
        //error = true; //not needed: _type is already Error and won't change if control gets here
        addError("Function " + _name + " has no definition");
        SeExprNode::prep(SeExprType::AnyType(), env);
    } else {
        _type = _func->retType();
        _nargs = numChildren();

        if(_nargs < _func->minArgs()) {
            error = true;
            addError("Too few args for function "+_name);
            SeExprNode::prep(SeExprType::AnyType(), env);
        } else if(_nargs > _func->maxArgs() && _func->maxArgs() >= 0) {
            error = true;
            addError("Too many args for function "+_name);
            SeExprNode::prep(SeExprType::AnyType(), env);
        }
        else {
            _vecArgs.resize(_nargs);
            _scalarArgs.resize(_nargs);

            if(_func->type() == SeExprFunc::FUNCX) {
                //FuncX function:
                if(!_func->funcx()->isThreadSafe()) _expr->setThreadUnsafe(_name);
                if(!(_func->funcx()->prep(this, wanted, env)).isValid())
                    error = true;
            }
            else {
                //standard function:
                //TODO: give actual name, not placeholder - or take out name altogether
                error = !(prepArgs("placeholder",//_func->name(),
                                   (_func->isScalar() ? SeExprType::FP1Type() : SeExprType::FPNType(3)),
                                   env));
                //TODO:
                // check if this call is lifted/promoted
                // combine both checks?
            }
        }
    }

    if(error)
        _type = SeExprType::ErrorType();

    return _type;
}

SeVec3d*
SeExprFuncNode::evalArgs() const
{
    SeVec3d* a = vecArgs();
    for (int i = 0; i < _nargs; i++) {
	const SeExprNode* child = SeExprNode::child(i);
	SeVec3d& A = a[i];
	child->eval(A);
	if (!child->isVec()) A[1] = A[2] = A[0];
    }
    return a;
}

SeVec3d
SeExprFuncNode::evalArg(int n) const
{
    SeVec3d arg;
    const SeExprNode* child = SeExprNode::child(n);
    child->eval(arg);
    if (!child->isVec()) arg[1] = arg[2] = arg[0];
    return arg;
}

bool
SeExprFuncNode::isStrArg(int n) const
{
    return n < _nargs && dynamic_cast<const SeExprStrNode*>(child(n)) != 0;
}

std::string
SeExprFuncNode::getStrArg(int n) const
{
    if (n < _nargs)
	return static_cast<const SeExprStrNode*>(child(n))->str();
    return "";
}


void
SeExprFuncNode::eval(SeVec3d& result) const
{
    if (!_func) { result = 0.0; return; }

    // funcx is a catchall that does all its own processing
    if (_func->type() == SeExprFunc::FUNCX) {
	_func->funcx()->eval(this, result);
	return;
    }

    // handle the case of a scalar func applied to a vector
    bool applyScalarToVec = _isVec && _func->isScalar();
    //bool applyScalarToVec = _isVec && !_func->isVec();
    int niter = applyScalarToVec ? 3 : 1;

    // eval args and call the function
    SeVec3d* a = evalArgs();
    for (int i = 0; i < niter; i++) {
	switch (_func->type()) {
	default: 
	    result[i] = result[1] = result[2] = 0;
	    break;
	case SeExprFunc::FUNC0:
	    result[i] = _func->func0()(); 
	    break;
	case SeExprFunc::FUNC1:
	    result[i] = _func->func1()(a[0][i]);
	    break;
	case SeExprFunc::FUNC2:
	    result[i] = _func->func2()(a[0][i], a[1][i]);
	    break;
	case SeExprFunc::FUNC3:
	    result[i] = _func->func3()(a[0][i], a[1][i], a[2][i]);
	    break;
	case SeExprFunc::FUNC4:
	    result[i] = _func->func4()(a[0][i], a[1][i], a[2][i], a[3][i]);
	    break;
	case SeExprFunc::FUNC5:
	    result[i] = _func->func5()(a[0][i], a[1][i], a[2][i], a[3][i],
				       a[4][i]);
	    break;
	case SeExprFunc::FUNC6:
	    result[i] = _func->func6()(a[0][i], a[1][i], a[2][i], a[3][i],
				       a[4][i], a[5][i]);
	    break;
	case SeExprFunc::FUNCN: 
	    {
		double* d = scalarArgs();
		for (int n = 0; n < _nargs; n++) d[n] = a[n][i];
		result[i] = _func->funcn()(_nargs, d);
		break;
	    }
	case SeExprFunc::FUNC1V:
	    result[i] = _func->func1v()(a[0]);
	    break;
	case SeExprFunc::FUNC2V:
	    result[i] = _func->func2v()(a[0], a[1]);
	    break;
	case SeExprFunc::FUNCNV: 
	    result[i] = _func->funcnv()(_nargs, a);
	    break;
	case SeExprFunc::FUNC1VV:
	    result = _func->func1vv()(a[0]);
	    break;
	case SeExprFunc::FUNC2VV:
	    result = _func->func2vv()(a[0], a[1]);
	    break;
	case SeExprFunc::FUNCNVV: 
	    result = _func->funcnvv()(_nargs, a);
	    break;
	}
    }

}