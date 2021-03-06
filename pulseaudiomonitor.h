#ifndef PULSEAUDIOMONITOR_H
#define PULSEAUDIOMONITOR_H


#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include <pulse/simple.h>
#include <pulse/error.h>
#include <QByteArray>
#include <QMutex>

#include <QThread>
#include <atomic>

#define BUFSIZE (1024*1)
//#define BUFSIZE 4096
//#define BUFSIZE 5600
//#define BUFSIZE (8192)
//#define BUFSIZE (8192*2)

class PulseAudioMonitor : public QThread
{
    Q_OBJECT

public:
     PulseAudioMonitor(QObject *parent = 0);
     ~PulseAudioMonitor();

    QMutex m_mutex;
    float m_left[BUFSIZE];
    float m_right[BUFSIZE];
    int currentPos = 0;
    std::atomic_bool modified;

public:
    void run();

signals:
    void dataReady(const QByteArray& data, const pa_sample_spec& spec );

public slots:
    void stop();

private:
    bool running;
};

#endif // PULSEAUDIOMONITOR_H
