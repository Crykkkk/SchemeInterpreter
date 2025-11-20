/**
 * @file evaluation.cpp
 * @brief Expression evaluation implementation for the Scheme interpreter
 * @author luke36
 * 
 * This file implements evaluation methods for all expression types in the Scheme
 * interpreter. Functions are organized according to ExprType enumeration order
 * from Def.hpp for consistency and maintainability.
 */

#include "Def.hpp"
#include "value.hpp"
#include "expr.hpp" 
#include "RE.hpp"
#include "syntax.hpp"
#include <algorithm>
#include <cstring>
#include <iostream>
#include <numeric>
#include <ostream>
#include <set>
#include <utility>
#include <vector>
#include <map>
#include <climits>
#include <string>

extern std::map<std::string, ExprType> primitives;
extern std::map<std::string, ExprType> reserved_words;

static int gcd(int a, int b) {
    while (b != 0) {
        int temp = b;
        b = a % b;
        a = temp;
    }
    return a;
}

static std::map<ExprType, std::pair<Expr, std::vector<std::string>>> primitive_map = {
    {E_VOID,     {new MakeVoid(), {}}},
    {E_EXIT,     {new Exit(), {}}},
    {E_BOOLQ,    {new IsBoolean(new Var("parm")), {"parm"}}},
    {E_INTQ,     {new IsFixnum(new Var("parm")), {"parm"}}},
    {E_NULLQ,    {new IsNull(new Var("parm")), {"parm"}}},
    {E_PAIRQ,    {new IsPair(new Var("parm")), {"parm"}}},
    {E_PROCQ,    {new IsProcedure(new Var("parm")), {"parm"}}},
    {E_SYMBOLQ,  {new IsSymbol(new Var("parm")), {"parm"}}},
    {E_STRINGQ,  {new IsString(new Var("parm")), {"parm"}}},
    // 不确定要不要加一个 E_LISTQ
    {E_DISPLAY,  {new Display(new Var("parm")), {"parm"}}},
    {E_PLUS,     {new PlusVar({}),  {"@args"}}}, // 只要 plus 就都是E_plus，但是就需要考虑这是对谁的
    {E_MINUS,    {new MinusVar({}), {"@args"}}},
    {E_MUL,      {new MultVar({}),  {"@args"}}},
    {E_DIV,      {new DivVar({}),   {"@args"}}},
    
    {E_MODULO,   {new Modulo(new Var("parm1"), new Var("parm2")), {"parm1","parm2"}}},
    {E_EXPT,     {new Expt(new Var("parm1"), new Var("parm2")), {"parm1","parm2"}}},
    {E_EQQ,      {new EqualVar({}), {}}},
};

Value Fixnum::eval(Assoc &e) { // evaluation of a fixnum
    return IntegerV(n);
}

Value RationalNum::eval(Assoc &e) { // evaluation of a rational number
    return RationalV(numerator, denominator);
}

Value StringExpr::eval(Assoc &e) { // evaluation of a string
    return StringV(s);
}

Value True::eval(Assoc &e) { // evaluation of #t
    return BooleanV(true);
}

Value False::eval(Assoc &e) { // evaluation of #f
    return BooleanV(false);
}

Value MakeVoid::eval(Assoc &e) { // (void)
    return VoidV();
}

Value Exit::eval(Assoc &e) { // (exit)
    return TerminateV();
}

Value Unary::eval(Assoc &e) { // evaluation of single-operator primitive
    return evalRator(rand->eval(e));
}

Value Binary::eval(Assoc &e) { // evaluation of two-operators primitive
    return evalRator(rand1->eval(e), rand2->eval(e));
}

Value Variadic::eval(Assoc &e) { // evaluation of multi-operator primitive
    std::vector<Value> eval_outcome;
    if (!rands.empty()) {
        for (int i = 0; i < rands.size(); i++) {
            eval_outcome.push_back(rands[i]->eval(e));
        }
        return evalRator(eval_outcome);
    }
    Value arg_list = find("@args", e);
    if (arg_list.get() == nullptr) {
        return evalRator(eval_outcome); 
    }
    // 找到了 @args，说明是 apply 调用，开始解包
    Value current = arg_list;
    while (current->v_type == V_PAIR) {
        Pair* p = dynamic_cast<Pair*>(current.get());
        eval_outcome.push_back(p->car);
        current = p->cdr;
    }
    return evalRator(eval_outcome);
}

