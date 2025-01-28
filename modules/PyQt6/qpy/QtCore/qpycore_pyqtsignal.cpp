// This contains the implementation of the pyqtSignal type.
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

#include <QtGlobal>
#include <QByteArray>
#include <QMetaObject>

#include "qpycore_chimera.h"
#include "qpycore_misc.h"
#include "qpycore_pyqtboundsignal.h"
#include "qpycore_pyqtsignal.h"


// The type object.
PyTypeObject *qpycore_pyqtSignal_TypeObject;


// Forward declarations.
extern "C" {
static PyObject *pyqtSignal_call(PyObject *self, PyObject *args, PyObject *kw);
static void pyqtSignal_dealloc(PyObject *self);
static PyObject *pyqtSignal_descr_get(PyObject *self, PyObject *obj,
        PyObject *type);
static int pyqtSignal_init(PyObject *self, PyObject *args, PyObject *kwd_args);
static PyObject *pyqtSignal_repr(PyObject *self);
static PyObject *pyqtSignal_mp_subscript(PyObject *self, PyObject *subscript);
static PyObject *pyqtSignal_get_doc(PyObject *self, void *);
static PyObject *pyqtSignal_get_signatures(PyObject *self, void *);
}

static int init_signal_from_types(qpycore_pyqtSignal *ps, const char *name,
        const QList<QByteArray> *parameter_names, int revision,
        PyObject *types);
static void append_overload(qpycore_pyqtSignal *ps);
static bool is_signal_name(const char *sig, const QByteArray &name);


// Doc-strings.
PyDoc_STRVAR(pyqtSignal_doc,
"pyqtSignal(*types, name: str = ..., revision: int = ..., arguments: Sequence = ...) -> PYQT_SIGNAL\n"
"\n"
"types is normally a sequence of individual types.  Each type is either a\n"
"type object or a string that is the name of a C++ type.  Alternatively\n"
"each type could itself be a sequence of types each describing a different\n"
"overloaded signal.\n"
"name is the optional C++ name of the signal.  If it is not specified then\n"
"the name of the class attribute that is bound to the signal is used.\n"
"revision is the optional revision of the signal that is exported to QML.\n"
"If it is not specified then 0 is used.\n"
"arguments is the optional sequence of the names of the signal's arguments.\n");

PyDoc_STRVAR(pyqtSignal_signatures_doc,
"The signatures of each of the overloaded signals");


// The getters/setters.
static PyGetSetDef pyqtSignal_getset[] = {
    {const_cast<char *>("__doc__"), pyqtSignal_get_doc, NULL, NULL, NULL},
    {const_cast<char *>("signatures"), pyqtSignal_get_signatures, NULL,
            const_cast<char *>(pyqtSignal_signatures_doc), NULL},
    {NULL, NULL, NULL, NULL, NULL}
};


// Define the slots.
static PyType_Slot qpycore_pyqtSignal_Slots[] = {
    {Py_tp_new,         (void *)PyType_GenericNew},
    {Py_tp_init,        (void *)pyqtSignal_init},
    {Py_tp_dealloc,     (void *)pyqtSignal_dealloc},
    {Py_tp_doc,         (void *)pyqtSignal_doc},
    {Py_tp_repr,        (void *)pyqtSignal_repr},
    {Py_tp_call,        (void *)pyqtSignal_call},
    {Py_tp_descr_get,   (void *)pyqtSignal_descr_get},
    {Py_mp_subscript,   (void *)pyqtSignal_mp_subscript},
    {Py_tp_getset,      pyqtSignal_getset},
    {0,                 0}
};


// Define the type.
static PyType_Spec qpycore_pyqtSignal_Spec = {
    "PyQt6.QtCore.pyqtSignal",
    sizeof (qpycore_pyqtSignal),
    0,
    Py_TPFLAGS_DEFAULT,
    qpycore_pyqtSignal_Slots
};


