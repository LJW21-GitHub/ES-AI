// This is the post-initialisation support code for the QtCore module.
//
// Copyright (c) 2024 Riverbank Computing Limited <info@riverbankcomputing.com>
// 
// This file is part of PyQt6.
// 
// This file may be used under the terms of the GNU General Public License
// version 3.0 as published by the Free Software Foundation and appearing in
// the file LICENSE included in the packaging of this file.  Please review the
// following information to ensure the GNU General Public License version 3.0
// requirements will be met: http://www.gnu.org/copyleft/gpl.html.
// 
// If you do not wish to use this file under the terms of the GPL version 3.0
// then you may purchase a commercial license.  For more information contact
// info@riverbankcomputing.com.
// 
// This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
// WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.


#include <Python.h>

#include <QRecursiveMutex>

#include "qpycore_api.h"
#include "qpycore_event_handlers.h"
#include "qpycore_objectified_strings.h"
#include "qpycore_pyqtboundsignal.h"
#include "qpycore_pyqtmethodproxy.h"
#include "qpycore_pyqtproperty.h"
#include "qpycore_pyqtpyobject.h"
#include "qpycore_pyqtsignal.h"
#include "qpycore_pyqtslotproxy.h"
#include "qpycore_types.h"

#include "sipAPIQtCore.h"


// The objectified strings.
PyObject *qpycore_dunder_name;
PyObject *qpycore_dunder_mro;
PyObject *qpycore_dunder_pyqtsignature;


// Perform any required post-initialisation.
void qpycore_post_init(PyObject *module_dict)
{
    // Register the event handlers.
    qpycore_register_event_handlers();

    // Initialise the pyqtProperty type and add it to the module dictionary.
    if (!qpycore_pyqtProperty_init_type())
        Py_FatalError("PyQt6.QtCore: Failed to initialise pyqtProperty type");

    if (PyDict_SetItemString(module_dict, "pyqtProperty",
                (PyObject *)qpycore_pyqtProperty_TypeObject) < 0)
        Py_FatalError("PyQt6.QtCore: Failed to set pyqtProperty type");

    // Initialise the pyqtSignal type and add it to the module dictionary.
    if (!qpycore_pyqtSignal_init_type())
        Py_FatalError("PyQt6.QtCore: Failed to initialise pyqtSignal type");

    if (PyDict_SetItemString(module_dict, "pyqtSignal",
                (PyObject *)qpycore_pyqtSignal_TypeObject) < 0)
        Py_FatalError("PyQt6.QtCore: Failed to set pyqtSignal type");

    // Initialise the pyqtBoundSignal type and add it to the module dictionary.
    if (!qpycore_pyqtBoundSignal_init_type())
        Py_FatalError("PyQt6.QtCore: Failed to initialise pyqtBoundSignal type");

    if (PyDict_SetItemString(module_dict, "pyqtBoundSignal",
                (PyObject *)qpycore_pyqtBoundSignal_TypeObject) < 0)
        Py_FatalError("PyQt6.QtCore: Failed to set pyqtBoundSignal type");

    // Initialise the private pyqtMethodProxy type.
    if (!qpycore_pyqtMethodProxy_init_type())
        Py_FatalError("PyQt6.QtCore: Failed to initialise pyqtMethodProxy type");

    // Register the C++ type that wraps Python objects.
    qRegisterMetaType<PyQt_PyObject>("PyQt_PyObject");

    // Register the lazy attribute getter.
    if (sipRegisterAttributeGetter(sipType_QObject, qpycore_get_lazy_attr) < 0)
        Py_FatalError("PyQt6.QtCore: Failed to register attribute getter");

    // Objectify some strings.
    qpycore_dunder_name = PyUnicode_FromString("__name__");

    if (!qpycore_dunder_name)
        Py_FatalError("PyQt6.QtCore: Failed to objectify '__name__'");

    qpycore_dunder_mro = PyUnicode_FromString("__mro__");

    if (!qpycore_dunder_mro)
        Py_FatalError("PyQt6.QtCore: Failed to objectify '__mro__'");

    qpycore_dunder_pyqtsignature = PyUnicode_FromString("__pyqtSignature__");

    if (!qpycore_dunder_pyqtsignature)
        Py_FatalError("PyQt6.QtCore: Failed to objectify '__pyqtSignature__'");

    // Create the mutex that serialises access to the slot proxies.  We don't
    // use a statically initialised one because Qt needs some things to be
    // initialised first (at least for Windows) and this is the only way to
    // guarantee things are done in the right order.
    PyQtSlotProxy::mutex = new QRecursiveMutex();

    // Load the embedded qt.conf file if there is a bundled copy of Qt.
    if (!qpycore_qt_conf())
        Py_FatalError("PyQt6.QtCore: Unable to embed qt.conf");
}
