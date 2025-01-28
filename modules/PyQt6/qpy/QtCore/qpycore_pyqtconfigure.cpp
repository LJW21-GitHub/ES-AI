// This implements the helper for QObject.pyqtConfigure().
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

#include <QByteArray>
#include <QMetaObject>
#include <QObject>
#include <QVariant>

#include "qpycore_api.h"
#include "qpycore_chimera.h"
#include "qpycore_misc.h"
#include "qpycore_pyqtboundsignal.h"

#include "sipAPIQtCore.h"


// The result of handling a keyword argument.
enum ArgStatus
{
    AsError,
    AsHandled,
    AsUnknown
};


static ArgStatus handle_argument(PyObject *self, QObject *qobj,
        PyObject *name_obj, PyObject *value_obj);


// This is the helper for QObject.pyqtConfigure().
PyObject *qpycore_pyqtconfigure(PyObject *self, PyObject *args, PyObject *kwds)
{
    // Check there are no positional arguments.
    if (PyTuple_Size(args) > 0)
    {
        PyErr_SetString(PyExc_TypeError,
                "QObject.pyqtConfigure() has no positional arguments");
        return 0;
    }

    // Get the QObject self.
    QObject *qobj = reinterpret_cast<QObject *>(
            sipGetCppPtr((sipSimpleWrapper *)self, sipType_QObject));

    if (!qobj)
        return 0;

    PyObject *name_obj, *value_obj;
    Py_ssize_t pos = 0;

    while (PyDict_Next(kwds, &pos, &name_obj, &value_obj))
    {
        ArgStatus as = handle_argument(self, qobj, name_obj, value_obj);

        if (as == AsError)
            return 0;

        if (as == AsUnknown)
        {
            PyErr_Format(PyExc_AttributeError,
                    "'%S' is not the name of a Qt property or signal",
                    name_obj);

            return 0;
        }
    }

    Py_INCREF(Py_None);
    return Py_None;
}


// This is the helper for the QObject %FinalisationCode.
int qpycore_qobject_finalisation(PyObject *self, QObject *qobj, PyObject *kwds,
        PyObject **updated_kwds)
{
    // This will ensure that the type's QMetaObject has been constructed.
    qobj->metaObject();

    // Handle the trivial case.
    if (!kwds)
        return 0;

    // The dict we will be removing handled arguments from, 0 if it needs to be
    // created.
    PyObject *unused = (updated_kwds ? 0 : kwds);

    PyObject *name_obj, *value_obj;
    Py_ssize_t pos = 0;

    while (PyDict_Next(kwds, &pos, &name_obj, &value_obj))
    {
        ArgStatus as = handle_argument(self, qobj, name_obj, value_obj);

        if (as == AsError)
            return -1;

        if (as == AsHandled)
        {
            if (!unused)
            {
                unused = PyDict_Copy(kwds);

                if (!unused)
                    return -1;

                *updated_kwds = unused;
            }

            if (PyDict_DelItem(unused, name_obj) < 0)
            {
                if (updated_kwds)
                    Py_DECREF(unused);

                return -1;
            }
        }
    }

    return 0;
}


// Handle a single keyword argument.
static ArgStatus handle_argument(PyObject *self, QObject *qobj,
        PyObject *name_obj, PyObject *value_obj)
{
    const QMetaObject *mo = qobj->metaObject();

    // Get the name encoded name.
    QByteArray enc_name = qpycore_convert_ASCII(name_obj);

    if (enc_name.isNull())
        return AsError;

    // See if it is a property.
    int idx = mo->indexOfProperty(enc_name.constData());

    if (idx >= 0)
    {
        QMetaProperty prop = mo->property(idx);

        // A negative type means a QVariant property.
        if (prop.userType() >= 0)
        {
            bool ok;
            QVariant value = qpycore_PyObject_AsQVariant(prop, value_obj, &ok);

            if (!ok)
                return AsError;

            qobj->setProperty(enc_name.constData(), value);
        }
        else
        {
            int value_state, iserr = 0;

            QVariant *value = reinterpret_cast<QVariant *>(
                    sipForceConvertToType(value_obj, sipType_QVariant, 0,
                            SIP_NOT_NONE, &value_state, &iserr));

            if (iserr)
                return AsError;

            qobj->setProperty(enc_name.constData(), *value);

            sipReleaseType(value, sipType_QVariant, value_state);
        }
    }
    else
    {
        bool unknown = true;

        // See if it is a signal.
        PyObject *sig = PyObject_GetAttr(self, name_obj);

        if (sig)
        {
            if (PyObject_TypeCheck(sig, qpycore_pyqtBoundSignal_TypeObject))
            {
                static PyObject *connect_obj = NULL;

                if (!connect_obj)
                {
                    connect_obj = PyUnicode_FromString("connect");

                    if (!connect_obj)
                    {
                        Py_DECREF(sig);
                        return AsError;
                    }
                }

                // Connect the slot.
                PyObject *res = PyObject_CallMethodObjArgs(sig, connect_obj,
                        value_obj, 0);

                if (!res)
                {
                    Py_DECREF(sig);
                    return AsError;
                }

                Py_DECREF(res);

                unknown = false;
            }

            Py_DECREF(sig);
        }

        if (unknown)
        {
            PyErr_Clear();
            return AsUnknown;
        }
    }

    return AsHandled;
}