// The __doc__ getter.
static PyObject *pyqtSignal_get_doc(PyObject *self, void *)
{
    qpycore_pyqtSignal *ps = (qpycore_pyqtSignal *)self;

    // Make sure we have the default signal.
    ps = ps->default_signal;

    QByteArray doc;

    // Get any docstrings from any non-signal overloads.
    if (ps->non_signals && ps->non_signals->ml_doc)
    {
        doc.append('\n');
        doc.append(ps->non_signals->ml_doc);
    }

    // Get any docstrings from the signals.
    do
    {
        const char *docstring = ps->docstring;

        if (docstring)
        {
            if (*docstring == '\1')
                ++docstring;

            doc.append('\n');
            doc.append(docstring);
            doc.append(" [signal]");
        }

        ps = ps->next;
    }
    while (ps);

    if (doc.isEmpty())
    {
        Py_INCREF(Py_None);
        return Py_None;
    }

    return PyUnicode_FromString(doc.constData() + 1);
}


// The 'signatures' getter.
static PyObject *pyqtSignal_get_signatures(PyObject *self, void *)
{
    qpycore_pyqtSignal *head = ((qpycore_pyqtSignal *)self)->default_signal;
    qpycore_pyqtSignal *ps;
    PyObject *sigs;
    Py_ssize_t sig_nr;

    // Count the number of overloads.
    for (sig_nr = 0, ps = head; ps; ps = ps->next, ++sig_nr)
        ;

    sigs = PyTuple_New(sig_nr);
    if (!sigs)
        return 0;

    for (sig_nr = 0, ps = head; ps; ps = ps->next, ++sig_nr)
    {
        PyObject *sig = PyUnicode_FromString(
                ps->parsed_signature->signature.constData() + 1);

        if (!sig || PyTuple_SetItem(sigs, sig_nr, sig))
        {
            Py_DECREF(sigs);
            return 0;
        }
    }

    return sigs;
}


// The type repr slot.
static PyObject *pyqtSignal_repr(PyObject *self)
{
    qpycore_pyqtSignal *ps = (qpycore_pyqtSignal *)self;

    return PyUnicode_FromFormat("<unbound PYQT_SIGNAL %s>",
            ps->parsed_signature->signature.constData() + 1);
}


// The type call slot.
static PyObject *pyqtSignal_call(PyObject *self, PyObject *args, PyObject *kw)
{
    return qpycore_call_signal_overload((qpycore_pyqtSignal *)self, 0, args,
            kw);
}


// The type dealloc slot.
static void pyqtSignal_dealloc(PyObject *self)
{
    qpycore_pyqtSignal *ps = (qpycore_pyqtSignal *)self;

    delete ps->parsed_signature;

    if (ps->parameter_names)
        delete ps->parameter_names;

    // If we are the default then we own the overloads references.
    if (ps == ps->default_signal)
    {
        qpycore_pyqtSignal *next = ps->next;

        while (next)
        {
            ps = next;
            next = ps->next;

            Py_DECREF((PyObject *)ps);
        }
    }

    PyObject_Del(self);
}


// The type descriptor get slot.
static PyObject *pyqtSignal_descr_get(PyObject *self, PyObject *obj,
        PyObject *)
{
    qpycore_pyqtSignal *ps = (qpycore_pyqtSignal *)self;

    // Return the unbound signal if there is nothing to bind it to.
    if (obj == NULL || obj == Py_None)
    {
        Py_INCREF(self);
        return self;
    }

    // Get the QObject.
    int is_err = 0;
    void *qobject = sipForceConvertToType(obj, sipType_QObject, 0,
            SIP_NO_CONVERTORS, 0, &is_err);

    if (is_err)
        return 0;

    // Return the bound signal.
    return qpycore_pyqtBoundSignal_New(ps, obj,
            reinterpret_cast<QObject *>(qobject));
}


