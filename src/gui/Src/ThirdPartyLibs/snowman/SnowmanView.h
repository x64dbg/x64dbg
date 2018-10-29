#ifndef SNOWMANVIEW_H
#define SNOWMANVIEW_H

class QWidget;

struct SnowmanRange
{
    duint start;
    duint end;
};

extern "C" __declspec(dllimport) QWidget* CreateSnowman(QWidget* parent);
extern "C" __declspec(dllimport) void DecompileAt(QWidget* snowman, dsint start, dsint end);
extern "C" __declspec(dllimport) void DecompileRanges(QWidget* snowman, const SnowmanRange* ranges, duint count);
extern "C" __declspec(dllimport) void CloseSnowman(QWidget* snowman);

#endif // SNOWMANVIEW_H
