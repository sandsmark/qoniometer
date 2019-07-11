#ifndef WIDGET_H
#define WIDGET_H

#include <QOpenGLWidget>
#include <QMultiMap>
#include <QMediaPlayer>
#include <QAudioBuffer>

class QAudioRecorder;

typedef struct {
    float *c0;
    float *c1;

    size_t rp;
    size_t wp;
    size_t len;
} gmringbuf;

inline gmringbuf * gmrb_alloc(size_t siz) {
    gmringbuf *rb  = (gmringbuf*) malloc(sizeof(gmringbuf));
    rb->c0 = (float*) malloc(siz * sizeof(float));
    rb->c1 = (float*) malloc(siz * sizeof(float));
    rb->len = siz;
    rb->rp = 0;
    rb->wp = 0;
    return rb;
}

inline void gmrb_free(gmringbuf *rb) {
    free(rb->c0);
    free(rb->c1);
    free(rb);
}

inline size_t gmrb_write_space(gmringbuf *rb) {
    if (rb->rp == rb->wp) return (rb->len -1);
    return ((rb->len + rb->rp - rb->wp) % rb->len) -1;
}

inline size_t gmrb_read_space(gmringbuf *rb) {
    return ((rb->len + rb->wp - rb->rp) % rb->len);
}

inline int gmrb_read_one(gmringbuf *rb, float *c0, float *c1) {
    if (gmrb_read_space(rb) < 1) return -1;
    *c0 = rb->c0[rb->rp];
    *c1 = rb->c1[rb->rp];
    rb->rp = (rb->rp + 1) % rb->len;
    return 0;
}

inline int gmrb_read(gmringbuf *rb, float *c0, float *c1, size_t len) {
    if (gmrb_read_space(rb) < len) return -1;
    if (rb->rp + len <= rb->len) {
        memcpy((void*) c0, (void*) &rb->c0[rb->rp], len * sizeof (float));
        memcpy((void*) c1, (void*) &rb->c1[rb->rp], len * sizeof (float));
    } else {
        int part = rb->len - rb->rp;
        int remn = len - part;
        memcpy((void*) c0, (void*) &rb->c0[rb->rp], part * sizeof (float));
        memcpy((void*) c1, (void*) &rb->c1[rb->rp], part * sizeof (float));
        memcpy((void*) &c0[part], (void*) rb->c0, remn * sizeof (float));
        memcpy((void*) &c1[part], (void*) rb->c1, remn * sizeof (float));
    }
    rb->rp = (rb->rp + len) % rb->len;
    return 0;
}


inline int gmrb_write(gmringbuf *rb, float *c0, float *c1, size_t len) {
    if (gmrb_write_space(rb) < len) return -1;
    if (rb->wp + len <= rb->len) {
        memcpy((void*) &rb->c0[rb->wp], (void*) c0, len * sizeof(float));
        memcpy((void*) &rb->c1[rb->wp], (void*) c1, len * sizeof(float));
    } else {
        int part = rb->len - rb->wp;
        int remn = len - part;

        memcpy((void*) &rb->c0[rb->wp], (void*) c0, part * sizeof(float));
        memcpy((void*) &rb->c1[rb->wp], (void*) c1, part * sizeof(float));

        memcpy((void*) rb->c0, (void*) &c0[part], remn * sizeof(float));
        memcpy((void*) rb->c1, (void*) &c1[part], remn * sizeof(float));
    }
    rb->wp = (rb->wp + len) % rb->len;
    return 0;
}

inline void gmrb_read_clear(gmringbuf *rb) {
    rb->rp = rb->wp;
}

class Widget : public QOpenGLWidget
{
    Q_OBJECT

public:
    Widget(QWidget *parent = 0);
    ~Widget();

protected:
    virtual void paintEvent(QPaintEvent *);
    virtual void resizeEvent(QResizeEvent*);
    virtual void keyPressEvent(QKeyEvent *);

private slots:
    void processBuffer(QAudioBuffer buffer);

private:
    //QMediaPlayer *m_player;
    gmringbuf *m_ringBuffer;
    QAudioRecorder *m_audioRecorder;
};

#endif // WIDGET_H
