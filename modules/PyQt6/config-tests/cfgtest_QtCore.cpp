#include <QCoreApplication>
#include <QFile>
#include <QLibraryInfo>
#include <QTextStream>


int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    QFile outf(argv[1]);

    if (!outf.open(QIODevice::WriteOnly|QIODevice::Truncate|QIODevice::Text))
        return 1;

    QTextStream out(&outf);

    // This is not a feature and needs to be handled separately.
#if defined(QT_SHARED) || defined(QT_DLL)
    out << "shared\n";
#else
    out << "static\n";
#endif

    // Determine which features should be disabled.

#if defined(QT_NO_PROCESS)
    out << "PyQt_Process\n";
#endif

    // qreal is double unless QT_COORD_TYPE is defined.
    if (sizeof (qreal) != sizeof (double))
        out << "PyQt_qreal_double\n";

#if QT_VERSION < 0x060500 || !QT_CONFIG(permissions)
    out << "PyQt_Permissions";
#endif

    return 0;
}
