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
#include <QDBusReply>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QMouseEvent>
#include <QPainterPath>

Widget::Widget(QWidget *parent) :
//      m_currentEffect(Splines),
      m_currentEffect(Out),
      m_ghost(128),
      m_smoother(1.)
{
    m_showTimer = new QTimer;
    m_showTimer->setSingleShot(true);
    m_showTimer->setInterval(1000);
    connect(m_showTimer, &QTimer::timeout, this, &Widget::onShow);

    setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::X11BypassWindowManagerHint | Qt::WindowDoesNotAcceptFocus | Qt::BypassWindowManagerHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_X11DoNotAcceptFocus);
    setAttribute(Qt::WA_TransparentForMouseEvents);
    resize(150, 150);
    m_repaintTimer = new QTimer(this);
    m_repaintTimer->setInterval(50);
    //repaintTimer->setInterval(50);
    connect(m_repaintTimer, SIGNAL(timeout()), this, SLOT(update()));
    m_repaintTimer->start();

    m_mediaPlayer = new CoverHandler(this);
    qDebug() << m_mediaPlayer->pictureUrl();

    m_monitor.start();

    updatePosition();

    connect(qApp->desktop(), &QDesktopWidget::screenCountChanged, this, &Widget::updatePosition);
    connect(qApp->desktop(), &QDesktopWidget::resized, this, &Widget::updatePosition);
    connect(m_mediaPlayer, &CoverHandler::coverUpdated, this, &Widget::updatePosition);
    //connect(m_mediaPlayer, &CoverHandler::started, m_repaintTimer, &QTimer::start);
    connect(m_mediaPlayer, SIGNAL(started()), m_repaintTimer, SLOT(start()));
    connect(m_mediaPlayer, SIGNAL(stopped()), m_repaintTimer, SLOT(stop()));
}

Widget::~Widget()
{
}

void Widget::updatePosition()
{
    const QRect coverRect = m_mediaPlayer->cover().rect();
    if (!coverRect.isEmpty() && coverRect.size() != size()) {
        qDebug() << "Resizing to cover";
        resize(m_mediaPlayer->cover().size());
    }
    //if (m_overlay.size() != size()) {
    //    m_overlay = QImage(size(), QImage::Format_ARGB32_Premultiplied);
    //    m_overlay.fill(Qt::transparent);
    //}

    int offsetX = qApp->desktop()->availableGeometry(qApp->desktop()->screenCount() - 1).width() - width();
    for (int screen = 1; screen < qApp->desktop()->screenCount(); screen++) {
        offsetX += qApp->desktop()->availableGeometry(screen).width();
    }

    int offsetY = qApp->desktop()->availableGeometry(qApp->desktop()->screenCount() - 1).height() - height() - 20;
    move(offsetX, offsetY);
}

void Widget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.fillRect(rect(), Qt::transparent);
    painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(QPen(QColor(0, 0, 0, 64), 3));

    const QImage cover = m_mediaPlayer->cover();

    painter.setOpacity(0.3);
    painter.drawImage(rect(), m_mediaPlayer->cover());

    painter.setCompositionMode(QPainter::CompositionMode_Lighten);
    switch(m_currentEffect) {
    case Out:
        doOut(painter);
        break;
    case Dots:
        doDots(painter);
        break;
    case Lines:
        doLines(painter);
        break;
    case Splines:
        doSplines(painter);
        break;
    case Colors:
        doColors(painter);
        break;
    default:
        break;
    }

    //{
    //    QPainter painter(this);
    //    painter.fillRect(rect(), Qt::transparent);
    //    painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
    //    painter.setRenderHint(QPainter::Antialiasing);
    //    painter.setPen(QPen(QColor(0, 0, 0, 64), 3));

    //    const QImage cover = m_mediaPlayer->cover();

    //    painter.setOpacity(0.3);
    //    painter.drawImage(rect(), m_mediaPlayer->cover());
    //    painter.setCompositionMode(QPainter::CompositionMode_Lighten);
    //    painter.drawImage(rect(), m_overlay);
    //}

}