// HELPER FUNCTION OF CONVERTING TO NUMBER
std::pair<bool, std::pair<int, int>> parse_rational(const std::string &s) {
    int n = s.size();
    int i = 0;
    int sign = 1;

    // optional sign
    if (s[i] == '+') i++;
    else if (s[i] == '-') sign = -1, i++;

    if (i >= n) return {false, {0,1}};

    // integer part
    long long int_part = 0;
    bool has_int = false;

    while (i < n && isdigit(s[i])) {
        has_int = true;
        int_part = int_part * 10 + (s[i] - '0');
        i++;
    }

    // fractional part
    long long frac_part = 0;
    long long frac_den = 1;
    bool has_frac = false;

    if (i < n && s[i] == '.') {
        i++;
        while (i < n && isdigit(s[i])) {
            has_frac = true;
            frac_part = frac_part * 10 + (s[i] - '0');
            frac_den *= 10;
            i++;
        }
    }

    // scientific notation
    long long exp_val = 0;
    int exp_sign = 1;

    if (i < n && (s[i] == 'e' || s[i] == 'E')) {
        i++;
        if (i < n && (s[i] == '+' || s[i] == '-')) {
            exp_sign = (s[i] == '-') ? -1 : 1;
            i++;
        }
        if (i >= n || !isdigit(s[i])) return {false, {0,1}};
        while (i < n && isdigit(s[i])) {
            exp_val = exp_val * 10 + (s[i] - '0');
            i++;
        }
    }

    // no stray characters
    if (i != n) return {false, {0,1}};

    if (!has_int && !has_frac) return {false, {0,1}};

    // Build rational: (int_part + frac_part/frac_den) * 10^(exp_sign * exp_val)
    long long num = int_part * frac_den + frac_part;
    long long den = frac_den;

    // apply exponent
    long long e = exp_val;

    if (exp_sign == 1) {
        while (e--) {
            num *= 10;
        }
    } else {
        while (e--) {
            den *= 10;
        }
    }

    num *= sign;
    int Gcd = gcd(num, den);
    num /= Gcd;
    den /= Gcd;
    if (den < 0) {
      num = -num;
      den = -den;
    }
    
    return {true, {num, den}};
}

Value Var::eval(Assoc &e) { // evaluation of variable 对多变量的 eval
    if (!x.size()) throw RuntimeError("the var should not be a blank");

    std::set<char> invalid_first = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '.', '@'};
    if (invalid_first.find(x[0]) != invalid_first.end()) throw RuntimeError("the first character of var is invalid");
    for (int i = 0; i < x.size(); i++) {
        if (x[i] == '#' || x[i] == '\'' || x[i] == '\"' || x[i] == '`') throw RuntimeError("the var cannot contain invalid char");
    }
    // try to convert to number
    auto try_convert = parse_rational(x);
    if (try_convert.first) {
        if (try_convert.second.second == 1) return IntegerV(try_convert.second.first);
        return RationalV(try_convert.second.first, try_convert.second.second);
    }
    if (isdigit(x[0])) throw RuntimeError("Cannot convert to a number but start with number char");

    Value matched_value = find(x, e);
    if (matched_value.get() == nullptr) { // 没被定义的情况
        if (primitives.count(x)) {
            auto it = primitive_map.find(primitives[x]);
            if (it != primitive_map.end()) {
                auto proto = it->second;
                Assoc empty_env = empty();
                return ProcedureV(proto.second, proto.first, empty_env);
            }
        }
        // if (reserved_words.count(x)) {

        // }
        throw RuntimeError("The variable is not define in the scope"); // 环境里也没有，也不是保留字
    }
    return matched_value; //已经定义过了的情况
}

