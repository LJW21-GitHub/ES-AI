// This is the implementation of the QPyQmlValidatorProxy class.
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


#include "qpyqmlvalidator.h"

#include "sipAPIQtQml.h"


// Forward declarations.
static void bad_result(PyObject *res, const char *context);

// The list of registered Python types.
QList<PyTypeObject *> QPyQmlValidatorProxy::pyqt_types;

// The set of proxies in existence.
QSet<QObject *> QPyQmlValidatorProxy::proxies;


// The ctor.
QPyQmlValidatorProxy::QPyQmlValidatorProxy(QObject *parent) :
        QValidator(parent), py_proxied(0)
{
    proxies.insert(this);
}


// The dtor.
QPyQmlValidatorProxy::~QPyQmlValidatorProxy()
{
    proxies.remove(this);

    SIP_BLOCK_THREADS
    Py_XDECREF(py_proxied);
    SIP_UNBLOCK_THREADS

    if (!proxied.isNull())
        delete proxied.data();
}


// Called when QML has connected to a signal of the proxied object.  Note that
// we also used to implement the corresponding disconnectNotify() to
// disconnect the signal.  However this resulted in deadlocks and shouldn't be
// necessary anyway because it was only happening when the object that made the
// connection was itself being destroyed.
void QPyQmlValidatorProxy::connectNotify(const QMetaMethod &sig)
{
    QByteArray signal_sig = signalSignature(sig);

    // The signal has actually been connected to the proxy, so do the same from
    // the proxied object to the proxy.  Use Qt::UniqueConnection in case the
    // object (ie. model) is used in more than one view.
    QObject::connect(proxied, signal_sig.constData(), this,
            signal_sig.constData(), Qt::UniqueConnection);
}


// Return what SIGNAL() would return for a method.
QByteArray QPyQmlValidatorProxy::signalSignature(const QMetaMethod &signal)
{
    QByteArray signal_sig(signal.methodSignature());
    signal_sig.prepend('2');

    return signal_sig;
}


// Delegate to the real object.
const QMetaObject *QPyQmlValidatorProxy::metaObject() const
{
    return !proxied.isNull() ? proxied->metaObject() : QObject::metaObject();
}


// Delegate to the real object.
void *QPyQmlValidatorProxy::qt_metacast(const char *_clname)
{
    return !proxied.isNull() ? proxied->qt_metacast(_clname) : 0;
}


// Delegate to the real object.
int QPyQmlValidatorProxy::qt_metacall(QMetaObject::Call call, int idx, void **args)
{
    if (idx < 0)
        return idx;

    if (proxied.isNull())
        return QObject::qt_metacall(call, idx, args);

    const QMetaObject *proxied_mo = proxied->metaObject();

    // See if a signal defined in the proxied object might be being invoked.
    // Note that we used to use sender() but this proved unreliable.
    if (call == QMetaObject::InvokeMetaMethod && proxied_mo->method(idx).methodType() == QMetaMethod::Signal)
    {
        // Get the meta-object of the class that defines the signal.
        while (idx < proxied_mo->methodOffset())
        {
            proxied_mo = proxied_mo->superClass();
            Q_ASSERT(proxied_mo);
        }

        // Relay the signal to QML.
        QMetaObject::activate(this, proxied_mo,
                idx - proxied_mo->methodOffset(), args);

        return idx - (proxied_mo->methodCount() - proxied_mo->methodOffset());
    }

    return proxied->qt_metacall(call, idx, args);
}


// Add a new Python type and return its number.
int QPyQmlValidatorProxy::addType(PyTypeObject *type)
{
    pyqt_types.append(type);

    return pyqt_types.size() - 1;
}


// Create the Python instance.
void QPyQmlValidatorProxy::createPyObject(QObject *parent)
{
    static const sipTypeDef *validator_td = 0;

    SIP_BLOCK_THREADS

    if (!validator_td)
        validator_td = sipFindType("QValidator");

    if (validator_td)
    {
        py_proxied = sipCallMethod(NULL, (PyObject *)pyqt_types.at(typeNr()),
                "D", parent, validator_td, NULL);

        if (py_proxied)
            proxied = reinterpret_cast<QValidator *>(
                    sipGetAddress((sipSimpleWrapper *)py_proxied));
        else
            pyqt6_qtqml_err_print();
    }
    else
    {
        PyErr_SetString(PyExc_TypeError, "unknown type 'QValidator'");
        pyqt6_qtqml_err_print();
    }

    SIP_UNBLOCK_THREADS
}


// Resolve any proxy.
void *QPyQmlValidatorProxy::resolveProxy(void *proxy)
{
    QObject *qobj = reinterpret_cast<QObject *>(proxy);

    // We have to search for proxy instances because we have subverted the
    // usual sub-class detection mechanism.
    if (proxies.contains(qobj))
        return static_cast<QPyQmlValidatorProxy *>(qobj)->proxied.data();

    // It's not a proxy.
    return proxy;
}


