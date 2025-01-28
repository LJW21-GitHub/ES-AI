# This is the PyQt6 build script.
#
# Copyright (c) 2024 Riverbank Computing Limited <info@riverbankcomputing.com>
# 
# This file is part of PyQt6.
# 
# This file may be used under the terms of the GNU General Public License
# version 3.0 as published by the Free Software Foundation and appearing in
# the file LICENSE included in the packaging of this file.  Please review the
# following information to ensure the GNU General Public License version 3.0
# requirements will be met: http://www.gnu.org/copyleft/gpl.html.
# 
# If you do not wish to use this file under the terms of the GPL version 3.0
# then you may purchase a commercial license.  For more information contact
# info@riverbankcomputing.com.
# 
# This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
# WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.


import glob
import os
import sys

from pyqtbuild import PyQtBindings, PyQtProject, QmakeTargetInstallable
from sipbuild import (Buildable, BuildableModule, Installable, Option,
        UserException)


# The minimum sip module ABI version needed.
ABI_VERSION = '13.8'


class PyQt(PyQtProject):
    """ The PyQt6 project. """

    def __init__(self):
        """ Initialise the project. """

        # We specify the name of the sip module because PyQt-builder doesn't
        # provide it if we are creating an sdist.
        super().__init__(abi_version=ABI_VERSION, sip_module='PyQt6.sip',
                dunder_init=True, tag_prefix='Qt', console_scripts=[
                    'pylupdate6 = PyQt6.lupdate.pylupdate:main',
                    'pyuic6 = PyQt6.uic.pyuic:main'])

        # Each set of bindings must appear after any set they depend on.
        # QtLocation is still to be ported to Qt6.
        self.bindings_factories = [QtCore, QtNetwork, QtGui, QtQml, QtWidgets,
                QtDBus, QtDesigner, QtHelp, QtOpenGL, QtOpenGLWidgets,
                QtPrintSupport, QtQuick, QtQuick3D, QtQuickWidgets, QtSql,
                QtSvg, QtSvgWidgets, QtTest, QtXml, QtMultimedia,
                QtMultimediaWidgets, QtPositioning, QtRemoteObjects, QtSensors,
                QtSerialPort, QtWebChannel, QtWebSockets, QtBluetooth, QtNfc,
                QtPdf, QtPdfWidgets, QtSpatialAudio, QtTextToSpeech,
                QAxContainer]

    def apply_user_defaults(self, tool):
        """ Set default values where needed. """

        if self.license_dir is None:
            self.license_dir = os.path.join(self.root_dir, 'sip')
        else:
            self.license_dir = os.path.abspath(self.license_dir)

        super().apply_user_defaults(tool)

        if not self.tools:
            self.console_scripts = []

    def get_dunder_init(self):
        """ Return the contents of the __init__.py file to install. """

        with open(os.path.join(self.root_dir, '__init__.py')) as f:
            dunder_init = f.read()

        if self.py_platform == 'win32':
            dunder_init += """

def find_qt():
    import os, sys

    qtcore_dll = '\\\\Qt6Core.dll'

    dll_dir = os.path.dirname(sys.executable)
    if not os.path.isfile(dll_dir + qtcore_dll):
        path = os.environ['PATH']

        dll_dir = os.path.dirname(__file__) + '\\\\Qt6\\\\bin'
        if os.path.isfile(dll_dir + qtcore_dll):
            path = dll_dir + ';' + path
            os.environ['PATH'] = path
        else:
            for dll_dir in path.split(';'):
                if os.path.isfile(dll_dir + qtcore_dll):
                    break
            else:
                return

    try:
        os.add_dll_directory(dll_dir)
    except AttributeError:
        pass


find_qt()
del find_qt
"""

        return dunder_init

    def get_options(self):
        """ Return the sequence of configurable options. """

        # Get the standard options.
        options = super().get_options()

        # Add our new options.
        options.append(
                Option('confirm_license', option_type=bool,
                        help="confirm acceptance of the license"))

        options.append(
                Option('license_dir', option_type=str,
                        help="the license file can be found in DIR",
                        metavar="DIR"))

        options.append(
                Option('qt_shared', option_type=bool,
                        help="assume Qt has been built as shared libraries"))

        options.append(
                Option('designer_plugin', option_type=bool, inverted=True,
                        help="disable the building of the Python plugin for Qt Designer"))

        options.append(
                Option('qml_plugin', option_type=bool, inverted=True,
                        help="disable the building of the Python plugin for qmlscene"))

        options.append(
                Option('dbus', option_type=str,
                        help="the directory containing the dbus/dbus-python.h file",
                        metavar="DIR"))

        options.append(
                Option('dbus_python', option_type=bool, inverted=True,
                        help="disable the Qt support for the dbus-python package"))

        options.append(
                Option('tools', option_type=bool, inverted=True,
                        help="disable the installation of pyuic6 and pylupdate6"))

        return options

    def update(self, tool):
        """ Update the configuration. """

        if tool not in Option.BUILD_TOOLS:
            return

        # Check we support the version of Qt.
        if self.builder.qt_version >> 16 != 6:
            raise UserException(
                    "Qt v6 is required, not v{0}".format(
                            self.builder.qt_version_str))

        # Automatically confirm the license if there might not be a command
        # line option to do so.
        if tool == 'pep517':
            self.confirm_license = True

        self._check_license()

        # Handle the platform tag.
        platform_tags_map = {
            'android':  'Android',
            'darwin':   'macOS',
            'ios':      'iOS',
            'wasm':     'WebAssembly',
            'win32':    'Windows',
        }

        self.bindings['QtCore'].tags.append(
                platform_tags_map.get(self.py_platform, 'Linux'))

        # Make sure the bindings are buildable.
        super().update(tool)

        # PyQt6-WebEngine needs to know if QtWebChannel is available.
        if 'QtWebChannel' not in self.bindings:
            qtcore = self.bindings.get('QtCore')
            if qtcore is not None:
                qtcore.disabled_features.append('PyQt_WebChannel')

        # Always install the lupdate module.
        installable = Installable('lupdate', target_subdir='PyQt6')
        installable.files.append(os.path.join(self.root_dir, 'lupdate'))
        self.installables.append(installable)

        # Always install the uic module.
        installable = Installable('uic', target_subdir='PyQt6')
        installable.files.append(os.path.join(self.root_dir, 'uic'))
        self.installables.append(installable)

        # If any set of bindings is being built as a debug version then assume
        # the plugins and DBus support should as well.
        for bindings in self.bindings.values():
            if bindings.debug:
                others_debug = True
                break
        else:
            others_debug = self.py_debug

        # Add the plugins.  For the moment we don't include them in wheels.
        # This may change when we improve the bundling of Qt.
        if tool in ('build', 'install'):
            if self.designer_plugin and 'QtDesigner' in self.bindings:
                self._add_plugin('designer', "Qt Designer", 'pyqt6',
                        'designer', others_debug)

            if self.qml_plugin and 'QtQml' in self.bindings:
                self._add_plugin('qmlscene', "qmlscene", 'pyqt6qmlplugin',
                        'PyQt6', others_debug)

        # Add the dbus-python support.
        if self.dbus_python:
            self._add_dbus(others_debug)

    def _add_dbus(self, debug):
        """ Add the dbus-python support. """

        self.progress(
                "Checking to see if the dbus-python support should be built")

        # See if dbus-python is installed.
        try:
            import dbus.mainloop
        except ImportError:
            self.progress(
                    "The dbus-python package does not seem to be installed.")
            return

        dbus_module_dir = dbus.mainloop.__path__[0]

        # Get the flags for the DBus library.
        dbus_inc_dirs = []
        dbus_lib_dirs = []
        dbus_libs = []

        args = ['pkg-config', '--cflags-only-I', '--libs dbus-1']

        for line in self.read_command_pipe(args, fatal=False):
            for flag in line.strip().split():
                if flag.startswith('-I'):
                    dbus_inc_dirs.append(flag[2:])
                elif flag.startswith('-L'):
                    dbus_lib_dirs.append(flag[2:])
                elif flag.startswith('-l'):
                    dbus_libs.append(flag[2:])

        if not any([dbus_inc_dirs, dbus_lib_dirs, dbus_libs]):
            self.progress("DBus v1 does not seem to be installed.")

        # Try and find dbus-python.h.  The current PyPI package doesn't install
        # it.  We look where DBus itself is installed.
        if self.dbus:
            dbus_inc_dirs.append(self.dbus)

        for d in dbus_inc_dirs:
            if os.path.isfile(os.path.join(d, 'dbus', 'dbus-python.h')):
                break
        else:
            self.progress(
                    "dbus/dbus-python.h could not be found and so the "
                    "dbus-python support module will be disabled. If "
                    "dbus-python is installed then use the --dbus argument to "
                    "explicitly specify the directory containing "
                    "dbus/dbus-python.h.")
            return

        # Create the buildable.
        sources_dir = os.path.join(self.root_dir, 'dbus')

        buildable = BuildableModule(self, 'dbus', 'dbus.mainloop.pyqt6',
                uses_limited_api=True)
        buildable.builder_settings.append('QT -= gui')
        buildable.sources.extend(glob.glob(os.path.join(sources_dir, '*.cpp')))
        buildable.headers.extend(glob.glob(os.path.join(sources_dir, '*.h')))
        buildable.include_dirs.extend(dbus_inc_dirs)
        buildable.library_dirs.extend(dbus_lib_dirs)
        buildable.libraries.extend(dbus_libs)
        buildable.debug = debug

        self.buildables.append(buildable)

    def _add_plugin(self, name, user_name, target_name, target_subdir, debug):
        """ Add a plugin to the project buildables. """

        builder = self.builder

        # Check we have a shared interpreter library.
        if not self.py_pylib_shlib:
            self.progress("The {0} plugin was disabled because a shared Python library couldn't be found.".format(user_name))
            return

        # Where the plugin will (eventually) be installed.
        target_plugin_dir = os.path.join(
                builder.qt_configuration['QT_INSTALL_PLUGINS'], target_subdir)

        # Create the buildable and add it to the builder.
        buildable = Buildable(self, name)
        self.buildables.append(buildable)

        # The platform-specific name of the plugin file.
        if self.py_platform == 'win32':
            target_name = target_name + '.dll'
        elif self.py_platform == 'darwin':
            target_name = 'lib' + target_name + '.dylib'
        else:
            target_name = 'lib' + target_name + '.so'

        # Create the corresponding installable.
        installable = QmakeTargetInstallable(target_name, target_plugin_dir)
        buildable.installables.append(installable)

        # Create the .pro file.
        self.progress(
                "Generating the {0} plugin .pro file".format(user_name))

        root_plugin_dir = os.path.join(self.root_dir, name)

        with open(os.path.join(root_plugin_dir, name + '.pro-in')) as f:
            prj = f.read()

        prj = prj.replace('@QTCONFIG@', 'debug' if debug else 'release')
        prj = prj.replace('@PYINCDIR@',
                builder.qmake_quote(self.py_include_dir))
        prj = prj.replace('@SIPINCDIR@', builder.qmake_quote(self.build_dir))
        prj = prj.replace('@PYLINK@',
                '-L{} -l{}'.format(self.py_pylib_dir, self.py_pylib_lib))
        prj = prj.replace('@PYSHLIB@', self.py_pylib_shlib)
        prj = prj.replace('@QTPLUGINDIR@',
                builder.qmake_quote(target_plugin_dir))

        # Write the .pro file.
        pro_path = os.path.join(buildable.build_dir, name + '.pro')
        pro_f = self.open_for_writing(pro_path)

        pro_f.write(prj)

        pro_f.write('''
INCLUDEPATH += {}
VPATH = {}
'''.format(builder.qmake_quote(root_plugin_dir), builder.qmake_quote(root_plugin_dir)))

        pro_f.write('\n'.join(builder.qmake_settings) + '\n')

        pro_f.close()

    def _check_license(self):
        """ Handle the validation of the PyQt6 license. """

        license_dat = os.path.join(self.root_dir, 'license.dat')

        if os.path.isfile(license_dat):
            ltype = lname = lfile = None

            with open(license_dat) as lf:
                for line in lf:
                    parts = line.split('=')
                    if len(parts) == 2:
                        name, value = parts

                        name = name.strip()
                        value = value.strip()[1:-1]

                        if name == 'LicenseType':
                            ltype = value
                        elif name == 'LicenseName':
                            lname = value
                        elif name == 'LicenseFile':
                            lfile = value

            if lname is None or lfile is None:
                ltype = None
        else:
            ltype = None

        # Default to the GPL.
        if ltype is None:
            ltype = 'GPL'
            lname = "GNU General Public License"
            lfile = 'pyqt-gpl.sip'

        self.progress(
                "This is the {0} version of PyQt {1} (licensed under the {2}) "
                        "for Python {3} on {4}.".format(
                                ltype, self.version_str, lname,
                                sys.version.split()[0], sys.platform))

        # Confirm the license if not already done.
        if not self.confirm_license:
            loptions = """
Type 'L' to view the license.
"""

            sys.stdout.write(loptions)
            sys.stdout.write("""Type 'yes' to accept the terms of the license.
Type 'no' to decline the terms of the license.

""")

            while True:
                sys.stdout.write("Do you accept the terms of the license? ")
                sys.stdout.flush()

                try:
                    resp = sys.stdin.readline()
                except KeyboardInterrupt:
                    raise SystemExit
                except:
                    resp = ""

                resp = resp.strip().lower()

                if resp == "yes":
                    break

                if resp == "no":
                    sys.exit()

                if resp == 'l':
                    os.system('more LICENSE')

        # Check that the license file exists and fix its syntax.
        src_lfile = os.path.join(self.license_dir, lfile)

        if os.path.isfile(src_lfile):
            self.progress("Found the license file '{0}'.".format(lfile))
            self._fix_license(src_lfile,
                    os.path.join(self.build_dir, lfile + '5'))

            # Make sure sip can find the license file.
            self.sip_include_dirs.append(self.build_dir)
        else:
            raise UserException(
                    "Please copy the license file '{0}' to '{1}'".format(lfile,
                            self.license_dir))

    def _fix_license(self, src_lfile, dst_lfile):
        """ Copy the license file and fix it so that it conforms to the SIP v5
        syntax.
        """

        with open(src_lfile) as f:
            f5 = self.open_for_writing(dst_lfile)

            for line in f:
                if line.startswith('%License'):
                    anno_start = line.find('/')
                    anno_end = line.rfind('/')

                    if anno_start < 0 or anno_end < 0 or anno_start == anno_end:
                        f5.close()

                        raise UserException(
                                "'{0}' has missing annotations".format(name))

                    annos = line[anno_start + 1:anno_end].split(', ')
                    annos5 = [anno[0].lower() + anno[1:] for anno in annos]

                    f5.write('%License(')
                    f5.write(', '.join(annos5))
                    f5.write(')\n')
                else:
                    f5.write(line)

            f5.close()