Value Plus::evalRator(const Value &rand1, const Value &rand2) { // +
    if (rand1->v_type == V_INT && rand2->v_type == V_INT) {
        return IntegerV((dynamic_cast<Integer*>(rand1.get())->n) + (dynamic_cast<Integer*>(rand2.get())->n));
    }
    if (rand1->v_type == V_INT && rand2->v_type == V_RATIONAL) {
        int Int = dynamic_cast<Integer*>(rand1.get())->n;
        int Ra_d = dynamic_cast<Rational*>(rand2.get())->denominator;
        int Ra_n = dynamic_cast<Rational*>(rand2.get())->numerator;
        return RationalV(Int * Ra_d + Ra_n, Ra_d); // 不可能改变同余性
    }
    if (rand1->v_type == V_RATIONAL && rand2->v_type == V_INT) {
        int Int = dynamic_cast<Integer*>(rand2.get())->n;
        int Ra_d = dynamic_cast<Rational*>(rand1.get())->denominator;
        int Ra_n = dynamic_cast<Rational*>(rand1.get())->numerator;
        return RationalV(Int * Ra_d + Ra_n, Ra_d);
   }
    if (rand1->v_type == V_RATIONAL && rand2->v_type == V_RATIONAL) {
        int Ra1_d = dynamic_cast<Rational*>(rand1.get())->denominator;
        int Ra1_n = dynamic_cast<Rational*>(rand1.get())->numerator;
        int Ra2_d = dynamic_cast<Rational*>(rand2.get())->denominator;
        int Ra2_n = dynamic_cast<Rational*>(rand2.get())->numerator;
        int ret_d = Ra1_d * Ra2_d;
        int ret_n = Ra1_n * Ra2_d + Ra2_n * Ra1_d;
        int t = gcd(ret_d, ret_n);
        ret_d /= t;
        ret_n /= t;
        return (ret_d == 1 ? IntegerV(ret_n) : (RationalV(ret_n, ret_d)));
        // 理论上来说这个时候分母肯定是正的
    }
    throw(RuntimeError("Wrong typename"));
}

Value Minus::evalRator(const Value &rand1, const Value &rand2) { // -
    if (rand1->v_type == V_INT && rand2->v_type == V_INT) {
        return IntegerV((dynamic_cast<Integer*>(rand1.get())->n) - (dynamic_cast<Integer*>(rand2.get())->n));
    }
    if (rand1->v_type == V_INT && rand2->v_type == V_RATIONAL) {
        int Int = dynamic_cast<Integer*>(rand1.get())->n;
        int Ra_d = dynamic_cast<Rational*>(rand2.get())->denominator;
        int Ra_n = dynamic_cast<Rational*>(rand2.get())->numerator;
        return RationalV(Int * Ra_d - Ra_n, Ra_d); // 不可能改变同余性
    }
    if (rand1->v_type == V_RATIONAL && rand2->v_type == V_INT) {
        int Int = dynamic_cast<Integer*>(rand2.get())->n;
        int Ra_d = dynamic_cast<Rational*>(rand1.get())->denominator;
        int Ra_n = dynamic_cast<Rational*>(rand1.get())->numerator;
        return RationalV(- Int * Ra_d + Ra_n, Ra_d);
   }
    if (rand1->v_type == V_RATIONAL && rand2->v_type == V_RATIONAL) {
        int Ra1_d = dynamic_cast<Rational*>(rand1.get())->denominator;
        int Ra1_n = dynamic_cast<Rational*>(rand1.get())->numerator;
        int Ra2_d = dynamic_cast<Rational*>(rand2.get())->denominator;
        int Ra2_n = dynamic_cast<Rational*>(rand2.get())->numerator;
        int ret_d = Ra1_d * Ra2_d;
        int ret_n = Ra1_n * Ra2_d - Ra2_n * Ra1_d;
        int t = gcd(ret_d, ret_n);
        ret_d /= t;
        ret_n /= t;
        return (ret_d == 1 ? IntegerV(ret_n) : (RationalV(ret_n, ret_d)));
        // 理论上来说这个时候分母肯定是正的
    }
    //TODO: To complete the substraction logic
    throw(RuntimeError("Wrong typename"));
}

Value Mult::evalRator(const Value &rand1, const Value &rand2) { // *
    if (rand1->v_type == V_INT && rand2->v_type == V_INT) {
        return IntegerV((dynamic_cast<Integer*>(rand1.get())->n) * (dynamic_cast<Integer*>(rand2.get())->n));
    }
    if (rand1->v_type == V_INT && rand2->v_type == V_RATIONAL) {
        int Int = dynamic_cast<Integer*>(rand1.get())->n;
        int Ra_d = dynamic_cast<Rational*>(rand2.get())->denominator;
        int Ra_n = dynamic_cast<Rational*>(rand2.get())->numerator;
        int final_n = Int * Ra_n;
        int t = gcd(final_n, Ra_d);
        final_n /= t;
        Ra_d /= t;
        return (Ra_d == 1 ? IntegerV(final_n) : (RationalV(final_n, Ra_d)));
    }
    if (rand1->v_type == V_RATIONAL && rand2->v_type == V_INT) {
        int Int = dynamic_cast<Integer*>(rand2.get())->n;
        int Ra_d = dynamic_cast<Rational*>(rand1.get())->denominator;
        int Ra_n = dynamic_cast<Rational*>(rand1.get())->numerator;
        int final_n = Int * Ra_n;
        int t = gcd(final_n, Ra_d);
        final_n /= t;
        Ra_d /= t;
        return (Ra_d == 1 ? IntegerV(final_n) : (RationalV(final_n, Ra_d)));
   }
    if (rand1->v_type == V_RATIONAL && rand2->v_type == V_RATIONAL) {
        int Ra1_d = dynamic_cast<Rational*>(rand1.get())->denominator;
        int Ra1_n = dynamic_cast<Rational*>(rand1.get())->numerator;
        int Ra2_d = dynamic_cast<Rational*>(rand2.get())->denominator;
        int Ra2_n = dynamic_cast<Rational*>(rand2.get())->numerator;
        int ret_d = Ra1_d * Ra2_d;
        int ret_n = Ra1_n * Ra2_n;
        int t = gcd(ret_n, ret_d);
        ret_n /= t;
        ret_d /= t;
        return (ret_d == 1 ? IntegerV(ret_n) : (RationalV(ret_n, ret_d)));
    }
    throw(RuntimeError("Wrong typename"));
}