// Create an instance of the attached properties.
QObject *QPyQmlValidatorProxy::createAttachedProperties(PyTypeObject *py_type,
        QObject *parent)
{
    QObject *qobj = 0;

    SIP_BLOCK_THREADS

    PyObject *obj = sipCallMethod(NULL, (PyObject *)py_type, "D", parent,
            sipType_QObject, NULL);

    if (obj)
    {
        qobj = reinterpret_cast<QObject *>(
                sipGetAddress((sipSimpleWrapper *)obj));

        // It should always have a parent, but just in case...
        if (parent)
            Py_DECREF(obj);
    }
    else
    {
        pyqt6_qtqml_err_print();
    }

    SIP_UNBLOCK_THREADS

    return qobj;
}


// Invoked when a class parse begins.
void QPyQmlValidatorProxy::pyClassBegin()
{
    if (!py_proxied)
        return;

    SIP_BLOCK_THREADS

    bool ok = false;

    static PyObject *method_name = 0;

    if (!method_name)
        method_name = PyUnicode_FromString("classBegin");

    if (method_name)
    {
        PyObject *res = PyObject_CallMethodObjArgs(py_proxied, method_name,
                NULL);

        if (res)
        {
            if (res == Py_None)
                ok = true;
            else
                bad_result(res, "classBegin()");

            Py_DECREF(res);
        }
    }

    if (!ok)
        pyqt6_qtqml_err_print();

    SIP_UNBLOCK_THREADS
}


// Invoked when a component parse completes.
void QPyQmlValidatorProxy::pyComponentComplete()
{
    if (!py_proxied)
        return;

    SIP_BLOCK_THREADS

    bool ok = false;

    static PyObject *method_name = 0;

    if (!method_name)
        method_name = PyUnicode_FromString("componentComplete");

    if (method_name)
    {
        PyObject *res = PyObject_CallMethodObjArgs(py_proxied, method_name,
                NULL);

        if (res)
        {
            if (res == Py_None)
                ok = true;
            else
                bad_result(res, "componentComplete()");

            Py_DECREF(res);
        }
    }

    if (!ok)
        pyqt6_qtqml_err_print();

    SIP_UNBLOCK_THREADS
}


// Invoked to set the target property of a property value source.
void QPyQmlValidatorProxy::pySetTarget(const QQmlProperty &target)
{
    if (!py_proxied)
        return;

    SIP_BLOCK_THREADS

    bool ok = false;

    static PyObject *method_name = 0;

    if (!method_name)
        method_name = PyUnicode_FromString("setTarget");

    if (method_name)
    {
        QQmlProperty *target_heap = new QQmlProperty(target);

        PyObject *py_target = sipConvertFromNewType(target_heap,
                sipType_QQmlProperty, 0);

        if (!py_target)
        {
            delete target_heap;
        }
        else
        {
            PyObject *res = PyObject_CallMethodObjArgs(py_proxied, method_name,
                    py_target, NULL);

            Py_DECREF(py_target);

            if (res)
            {
                if (res == Py_None)
                    ok = true;
                else
                    bad_result(res, "setTarget()");

                Py_DECREF(res);
            }
        }
    }

    if (!ok)
        pyqt6_qtqml_err_print();

    SIP_UNBLOCK_THREADS
}


// Raise an exception for an unexpected result.
static void bad_result(PyObject *res, const char *context)
{
    PyErr_Format(PyExc_TypeError, "unexpected result from %s: %S", context,
            res);
}


// The proxy type implementations.
#define QPYQML_VALIDATOR_PROXY_IMPL(n) \
QPyQmlValidator##n::QPyQmlValidator##n(QObject *parent) : QPyQmlValidatorProxy(parent) \
{ \
    createPyObject(parent); \
} \
QObject *QPyQmlValidator##n::attachedProperties(QObject *parent) \
{ \
    return createAttachedProperties(attachedPyType, parent); \
} \
void QPyQmlValidator##n::classBegin() \
{ \
    pyClassBegin(); \
} \
void QPyQmlValidator##n::componentComplete() \
{ \
    pyComponentComplete(); \
} \
void QPyQmlValidator##n::setTarget(const QQmlProperty &target) \
{ \
    pySetTarget(target); \
} \
QMetaObject QPyQmlValidator##n::staticMetaObject; \
PyTypeObject *QPyQmlValidator##n::attachedPyType


QPYQML_VALIDATOR_PROXY_IMPL(0);
QPYQML_VALIDATOR_PROXY_IMPL(1);
QPYQML_VALIDATOR_PROXY_IMPL(2);
QPYQML_VALIDATOR_PROXY_IMPL(3);
QPYQML_VALIDATOR_PROXY_IMPL(4);
QPYQML_VALIDATOR_PROXY_IMPL(5);
QPYQML_VALIDATOR_PROXY_IMPL(6);
QPYQML_VALIDATOR_PROXY_IMPL(7);
QPYQML_VALIDATOR_PROXY_IMPL(8);
QPYQML_VALIDATOR_PROXY_IMPL(9);


// The reimplementations of the QValidator virtuals.

void QPyQmlValidatorProxy::fixup(QString &input) const
{
    if (!proxied.isNull())
        proxied->fixup(input);
}


QValidator::State QPyQmlValidatorProxy::validate(QString &input, int &pos) const
{
    if (!proxied.isNull())
        return proxied->validate(input, pos);

    return QValidator::Invalid;
}
