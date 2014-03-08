#ifndef MAIN_H
#define MAIN_H

#include <QApplication>

class MyApplication : public QApplication
{
public:
    MyApplication(int& argc, char** argv);
    bool notify(QObject* receiver, QEvent* event);
};

int main(int argc, char *argv[]);


#endif // MAIN_H
