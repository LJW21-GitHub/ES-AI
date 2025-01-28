// This is the initialisation support code for the QtQml module.
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

#include "qpyqml_api.h"
#include "qpyqmllistproperty.h"
#include "qpyqmllistpropertywrapper.h"
#include "qpyqmlobject.h"

#include "sipAPIQtQml.h"


// Imports from QtCore.
pyqt6_qtqml_err_print_t pyqt6_qtqml_err_print;


// Perform any required initialisation.
void qpyqml_post_init(PyObject *module_dict)
{
    // QtCore imports.
    pyqt6_qtqml_err_print = (pyqt6_qtqml_err_print_t)sipImportSymbol(
            "pyqt6_err_print");
    Q_ASSERT(pyqt6_qtqml_err_print);

    // Register the our list type.  Note that Q_DECLARE_METATYPE doesn't seem
    // to work.
    qMetaTypeId<QQmlListProperty<QObject> >();

    // Initialise the QQmlListProperty type.
    if (!qpyqml_QQmlListProperty_init_type())
        Py_FatalError("PyQt6.QtQml: Failed to initialise QQmlListProperty type");

    // Create the only instance and add it to the module dictionary.
    PyObject *inst = PyObject_CallFunction(
            (PyObject *)qpyqml_QQmlListProperty_TypeObject, "s",
            "QQmlListProperty<QObject>");

    if (!inst)
        Py_FatalError("PyQt6.QtQml: Failed to create QQmlListProperty instance");

    if (PyDict_SetItemString(module_dict, "QQmlListProperty", inst) < 0)
        Py_FatalError("PyQt6.QtQml: Failed to set QQmlListProperty instance");

    // Initialise the private QQmlListPropertyWrapper type.
    if (!qpyqml_QQmlListPropertyWrapper_init_type())
        Py_FatalError("PyQt6.QtQml: Failed to initialise QQmlListPropertyWrapper type");

    // Register the proxy resolver.
    if (sipRegisterProxyResolver(sipType_QObject, QPyQmlObjectProxy::resolveProxy) < 0)
        Py_FatalError("PyQt6.QtQml: Failed to register proxy resolver");
}
