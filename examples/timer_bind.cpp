#include <iostream>
#include <memory>
#include <functional>
#include "SLLooper.h"

class MyClass {
public:
    void onTimeout(int value) {
        std::cout << "MyClass::onTimeout called with value: " << value << std::endl;
    }
};

int main() {
    auto looper = std::make_shared<SLLooper>();
    MyClass obj;

    // Dùng std::bind để bind method và tham số
    auto timer = looper->addTimer(std::bind(&MyClass::onTimeout, &obj, 42), 2s);

    std::this_thread::sleep_for(3s);
    return 0;
}