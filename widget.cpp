#include "widget.h"
#include <QDebug>
#include <qmath.h>
#include <QPainter>
#include <QElapsedTimer>
#include <QMouseEvent>
#include <QAudioProbe>
#include <QTimer>
#include <QFileInfo>

Widget::Widget(QWidget *parent)
    : QOpenGLWidget(parent),
      m_player(new QMediaPlayer (this))
{
    setWindowFlags(Qt::Dialog);
    resize(640, 480);

    const int rate = 48000;
    m_ringBuffer = gmrb_alloc(rate/2);

    QAudioProbe *probe = new QAudioProbe;
    connect(probe, &QAudioProbe::audioBufferProbed, this, &Widget::processBuffer);
    probe->setSource(m_player);

    m_player->setMedia(QUrl::fromLocalFile(QFileInfo("./winnar.mp3").absoluteFilePath()));
//    m_player->play();
    m_player->setVolume(50);

    QTimer *repaintTimer = new QTimer(this);
    repaintTimer->setInterval(20);
    connect(repaintTimer, SIGNAL(timeout()), this, SLOT(update()));
    repaintTimer->start();
}

Widget::~Widget()
{
}

void Widget::paintEvent(QPaintEvent *)
{
    QElapsedTimer timer;
    timer.start();

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.fillRect(rect(), QColor(0, 0, 0, 64));
    painter.setPen(QPen(QColor(255, 255, 255, 32), 1));

    float lastX, lastY;
    for (size_t i=1; i<gmrb_read_space(m_ringBuffer); i++) {
        float left, right;
        if (gmrb_read_one(m_ringBuffer, &left, &right)) {
            break;
        }

        const float ax = left - right;
        const float ay = left + right;
        const float x = (width() / 2) - ax * width() / 3;
        const float y = (height() / 2) - ay * height() / 3;
        if (i > 1) {
            painter.drawLine(lastX, lastY, x, y);
        }
        lastX = x;
        lastY = y;
    }
}

void Widget::resizeEvent(QResizeEvent *event)
{
    QOpenGLWidget::resizeEvent(event);
    update();
}

void Widget::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Space) {
        m_player->play();
    }
}

void Widget::processBuffer(QAudioBuffer buffer)
{
    if (buffer.format().channelCount() != 2) {
        qDebug() << buffer.format();
        return;
    }

    const int numSamples = buffer.sampleCount() / 2;
    float left[numSamples];
    float right[numSamples];
    switch(buffer.format().sampleType()) {
    case QAudioFormat::Float: {
        for (int j=0, i=0; i<buffer.sampleCount() - 1; i+=2) {
            left[j] = buffer.data<float>()[i];
            right[j] = buffer.data<float>()[i+1];
            j++;
        }
        break;
    }
    case QAudioFormat::UnSignedInt: {
        if (buffer.format().sampleSize() == 16) {
            for (int j=0, i=0; i<buffer.sampleCount() - 1; i+=2) {
                left[j] = buffer.data<quint16>()[i] / 65536.f;
                right[j] = buffer.data<quint16>()[i+1] / 65536.f;
                j++;
            }
        } else if (buffer.format().sampleSize() == 8) {
            for (int j=0, i=0; i<buffer.sampleCount() - 1; i+=2) {
                left[j] = buffer.data<quint8>()[i] / 255.f;
                right[j] = buffer.data<quint8>()[i+1] / 255.f;
                j++;
            }
        }
        break;
    }
    case QAudioFormat::SignedInt: {
        if (buffer.format().sampleSize() == 16) {
            for (int j=0, i=0; i<buffer.sampleCount() - 1; i+=2) {
                left[j] = buffer.data<qint16>()[i] / 65536.f;
                right[j] = buffer.data<qint16>()[i+1] / 65536.f;
                j++;
            }
        } else if (buffer.format().sampleSize() == 8) {
            for (int j=0, i=0; i<buffer.sampleCount() - 1; i+=2) {
                left[j] = buffer.data<quint8>()[i] / 128.f;
                right[j] = buffer.data<quint8>()[i+1] / 128.f;
                j++;
            }
        }
        break;
    }
    default:
        qDebug() << buffer.format();
        return;
    }
    gmrb_write(m_ringBuffer, left, right, numSamples);
}