Value Div::evalRator(const Value &rand1, const Value &rand2) { // /
    if (rand1->v_type == V_INT && rand2->v_type == V_INT) {
        int a = dynamic_cast<Integer*>(rand1.get())->n;
        int b = dynamic_cast<Integer*>(rand2.get())->n;
        if (!b) throw RuntimeError("division with 0");
        int t = gcd(a, b);
        a /= t; b /= t;
        return (b == 1 ? IntegerV(a) : (RationalV(a, b)));
    }
    if (rand1->v_type == V_INT && rand2->v_type == V_RATIONAL) {
        int INT = dynamic_cast<Integer*>(rand1.get())->n;
        int Frac_d = dynamic_cast<Rational*>(rand2.get())->denominator;
        int Frac_n = dynamic_cast<Rational*>(rand2.get())->numerator;
        if (!Frac_n) throw RuntimeError("division with 0");
        int ret_d = Frac_n;
        int ret_n = INT * Frac_d;
        int t = gcd(ret_d, ret_n);
        ret_d /= t;
        ret_n /= t;
        return (ret_d == 1 ? IntegerV(ret_n) : (RationalV(ret_n, ret_d)));
    }
    if (rand1->v_type == V_RATIONAL && rand2->v_type == V_INT) {
        int Frac_n = dynamic_cast<Rational*>(rand1.get())->numerator;
        int Frac_d = dynamic_cast<Rational*>(rand1.get())->denominator;
        int INT = dynamic_cast<Integer*>(rand2.get())->n;
        if (!INT) throw RuntimeError("division with 0");
        int ret_d = Frac_d * INT;
        int ret_n = Frac_n;
        int t = gcd(ret_d, ret_n);
        ret_d /= t;
        ret_n /= t;
        return (ret_d == 1 ? IntegerV(ret_n) : (RationalV(ret_n, ret_d)));       
    }
    if (rand1->v_type == V_RATIONAL && rand2->v_type == V_RATIONAL){
        int Ra1_d = dynamic_cast<Rational*>(rand1.get())->denominator;
        int Ra1_n = dynamic_cast<Rational*>(rand1.get())->numerator;
        int Ra2_d = dynamic_cast<Rational*>(rand2.get())->denominator;
        int Ra2_n = dynamic_cast<Rational*>(rand2.get())->numerator;
        if (!Ra2_n) throw RuntimeError("division with 0");
        int ret_d = Ra1_d * Ra2_n;
        int ret_n = Ra1_n * Ra2_d;
        int t = gcd(ret_n, ret_d);
        ret_n /= t;
        ret_d /= t;
        return (ret_d == 1 ? IntegerV(ret_n) : (RationalV(ret_n, ret_d)));
    }
    throw(RuntimeError("Wrong typename"));
}

Value Modulo::evalRator(const Value &rand1, const Value &rand2) { // modulo
    if (rand1->v_type == V_INT && rand2->v_type == V_INT) {
        int dividend = dynamic_cast<Integer*>(rand1.get())->n;
        int divisor = dynamic_cast<Integer*>(rand2.get())->n;
        if (divisor == 0) {
            throw(RuntimeError("Division by zero"));
        }
        return IntegerV(dividend % divisor);
    }
    throw(RuntimeError("modulo is only defined for integers"));
}

Value PlusVar::evalRator(const std::vector<Value> &args) { // + with multiple args
    //DONE: To complete the addition logic
    if (args.empty()) return IntegerV(0);
    if (args.size() == 1) return args[0];
    Value a = args[0];
    Value b = NULL;
    struct Plus plustool(NULL, NULL);
    for (int i = 1; i < args.size(); i++) {
        b = args[i];
        a = plustool.evalRator(a, b);
    }
    return a;
}

