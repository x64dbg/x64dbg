/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2014 Pavel Strakhov
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */
#ifndef PaneWidget_H
#define PaneWidget_H

#include <QWidget>
#include <QSplitter>
#include <QTabWidget>
#include <QTabBar>
#include <QTimer>
#include <QRubberBand>
#include <QHash>
#include <QVariant>
#include <QLabel>

class Pane;
class PaneSerialize;

class PaneWidget : public QWidget {
  Q_OBJECT

  Q_PROPERTY(int suggestionSwitchInterval READ suggestionSwitchInterval WRITE setSuggestionSwitchInterval)
  Q_PROPERTY(int borderSensitivity READ borderSensitivity WRITE setBorderSensitivity)
  Q_PROPERTY(int rubberBandLineWidth READ rubberBandLineWidth WRITE setRubberBandLineWidth)

public:

  explicit PaneWidget(QWidget *parent = 0);
  virtual ~PaneWidget();

  enum AreaPointerType {
    InLastUsedArea, // The area widgets has been added to most recently.
    InNewFloatingArea, // New area in a detached window.
    InEmptySpace, // Area inside the manager widget (only available when there is no widgets in it).
    NoArea, // widget is hidden.
    AddTo, // Existing area specified in AreaPointer argument.
    LeftOf, // New area to the left of the area specified in AreaPointer argument.
    RightOf, // New area to the right of the area specified in AreaPointer argument.
    TopOf, // New area to the top of the area specified in AreaPointer argument.
    BottomOf // New area to the bottom of the area specified in AreaPointer argument.
  };

  /*!
   * \brief The AreaPointer class represents a place where widgets should be moved.
   */
  class AreaPointer {
  public:

    AreaPointer(AreaPointerType type = NoArea, Pane* area = 0);
    AreaPointerType type() const { return m_type; }
    Pane* area() const;

  private:
    AreaPointerType m_type;
    QWidget* m_widget;
    QWidget* widget() const { return m_widget; }
    AreaPointer(AreaPointerType type, QWidget* widget);
    void setWidget(QWidget* widget);

    friend class PaneWidget;

  };

  void addWidget(QWidget* widget, const AreaPointer& area);
  void addWidgets(QList<QWidget*> widgets, const AreaPointer& area);
  Pane* areaOf(QWidget* widget) const;

  void moveWidget(QWidget* widget, AreaPointer area);
  void moveWidgets(QList<QWidget*> widgets, AreaPointer area);
  void removeWidget(QWidget* widget);


  const QList<QWidget*>& widgets() { return m_widgets; }
  void hidewidget(QWidget* widget) { moveWidget(widget, NoArea); }

  QVariant saveState();
  void restoreState(const QVariant& data);

  void setSuggestionSwitchInterval(int msec);
  int suggestionSwitchInterval();
  int borderSensitivity() { return m_borderSensitivity; }
  void setBorderSensitivity(int pixels);
  void setRubberBandLineWidth(int pixels);
  int rubberBandLineWidth() { return m_rubberBandLineWidth; }

  QRubberBand* rectRubberBand() { return m_rectRubberBand; }
  QRubberBand* lineRubberBand() { return m_lineRubberBand; }


signals:
  void widgetVisibilityChanged(QWidget* widget, bool visible);

private:
  QList<QWidget*> m_widgets; // all added widgets
  QList<Pane*> mPanes; // all areas for this manager
  QList<PaneSerialize*> m_wrappers; // all wrappers for this manager
  int m_borderSensitivity;
  int m_rubberBandLineWidth;
  // list of widgets that are currently dragged, or empty list if there is no current drag
  QList<QWidget*> m_draggedwidgets;
  QLabel* m_dragIndicator; // label used to display dragged content

  QRubberBand* m_rectRubberBand; // placeholder objects used for displaying drop suggestions
  QRubberBand* m_lineRubberBand;
  QList<AreaPointer> m_suggestions; //full list of suggestions for current cursor position
  int m_dropCurrentSuggestionIndex; // index of currently displayed drop suggestion
                                    // (e.g. always 0 if there is only one possible drop location)
  QTimer m_dropSuggestionSwitchTimer; // used for switching drop suggestions

  // last widget used for adding widgets, or 0 if there isn't one
  // (warning: may contain pointer to deleted object)
  Pane* m_lastUsedArea;
  void handleNoSuggestions();
  //remove widget from its area (if any) and set parent to 0
  void releasewidget(QWidget* widget);
  void simplifyLayout(); //remove constructions that became useless
  void startDrag(const QList<QWidget*>& widgets);

  QVariantMap saveSplitterState(QSplitter* splitter);
  QSplitter* restoreSplitterState(const QVariantMap& data);
  void findSuggestions(PaneSerialize *wrapper);
  QRect sideSensitiveArea(QWidget* widget, AreaPointerType side);
  QRect sidePlaceHolderRect(QWidget* widget, AreaPointerType side);

  void updateDragPosition();
  void finishDrag();
  bool dragInProgress() { return !m_draggedwidgets.isEmpty(); }

  friend class Pane;
  friend class PaneSerialize;

protected:

  virtual QSplitter* createSplitter();
  virtual Pane *createArea();
  virtual QPixmap generateDragPixmap(const QList<QWidget *> &widgets);

private slots:
  void showNextDropSuggestion();
  void tabCloseRequested(int index);

};

#endif // PaneWidget_H