class QAxContainer(PyQtBindings):
    """ The QAxContainer bindings. """

    def __init__(self, project):
        """ Initialise the bindings. """

        super().__init__(project, 'QAxContainer', qmake_QT=['axcontainer'],
                test_headers=['qaxobject.h'], test_statement='new QAxObject()')


class QtBluetooth(PyQtBindings):
    """ The QtBluetooth bindings. """

    def __init__(self, project):
        """ Initialise the bindings. """

        super().__init__(project, 'QtBluetooth', qmake_QT=['bluetooth'],
                test_headers=['qbluetoothaddress.h'],
                test_statement='new QBluetoothAddress()')


class QtCore(PyQtBindings):
    """ The QtCore bindings. """

    def __init__(self, project):
        """ Initialise the bindings. """

        super().__init__(project, 'QtCore', qmake_QT=['-gui'],
                define_macros=['QT_KEYPAD_NAVIGATION'])

    def handle_test_output(self, test_output):
        """ Handle the output from the external test program and return True if
        the bindings are buildable.
        """

        project = self.project

        if not project.qt_shared and test_output[0] == 'shared':
            project.qt_shared = True

            # If permissions are available make sure the static plugins get
            # compiled in.
            if 'PyQt_Permissions' not in test_output:
                self.builder_settings.append('CONFIG += permssions')
                self.builder_settings.append(
                        'QMAKE_INFO_PLIST = ' + self._info_plist())

        return super().handle_test_output(test_output[1:])

    def _info_plist(self):
        """ Create an Info.plist that contains entries for all supported
        permissions and return the absolute name of the file.
        """

        keys = ('NSBluetoothAlwaysUsageDescription',
                'NSCalendarsUsageDescription', 'NSCameraUsageDescription',
                'NSContactsUsageDescription', 'NSLocationUsageDescription',
                'NSMicrophoneUsageDescription')

        content = '\n'.join(
                ['<key>{}</key><string>Dummy</string>'.format(k) for k in keys])

        info_plist = os.path.join(self.project.build_dir, 'Info.plist')

        with open(info_plist, 'w') as f:
            f.write(
'''<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple Computer//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
{}
</dict>
</plist>
'''.format(content))

        return info_plist


