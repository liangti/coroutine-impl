#ifndef COROUTINE_UTILS
#define COROUTINE_UTILS

#include <iostream>
#include <exception>
#include <string>

struct CoException: public std::exception{
public:
    CoException(std::string message): message(message){}
    inline std::string get_message(){
        return message;
    }
private:
    std::string message;
};

// global utilities
inline void EmitError(const char* message){
    std::cerr << "Error: " << message << std::endl;
}

// global external utilities
inline void EmitLog(const char* message){
    std::cout << "Log: " << message << std::endl;
}


#endif