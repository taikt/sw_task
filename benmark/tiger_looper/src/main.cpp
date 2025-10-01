#include "myhandler.h"
#include "SLLooper.h"
#include "Refbase.h"
#include <iostream>
#include <string>
using namespace std;


int main() {
    std::shared_ptr<SLLooper> looper = std::make_shared<SLLooper>();
    std::shared_ptr<myhandler> _h = std::make_shared<myhandler>(looper);
    std::shared_ptr<RefBase> msg = std::make_shared<RefBase>();
    msg->id = 1;
    //cout<<"call exit looper\n";
    //looper->exit();
    _h->sendMessage(_h->obtainMessage(TEST1, msg));

    std::shared_ptr<RefBase> msg2 = std::make_shared<RefBase>();
    msg2->id = 2;
    _h->sendMessage(_h->obtainMessage(TEST2, msg2));
    while(1){}

    return 0;
}