class QtDBus(PyQtBindings):
    """ The QtDBus bindings. """

    def __init__(self, project):
        """ Initialise the bindings. """

        super().__init__(project, 'QtDBus', qmake_QT=['dbus', '-gui'],
                test_headers=['qdbusconnection.h'],
                test_statement='QDBusConnection::systemBus()')


class QtDesigner(PyQtBindings):
    """ The QtDesigner bindings. """

    def __init__(self, project):
        """ Initialise the bindings. """

        super().__init__(project, 'QtDesigner', qmake_QT=['designer'],
                test_headers=['QExtensionFactory',
                        'QtUiPlugin/customwidget.h'],
                test_statement='new QExtensionFactory()')

    def is_buildable(self):
        """ Return True if the bindings are buildable. """

        project = self.project

        if not project.qt_shared:
            project.progress(
                    "The QtDesigner bindings are disabled with a static Qt "
                    "installation")
            return False

        return super().is_buildable()


class QtGui(PyQtBindings):
    """ The QtGui bindings. """

    def __init__(self, project):
        """ Initialise the bindings. """

        super().__init__(project, 'QtGui')


class QtHelp(PyQtBindings):
    """ The QtHelp bindings. """

    def __init__(self, project):
        """ Initialise the bindings. """

        super().__init__(project, 'QtHelp', qmake_QT=['help'],
                test_headers=['qhelpengine.h'],
                test_statement='new QHelpEngine("foo")')


