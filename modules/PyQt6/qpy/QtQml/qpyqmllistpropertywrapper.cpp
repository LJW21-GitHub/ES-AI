// This contains the implementation of the QQmlListPropertyWrapper type.
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

#include <sip.h>

#include "qpyqmllistpropertywrapper.h"


// The type object.
PyTypeObject *qpyqml_QQmlListPropertyWrapper_TypeObject;


// Forward declarations.
extern "C" {
static void QQmlListPropertyWrapper_dealloc(PyObject *self);
static Py_ssize_t QQmlListPropertyWrapper_sq_length(PyObject *self);
static PyObject *QQmlListPropertyWrapper_sq_concat(PyObject *self,
        PyObject *other);
static PyObject *QQmlListPropertyWrapper_sq_repeat(PyObject *self,
        Py_ssize_t count);
static PyObject *QQmlListPropertyWrapper_sq_item(PyObject *self, Py_ssize_t i);
static int QQmlListPropertyWrapper_sq_ass_item(PyObject *self, Py_ssize_t i,
        PyObject *value);
static int QQmlListPropertyWrapper_sq_contains(PyObject *self,
        PyObject *value);
static PyObject *QQmlListPropertyWrapper_sq_inplace_concat(PyObject *self,
        PyObject *other);
static PyObject *QQmlListPropertyWrapper_sq_inplace_repeat(PyObject *self,
        Py_ssize_t count);
}

static PyObject *get_list(PyObject *self);


// Define the slots.
static PyType_Slot qpyqml_QQmlListPropertyWrapper_Slots[] = {
    {Py_tp_dealloc,     (void *)QQmlListPropertyWrapper_dealloc},
    {Py_sq_length,      (void *)QQmlListPropertyWrapper_sq_length},
    {Py_sq_concat,      (void *)QQmlListPropertyWrapper_sq_concat},
    {Py_sq_repeat,      (void *)QQmlListPropertyWrapper_sq_repeat},
    {Py_sq_item,        (void *)QQmlListPropertyWrapper_sq_item},
    {Py_sq_ass_item,    (void *)QQmlListPropertyWrapper_sq_ass_item},
    {Py_sq_contains,    (void *)QQmlListPropertyWrapper_sq_contains},
    {Py_sq_inplace_concat,  (void *)QQmlListPropertyWrapper_sq_inplace_concat},
    {Py_sq_inplace_repeat,  (void *)QQmlListPropertyWrapper_sq_inplace_repeat},
    {0,                 0}
};


// Define the type.
static PyType_Spec qpyqml_QQmlListPropertyWrapper_Spec = {
    "PyQt6.QtQml.QQmlListPropertyWrapper",
    sizeof (qpyqml_QQmlListPropertyWrapper),
    0,
    Py_TPFLAGS_DEFAULT,
    qpyqml_QQmlListPropertyWrapper_Slots
};


// Initialise the type and return true if there was no error.
bool qpyqml_QQmlListPropertyWrapper_init_type()
{
    qpyqml_QQmlListPropertyWrapper_TypeObject = (PyTypeObject *)PyType_FromSpec(
            &qpyqml_QQmlListPropertyWrapper_Spec);

    return qpyqml_QQmlListPropertyWrapper_TypeObject;
}


// Create the wrapper object.
PyObject *qpyqml_QQmlListPropertyWrapper_New(QQmlListProperty<QObject> *prop,
        PyObject *list)
{
    qpyqml_QQmlListPropertyWrapper *obj;

    obj = PyObject_New(qpyqml_QQmlListPropertyWrapper,
            qpyqml_QQmlListPropertyWrapper_TypeObject);

    if (!obj)
        return 0;

    obj->qml_list_property = prop;
    obj->py_list = list;

    return (PyObject *)obj;
}


// The type dealloc slot.
static void QQmlListPropertyWrapper_dealloc(PyObject *self)
{
    delete ((qpyqml_QQmlListPropertyWrapper *)self)->qml_list_property;

    PyObject_Del(self);
}


// Return the underlying list.  Return 0 and raise an exception if there wasn't
// one.
static PyObject *get_list(PyObject *self)
{
    PyObject *list = ((qpyqml_QQmlListPropertyWrapper *)self)->py_list;

    if (!list)
    {
        PyErr_SetString(PyExc_TypeError,
                "there is no object bound to QQmlListProperty");
        return 0;
    }

    // Make sure it has sequence methods.
    if (!PySequence_Check(list))
    {
        PyErr_SetString(PyExc_TypeError,
                "object bound to QQmlListProperty is not a sequence");
        return 0;
    }

    return list;
}


// The proxy sequence methods.

static Py_ssize_t QQmlListPropertyWrapper_sq_length(PyObject *self)
{
    PyObject *list = get_list(self);

    if (!list)
        return -1;

    return PySequence_Size(list);
}

static PyObject *QQmlListPropertyWrapper_sq_concat(PyObject *self,
        PyObject *other)
{
    PyObject *list = get_list(self);

    if (!list)
        return 0;

    return PySequence_Concat(list, other);
}

static PyObject *QQmlListPropertyWrapper_sq_repeat(PyObject *self,
        Py_ssize_t count)
{
    PyObject *list = get_list(self);

    if (!list)
        return 0;

    return PySequence_Repeat(list, count);
}

static PyObject *QQmlListPropertyWrapper_sq_item(PyObject *self, Py_ssize_t i)
{
    PyObject *list = get_list(self);

    if (!list)
        return 0;

    return PySequence_GetItem(list, i);
}

static int QQmlListPropertyWrapper_sq_ass_item(PyObject *self, Py_ssize_t i,
        PyObject *value)
{
    PyObject *list = get_list(self);

    if (!list)
        return -1;

    return PySequence_SetItem(list, i, value);
}

static int QQmlListPropertyWrapper_sq_contains(PyObject *self, PyObject *value)
{
    PyObject *list = get_list(self);

    if (!list)
        return -1;

    return PySequence_Contains(list, value);
}

static PyObject *QQmlListPropertyWrapper_sq_inplace_concat(PyObject *self,
        PyObject *other)
{
    PyObject *list = get_list(self);

    if (!list)
        return 0;

    return PySequence_InPlaceConcat(list, other);
}

static PyObject *QQmlListPropertyWrapper_sq_inplace_repeat(PyObject *self,
        Py_ssize_t count)
{
    PyObject *list = get_list(self);

    if (!list)
        return 0;

    return PySequence_InPlaceRepeat(list, count);
}