Value MinusVar::evalRator(const std::vector<Value> &args) { // - with multiple args
    if (args.empty()) throw RuntimeError("invalid arg num");
    Value a = args[0];
    Value b = NULL;
    struct Minus minustool(NULL, NULL);
    if (args.size() == 1) return minustool.evalRator(IntegerV(0), a);
    for (int i = 1; i < args.size(); i++) {
        b = args[i];
        a = minustool.evalRator(a, b);
    }
    return a;
}

Value MultVar::evalRator(const std::vector<Value> &args) { // * with multiple args
    if (args.empty()) return IntegerV(1);
    if (args.size() == 1) return args[0];
    Value a = args[0];
    Value b = NULL;
    struct Mult multtool(NULL, NULL);
    for (int i = 1; i < args.size(); i++) {
        b = args[i];
        a = multtool.evalRator(a, b);
    }
    return a;
}

Value DivVar::evalRator(const std::vector<Value> &args) { // / with multiple args
    if (args.empty()) throw RuntimeError("Invalid arg num");
    Value a = args[0];
    Value b = NULL;
    struct Div divtool(NULL, NULL);
    if (args.size() == 1) return divtool.evalRator(IntegerV(1), a);
    for (int i = 1; i < args.size(); i++) {
        b = args[i];
        a = divtool.evalRator(a, b);
    }
    return a;
}

Value Expt::evalRator(const Value &rand1, const Value &rand2) { // expt
    if (rand1->v_type == V_INT && rand2->v_type == V_INT) {
        int base = dynamic_cast<Integer*>(rand1.get())->n;
        int exponent = dynamic_cast<Integer*>(rand2.get())->n;
        
        if (exponent < 0) {
            throw(RuntimeError("Negative exponent not supported for integers"));
        }
        if (base == 0 && exponent == 0) {
            throw(RuntimeError("0^0 is undefined"));
        }
        
        long long result = 1;
        long long b = base;
        int exp = exponent;
        
        while (exp > 0) {
            if (exp % 2 == 1) {
                result *= b;
                if (result > INT_MAX || result < INT_MIN) {
                    throw(RuntimeError("Integer overflow in expt"));
                }
            }
            b *= b;
            if (b > INT_MAX || b < INT_MIN) {
                if (exp > 1) {
                    throw(RuntimeError("Integer overflow in expt"));
                }
            }
            exp /= 2;
        }
        
        return IntegerV((int)result);
    }
    throw(RuntimeError("Wrong typename"));
}

//A FUNCTION TO SIMPLIFY THE COMPARISON WITH INTEGER AND RATIONAL NUMBER
int compareNumericValues(const Value &v1, const Value &v2) {
    if (v1->v_type == V_INT && v2->v_type == V_INT) {
        int n1 = dynamic_cast<Integer*>(v1.get())->n;
        int n2 = dynamic_cast<Integer*>(v2.get())->n;
        return (n1 < n2) ? -1 : (n1 > n2) ? 1 : 0;
    }
    else if (v1->v_type == V_RATIONAL && v2->v_type == V_INT) {
        Rational* r1 = dynamic_cast<Rational*>(v1.get());
        int n2 = dynamic_cast<Integer*>(v2.get())->n;
        int left = r1->numerator;
        int right = n2 * r1->denominator;
        return (left < right) ? -1 : (left > right) ? 1 : 0;
    }
    else if (v1->v_type == V_INT && v2->v_type == V_RATIONAL) {
        int n1 = dynamic_cast<Integer*>(v1.get())->n;
        Rational* r2 = dynamic_cast<Rational*>(v2.get());
        int left = n1 * r2->denominator;
        int right = r2->numerator;
        return (left < right) ? -1 : (left > right) ? 1 : 0;
    }
    else if (v1->v_type == V_RATIONAL && v2->v_type == V_RATIONAL) {
        Rational* r1 = dynamic_cast<Rational*>(v1.get());
        Rational* r2 = dynamic_cast<Rational*>(v2.get());
        int left = r1->numerator * r2->denominator;
        int right = r2->numerator * r1->denominator;
        return (left < right) ? -1 : (left > right) ? 1 : 0;
    }
    throw RuntimeError("Wrong typename in numeric comparison");
}

Value Less::evalRator(const Value &rand1, const Value &rand2) { // < // 需要使用上面的compare函数简单化问题
    int ans = compareNumericValues(rand1, rand2);
    return (BooleanV(ans == -1));
}