// The type init slot.
static int pyqtSignal_init(PyObject *self, PyObject *args, PyObject *kwd_args)
{
    qpycore_pyqtSignal *ps = (qpycore_pyqtSignal *)self;

    // Get the keyword arguments.
    PyObject *name_obj = 0;
    const char *name = 0;
    int revision = 0;
    QList<QByteArray> *parameter_names = 0;

    if (kwd_args)
    {
        Py_ssize_t pos = 0;
        PyObject *key, *value;

        while (PyDict_Next(kwd_args, &pos, &key, &value))
        {
            if (PyUnicode_CompareWithASCIIString(key, "name") == 0)
            {
                name_obj = value;
                name = sipString_AsASCIIString(&name_obj);

                if (!name)
                {
                    PyErr_Format(PyExc_TypeError,
                            "signal 'name' must be a str, not %s",
                            sipPyTypeName(Py_TYPE(value)));

                    return -1;
                }
            }
            else if (PyUnicode_CompareWithASCIIString(key, "revision") == 0)
            {
                revision = sipLong_AsInt(value);

                if (PyErr_Occurred())
                {
                    if (PyErr_ExceptionMatches(PyExc_TypeError))
                        PyErr_Format(PyExc_TypeError,
                                "signal 'revision' must be an int, not %s",
                                sipPyTypeName(Py_TYPE(value)));

                    Py_XDECREF(name_obj);
                    return -1;
                }
            }
            else if (PyUnicode_CompareWithASCIIString(key, "arguments") == 0)
            {
                bool ok = true;

                if (PySequence_Check(value))
                {
                    Py_ssize_t len = PySequence_Size(value);

                    parameter_names = new QList<QByteArray>;

                    for (Py_ssize_t i = 0; i < len; ++i)
                    {
                        PyObject *py_attr = PySequence_GetItem(value, i);

                        if (!py_attr)
                        {
                            ok = false;
                            break;
                        }

                        QByteArray attr = qpycore_convert_ASCII(py_attr);
                        Py_DECREF(py_attr);

                        if (attr.isNull())
                        {
                            ok = false;
                            break;
                        }

                        parameter_names->append(attr);
                    }
                }
                else
                {
                    ok = false;
                }

                if (!ok)
                {
                    PyErr_Format(PyExc_TypeError,
                            "signal 'attribute_names' must be a sequence of str, not %s",
                            sipPyTypeName(Py_TYPE(value)));

                    if (parameter_names)
                        delete parameter_names;

                    Py_XDECREF(name_obj);
                    return -1;
                }
            }
            else
            {
                PyErr_Format(PyExc_TypeError,
                        "pyqtSignal() got an unexpected keyword argument '%U'",
                        key);

                Py_XDECREF(name_obj);
                return -1;
            }
        }
    }

    // If there is at least one argument and it is a sequence then assume all
    // arguments are sequences.  Unfortunately a string is also a sequence so
    // check for tuples and lists explicitly.
    if (PyTuple_Size(args) > 0 && (PyTuple_Check(PyTuple_GetItem(args, 0)) || PyList_Check(PyTuple_GetItem(args, 0))))
    {
        for (Py_ssize_t i = 0; i < PyTuple_Size(args); ++i)
        {
            PyObject *types = PySequence_Tuple(PyTuple_GetItem(args, i));

            if (!types)
            {
                PyErr_SetString(PyExc_TypeError,
                        "pyqtSignal() argument expected to be sequence of types");

                if (name)
                {
                    Py_DECREF(name_obj);
                }

                return -1;
            }

            int rc;

            if (i == 0)
            {
                // The first is the default.
                rc = init_signal_from_types(ps, name, parameter_names,
                        revision, types);
            }
            else
            {
                qpycore_pyqtSignal *overload = (qpycore_pyqtSignal *)PyType_GenericNew(qpycore_pyqtSignal_TypeObject, 0, 0);

                if (!overload)
                {
                    rc = -1;
                }
                else if ((rc = init_signal_from_types(overload, name, 0, revision, types)) < 0)
                {
                    Py_DECREF((PyObject *)overload);
                }
                else
                {
                    overload->default_signal = ps;
                    append_overload(overload);
                }
            }

            Py_DECREF(types);

            if (rc < 0)
            {
                if (name)
                {
                    Py_DECREF(name_obj);
                }

                return -1;
            }
        }
    }
    else if (init_signal_from_types(ps, name, parameter_names, revision, args) < 0)
    {
        if (name)
        {
            Py_DECREF(name_obj);
        }

        return -1;
    }

    if (name)
    {
        Py_DECREF(name_obj);
    }

    return 0;
}


