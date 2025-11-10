#ifndef RUNTIMEERROR
#define RUNTIMEERROR

#include <exception>
#include <string>

class RuntimeError : std::exception { // 继承exception
    private:
        std::string s;
    public:
        RuntimeError(std::string); // 构造函数，就是初始化s
        std::string message() const; // 报错
};

#endif