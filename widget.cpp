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
#include <QApplication>
#include <QDesktopWidget>
#include "inlinehsv.h"

Widget::Widget(QWidget *parent)
    //: QOpenGLWidget(parent),
    : QWidget(parent),
      m_currentEffect(Colors),
//      m_currentEffect(Dots),
      m_ghost(32)
{
    //setAutoFillBackground(false);
    setWindowFlags(Qt::WindowStaysOnTopHint | Qt::X11BypassWindowManagerHint | Qt::WindowTransparentForInput);
    //setWindowOpacity(0.75);
    //setWindowFlags(Qt::WindowStaysOnTopHint | Qt::X11BypassWindowManagerHint | Qt::WindowTransparentForInput);
    //setWindowFlags(Qt::WindowStaysOnTopHint | Qt::X11BypassWindowManagerHint | Qt::WindowDoesNotAcceptFocus | Qt::Tool | Qt::WindowTransparentForInput);
    //setWindowFlags(Qt::Dialog | Qt::X11BypassWindowManagerHint | Qt::Tool);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_X11NetWmWindowTypeDND);
    //setAttribute(Qt::WA_X11NetWmWindowTypeNotification);
    //setAttribute(Qt::WA_TransparentForMouseEvents);
    //setWindowFlags(Qt::Dialog | Qt::WindowStaysOnTopHint | Qt::FramelessWindowHint);
    QRect desktopRect(qApp->desktop()->availableGeometry(this));
    resize(400, 200);
    move(desktopRect.x() + desktopRect.width() - width(), 0);
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
    if (!m_monitor.modified) {
        return;
    }
    if (m_buffer.size() != size()) {
        m_buffer = QImage(size(), QImage::Format_ARGB32);
        m_buffer.fill(Qt::transparent);
    }
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
    QPainter p(this);
    p.drawImage(0, 0, m_buffer);
}

void Widget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    //QOpenGLWidget::resizeEvent(event);
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
    //painter.fillRect(rect(), QColor(0, 0, 0, 0));
    //painter.fillRect(rect(), QColor(0, 0, 0, m_ghost));
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(QPen(QColor(255, 255, 255 ), 1, Qt::SolidLine, Qt::RoundCap));

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
        const float y = centerY - ay * height();
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
    QPainter painter(&m_buffer);
    //painter.setPen(QPen(QColor(255, 255, 255 ), 5));
    painter.setCompositionMode(QPainter::CompositionMode_DestinationIn);
    painter.fillRect(m_buffer.rect(), QColor(0, 0, 0, 190));
    //painter.setRenderHint(QPainter::Antialiasing);

    float lastX, lastY;
    const int centerX = width() / 2;
    const int centerY = height() / 2;
    const int scale = qMin(height(), width()) /2.1;
    m_monitor.m_mutex.lock();
    QPen pen;
    pen.setCapStyle(Qt::RoundCap);
    InlineHSV hsv;
    hsv.setRGB(255, 255, 255);

    pen.setWidth(3);
    pen.setColor(QColor(0, 0, 0, 64));
    painter.setPen(pen);
    const int sampleCount = sizeof(m_monitor.m_left)/sizeof(m_monitor.m_left[0]);
#if 1
    painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
    painter.setOpacity(0.01);
    for (int i=0; i<sampleCount; i++) {
        const float left = m_monitor.m_left[i];
        const float right = m_monitor.m_right[i];

        const float ay = left - right;
        const float ax = left + right;
        //const float angle = qAtan2(1./ay, 1. - ax) * M_PI + M_PI * .75;
        const float angle = qAtan2(right,  left) + M_PI ;
        const float dist = 1.-qSqrt(qAbs((ax + ay ) ));
        //const float dist = qAbs(ax * ax + ay * ay) * 3;
        const float x = centerX + cos(angle) * scale * dist*dist *1.5 ;
        const float y = centerY + sin(angle) * scale * dist*dist;

        pen.setWidth(20);
        pen.setColor(QColor(0, 0, 0));//, 64));
        painter.setPen(pen);
        painter.drawPoint(x, y);

        //pen.setWidth(3);
        //pen.setColor(QColor(255, 255, 255, 32));
        //painter.setPen(pen);
        //painter.drawPoint(x, y);
    }
    painter.setOpacity(1);
#endif
    painter.setCompositionMode(QPainter::CompositionMode_Source);
    for (int i=0; i<sampleCount; i++) {
        const float left = m_monitor.m_left[i];
        const float right = m_monitor.m_right[i];

        const float ay = left - right;
        const float ax = left + right;
        //const float angle = qAtan2(1./ay, 1. - ax) * M_PI + M_PI * .75;
        const float angle = qAtan2(right,  left) + M_PI ;
        const float dist = 1.-qSqrt(qAbs((ax + ay ) ));
        //const float dist = qAbs(ax * ax + ay * ay) * 3;
        const float x = centerX + cos(angle) * scale * dist*dist * 1.5;
        const float y = centerY + sin(angle) * scale * dist*dist;

        if (i > 1) {
            const float incre =  60 - qSqrt(float(i) / sampleCount)* 60;
            //const float incre = i * 359 / sampleCount;
            const float speed = qBound(0.f, (qAbs(ay - lastY) + qAbs(ax - lastX)) + 0.75f, 1.f);
            const float speed2 = qBound(0.f, (qAbs(ay - lastY) + qAbs(ax - lastX)), 1.f);
            //const float tres = sin(ax) * cos(speed2);
            const float distance = 1.0f  - qBound(0.001f, (qAbs(ay) + qAbs(ax)), 1.f);
            //const float distance = 1. - qBound(0., qSqrt((ax + 1.) * (ax + 1.) + (ay + 1.) * (ay + 1.)), 1.);
            hsv.convertHSV2RGB(incre, speed * 128.f + 128.f, qPow(distance + 0.3, 1.5) * speed * 255.f);
            //hsv.convertHSV2RGB(incre, speed * 128.f + 128.f, speed * 192.f + 64.f);
//            hsv.convertHSV2RGB(incre, speed, speed2);
            pen.setColor(qRgb(hsv.red(), hsv.green(), hsv.blue()));
            //pen.setColor(qRgba(hsv.red(), hsv.green(), hsv.blue(), 255 - 255 * distance));
//            pen.setColor(QColor::fromHsv(incre, speed, speed2, 128));

//            pen.setColor(QColor::fromHsv(qBound(0., distance * 359.0, 359.), speed, speed2, 128));
//            pen.setColor(QColor::fromHsv(, 255, qBound(0.f, qAbs(y - lastY) + qAbs(x - lastX), 255.f)));
//            pen.setColor(QColor::fromHsv(qAbs(ay) * 255, qAbs(ax) * 255, 255.0 * qSqrt(ax * ax + ay*ay)));
//            pen.setWidth(qMax(50.0 * qSqrt(ax * ax + ay*ay), 1.));
            pen.setWidth(qBound(1.5f, distance * distance * speed2 * scale / 2.75f, 10.f));
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
    //QPainter painter(this);
    QPainter painter(&m_buffer);
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