void Widget::resizeEvent(QResizeEvent *event)
{
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

void Widget::doOut(QPainter &painter)
{
    QElapsedTimer timer;
    timer.start();

    const int centerX = width() / 2;
    const int centerY = height() / 2;

    //QPainter painter(this);
    //painter.setCompositionMode(QPainter::CompositionMode_Darken);
    painter.fillRect(rect(), QColor(0, 0, 0, m_ghost));
    //painter.setRenderHint(QPainter::Antialiasing);
    //painter.setCompositionMode(QPainter::RasterOp_NotSourceAndNotDestination);
    //painter.drawImage(QRect(rect().width() - cover.width(), rect().height() - cover.height(), width(), height()), m_mediaPlayer->cover());

    const int scale = qMin(height(), width()) / 2.;
    m_monitor.m_mutex.lock();
    const int samplecount = sizeof(m_monitor.m_left)/sizeof(m_monitor.m_left[0]);

    painter.setRenderHint(QPainter::Antialiasing, false);
    painter.setPen(QPen(QColor(255, 255, 255, 128), 1));
    //painter.setCompositionMode(QPainter::CompositionMode_SourceOver);

    float lastMaxVal = 1./std::max(m_smoother.lastValue, 0.001);
    float maxVal = -1;
    for (int i=0; i<samplecount; i++) {
        int posToRead = i + m_monitor.currentPos;
        posToRead %= samplecount;
        const float left = m_monitor.m_left[posToRead];
        const float right = m_monitor.m_right[posToRead];


        const float ay = left - right;
        const float ax = left + right;
        const float ang = atan2(ay, ax);

        const float x = centerX - ax * scale / lastMaxVal;
        const float y = centerY - ay * scale * 2 / lastMaxVal;
        const float nx = x + cos(ang) * std::hypot(ax, ay) * 20;
        const float ny = y + sin(ang) * std::hypot(ax, ay) * 20;
//        painter.setPen(QPen(QColor(255, 255, 255, (pow(ax, 2) + pow(ay, 2)) * 128), 1, Qt::SolidLine, Qt::RoundCap));
//        painter.setPen(QPen(QColor(255, 255, 255, std::hypot(ax, ay) * 255), 1, Qt::SolidLine, Qt::RoundCap));
        painter.setOpacity(std::hypot(x, y) ? std::hypot(x, y) / height() : 0.);
        if (i > 1) {
            painter.drawLine(x, y, nx, ny);
        }
        maxVal = std::max(std::abs(ay), maxVal);
        maxVal = std::max(std::abs(ax), maxVal);
        //lastX = x;
        //lastY = y;
    }
    m_monitor.m_mutex.unlock();
    if (maxVal > 0) {
        m_smoother.getSmoothed(1./maxVal);
    }
}

void Widget::doDots(QPainter &painter)
{
    QElapsedTimer timer;
    timer.start();

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

void Widget::doLines(QPainter &painter)
{
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(QPen(QColor(255, 255, 255 ), 5));
    painter.fillRect(rect(), QColor(0, 0, 0, m_ghost));

    float lastX, lastY;
    const int centerX = width() / 2;
    const int centerY = height() / 2;
    const int scale = qMin(height(), width());
    m_monitor.m_mutex.lock();
    const int samplecount = sizeof(m_monitor.m_left)/sizeof(m_monitor.m_left[0]);
    for (int i=1; i<samplecount; i+=2) {
        const float pleft = m_monitor.m_left[i-1];
        const float pright = m_monitor.m_right[i-1];

        const float left = m_monitor.m_left[i];
        const float right = m_monitor.m_right[i];


        const float pay = pleft - pright;
        const float pax = pleft + pright;
        const float px = centerX - pax * scale;
        const float py = centerY - pay * scale;

        const float ay = left - right;
        const float ax = left + right;
        const float x = centerX - ax * scale;
        const float y = centerY - ay * scale;
        if (i > 1) {
            painter.setPen(QPen(QColor(255, 255, 255, 32), 10 - 10 * qMax(qAbs(ax), qAbs(ay))));
            painter.drawLine(px, py, x, y);
        }
        lastX = x;
        lastY = y;
    }
    m_monitor.m_mutex.unlock();
}

void Widget::doColors(QPainter &painter)
{
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

        //const float x = centerX - ax * scale;
        //const float y = centerY - ay * scale;

        //const float x = centerX - scale / ax;
        //const float y = centerY - scale / ay;
        //const float y = centerY - ay * height() * 2.;
        if (Q_LIKELY(i > 1)) {
            const float incre = i * 359 / sampleCount;
            const float speed = qBound(0.f, (std::abs(ay - lastY) + std::abs(ax - lastX)) + 0.75f, 1.f);
            const float speed2 = qBound(0.f, (std::abs(ay - lastY) + std::abs(ax - lastX)), 1.f);
            //const float tres = sin(ax) * cos(speed2);
            const float distance = 1.0f  - qBound(0.f, (std::abs(ay) + std::abs(ax)), 1.f);
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
            const float x = centerX - qSqrt(std::abs(ax)) * scale * ax/std::abs(ax);
            const float y = centerY - qSqrt(std::abs(ay)) * scale * ay/std::abs(ay);
            painter.drawPoint(x, y);
//            painter.drawLine(lastX, lastY, x, y);
        }
        lastX = ax;
        lastY = ay;
    }
    m_monitor.m_mutex.unlock();
}

void Widget::doSplines(QPainter &painter)
{
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(QPen(QColor(255, 255, 255, 192 ), 0.1));
    //painter.fillRect(rect(), QColor(0, 0, 0, m_ghost));
    painter.fillRect(rect(), QColor(0, 0, 0, m_ghost));

    const int centerX = width() / 2;
    const int centerY = height() / 2;
    const int scale = qMin(height(), width());
    m_monitor.m_mutex.lock();
    QPainterPath path;
    //path.moveTo(centerX, centerY);
    const int samplecount = sizeof(m_monitor.m_left)/sizeof(m_monitor.m_left[0]);

    for (int i=1; i<samplecount; i+=2) {
        const float left = m_monitor.m_left[i];
        const float right = m_monitor.m_right[i];
        const float leftPrev = m_monitor.m_left[i-1];
        const float rightPrev = m_monitor.m_right[i-1];


        const float ax = left + right;
        const float ay = left - right;
        const float x = centerX - ax * scale;
        const float y = centerY - ay * scale;
        //const float y = centerY - ay * height() * 1.1;

        const float axPrev = leftPrev + rightPrev;
        const float ayPrev = leftPrev - rightPrev;
        const float xPrev = centerX - axPrev * scale;
        const float yPrev = centerY - ayPrev * scale;

        const float halfPrevX = (x - xPrev) / 2. + xPrev;
        const float halfPrevY = (y - yPrev) / 2. + yPrev;

        if (i < 2) {
            path.moveTo(x, y);
        } else {
            path.quadTo(xPrev, yPrev, halfPrevX, halfPrevY);
        }
    }
    painter.drawPath(path);
    m_monitor.m_mutex.unlock();
}

CoverHandler::CoverHandler(QObject *parent) :
    QDBusAbstractInterface("org.mpris.MediaPlayer2.spotify",
                           "/org/mpris/MediaPlayer2",
                           "org.freedesktop.DBus.Properties",
                           QDBusConnection::sessionBus(),
                           parent)
{
    m_nam = new QNetworkAccessManager(this);
//    m_nam->setRedirectPolicy(QNetworkRequest::NoLessSafeRedirectPolicy);
    m_nam->setRedirectPolicy(QNetworkRequest::UserVerifiedRedirectPolicy);
    connect(m_nam, &QNetworkAccessManager::finished, this, &CoverHandler::downloadFinished);



    connect(this, &CoverHandler::PropertiesChanged, this, &CoverHandler::updateUrl);
    updateUrl();
}

QUrl CoverHandler::pictureUrl()
{
    return QUrl(m_coverUrl);
}

void CoverHandler::updateUrl()
{
    QDBusReply<QVariantMap> rep = call(QStringLiteral("GetAll"), QString("org.mpris.MediaPlayer2.Player"));
    qDebug() << rep.error();

    QVariantMap foo = qdbus_cast<QVariantMap>(rep.value());
    for (const QString &key : foo.keys()) {
        qDebug() << key << foo[key];
    }
    QString playbackState = qdbus_cast<QString>(rep.value()["PlaybackStatus"]);
    if (playbackState == "Playing" && !m_playing) {
        m_playing = true;
        emit started();
    } else if (m_playing) {
        m_playing = false;
        emit stopped();
    }


    QUrl artUrl = qdbus_cast<QVariantMap>(rep.value()["Metadata"])["mpris:artUrl"].toString();
    artUrl.setHost("i.scdn.co");
    if (artUrl == m_coverUrl) {
        return;
    }

    m_coverUrl = artUrl;
    if (!m_coverUrl.isValid() || m_coverUrl.isEmpty()) {
        qWarning() << "Invalid cover url";
        return;
    }
    qDebug() << "Downloading" << m_coverUrl;
    QNetworkReply *reply = m_nam->get(QNetworkRequest(m_coverUrl));
    connect(reply, &QNetworkReply::redirected, reply, &QNetworkReply::redirectAllowed);
}

void CoverHandler::onPropertiesChanged()
{
    qDebug() << "Playback changed";
}

void CoverHandler::downloadFinished(QNetworkReply *reply)
{
    qDebug() << "Download complete";
    m_cover = QImage::fromData(reply->readAll());
    emit coverUpdated();

    reply->deleteLater();
}

void Widget::onShow()
{
    if (isVisible()) {
        return;
    }

    if (underMouse()) {
        hide();
        m_showTimer->start();
        return;
    }

    show();
}

void Widget::enterEvent(QEvent *)
{
    hide();
    m_showTimer->start();
}

void Widget::leaveEvent(QEvent *)
{
    setWindowOpacity(1);
}

void Widget::mouseMoveEvent(QMouseEvent *e)
{
    if (m_showTimer->isActive()) {
        hide();
        m_showTimer->start();
    }
}

void Widget::mousePressEvent(QMouseEvent *e)
{
    e->ignore();
    hide();
    m_showTimer->start();
}
