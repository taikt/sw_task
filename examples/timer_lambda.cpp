#include <iostream>
#include <memory>
#include <functional>
#include "SLLooper.h"
using namespace std::chrono_literals;
using namespace swt;

class MyClass {
    public:
        void onTimeout(int value) {
            std::cout << "Timeout value: " << value << std::endl;
        }
    };
    
    int main() {
        auto looper = std::make_shared<SLLooper>();
        MyClass obj;
    
        int param = 42;
        auto timer = looper->addTimer([&obj, param]() {
            obj.onTimeout(param);
        }, 2s);
    
        std::this_thread::sleep_for(3s);
    }