// The mapping subscript slot.
static PyObject *pyqtSignal_mp_subscript(PyObject *self, PyObject *subscript)
{
    qpycore_pyqtSignal *ps = (qpycore_pyqtSignal *)self;

    PyObject *sig = (PyObject *)qpycore_find_signal(ps, subscript,
            "an unbound signal type argument");

    Py_XINCREF(sig);

    return sig;
}


// Initialise the type and return true if there was no error.
bool qpycore_pyqtSignal_init_type()
{
    qpycore_pyqtSignal_TypeObject = (PyTypeObject *)PyType_FromSpec(
            &qpycore_pyqtSignal_Spec);

    return qpycore_pyqtSignal_TypeObject;
}


// Create a new signal instance.
qpycore_pyqtSignal *qpycore_pyqtSignal_New(const char *signature, bool *fatal)
{
    // Assume any error is fatal.
    if (fatal)
        *fatal = true;

    // See if we have two variants of the signature, ie. one with namespaces
    // stripped that we can use with connect() and another that we can parse.
    const char *full_sig = strchr(signature, '|');

    Chimera::Signature *parsed_signature = Chimera::parse(
            (full_sig ? full_sig + 1 : signature), "a signal argument");

    // At first glance the parse should never fail because the signature
    // originates from the .sip file.  However it might if it includes a type
    // that has been forward declared, but not yet defined.  The only example
    // in PyQt is the declaration of QWidget by QSignalMapper.  Therefore we
    // assume the error isn't fatal.
    if (!parsed_signature)
    {
        if (fatal)
            *fatal = false;

        return 0;
    }

    if (full_sig)
        // Use the variant with namespaces stripped.
        parsed_signature->signature = QByteArray(signature,
                full_sig - signature);

    parsed_signature->signature.prepend('2');

    // Create and initialise the signal.
    qpycore_pyqtSignal *ps = (qpycore_pyqtSignal *)PyType_GenericNew(
            qpycore_pyqtSignal_TypeObject, 0, 0);

    if (!ps)
    {
        delete parsed_signature;
        return 0;
    }

    ps->default_signal = ps;
    ps->next = 0;
    ps->docstring = 0;
    ps->parameter_names = 0;
    ps->revision = 0;
    ps->parsed_signature = parsed_signature;
    ps->emitter = 0;
    ps->non_signals = 0;

    return ps;
}


// Find an overload that matches a subscript.
qpycore_pyqtSignal *qpycore_find_signal(qpycore_pyqtSignal *ps,
        PyObject *subscript, const char *context)
{
    // Make sure the subscript is a tuple.
    PyObject *args;

    if (PyTuple_Check(subscript))
    {
        args = subscript;
    }
    else
    {
        args = PyTuple_New(1);

        if (!args)
            return 0;

        PyTuple_SetItem(args, 0, subscript);
    }

    Py_INCREF(subscript);

    // Parse the subscript as a tuple of types.
    Chimera::Signature *ss_signature = Chimera::parse(args, 0, context);

    Py_DECREF(args);

    if (!ss_signature)
        return 0;

    // Search for an overload with this signature.
    qpycore_pyqtSignal *overload = ps->default_signal;

    do
    {
        if (overload->parsed_signature->arguments() == ss_signature->signature)
            break;

        overload = overload->next;
    }
    while (overload);

    delete ss_signature;

    if (!overload)
        PyErr_SetString(PyExc_KeyError,
                "there is no matching overloaded signal");

    return overload;
}


