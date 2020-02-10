#ifndef METHODINVOKER_H
#define METHODINVOKER_H

#include <functional>

#include "Bridge.h"

struct MethodInvoker
{
    template<class Func>
    static void invokeMethod(Func && fn)
    {
        using StdFunc = std::function<void()>;
        auto fnPtr = new StdFunc(std::forward<Func>(fn));
        GuiExecuteOnGuiThreadEx([](void* userdata)
        {
            auto stdFunc = (StdFunc*)userdata;
            (*stdFunc)();
            delete stdFunc;
        }, fnPtr);
    }
};

#endif // METHODINVOKER_H
