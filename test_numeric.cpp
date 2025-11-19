#include <iostream>
#include <string>

static int gcd(int a, int b) {
    while (b != 0) {
        int temp = b;
        b = a % b;
        a = temp;
    }
    return a;
}

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

int main(){
   std::string str;
   std::cin >> str;
   auto out = parse_rational(str);
   if (out.first) {
      if (out.second.second == 1) std::cout << out.second.first << std::endl;
      else std::cout << out.second.first << '\\' << out.second.second << std::endl;
   }
   else std::cout << "False" << std::endl;
}
