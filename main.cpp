#include "widget.h"
#include <QApplication>
#include <QSurfaceFormat>
#include <QDebug>

extern "C" {
    #include <termios.h>
    #include <unistd.h>
    #include <fcntl.h>
}

static termios s_originalAttributes;

static void exitHandler()
{
    tcsetattr(STDIN_FILENO, TCSANOW, &s_originalAttributes);
}

static void sigHandler(int)
{
    qDebug() << "Bye";
    qApp->quit();
}

int main(int argc, char *argv[])
{
    if (isatty(STDIN_FILENO)) {
        int oldFlags = fcntl(STDIN_FILENO, F_GETFL);
        fcntl(STDIN_FILENO, F_SETFL, oldFlags | O_NONBLOCK);

        tcgetattr(STDIN_FILENO, &s_originalAttributes);

        atexit(exitHandler);

        termios tattr;
        tcgetattr (STDIN_FILENO, &tattr);
        tattr.c_lflag &= ~(ICANON|ECHO); /* Clear ICANON and ECHO. */
        tattr.c_cc[VMIN] = 1;
        tattr.c_cc[VTIME] = 0;
        tcsetattr (STDIN_FILENO, TCSAFLUSH, &tattr);
    }

    QApplication a(argc, argv);
    signal(SIGINT, sigHandler);

    QSurfaceFormat fmt;
    fmt.setSamples(16);
    QSurfaceFormat::setDefaultFormat(fmt);

    Widget w;
    w.show();

    return a.exec();
}
