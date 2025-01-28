#include <QCoreApplication>
#include <QFile>
#include <QLibraryInfo>
#include <QTextStream>

#include <qfont.h>
#include <qglobal.h>


int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    QFile outf(argv[1]);

    if (!outf.open(QIODevice::WriteOnly|QIODevice::Truncate|QIODevice::Text))
        return 1;

    QTextStream out(&outf);

    // The link test.
    new QFont();

    // Determine which features should be disabled.

#if defined(QT_NO_ACCESSIBILITY)
    out << "PyQt_Accessibility\n";
#endif

#if !QT_CONFIG(opengles2)
    out << "PyQt_OpenGL_ES2\n";
#if defined(QT_NO_OPENGL)
    out << "PyQt_OpenGL\n";
#endif
#endif

#if defined(QT_NO_RAWFONT)
    out << "PyQt_RawFont\n";
#endif

#if defined(QT_NO_SESSIONMANAGER)
    out << "PyQt_SessionManager\n";
#endif

#if !QT_CONFIG(vulkan) || !__has_include(<vulkan/vulkan.h>)
    out << "PyQt_Vulkan\n";
#endif

    return 0;
}