Value LessEq::evalRator(const Value &rand1, const Value &rand2) { // <=
    int ans = compareNumericValues(rand1, rand2);
    return (BooleanV((ans == -1) || (ans == 0)));
}

Value Equal::evalRator(const Value &rand1, const Value &rand2) { // =
    int ans = compareNumericValues(rand1, rand2);
    return (BooleanV(ans == 0));
}

Value GreaterEq::evalRator(const Value &rand1, const Value &rand2) { // >=
    int ans = compareNumericValues(rand1, rand2);
    return (BooleanV((ans == 0) || (ans == 1)));
}

Value Greater::evalRator(const Value &rand1, const Value &rand2) { // >
    int ans = compareNumericValues(rand1, rand2);
    return (BooleanV(ans == 1));
}

Value LessVar::evalRator(const std::vector<Value> &args) { // < with multiple args
    bool ans = true;
    for (int i = 1; i < args.size(); i++) {
        ans = ans && (compareNumericValues(args[i - 1], args[i]) == -1);
    }
    return BooleanV(ans);
}

Value LessEqVar::evalRator(const std::vector<Value> &args) { // <= with multiple args
    bool ans = true;
    for (int i = 1; i < args.size(); i++) {
        ans = ans && (compareNumericValues(args[i - 1], args[i]) != 1);
    }
    return BooleanV(ans);
}

Value EqualVar::evalRator(const std::vector<Value> &args) { // = with multiple args
    bool ans = true;
    for (int i = 1; i < args.size(); i++) {
        ans = ans && (compareNumericValues(args[i - 1], args[i]) == 0);
    }
    return BooleanV(ans);
}

Value GreaterEqVar::evalRator(const std::vector<Value> &args) { // >= with multiple args
    bool ans = true;
    for (int i = 1; i < args.size(); i++) {
        ans = ans && (compareNumericValues(args[i - 1], args[i]) != -1);
    }
    return BooleanV(ans);
}

Value GreaterVar::evalRator(const std::vector<Value> &args) { // > with multiple args
    bool ans = true;
    for (int i = 1; i < args.size(); i++) {
        ans = ans && (compareNumericValues(args[i - 1], args[i]) == 1);
    }
    return BooleanV(ans);
}

Value Cons::evalRator(const Value &rand1, const Value &rand2) { // cons
    return (PairV(rand1, rand2));
}

Value ListFunc::evalRator(const std::vector<Value> &args) { // list function
    //Done: To complete the list logic
    if (args.size() == 0) return NullV(); // NullV 表示空表
    Value my_pair = NullV();
    for (int i = args.size() - 1; i >= 0; i--) {
        my_pair = PairV(args[i], my_pair);
    }
    return my_pair;
}

Value IsList::evalRator(const Value &rand) { // list?
    //Done: To complete the list? logic
    if (rand->v_type == V_NULL) return BooleanV(true);
    else if (rand->v_type == V_PAIR) {
        Value tail = rand;
        while (dynamic_cast<Pair*>(tail.get()) != nullptr) {
            tail = dynamic_cast<Pair*>(tail.get())->cdr;
        }
        return BooleanV(tail->v_type == V_NULL);
    }
    return BooleanV(false);
}

Value Car::evalRator(const Value &rand) { // car
    if (rand->v_type == V_PAIR) {
        return (dynamic_cast<Pair*>(rand.get())->car);
    }
    throw RuntimeError("Wrong typename");
}

Value Cdr::evalRator(const Value &rand) { // cdr
    if (rand->v_type == V_PAIR) {
        return (dynamic_cast<Pair*>(rand.get())->cdr);
    }
    throw RuntimeError("Wrong typename");
}

Value SetCar::evalRator(const Value &rand1, const Value &rand2) { // set-car!
    //TODO: To complete the set-car! logic
}

Value SetCdr::evalRator(const Value &rand1, const Value &rand2) { // set-cdr!
   //TODO: To complete the set-cdr! logic
}

Value IsEq::evalRator(const Value &rand1, const Value &rand2) { // eq?
    // 检查类型是否为 Integer
    if (rand1->v_type == V_INT && rand2->v_type == V_INT) {
        return BooleanV((dynamic_cast<Integer*>(rand1.get())->n) == (dynamic_cast<Integer*>(rand2.get())->n));
    }
    // 检查类型是否为 Boolean
    else if (rand1->v_type == V_BOOL && rand2->v_type == V_BOOL) {
        return BooleanV((dynamic_cast<Boolean*>(rand1.get())->b) == (dynamic_cast<Boolean*>(rand2.get())->b));
    }
    // 检查类型是否为 Symbol
    else if (rand1->v_type == V_SYM && rand2->v_type == V_SYM) {
        return BooleanV((dynamic_cast<Symbol*>(rand1.get())->s) == (dynamic_cast<Symbol*>(rand2.get())->s));
    }
    // 检查类型是否为 Null 或 Void
    else if ((rand1->v_type == V_NULL && rand2->v_type == V_NULL) ||
             (rand1->v_type == V_VOID && rand2->v_type == V_VOID)) {
        return BooleanV(true);
    } else {
        return BooleanV(rand1.get() == rand2.get());
    }
}