// Initialise a signal when given a tuple of types.
static int init_signal_from_types(qpycore_pyqtSignal *ps, const char *name,
        const QList<QByteArray> *parameter_names, int revision,
        PyObject *types)
{
    Chimera::Signature *parsed_signature = Chimera::parse(types, name,
            "a pyqtSignal() type argument");

    if (!parsed_signature)
        return -1;

    if (name)
        parsed_signature->signature.prepend('2');

    ps->default_signal = ps;
    ps->next = 0;
    ps->docstring = 0;
    ps->parameter_names = parameter_names;
    ps->revision = revision;
    ps->parsed_signature = parsed_signature;
    ps->emitter = 0;
    ps->non_signals = 0;

    return 0;
}


// Append an overload to the default signal's list.
static void append_overload(qpycore_pyqtSignal *ps)
{
    // Append to the list of overloads.
    qpycore_pyqtSignal **tailp = &ps->default_signal->next;

    while (*tailp)
        tailp = &(*tailp)->next;

    *tailp = ps;
}


// Give a signal a name if it hasn't already got one.
void qpycore_set_signal_name(qpycore_pyqtSignal *ps, const char *type_name,
        const char *name)
{
    ps = ps->default_signal;

    // If the signature already has a name then they all have and there is
    // nothing more to do.
    if (!ps->parsed_signature->signature.startsWith('('))
        return;

    do
    {
        QByteArray &sig = ps->parsed_signature->signature;

        sig.prepend(name);
        sig.prepend('2');

        QByteArray &py_sig = ps->parsed_signature->py_signature;

        py_sig.prepend(name);
        py_sig.prepend('.');
        py_sig.prepend(type_name);

        ps = ps->next;
    }
    while (ps);
}


// Handle the getting of a lazy attribute, ie. a native Qt signal.
int qpycore_get_lazy_attr(const sipTypeDef *td, PyObject *dict)
{
    const pyqt6ClassPluginDef *plugin = reinterpret_cast<const pyqt6ClassPluginDef *>(sipTypePluginData(td));

    // Handle the trvial cases.
    if (!plugin)
        return 0;

    const pyqt6QtSignal *sigs = plugin->qt_signals;

    if (!sigs)
        return 0;

    QByteArray default_name;
    qpycore_pyqtSignal *default_signal = 0;

    do
    {
        // See if we have come to the end of the current signal.
        if (default_signal && !is_signal_name(sigs->signature, default_name))
        {
            if (PyDict_SetItemString(dict, default_name.constData(), (PyObject *)default_signal) < 0)
                return -1;

            default_signal = 0;
        }

        bool fatal;

        qpycore_pyqtSignal *sig = qpycore_pyqtSignal_New(sigs->signature,
                &fatal);

        if (!sig)
        {
            if (fatal)
                return -1;

            PyErr_Clear();
            continue;
        }

        sig->docstring = sigs->docstring;
        sig->emitter = sigs->emitter;

        // See if this is a new default.
        if (default_signal)
        {
            sig->default_signal = default_signal;
            append_overload(sig);
        }
        else
        {
            sig->non_signals = sigs->non_signals;

            default_signal = sig->default_signal = sig;

            default_name = sigs->signature;
            default_name.truncate(default_name.indexOf('('));
        }
    }
    while ((++sigs)->signature);

    // Save the last one, if any (in case of a non-fatal error).
    if (!default_signal)
        return 0;

    return PyDict_SetItemString(dict, default_name.constData(),
            (PyObject *)default_signal);
}


// Return true if a signal signatures has a particular name.
static bool is_signal_name(const char *sig, const QByteArray &name)
{
    return (qstrncmp(sig, name.constData(), name.size()) == 0 && sig[name.size()] == '(');
}


// Call a signal's overloaded method (if there is one).
PyObject *qpycore_call_signal_overload(qpycore_pyqtSignal *ps, PyObject *bound,
        PyObject *args, PyObject *kw)
{
    if (!ps->non_signals)
    {
        PyErr_SetString(PyExc_TypeError, "native Qt signal is not callable");
        return 0;
    }

    PyObject *func = PyCFunction_New(ps->non_signals, bound);

    if (!func)
        return 0;

    PyObject *result = PyObject_Call(func, args, kw);

    Py_DECREF(func);

    return result;
}
