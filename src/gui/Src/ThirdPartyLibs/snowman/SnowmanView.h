#ifndef SNOWMANVIEW_H
#define SNOWMANVIEW_H

#include <QWidget>

class SnowmanView : public QWidget
{
    Q_OBJECT
};

extern "C" __declspec(dllexport) SnowmanView* CreateSnowman(QWidget* parent);
extern "C" __declspec(dllexport) void DecompileAt(SnowmanView* snowman, dsint start, dsint end);
extern "C" __declspec(dllexport) void CloseSnowman(SnowmanView* snowman);

#endif // SNOWMANVIEW_H
