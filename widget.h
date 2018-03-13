#ifndef WIDGET_H
#define WIDGET_H

#include <QOpenGLWidget>
#include <QMultiMap>
#include "pulseaudiomonitor.h"

//class Widget : public QOpenGLWidget
class Widget : public QWidget
{
    Q_OBJECT
    enum Effect {
        Dots,
        Lines,
        Splines,
        Colors,
        Out,
        EffectCount
    };

public:
    Widget(QWidget *parent = 0);
    ~Widget();

protected:
    virtual void paintEvent(QPaintEvent *);
    virtual void resizeEvent(QResizeEvent*);
    virtual void keyPressEvent(QKeyEvent *);

private slots:
    void updatePosition();

private:
    void doOut();
    void doDots();
    void doLines();
    void doColors();
    void doSplines();

    PulseAudioMonitor m_monitor;
    Effect m_currentEffect;
    int m_ghost;
};

#endif // WIDGET_H
