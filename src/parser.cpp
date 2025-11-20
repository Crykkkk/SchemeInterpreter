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
#include <utility>
#include <vector>

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
    SymbolSyntax *id = dynamic_cast<SymbolSyntax*>(stxs[0].get());
    if (id == nullptr) { // 基类指针无法转派生类，代表他不是symbol
        //TODO: TO COMPLETE THE LOGIC
        vector<Expr> parameters; 
        for (int i = 1; i < stxs.size(); i++) {
            parameters.push_back(stxs[i]->parse(env));
        }
        return (new Apply(stxs[0]->parse(env), parameters));
    }
    else{
    string op = id->s; // a string，应该是对应的syntax文本
    if (find(op, env).get() != nullptr) {
        //TODO: TO COMPLETE THE PARAMETER PARSER LOGIC
        vector<Expr> parameters; 
        for (int i = 1; i < stxs.size(); i++) {
            parameters.push_back(stxs[i]->parse(env));
        }
        return (new Apply(stxs[0]->parse(env), parameters));
    }
    if (primitives.count(op) != 0) { // map 的 count 方法，是否存在特定键的元素
        vector<Expr> parameters; 
        for (int i = 1; i < stxs.size(); i++) {
            parameters.push_back(stxs[i]->parse(env));
        }
        // return (new Apply(stxs[0]->parse(env), parameters));
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
        } else if (op_type == E_NOT) {
            if (parameters.size() == 1) {
                return Expr(new Not(parameters[0]));
            }
            throw RuntimeError("Wrong arg number for not");
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
        } else if (op_type == E_LISTQ) {
            if (parameters.size() == 1) {
                return Expr(new IsList(parameters[0]));
            }
            throw RuntimeError("Wrong arg number for list?");  
        } else if (op_type == E_EQQ) {
            if (parameters.size() == 2) {
                return Expr(new IsEq(parameters[0], parameters[1]));
            }
            throw RuntimeError("Wrong arg number for eq?");  
        } else if (op_type == E_BOOLQ) {
            if (parameters.size() == 1) {
                return Expr(new IsBoolean(parameters[0]));
            }
            throw RuntimeError("Wrong arg number for boolean?");  
        } else if (op_type == E_INTQ) {
            if (parameters.size() == 1) {
                return Expr(new IsFixnum(parameters[0]));
            }
            throw RuntimeError("Wrong arg number for number?");  
        } else if (op_type == E_NULLQ) {
            if (parameters.size() == 1) {
                return Expr(new IsNull(parameters[0]));
            }
            throw RuntimeError("Wrong arg number for null?");  
        } else if (op_type == E_PAIRQ) {
            if (parameters.size() == 1) {
                return Expr(new IsPair(parameters[0]));
            }
            throw RuntimeError("Wrong arg number for pair?");  
        } else if (op_type == E_PROCQ) {
            if (parameters.size() == 1) {
                return Expr(new IsProcedure(parameters[0]));
            }
            throw RuntimeError("Wrong arg number for procedure?");  
        } else if (op_type == E_SYMBOLQ) {
            if (parameters.size() == 1) {
                return Expr(new IsSymbol(parameters[0]));
            }
            throw RuntimeError("Wrong arg number for symbol?");  
        } else if (op_type == E_STRINGQ) {
            if (parameters.size() == 1) {
                return Expr(new IsString(parameters[0]));
            }
            throw RuntimeError("Wrong arg number for string?");  
        } else if (op_type == E_VOID) {
            if (parameters.size() == 0) {
                return Expr(new MakeVoid());
            }
            throw RuntimeError("Wrong arg number for void");
        } else if (op_type == E_DISPLAY) {
            if (parameters.size() == 1) {
                return Expr(new Display(parameters[0]));
            }
            throw RuntimeError("Wrong arg number for display");
        } else if (op_type == E_EXIT) {
            if (parameters.size() == 0) {
                return Expr(new Exit());
            }
            throw RuntimeError("Wrong arg number for Exit");
        } else if (op_type == E_SETCAR) {
            if (stxs.size() != 3) throw RuntimeError("Wrong arg num for setcar!");
            return (new SetCar(stxs[1]->parse(env), stxs[2]->parse(env)));
        } else if (op_type == E_SETCDR) {
            if (stxs.size() != 3) throw RuntimeError("Wrong arg num for setcdr!");
            return (new SetCdr(stxs[1]->parse(env), stxs[2]->parse(env)));
        }
        else {
            //TODO: TO COMPLETE THE LOGIC
            throw RuntimeError("What else could it be?");
        }
    }
    if (reserved_words.count(op) != 0) {
    	switch (reserved_words[op]) {
			//TODO: TO COMPLETE THE reserve_words PARSER LOGIC 注意这里的 stxs 是 1-base
            case E_QUOTE:
                if (stxs.size() != 2) throw RuntimeError("Invalid quote format");
                return Expr(new Quote(stxs[1]));
                break;
            case E_IF:
                if (stxs.size() != 4) throw RuntimeError("Invalid if format");
                return Expr(new If(stxs[1]->parse(env), stxs[2]->parse(env), stxs[3]->parse(env)));
                break;
            case E_COND:{
                if (stxs.size() == 1) throw RuntimeError("Wrong arg num for Cond"); // 第一个是 cond 本身
                std::vector<std::vector<Expr>> Args;
                for (int i = 1; i < stxs.size(); i++) {
                    List* argi = dynamic_cast<List*>(stxs[i].get());
                    if (argi == nullptr) throw RuntimeError("Wrong arg format for Cond");
                    vector<Expr> argi_v;
                    SymbolSyntax* maybe_else = dynamic_cast<SymbolSyntax*>(argi->stxs[0].get());
                    if (maybe_else != nullptr && maybe_else->s == "else" && find("else", env).get() == nullptr) {
                        if (i != stxs.size() - 1) throw RuntimeError("else must be last clause");
                        if (argi->stxs.size() == 1) throw RuntimeError("else must has a else clause");
                        else argi_v.push_back(TrueSyntax().parse(env)); // 如果真的是else就直接放个true
                    }
                    else argi_v.push_back(argi->stxs[0]->parse(env));
                    for (int j = 1; j < argi->stxs.size(); j++) {
                        argi_v.push_back(argi->stxs[j]->parse(env));
                    }
                    Args.push_back(argi_v);
                }
                return Expr(new Cond(Args));
                break;}
            case E_BEGIN:{
                std::vector<Expr> begin_arg;
                for (int i = 1; i < stxs.size(); i++) {
                    begin_arg.push_back(stxs[i]->parse(env)); // 有时候似乎必须用->，而不能随便混过来
                }
                return Expr(new Begin(begin_arg));
                break;
            }
            case E_LAMBDA:{
                if (stxs.size() < 3) throw RuntimeError("Invalid arg num of lambda");
                List* paras = dynamic_cast<List*>(stxs[1].get());
                if (paras == nullptr) throw RuntimeError("Invalid arg format for lambda");
                vector<string> real_paras;
                Assoc parse_env = env; 
                for (int i = 0; i < paras->stxs.size(); i++) {
                    SymbolSyntax* this_para = dynamic_cast<SymbolSyntax*>(paras->stxs[i].get());
                    if (this_para == nullptr) throw RuntimeError("Invalid parameter format for lambda");
                    real_paras.push_back(this_para->s);
                    parse_env = extend(this_para->s, VoidV(), parse_env);
                }
                std::vector<Expr> ld_e;
                for (int i = 2; i < stxs.size(); i++) {
                    ld_e.push_back(stxs[i]->parse(parse_env));
                }
                return (new Lambda(real_paras, new Begin(ld_e)));
                break;
            }
            case E_DEFINE:{
                if (stxs.size() < 3) throw RuntimeError("Invalid arg num for Define");
                if (SymbolSyntax* def_var = dynamic_cast<SymbolSyntax*>(stxs[1].get())) {
                    if (primitives.count(def_var->s) || reserved_words.count(def_var->s)) throw RuntimeError("the var's name shouldn't be a reserved name");
                    std::vector<Expr> ld_e;
                    for (int i = 2; i < stxs.size(); i++) {
                        ld_e.push_back(stxs[i]->parse(env));
                    }
                    return (new Define(def_var->s, new Begin(ld_e)));
                }

                if (List* def_var_lst = dynamic_cast<List*>(stxs[1].get())) {
                    if (def_var_lst->stxs.size() == 0) throw RuntimeError("no argument in define's function");
                    SymbolSyntax* def_var = dynamic_cast<SymbolSyntax*>(def_var_lst->stxs[0].get());
                    if (def_var == nullptr) {
                        throw RuntimeError("invalid var type for define");
                    }
                    if (primitives.count(def_var->s) || reserved_words.count(def_var->s)) throw RuntimeError("the var's name shouldn't be a reserved name");
                    
                    vector<string> lambda_paras;
                    Assoc parse_env = env;
                    for (int i = 1; i < def_var_lst->stxs.size(); i++) { // 注意从第二个参数开始进lambda
                        SymbolSyntax* this_para = dynamic_cast<SymbolSyntax*>(def_var_lst->stxs[i].get());
                        if (this_para == nullptr) throw RuntimeError("Invalid parameter format for lambda");
                        lambda_paras.push_back(this_para->s); 
                        parse_env = extend(this_para->s, VoidV(), parse_env);                       
                    }
                    std::vector<Expr> lambda_expr;
                    for (int i = 2; i < stxs.size(); i++) {
                        lambda_expr.push_back(stxs[i]->parse(parse_env));
                    }
                    return (new Define(def_var->s, new Lambda(lambda_paras,new Begin(lambda_expr))));
                }
                throw RuntimeError("invalid var type for define");
                break;
            }
            case E_LET: {
                if (stxs.size() <= 2) throw RuntimeError("invalid var num for LET");
                List* param_lst = dynamic_cast<List*>(stxs[1].get());
                if (!param_lst) throw RuntimeError("invalid param format for let 1");
                
                std::vector<std::pair<std::string, Expr>> bind;
                vector<string> shadowed_names;
                for (auto param : param_lst->stxs){
                    List* unpack_param = dynamic_cast<List*>(param.get());
                    if (!unpack_param) throw RuntimeError("invalid param format for let 2");
                    if (unpack_param->stxs.size() != 2) throw RuntimeError("invalid param format for let 3");
                    SymbolSyntax* this_formal = dynamic_cast<SymbolSyntax*>(unpack_param->stxs[0].get());
                    if (this_formal == nullptr) throw RuntimeError("invalid param format for let 4");
                    bind.push_back({this_formal->s, unpack_param->stxs[1]->parse(env)});
                    shadowed_names.push_back(this_formal->s);
                }
                
                Assoc parse_env = env; 
                for (const auto& name : shadowed_names) {
                    parse_env = extend(name, VoidV(), parse_env);
                }
                std::vector<Expr> ld_e;
                for (int i = 2; i < stxs.size(); i++) {
                    ld_e.push_back(stxs[i]->parse(parse_env));
                }
                return (new Let(bind, new Begin(ld_e)));
                break;
            }
            case E_LETREC: {
                if (stxs.size() <= 2) throw RuntimeError("invalid var num for letr");
                List* param_lst = dynamic_cast<List*>(stxs[1].get());
                if (!param_lst) throw RuntimeError("invalid param format for letr");
                vector<string> names;
                vector<Syntax> raw_exprs; 
                for (auto param : param_lst->stxs){
                    List* unpack_param = dynamic_cast<List*>(param.get());
                    if (!unpack_param || unpack_param->stxs.size() != 2) 
                        throw RuntimeError("invalid param format for letr");
                    SymbolSyntax* this_formal = dynamic_cast<SymbolSyntax*>(unpack_param->stxs[0].get());
                    if (!this_formal) throw RuntimeError("invalid param format for letr");
                    names.push_back(this_formal->s);
                    raw_exprs.push_back(unpack_param->stxs[1]); 
                }

                Assoc parse_env = env; 
                for (const auto& name : names) {
                    parse_env = extend(name, VoidV(), parse_env);
                }

                std::vector<std::pair<std::string, Expr>> bind;
                for (int i = 0; i < names.size(); i++) {
                    bind.push_back({names[i], raw_exprs[i]->parse(parse_env)});
                }
                std::vector<Expr> ld_e;
                for (int i = 2; i < stxs.size(); i++) {
                    ld_e.push_back(stxs[i]->parse(parse_env));
                }

                return (new Letrec(bind, new Begin(ld_e)));
                break;
            }
            case E_SET: {
                if (stxs.size() != 3) throw RuntimeError("Wrong arg num for set!");
                if (SymbolSyntax* target = dynamic_cast<SymbolSyntax*>(stxs[1].get())) { //不应该提前做检查
                    return (new Set(target->s, stxs[2]->parse(env)));
                }
                throw RuntimeError("target needs to be a symbol");
                break;
            }
        	default:
            	throw RuntimeError("Unknown reserved word: " + op);
    	}
    }
    vector<Expr> parameters; 
    for (int i = 1; i < stxs.size(); i++) {
        parameters.push_back(stxs[i]->parse(env));
    }
    return Expr(new Apply(stxs[0]->parse(env), parameters));
}
}
