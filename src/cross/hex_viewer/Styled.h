#pragma once

#include <type_traits>
#include <QWidget>
#include <QColor>

template<class Widget>
class Styled
{
    bool polished = false;

    friend class QColorWrapper;

protected:
    QWidget* widget() { return (Widget*)(this); }
};

class QColorWrapper
{
    QWidget* widget;
    QColor color;

public:
    explicit QColorWrapper(QWidget* widget, QColor defaultColor = {})
        : widget(widget)
        , color(defaultColor)
    {
    }

    QColor operator()() const
    {
        return get();
    }

    QColor get(bool ensurePolished = true) const
    {
        if (ensurePolished && widget)
        {
            widget->ensurePolished();
        }
        return color;
    }

    void set(QColor color)
    {
        this->color = color;
        this->widget = nullptr;
    }
};

#define CSS_COLOR(name, defaultColor)                                   \
    QColorWrapper name = QColorWrapper(Styled::widget(), defaultColor); \
    Q_PROPERTY(QColor name READ get_##name WRITE set_##name)            \
    QColor get_##name() const { return name.get(false); }               \
    void set_##name(QColor color) { name.set(color); }