class QtLocation(PyQtBindings):
    """ The QtLocation bindings. """

    def __init__(self, project):
        """ Initialise the bindings. """

        super().__init__(project, 'QtLocation', qmake_QT=['location'],
                test_headers=['qplace.h'], test_statement='new QPlace()')


class QtMultimedia(PyQtBindings):
    """ The QtMultimedia bindings. """

    def __init__(self, project):
        """ Initialise the bindings. """

        super().__init__(project, 'QtMultimedia', qmake_QT=['multimedia'],
                test_headers=['QAudioDevice'],
                test_statement='new QAudioDevice()')


class QtMultimediaWidgets(PyQtBindings):
    """ The QtMultimediaWidgets bindings. """

    def __init__(self, project):
        """ Initialise the bindings. """

        super().__init__(project, 'QtMultimediaWidgets',
                qmake_QT=['multimediawidgets'],
                test_headers=['QVideoWidget'],
                test_statement='new QVideoWidget()')


class QtNetwork(PyQtBindings):
    """ The QtNetwork bindings. """

    def __init__(self, project):
        """ Initialise the bindings. """

        super().__init__(project, 'QtNetwork', qmake_QT=['network', '-gui'])


class QtNfc(PyQtBindings):
    """ The QtNfc bindings. """

    def __init__(self, project):
        """ Initialise the bindings. """

        super().__init__(project, 'QtNfc', qmake_QT=['nfc', '-gui'],
                test_headers=['qnearfieldmanager.h'],
                test_statement='new QNearFieldManager()')


