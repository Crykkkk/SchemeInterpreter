/**
 * @file parser.cpp
 * @brief Parsing implementation for Scheme syntax tree to expression tree conversion
 * 
 * This file implements the parsing logic that converts syntax trees into
 * expression trees that can be evaluated.
 * primitive operations, and function applications.
 */

#include "RE.hpp"
#include "Def.hpp"
#include "syntax.hpp"
#include "value.hpp"
#include "expr.hpp"
#include <map>
#include <string>
#include <iostream>

#define mp make_pair
using std::string;
using std::vector;
using std::pair;

extern std::map<std::string, ExprType> primitives;
extern std::map<std::string, ExprType> reserved_words;

/**
 * @brief Default parse method (should be overridden by subclasses)
 */
Expr Syntax::parse(Assoc &env) { // 就是如果派生类没有写具体的parse方法会导致这个问题，所以报错
    throw RuntimeError("Unimplemented parse method");
}

Expr Number::parse(Assoc &env) { // n从何来，这个函数是 Number 类的一个成员函数，所以n从Number来，他们都是syntax下的子类，return的则是Expr
    return Expr(new Fixnum(n));
}

Expr RationalSyntax::parse(Assoc &env) {
    //TODO: complete the rational parser DONE
    if (denominator == 0) throw RuntimeError("Invalid denominator");
    else return Expr(new RationalNum(numerator, denominator));
}

Expr SymbolSyntax::parse(Assoc &env) {
    return Expr(new Var(s));
}

Expr StringSyntax::parse(Assoc &env) {
    return Expr(new StringExpr(s));
}

Expr TrueSyntax::parse(Assoc &env) {
    return Expr(new True());
}

Expr FalseSyntax::parse(Assoc &env) {
    return Expr(new False());
}

Expr List::parse(Assoc &env) {
    if (stxs.empty()) {
        return Expr(new Quote(Syntax(new List())));
    }

    //TODO: check if the first element is a symbol
    //If not, use Apply function to package to a closure;
    //If so, find whether it's a variable or a keyword;
    SymbolSyntax *id = dynamic_cast<SymbolSyntax*>(stxs[0].get());
    if (id == nullptr) { // 基类指针无法转派生类，代表他不是symbol
        //TODO: TO COMPLETE THE LOGIC
    }else{
    string op = id->s; // a string，应该是对应的syntax文本
    if (find(op, env).get() != nullptr) {
        //TODO: TO COMPLETE THE PARAMETER PARSER LOGIC
    }
    if (primitives.count(op) != 0) { // map 的 count 方法，是否存在特定键的元素
        vector<Expr> parameters; 
        //DONE: TO COMPLETE THE PARAMETER PARSER LOGICE
        for (int i = 1; i < stxs.size(); i++) {
            parameters.push_back(stxs[i]->parse(env));
        }
        ExprType op_type = primitives[op];
        
        if (op_type == E_PLUS) { 
            // Done：完成四则运算，比较与任意参数Ver
            if (parameters.size() == 2) {
                return Expr(new Plus(parameters[0], parameters[1])); 
            } else {
                return Expr(new PlusVar(parameters));
            }
        } else if (op_type == E_MINUS) {
            if (parameters.size() == 2) {
                return Expr(new Minus(parameters[0], parameters[1])); 
            } else {
                return Expr(new MinusVar(parameters));
            }
        } else if (op_type == E_MUL) {
            if (parameters.size() == 2) {
                return Expr(new Mult(parameters[0], parameters[1])); 
            } else {
                return Expr(new MultVar(parameters));
            }
        }  else if (op_type == E_DIV) {
            if (parameters.size() == 2) {
                return Expr(new Div(parameters[0], parameters[1])); 
            } else {
                return Expr(new DivVar(parameters));
            }
        } else if (op_type == E_MODULO) {
            if (parameters.size() != 2) {
                throw RuntimeError("Wrong number of arguments for modulo");
            }
            return Expr(new Modulo(parameters[0], parameters[1]));
        } else if (op_type == E_LIST) {
            return Expr(new ListFunc(parameters));
        } else if (op_type == E_LT) {
            if (parameters.size() == 2) {
                return Expr(new Less(parameters[0], parameters[1])); 
            } else {
                return Expr(new LessVar(parameters));
            }
        } else if (op_type == E_LE) {
            if (parameters.size() == 2) {
                return Expr(new LessEq(parameters[0], parameters[1])); 
            } else {
                return Expr(new LessEqVar(parameters));
            }
        } else if (op_type == E_EQ) {
            if (parameters.size() == 2) {
                return Expr(new Equal(parameters[0], parameters[1])); 
            } else {
                return Expr(new EqualVar(parameters));
            }
        } else if (op_type == E_GE) {
            if (parameters.size() == 2) {
                return Expr(new GreaterEq(parameters[0], parameters[1])); 
            } else {
                return Expr(new GreaterEqVar(parameters));
            }
        } else if (op_type == E_GT) {
             if (parameters.size() == 2) {
                return Expr(new Greater(parameters[0], parameters[1])); 
            } else {
                return Expr(new GreaterVar(parameters));
            }
        } else if (op_type == E_AND) {
            return Expr(new AndVar(parameters));
        } else if (op_type == E_OR) {
            return Expr(new OrVar(parameters));
        } else if (op_type == E_CONS) {
            if (parameters.size() == 2) {
                return Expr(new Cons(parameters[0], parameters[1]));
            }
            else throw RuntimeError("Wrong arg number for cons");
        } else if (op_type == E_CAR) {
            if (parameters.size() == 1) {
                return Expr(new Car(parameters[0]));
            }
            else throw RuntimeError("Wrong arg number for car");
        } else if (op_type == E_CDR) {
            if (parameters.size() == 1) {
                return Expr(new Cdr(parameters[0]));
            }
            else throw RuntimeError("Wrong arg number for cdr");           
        } else if (op_type == E_LISTQ) { // TODO 暂时实现不了，因为需要使用闭包，另外这里看来确实应该把所有?塞进来
            if (parameters.size() == 1) {
                return Expr(new IsList(parameters[0]));
            }
            throw RuntimeError("Wrong arg number for list?");  
        }
        else {
            //TODO: TO COMPLETE THE LOGIC
            throw RuntimeError("What else could it be?");
        }
    }

    if (reserved_words.count(op) != 0) {
    	switch (reserved_words[op]) {
			//TODO: TO COMPLETE THE reserve_words PARSER LOGIC
            case E_QUOTE:
                if (stxs.size() != 2) throw RuntimeError("Invalid quote format");
                return Expr(new Quote(stxs[1]));
                break;
        	default:
            	throw RuntimeError("Unknown reserved word: " + op);
    	}
    }

    //default: use Apply to be an expression
    //TODO: TO COMPLETE THE PARSER LOGIC
}
}
