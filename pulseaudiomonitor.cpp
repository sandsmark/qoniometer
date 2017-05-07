#include "pulseaudiomonitor.h"

#include <iostream>
#include <QDebug>

PulseAudioMonitor::PulseAudioMonitor(QObject *parent) :
    QThread(parent)
{
}

PulseAudioMonitor::~PulseAudioMonitor()
{
    stop();
}

void PulseAudioMonitor::run()
{
    running=true;

    pa_sample_spec ss;

    ss.format = PA_SAMPLE_FLOAT32LE;
//    ss.format = PA_SAMPLE_S16LE;
    ss.channels = 2;
//    ss.rate = 44800;
//    ss.rate = BUFSIZE * ;
    ss.rate = 44100;

    pa_simple *s = NULL;
    int error;

    pa_buffer_attr ba;
    ba.tlength = (uint32_t)-1;//pa_usec_to_bytes(500*1024, &ss);
//    ba.minreq = (uint32_t)-1;//pa_usec_to_bytes(500*1024, &ss);
    ba.fragsize = (sizeof(m_left) + sizeof(m_right)); //(uint32_t)-1; //Adjust this value if it's glitchy
    ba.minreq = ba.fragsize; //(uint32_t)-1;//pa_usec_to_bytes(500*1024, &ss);
//    ba.maxlength = (uint32_t)-1;//pa_usec_to_bytes(500*1024, &ss);
    ba.maxlength = ba.fragsize; //(uint32_t)-1;//pa_usec_to_bytes(500*1024, &ss);
    //| PA_STREAM_ADJUST_LATENCY
    /* Create the recording stream */
    //"alsa_output.pci-0000_00_1b.0.analog-stereo.monitor"

    if (!(s = pa_simple_new(NULL, "sandsmark er kul", PA_STREAM_RECORD , NULL  , "record", &ss, NULL, &ba, &error))) {
        fprintf(stderr, __FILE__": pa_simple_new() failed: %s\n", pa_strerror(error));
        return;
    }

    const int samplesPerUpdate = ba.fragsize/pa_sample_size(&ss);
    float buffer[samplesPerUpdate];

    while (running) {
        /* Record some data ... */
        if (pa_simple_read(s, buffer, sizeof(buffer), &error) < 0) {
            fprintf(stderr, __FILE__": pa_simple_read() failed: %s\n", pa_strerror(error));
            running = false;
            continue;
        }

        m_mutex.lock();
        for (int i=0, j=0; i < samplesPerUpdate - 1; i+=2) {
            m_left[j] = buffer[i];
            m_right[j] = buffer[i+1];
            j++;
        }
        m_mutex.unlock();
    }

    pa_simple_free(s);
}
void PulseAudioMonitor::stop()
{
    running=false;
    wait();
}