class QtOpenGL(PyQtBindings):
    """ The QtOpenGL bindings. """

    def __init__(self, project):
        """ Initialise the bindings. """

        super().__init__(project, 'QtOpenGL', qmake_QT=['opengl'],
                test_headers=['qopengldebug.h'],
                test_statement='new QOpenGLDebugLogger()')


class QtOpenGLWidgets(PyQtBindings):
    """ The QtOpenGLWidgets bindings. """

    def __init__(self, project):
        """ Initialise the bindings. """

        super().__init__(project, 'QtOpenGLWidgets',
                qmake_QT=['openglwidgets'], test_headers=['qopenglwidget.h'],
                test_statement='new QOpenGLWidget()')


class QtPdf(PyQtBindings):
    """ The QtPdf bindings. """

    def __init__(self, project):
        """ Initialise the bindings. """

        super().__init__(project, 'QtPdf', qmake_QT=['pdf'],
                test_headers=['qpdfdocument.h'],
                test_statement='new QPdfDocument(nullptr)')


class QtPdfWidgets(PyQtBindings):
    """ The QtPdfWidgets bindings. """

    def __init__(self, project):
        """ Initialise the bindings. """

        super().__init__(project, 'QtPdfWidgets', qmake_QT=['pdfwidgets'],
                test_headers=['qpdfview.h'],
                test_statement='new QPdfView(nullptr)')


