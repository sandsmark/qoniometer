#ifndef WIDGET_H
#define WIDGET_H

#include <QOpenGLWidget>
#include <QMultiMap>
#include <QDBusAbstractInterface>
#include "pulseaudiomonitor.h"
#include <QDBusPendingReply>
#include <QUrl>

class QNetworkAccessManager;
class QNetworkReply;

struct Smoother
{
    Smoother(double firstValue) : lastValue(firstValue) {}

    double getSmoothed(const double value) {
        lastValue = smoothFactor * value + (1.0 - smoothFactor)  * lastValue;
        return lastValue;
    }

    double lastValue = 0;
    double smoothFactor = 0.1;
};

class CoverHandler : public QDBusAbstractInterface
{
    Q_OBJECT
public:
    CoverHandler(QObject *parent);

    QUrl pictureUrl();
    const QImage &cover() { return m_cover; }

signals:
    void PropertiesChanged(const QString &interface_name, const QVariantMap &changed_properties, const QStringList &invalidated_properties);
    void coverUpdated();

    void started();
    void stopped();

private slots:
    void updateUrl();
    void downloadFinished(QNetworkReply *reply);
    void onPropertiesChanged();

private:
    QUrl m_coverUrl;
    QImage m_cover;
    bool m_playing = true;
    QNetworkAccessManager *m_nam;
};

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
    void paintEvent(QPaintEvent *) override;
    void resizeEvent(QResizeEvent*) override;
    void keyPressEvent(QKeyEvent *) override;
    void enterEvent(QEvent *) override;
    void leaveEvent(QEvent *) override;
    void mousePressEvent(QMouseEvent *e) override;
    void mouseMoveEvent(QMouseEvent *e) override;


private slots:
    void updatePosition();
    void onShow();

private:
    void doOut(QPainter &painter);
    void doDots(QPainter &painter);
    void doLines(QPainter &painter);
    void doColors(QPainter &painter);
    void doSplines(QPainter &painter);

    PulseAudioMonitor m_monitor;
    Effect m_currentEffect;
    int m_ghost;
    CoverHandler *m_mediaPlayer;
    QTimer *m_showTimer;
    QTimer *m_repaintTimer;
    QImage m_overlay;
    Smoother m_smoother;
};

#endif // WIDGET_H
