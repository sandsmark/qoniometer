#ifndef WIDGET_H
#define WIDGET_H

#include <QOpenGLWidget>
#include <QMultiMap>
#include "pulseaudiomonitor.h"

class Widget : public QOpenGLWidget
{
    Q_OBJECT
    enum Effect {
        Dots,
        Lines,
        Splines,
        Colors,
        EffectCount
    };

public:
    Widget(QWidget *parent = 0);
    ~Widget();

protected:
    virtual void paintEvent(QPaintEvent *);
    virtual void resizeEvent(QResizeEvent*);
    virtual void keyPressEvent(QKeyEvent *);

private:
    void doDots();
    void doLines();
    void doColors();
    void doSplines();

    PulseAudioMonitor m_monitor;
    Effect m_currentEffect;
    int m_ghost;
};

#endif // WIDGET_H