class QtPositioning(PyQtBindings):
    """ The QtPositioning bindings. """

    def __init__(self, project):
        """ Initialise the bindings. """

        super().__init__(project, 'QtPositioning', qmake_QT=['positioning'],
                test_headers=['qgeoaddress.h'],
                test_statement='new QGeoAddress()')


class QtPrintSupport(PyQtBindings):
    """ The QtPrintSupport bindings. """

    def __init__(self, project):
        """ Initialise the bindings. """

        super().__init__(project, 'QtPrintSupport', qmake_QT=['printsupport'])


class QtQml(PyQtBindings):
    """ The QtQml bindings. """

    def __init__(self, project):
        """ Initialise the bindings. """

        super().__init__(project, 'QtQml', qmake_QT=['qml'],
                test_headers=['qjsengine.h'], test_statement='new QJSEngine()')


class QtQuick(PyQtBindings):
    """ The QtQuick bindings. """

    def __init__(self, project):
        """ Initialise the bindings. """

        super().__init__(project, 'QtQuick', qmake_QT=['quick'],
                test_headers=['qquickwindow.h'],
                test_statement='new QQuickWindow()')


class QtQuick3D(PyQtBindings):
    """ The QtQuick3D bindings. """

    def __init__(self, project):
        """ Initialise the bindings. """

        super().__init__(project, 'QtQuick3D', qmake_QT=['quick3d'],
                test_headers=['qquick3d.h'],
                test_statement='QQuick3D::idealSurfaceFormat()')


class QtQuickWidgets(PyQtBindings):
    """ The QtQuickWidgets bindings. """

    def __init__(self, project):
        """ Initialise the bindings. """

        super().__init__(project, 'QtQuickWidgets', qmake_QT=['quickwidgets'],
                test_headers=['qquickwidget.h'],
                test_statement='new QQuickWidget()')


class QtRemoteObjects(PyQtBindings):
    """ The QtRemoteObjects bindings. """

    def __init__(self, project):
        """ Initialise the bindings. """

        super().__init__(project, 'QtRemoteObjects',
                qmake_QT=['remoteobjects', '-gui'],
                test_headers=['qtremoteobjectsversion.h'],
                test_statement='const char *v = QTREMOTEOBJECTS_VERSION_STR')


