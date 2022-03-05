#include <iostream>
#include <mutex>
class Loger{
    public:
    std::mutex _mutex;
    void log(const string& x){
        _mutex.lock();
        std::cout<<"log:"<<x<<"\n";
        _mutex.unlock();
    }
    void warning(const string& x){
        _mutex.lock();
        std::cout<<"warning:"<<x<<"\n";
        _mutex.unlock();
    }
    void error(const string& x){
        _mutex.lock();
        std::cout<<"error:"<<x<<"\n";
        _mutex.unlock();
    }
};