Value IsBoolean::evalRator(const Value &rand) { // boolean?
    return BooleanV(rand->v_type == V_BOOL);
}

Value IsFixnum::evalRator(const Value &rand) { // number?
    return BooleanV(rand->v_type == V_INT);
}

Value IsNull::evalRator(const Value &rand) { // null?
    return BooleanV(rand->v_type == V_NULL);
}

Value IsPair::evalRator(const Value &rand) { // pair?
    return BooleanV(rand->v_type == V_PAIR);
}

Value IsProcedure::evalRator(const Value &rand) { // procedure?
    return BooleanV(rand->v_type == V_PROC);
}

Value IsSymbol::evalRator(const Value &rand) { // symbol?
    return BooleanV(rand->v_type == V_SYM);
}

Value IsString::evalRator(const Value &rand) { // string?
    return BooleanV(rand->v_type == V_STRING);
}

Value Begin::eval(Assoc &e) {
    if (!es.size()) return VoidV();
    for (int j = 0; j < es.size() - 1; j++) {
        es[j]->eval(e);
    }
    return es[es.size() - 1]->eval(e);
}

Value Helper(Syntax s){
    if (dynamic_cast<Number*>(s.get()) != nullptr) {
        return IntegerV(dynamic_cast<Number*>(s.get())->n);
    }
    if (dynamic_cast<RationalSyntax*>(s.get()) != nullptr) {
        RationalSyntax* rat = dynamic_cast<RationalSyntax*>(s.get());
        return RationalV(rat->numerator, rat->denominator);
    }
    if (dynamic_cast<SymbolSyntax*>(s.get()) != nullptr) {
        return SymbolV(dynamic_cast<SymbolSyntax*>(s.get())->s);
    }
    if (dynamic_cast<StringSyntax*>(s.get()) != nullptr) {
        return StringV(dynamic_cast<StringSyntax*>(s.get())->s);
    }
    if (dynamic_cast<TrueSyntax*>(s.get()) != nullptr) {
        return BooleanV(true);
    }
    if (dynamic_cast<FalseSyntax*>(s.get()) != nullptr) {
        return BooleanV(false);
    }
    if (dynamic_cast<List*>(s.get()) != nullptr) {
        List* this_list = dynamic_cast<List*>(s.get());
        int i = this_list->stxs.size();
        if (!i) {
            return NullV();
        }
        std::vector<Value> first_list;
        std::vector<Value> mid_list;
        for (int j = 0; j < i; j++) {
            first_list.push_back(Helper(this_list->stxs[j]));
        }
        int is_pair = 0;
        for (int j = 0; j < i; j++) { 
            if (first_list[j]->v_type == V_SYM && dynamic_cast<Symbol*>(first_list[j].get())->s == ".") {
                if (is_pair == 1 || j == (i - 1) || j == 0) throw RuntimeError("Invalid dot expression"); // 难说能不能为0，会不会 . 作为一个函数？？
                is_pair = 1;
            }
            else {
                mid_list.push_back(first_list[j]);
            }
        }
        if (!is_pair) {
            Value my_pair = NullV();
            for (int i = mid_list.size() - 1; i >= 0; i--) {
                my_pair = PairV(mid_list[i], my_pair);
            }
            return my_pair;
        }
        else {
            Value my_pair = mid_list[mid_list.size() - 1];
            for (int i = mid_list.size() - 2; i >= 0; i--) {
                my_pair = PairV(mid_list[i], my_pair);
            }
            return my_pair;
        }
    }
    throw RuntimeError("What is your type??");
}

Value Quote::eval(Assoc& e) {
    //DONE: To complete the quote logic
    return Helper(s);
}

Value AndVar::eval(Assoc &e) { // and with short-circuit evaluation
    if (!rands.size()) return BooleanV(true);
    Value this_val = NULL;
    for (int i = 0; i < rands.size(); i++) {
        this_val = rands[i]->eval(e);
        if (this_val->v_type == V_BOOL && dynamic_cast<Boolean*>(this_val.get())->b == false) {
            return BooleanV(false);
        }
    }
    return this_val;
}

