#include "widget.h"
#include <QDebug>
#include <qmath.h>
#include <QPainter>
#include <QElapsedTimer>
#include <QMouseEvent>
#include <QAudioProbe>
#include <QTimer>
#include <QFileInfo>
#include <QMessageBox>
#include "inlinehsv.h"

Widget::Widget(QWidget *parent)
    : QOpenGLWidget(parent),
      m_currentEffect(Colors),
//      m_currentEffect(Dots),
      m_ghost(32)
{
    setWindowFlags(Qt::Dialog);
    resize(640, 480);
    QTimer *repaintTimer = new QTimer(this);
    repaintTimer->setInterval(50);
    //repaintTimer->setInterval(50);
    connect(repaintTimer, SIGNAL(timeout()), this, SLOT(update()));
    repaintTimer->start();

    m_monitor.start();
}

Widget::~Widget()
{
}

void Widget::paintEvent(QPaintEvent *)
{
    switch(m_currentEffect) {
    case Dots:
        doDots();
        break;
    case Lines:
        doLines();
        break;
    case Splines:
        doSplines();
        break;
    case Colors:
        doColors();
        break;
    default:
        break;
    }
}

void Widget::resizeEvent(QResizeEvent *event)
{
    QOpenGLWidget::resizeEvent(event);
    update();
}

void Widget::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape) {
        close();
    } else if (event->key() == Qt::Key_Space) {
        m_currentEffect = Effect((m_currentEffect + 1) % EffectCount);
    } else if (event->key() == Qt::Key_Up) {
        m_ghost = qMin(m_ghost + 10, 255);
    } else if (event->key() == Qt::Key_Down) {
        m_ghost = qMax(m_ghost - 10, 0);
    }
}

void Widget::doDots()
{
    QElapsedTimer timer;
    timer.start();

    QPainter painter(this);
    //QImage newBuf(size(), QImage::Format_ARGB32_Premultiplied);
    //newBuf.fill(Qt::transparent);
    //QPainter painter(&newBuf);
    //painter.setCompositionMode(QPainter::CompositionMode_Source);
    painter.fillRect(rect(), QColor(0, 0, 0, m_ghost));
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(QPen(QColor(255, 255, 255 ), 10, Qt::SolidLine, Qt::RoundCap));

    //float lastX, lastY;
//    gmringbuf *ringBuffer = m_monitor.getBuf();
//    for (size_t i=1; i<gmrb_read_space(ringBuffer); i++) {
//        float left, right;
//        if (gmrb_read_one(ringBuffer, &left, &right)) {
//            break;
//        }
    const int centerX = width() / 2;
    const int centerY = height() / 2;
    const int scale = qMin(height(), width());
    m_monitor.m_mutex.lock();
    const int samplecount = sizeof(m_monitor.m_left)/sizeof(m_monitor.m_left[0]);
    for (int i=0; i<samplecount; i++) {
        const float left = m_monitor.m_left[i];
        const float right = m_monitor.m_right[i];


        const float ay = left - right;
        const float ax = left + right;
        const float x = centerX - ax * scale;
        const float y = centerY - ay * height() * 1.3;
        if (i > 1) {
//            painter.drawLine(lastX, lastY, x, y);
            painter.drawPoint(x, y);
        }
        //lastX = x;
        //lastY = y;
    }
    m_monitor.m_mutex.unlock();
    //painter.end();
    //QPainter backPainter(this);
    //backPainter.fillRect(rect(), QColor(0, 0, 0, 128));
    //backPainter.drawImage(0, 0, newBuf);
//    m_monitor.releaseBuf();

}

void Widget::doLines()
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(QPen(QColor(255, 255, 255 ), 5));
    painter.fillRect(rect(), QColor(0, 0, 0, m_ghost));

    float lastX, lastY;
    const int centerX = width() / 2;
    const int centerY = height() / 2;
    const int scale = qMin(height(), width());
    m_monitor.m_mutex.lock();
    const int samplecount = sizeof(m_monitor.m_left)/sizeof(m_monitor.m_left[0]);
    for (int i=0; i<samplecount; i++) {
        const float left = m_monitor.m_left[i];
        const float right = m_monitor.m_right[i];


        const float ay = left - right;
        const float ax = left + right;
        const float x = centerX - ax * scale;
        const float y = centerY - ay * scale;
        if (i > 1) {
            painter.setPen(QPen(QColor(255, 255, 255, 32), 10 - 10 * qMax(qAbs(ax), qAbs(ay))));
            painter.drawLine(lastX, lastY, x, y);
        }
        lastX = x;
        lastY = y;
    }
    m_monitor.m_mutex.unlock();
}

void Widget::doColors()
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(QPen(QColor(255, 255, 255), 5));
    painter.fillRect(rect(), QColor(0, 0, 0, m_ghost));

    float lastX, lastY;
    const int centerX = width() / 2;
    const int centerY = height() / 2;
    const int scale = qMin(height(), width());
    m_monitor.m_mutex.lock();
    QPen pen;