class QtSensors(PyQtBindings):
    """ The QtSensors bindings. """

    def __init__(self, project):
        """ Initialise the bindings. """

        super().__init__(project, 'QtSensors', qmake_QT=['sensors'],
                test_headers=['qsensor.h'],
                test_statement='new QSensor(QByteArray())')


class QtSerialPort(PyQtBindings):
    """ The QtSerialPort bindings. """

    def __init__(self, project):
        """ Initialise the bindings. """

        super().__init__(project, 'QtSerialPort', qmake_QT=['serialport'],
                test_headers=['qserialport.h'],
                test_statement='new QSerialPort()')


class QtSpatialAudio(PyQtBindings):
    """ The QtSpatialAudio bindings. """

    def __init__(self, project):
        """ Initialise the bindings. """

        super().__init__(project, 'QtSpatialAudio', qmake_QT=['spatialaudio'],
                test_headers=['qaudioengine.h'],
                test_statement='new QAudioEngine()')


class QtSql(PyQtBindings):
    """ The QtSql bindings. """

    def __init__(self, project):
        """ Initialise the bindings. """

        super().__init__(project, 'QtSql', qmake_QT=['sql', 'widgets'],
                test_headers=['qsqldatabase.h'],
                test_statement='new QSqlDatabase()')


class QtSvg(PyQtBindings):
    """ The QtSvg bindings. """

    def __init__(self, project):
        """ Initialise the bindings. """

        super().__init__(project, 'QtSvg', qmake_QT=['svg'],
                test_headers=['qsvgrenderer.h'],
                test_statement='new QSvgRenderer()')


class QtSvgWidgets(PyQtBindings):
    """ The QtSvgWidgets bindings. """

    def __init__(self, project):
        """ Initialise the bindings. """

        super().__init__(project, 'QtSvgWidgets', qmake_QT=['svgwidgets'],
                test_headers=['qsvgwidget.h'],
                test_statement='new QSvgWidget()')


class QtTest(PyQtBindings):
    """ The QtTest bindings. """

    def __init__(self, project):
        """ Initialise the bindings. """

        super().__init__(project, 'QtTest', qmake_QT=['testlib', 'widgets'],
                test_headers=['QtTest'], test_statement='QTest::qSleep(0)')


class QtTextToSpeech(PyQtBindings):
    """ The QtTextToSpeech bindings. """

    def __init__(self, project):
        """ Initialise the bindings. """

        super().__init__(project, 'QtTextToSpeech',
                qmake_QT=['texttospeech', '-gui', 'qmlintegration'],
                test_headers=['QTextToSpeech'],
                test_statement='new QTextToSpeech()')


class QtWebChannel(PyQtBindings):
    """ The QtWebChannel bindings. """

    def __init__(self, project):
        """ Initialise the bindings. """

        super().__init__(project, 'QtWebChannel',
                qmake_QT=['webchannel', 'network', '-gui'],
                test_headers=['qwebchannel.h'],
                test_statement='new QWebChannel()')


class QtWebSockets(PyQtBindings):
    """ The QtWebSockets bindings. """

    def __init__(self, project):
        """ Initialise the bindings. """

        super().__init__(project, 'QtWebSockets',
                qmake_QT=['websockets', '-gui'], test_headers=['qwebsocket.h'],
                test_statement='new QWebSocket()')


class QtWidgets(PyQtBindings):
    """ The QtWidgets bindings. """

    def __init__(self, project):
        """ Initialise the bindings. """

        super().__init__(project, 'QtWidgets', qmake_QT=['widgets'],
                test_headers=['qwidget.h'], test_statement='new QWidget()')


class QtXml(PyQtBindings):
    """ The QtXml bindings. """

    def __init__(self, project):
        """ Initialise the bindings. """

        super().__init__(project, 'QtXml', qmake_QT=['xml', '-gui'],
                test_headers=['qdom.h'], test_statement='new QDomDocument()')