Value OrVar::eval(Assoc &e) { // or with short-circuit evaluation
    if (!rands.size()) return BooleanV(false);
    Value this_val = NULL;
    for (int i = 0; i < rands.size(); i++) {
        this_val = rands[i]->eval(e);
        if (this_val->v_type == V_BOOL && dynamic_cast<Boolean*>(this_val.get())->b == false) {
            continue;
        }
        return this_val;
    }
    return BooleanV(false);
}

Value Not::evalRator(const Value &rand) { // not
    if (rand->v_type == V_BOOL && dynamic_cast<Boolean*>(rand.get())->b == false) {
        return BooleanV(true);
    }
    else return BooleanV(false);
}

Value If::eval(Assoc &e) {
    Value pred = cond->eval(e);
    if (!pred.get()) {
        throw RuntimeError("Evaluation of condition resulted in an invalid value");
    }
    if (pred->v_type == V_BOOL && dynamic_cast<Boolean*>(pred.get())->b == false) {
        return (alter->eval(e));
    }
    else {
        return (conseq->eval(e));
    }
}

Value Cond::eval(Assoc &env) {
    for (int i = 0; i < clauses.size(); i++) {
        std::vector<Expr> this_clause = clauses[i];
        Value pred = this_clause[0]->eval(env);
        if (pred->v_type == V_BOOL && static_cast<Boolean*>(pred.get())->b == false) {
            continue;
        }
        else {
            if (this_clause.size() == 1) return pred;
            else {
                Value this_val = NULL;
                for (int j = 1; j < this_clause.size(); j++) {
                    this_val = this_clause[j]->eval(env);
                }
                return this_val;
            }
        }
    }
    return VoidV();
}

Value Lambda::eval(Assoc &env) { 
    //TODO: To complete the lambda logic
    return ProcedureV(x, e, env);
}

Value Apply::eval(Assoc &e) {
    Value proc_val = rator->eval(e); // 这是好习惯，没这么搞导致了 core dumped
    if (proc_val->v_type != V_PROC) {throw RuntimeError("Attempt to apply a non-procedure");}
    //TODO: TO COMPLETE THE CLOSURE LOGIC
    Procedure* clos_ptr = dynamic_cast<Procedure*>(proc_val.get()); 
    
    //TODO: TO COMPLETE THE ARGUMENT PARSER LOGIC
    std::vector<Value> args;
    for (int i = 0; i < rand.size(); i++) {
        args.push_back(rand[i]->eval(e));
    }
    if (auto varNode = dynamic_cast<Variadic*>(clos_ptr->e.get())) {
        //TODO 这时候就是procedure接受一个任意量数据，该怎么操作？这里主要的问题是variadic需要一个vector of value，但是我不能用它extend出assoc
        Value a = NULL;
        Value b = NullV();
        for (int i = args.size() - 1; i >= 0; i--) {
            a = args[i];
            b = PairV(a, b);
        }
        args.clear();
        args.push_back(b);
    }
    if (args.size() != clos_ptr->parameters.size()) throw RuntimeError("Wrong number of arguments");
    //: TO COMPLETE THE PARAMETERS' ENVIRONMENT LOGIC
    Assoc param_env = clos_ptr->env; // 用的是proc的env
    for (int i = 0; i < args.size(); i++) {
        param_env = extend(clos_ptr->parameters[i], args[i], param_env);
    } 
    return clos_ptr->e->eval(param_env);
}

Value Define::eval(Assoc &env) {
    //TODO: To complete the define logic
    env = extend(var, VoidV(), env);
    Value v = e->eval(env);
    env->v = v;
    return Value(VoidV());
}
Value Let::eval(Assoc &env) {
    //TODO: To complete the let logic
    Assoc new_env = env;  
    for (auto &p : bind) {
        Value val = p.second->eval(env); // 旧 env 中计算，这个跟letrec有区别
        new_env = extend(p.first, val, new_env); 
    }
    return body->eval(new_env);
}

Value Letrec::eval(Assoc &env) {
    //TODO: To complete the letrec logic
}

Value Set::eval(Assoc &env) {
    //TODO: To complete the set logic
}

Value Display::evalRator(const Value &rand) { // display function
    if (rand->v_type == V_STRING) {
        String* str_ptr = dynamic_cast<String*>(rand.get());
        std::cout << str_ptr->s;
    } else {
        rand->show(std::cout);
    }
    
    return VoidV();
}