//    pen.setWidth(30);
//    pen.setColor(QColor(255, 255, 255, 255));
    pen.setCapStyle(Qt::RoundCap);
    InlineHSV hsv;
    hsv.setRGB(255, 255, 255);
    const int sampleCount = sizeof(m_monitor.m_left)/sizeof(m_monitor.m_left[0]);
    for (int i=0; i<sampleCount; i++) {
        const float left = m_monitor.m_left[i];
        const float right = m_monitor.m_right[i];

//        const float leftNext = m_monitor.m_left[i+1];
//        const float rightNext = m_monitor.m_right[i+1];

        const float ay = left - right;
        const float ax = left + right;
        const float x = centerX - qSqrt(qAbs(ax)) * scale * ax/qAbs(ax);
        const float y = centerY - qSqrt(qAbs(ay)) * scale * ay/qAbs(ay);

        //const float x = centerX - ax * scale;
        //const float y = centerY - ay * scale;

        //const float x = centerX - scale / ax;
        //const float y = centerY - scale / ay;
        //const float y = centerY - ay * height() * 2.;
        if (i > 1) {
            const float incre = i * 359 / sampleCount;
            const float speed = qBound(0.f, (qAbs(ay - lastY) + qAbs(ax - lastX)) + 0.75f, 1.f);
            const float speed2 = qBound(0.f, (qAbs(ay - lastY) + qAbs(ax - lastX)), 1.f);
            //const float tres = sin(ax) * cos(speed2);
            const float distance = 1.0f  - qBound(0.f, (qAbs(ay) + qAbs(ax)), 1.f);
            //const float distance = 1. - qBound(0., qSqrt((ax + 1.) * (ax + 1.) + (ay + 1.) * (ay + 1.)), 1.);
            hsv.convertHSV2RGB(incre, speed * 128.f + 128.f, qPow(distance, 1.5) * speed * 255.f);
            //hsv.convertHSV2RGB(incre, speed * 128.f + 128.f, speed * 192.f + 64.f);
//            hsv.convertHSV2RGB(incre, speed, speed2);
            pen.setColor(qRgb(hsv.red(), hsv.green(), hsv.blue()));
            //pen.setColor(qRgba(hsv.red(), hsv.green(), hsv.blue(), 255 - 255 * distance));
//            pen.setColor(QColor::fromHsv(incre, speed, speed2, 128));

//            pen.setColor(QColor::fromHsv(qBound(0., distance * 359.0, 359.), speed, speed2, 128));
//            pen.setColor(QColor::fromHsv(, 255, qBound(0.f, qAbs(y - lastY) + qAbs(x - lastX), 255.f)));
//            pen.setColor(QColor::fromHsv(qAbs(ay) * 255, qAbs(ax) * 255, 255.0 * qSqrt(ax * ax + ay*ay)));
//            pen.setWidth(qMax(50.0 * qSqrt(ax * ax + ay*ay), 1.));
            pen.setWidth(qBound(1.f, speed2 * scale / 10.f, 30.f));
//            pen.setWidth(qBound(1.f, qAbs(ax - lastX) * scale + qAbs(ay - lastY) * scale / 100.f, 30.f));
            painter.setPen(pen);
//            painter.drawPoint(x, y);
            painter.drawPoint(x, y);
//            painter.drawLine(lastX, lastY, x, y);
        }
        lastX = ax;
        lastY = ay;
    }
    m_monitor.m_mutex.unlock();
}

void Widget::doSplines()
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(QPen(QColor(255, 255, 255, 255 ), 1));
    painter.fillRect(rect(), QColor(0, 0, 0, m_ghost));

    const int centerX = width() / 2;
    const int centerY = height() / 2;
    const int scale = qMin(height(), width());
    m_monitor.m_mutex.lock();
    QPainterPath path;
    path.moveTo(centerX, centerY);
    const int samplecount = sizeof(m_monitor.m_left)/sizeof(m_monitor.m_left[0]);
    for (int i=0; i<samplecount-1; i+=2) {
        const float left = m_monitor.m_left[i];
        const float right = m_monitor.m_right[i];
        const float leftNext = m_monitor.m_left[i+1];
        const float rightNext = m_monitor.m_right[i+1];


        const float ax = left + right;
        const float ay = left - right;
        const float x = centerX - ax * scale;
        //const float y = centerY - ay * scale;
        const float y = centerY - ay * height() * 1.1;

        const float axNext = leftNext + rightNext;
        const float ayNext = leftNext - rightNext;
        const float xNext = centerX - axNext * scale;
        const float yNext = centerY - ayNext * scale;

        path.quadTo(x, y, xNext, yNext);
    }
    painter.drawPath(path);
    m_monitor.m_mutex.unlock();
}
