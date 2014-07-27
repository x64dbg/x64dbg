#ifndef MAIN_H
#define MAIN_H

#include <QApplication>

class MyApplication : public QApplication
{
public:
    MyApplication(int& argc, char** argv);
    bool notify(QObject* receiver, QEvent* event);
    bool winEventFilter(MSG* message, long* result);
    static bool globalEventFilter(void* message);
};

int main(int argc, char *argv[]);


#endif // MAIN_H
