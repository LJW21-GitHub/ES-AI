// This is the implementation of the QPyQmlModelProxy class.
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


#include "qpyqmlmodel.h"

#include "sipAPIQtQml.h"


// Forward declarations.
static void bad_result(PyObject *res, const char *context);

// The list of registered Python types.
QList<PyTypeObject *> QPyQmlModelProxy::pyqt_types;

// The set of proxies in existence.
QSet<QObject *> QPyQmlModelProxy::proxies;


// The ctor.
QPyQmlModelProxy::QPyQmlModelProxy(QObject *parent) :
        QAbstractItemModel(parent), py_proxied(0)
{
    proxies.insert(this);
}


// The dtor.
QPyQmlModelProxy::~QPyQmlModelProxy()
{
    proxies.remove(this);

    SIP_BLOCK_THREADS
    Py_XDECREF(py_proxied);
    SIP_UNBLOCK_THREADS

    if (!proxied.isNull())
        // Deleting it now can cause a crash.
        proxied.data()->deleteLater();
}


// Called when QML has connected to a signal of the proxied object.  Note that
// we also used to implement the corresponding disconnectNotify() to
// disconnect the signal.  However this resulted in deadlocks and shouldn't be
// necessary anyway because it was only happening when the object that made the
// connection was itself being destroyed.
void QPyQmlModelProxy::connectNotify(const QMetaMethod &sig)
{
    QByteArray signal_sig(sig.methodSignature());

    // Avoid a warning message from QObject::connect().  This seems to happen
    // when a notification signal of the proxied object gets connected to a
    // property that is defined in QML.  I think it is benign...
    if (signal_sig.isEmpty())
        return;

    signal_sig.prepend('2');

    // The signal has actually been connected to the proxy, so do the same from
    // the proxied object to the proxy.  Use Qt::UniqueConnection in case the
    // object (ie. model) is used in more than one view.
    QObject::connect(proxied, signal_sig.constData(), this,
            signal_sig.constData(), Qt::UniqueConnection);
}


// Delegate to the real object.
const QMetaObject *QPyQmlModelProxy::metaObject() const
{
    return !proxied.isNull() ? proxied->metaObject() : QObject::metaObject();
}


// Delegate to the real object.
void *QPyQmlModelProxy::qt_metacast(const char *_clname)
{
    return !proxied.isNull() ? proxied->qt_metacast(_clname) : 0;
}


// Delegate to the real object.
int QPyQmlModelProxy::qt_metacall(QMetaObject::Call call, int idx, void **args)
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
int QPyQmlModelProxy::addType(PyTypeObject *type)
{
    pyqt_types.append(type);

    return pyqt_types.size() - 1;
}


// Create the Python instance.
void QPyQmlModelProxy::createPyObject(QObject *parent)
{
    static const sipTypeDef *model_td = 0;

    SIP_BLOCK_THREADS

    if (!model_td)
        model_td = sipFindType("QAbstractItemModel");

    if (model_td)
    {
        py_proxied = sipCallMethod(NULL, (PyObject *)pyqt_types.at(typeNr()),
                "D", parent, model_td, NULL);

        if (py_proxied)
        {
            // If there is no parent (which seems always to be the case) then
            // things seem to work better if we handle the destruction of the
            // Python and C++ objects separately.
            if (!parent)
                sipTransferTo(py_proxied, NULL);

            proxied = reinterpret_cast<QAbstractItemModel *>(
                    sipGetAddress((sipSimpleWrapper *)py_proxied));
        }
        else
        {
            pyqt6_qtqml_err_print();
        }
    }
    else
    {
        PyErr_SetString(PyExc_TypeError, "unknown type 'QAbstractItemModel'");
        pyqt6_qtqml_err_print();
    }

    SIP_UNBLOCK_THREADS
}


// Resolve any proxy.
void *QPyQmlModelProxy::resolveProxy(void *proxy)
{
    QObject *qobj = reinterpret_cast<QObject *>(proxy);

    // We have to search for proxy instances because we have subverted the
    // usual sub-class detection mechanism.
    if (proxies.contains(qobj))
        return static_cast<QPyQmlModelProxy *>(qobj)->proxied.data();

    // It's not a proxy.
    return proxy;
}


// Create an instance of the attached properties.
QObject *QPyQmlModelProxy::createAttachedProperties(PyTypeObject *py_type,
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
void QPyQmlModelProxy::pyClassBegin()
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
void QPyQmlModelProxy::pyComponentComplete()
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
void QPyQmlModelProxy::pySetTarget(const QQmlProperty &target)
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
#define QPYQML_MODEL_PROXY_IMPL(n) \
QPyQmlModel##n::QPyQmlModel##n(QObject *parent) : QPyQmlModelProxy(parent) \
{ \
    createPyObject(parent); \
} \
QObject *QPyQmlModel##n::attachedProperties(QObject *parent) \
{ \
    return createAttachedProperties(attachedPyType, parent); \
} \
void QPyQmlModel##n::classBegin() \
{ \
    pyClassBegin(); \
} \
void QPyQmlModel##n::componentComplete() \
{ \
    pyComponentComplete(); \
} \
void QPyQmlModel##n::setTarget(const QQmlProperty &target) \
{ \
    pySetTarget(target); \
} \
QMetaObject QPyQmlModel##n::staticMetaObject; \
PyTypeObject *QPyQmlModel##n::attachedPyType


QPYQML_MODEL_PROXY_IMPL(0);
QPYQML_MODEL_PROXY_IMPL(1);
QPYQML_MODEL_PROXY_IMPL(2);
QPYQML_MODEL_PROXY_IMPL(3);
QPYQML_MODEL_PROXY_IMPL(4);
QPYQML_MODEL_PROXY_IMPL(5);
QPYQML_MODEL_PROXY_IMPL(6);
QPYQML_MODEL_PROXY_IMPL(7);
QPYQML_MODEL_PROXY_IMPL(8);
QPYQML_MODEL_PROXY_IMPL(9);
QPYQML_MODEL_PROXY_IMPL(10);
QPYQML_MODEL_PROXY_IMPL(11);
QPYQML_MODEL_PROXY_IMPL(12);
QPYQML_MODEL_PROXY_IMPL(13);
QPYQML_MODEL_PROXY_IMPL(14);
QPYQML_MODEL_PROXY_IMPL(15);
QPYQML_MODEL_PROXY_IMPL(16);
QPYQML_MODEL_PROXY_IMPL(17);
QPYQML_MODEL_PROXY_IMPL(18);
QPYQML_MODEL_PROXY_IMPL(19);
QPYQML_MODEL_PROXY_IMPL(20);
QPYQML_MODEL_PROXY_IMPL(21);
QPYQML_MODEL_PROXY_IMPL(22);
QPYQML_MODEL_PROXY_IMPL(23);
QPYQML_MODEL_PROXY_IMPL(24);
QPYQML_MODEL_PROXY_IMPL(25);
QPYQML_MODEL_PROXY_IMPL(26);
QPYQML_MODEL_PROXY_IMPL(27);
QPYQML_MODEL_PROXY_IMPL(28);
QPYQML_MODEL_PROXY_IMPL(29);
QPYQML_MODEL_PROXY_IMPL(30);
QPYQML_MODEL_PROXY_IMPL(31);
QPYQML_MODEL_PROXY_IMPL(32);
QPYQML_MODEL_PROXY_IMPL(33);
QPYQML_MODEL_PROXY_IMPL(34);
QPYQML_MODEL_PROXY_IMPL(35);
QPYQML_MODEL_PROXY_IMPL(36);
QPYQML_MODEL_PROXY_IMPL(37);
QPYQML_MODEL_PROXY_IMPL(38);
QPYQML_MODEL_PROXY_IMPL(39);
QPYQML_MODEL_PROXY_IMPL(40);
QPYQML_MODEL_PROXY_IMPL(41);
QPYQML_MODEL_PROXY_IMPL(42);
QPYQML_MODEL_PROXY_IMPL(43);
QPYQML_MODEL_PROXY_IMPL(44);
QPYQML_MODEL_PROXY_IMPL(45);
QPYQML_MODEL_PROXY_IMPL(46);
QPYQML_MODEL_PROXY_IMPL(47);
QPYQML_MODEL_PROXY_IMPL(48);
QPYQML_MODEL_PROXY_IMPL(49);
QPYQML_MODEL_PROXY_IMPL(50);
QPYQML_MODEL_PROXY_IMPL(51);
QPYQML_MODEL_PROXY_IMPL(52);
QPYQML_MODEL_PROXY_IMPL(53);
QPYQML_MODEL_PROXY_IMPL(54);
QPYQML_MODEL_PROXY_IMPL(55);
QPYQML_MODEL_PROXY_IMPL(56);
QPYQML_MODEL_PROXY_IMPL(57);
QPYQML_MODEL_PROXY_IMPL(58);
QPYQML_MODEL_PROXY_IMPL(59);


// The reimplementations of the QAbstractItemModel virtuals.

QModelIndex QPyQmlModelProxy::index(int row, int column, const QModelIndex &parent) const
{
    if (!proxied.isNull())
        return proxied->index(row, column, parent);

    return QModelIndex();
}

QModelIndex QPyQmlModelProxy::parent(const QModelIndex &child) const
{
    if (!proxied.isNull())
        return proxied->parent(child);

    return QModelIndex();
}

QModelIndex QPyQmlModelProxy::sibling(int row, int column, const QModelIndex &idx) const
{
    if (!proxied.isNull())
        return proxied->sibling(row, column, idx);

    return QModelIndex();
}

int QPyQmlModelProxy::rowCount(const QModelIndex &parent) const
{
    if (!proxied.isNull())
        return proxied->rowCount(parent);

    return 0;
}

int QPyQmlModelProxy::columnCount(const QModelIndex &parent) const
{
    if (!proxied.isNull())
        return proxied->columnCount(parent);

    return 0;
}

bool QPyQmlModelProxy::hasChildren(const QModelIndex &parent) const
{
    if (!proxied.isNull())
        return proxied->hasChildren(parent);

    return false;
}

QVariant QPyQmlModelProxy::data(const QModelIndex &index, int role) const
{
    if (!proxied.isNull())
        return proxied->data(index, role);

    return QVariant();
}

bool QPyQmlModelProxy::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!proxied.isNull())
        return proxied->setData(index, value, role);

    return false;
}

QVariant QPyQmlModelProxy::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (!proxied.isNull())
        return proxied->headerData(section, orientation, role);

    return QVariant();
}

bool QPyQmlModelProxy::setHeaderData(int section, Qt::Orientation orientation, const QVariant &value, int role)
{
    if (!proxied.isNull())
        return proxied->setHeaderData(section, orientation, value, role);

    return false;
}

QMap<int, QVariant> QPyQmlModelProxy::itemData(const QModelIndex &index) const
{
    if (!proxied.isNull())
        return proxied->itemData(index);

    return QMap<int, QVariant>();
}

bool QPyQmlModelProxy::setItemData(const QModelIndex &index, const QMap<int, QVariant> &roles)
{
    if (!proxied.isNull())
        return proxied->setItemData(index, roles);

    return false;
}

QStringList QPyQmlModelProxy::mimeTypes() const
{
    if (!proxied.isNull())
        return proxied->mimeTypes();

    return QStringList();
}

QMimeData *QPyQmlModelProxy::mimeData(const QModelIndexList &indexes) const
{
    if (!proxied.isNull())
        return proxied->mimeData(indexes);

    return 0;
}

bool QPyQmlModelProxy::canDropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) const
{
    if (!proxied.isNull())
        return proxied->canDropMimeData(data, action, row, column, parent);

    return false;
}

bool QPyQmlModelProxy::dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent)
{
    if (!proxied.isNull())
        return proxied->dropMimeData(data, action, row, column, parent);

    return false;
}

Qt::DropActions QPyQmlModelProxy::supportedDropActions() const
{
    if (!proxied.isNull())
        return proxied->supportedDropActions();

    return Qt::IgnoreAction;
}

Qt::DropActions QPyQmlModelProxy::supportedDragActions() const
{
    if (!proxied.isNull())
        return proxied->supportedDragActions();

    return Qt::IgnoreAction;
}

bool QPyQmlModelProxy::insertRows(int row, int count, const QModelIndex &parent)
{
    if (!proxied.isNull())
        return proxied->insertRows(row, count, parent);

    return false;
}

bool QPyQmlModelProxy::insertColumns(int column, int count, const QModelIndex &parent)
{
    if (!proxied.isNull())
        return proxied->insertColumns(column, count, parent);

    return false;
}

bool QPyQmlModelProxy::removeRows(int row, int count, const QModelIndex &parent)
{
    if (!proxied.isNull())
        return proxied->removeRows(row, count, parent);

    return false;
}

bool QPyQmlModelProxy::removeColumns(int column, int count, const QModelIndex &parent)
{
    if (!proxied.isNull())
        return proxied->removeColumns(column, count, parent);

    return false;
}

bool QPyQmlModelProxy::moveRows(const QModelIndex &sourceParent, int sourceRow, int count, const QModelIndex &destinationParent, int destinationChild)
{
    if (!proxied.isNull())
        return proxied->moveRows(sourceParent, sourceRow, count,
                destinationParent, destinationChild);

    return false;
}

bool QPyQmlModelProxy::moveColumns(const QModelIndex &sourceParent, int sourceColumn, int count, const QModelIndex &destinationParent, int destinationChild)
{
    if (!proxied.isNull())
        return proxied->moveColumns(sourceParent, sourceColumn, count,
                destinationParent, destinationChild);

    return false;
}

void QPyQmlModelProxy::fetchMore(const QModelIndex &parent)
{
    if (!proxied.isNull())
        proxied->fetchMore(parent);
}

bool QPyQmlModelProxy::canFetchMore(const QModelIndex &parent) const
{
    if (!proxied.isNull())
        return proxied->canFetchMore(parent);

    return false;
}

Qt::ItemFlags QPyQmlModelProxy::flags(const QModelIndex &index) const
{
    if (!proxied.isNull())
        return proxied->flags(index);

    return Qt::NoItemFlags;
}

void QPyQmlModelProxy::sort(int column, Qt::SortOrder order)
{
    if (!proxied.isNull())
        proxied->sort(column, order);
}

QModelIndex QPyQmlModelProxy::buddy(const QModelIndex &index) const
{
    if (!proxied.isNull())
        return proxied->buddy(index);

    return QModelIndex();
}

QModelIndexList QPyQmlModelProxy::match(const QModelIndex &start, int role, const QVariant &value, int hits, Qt::MatchFlags flags) const
{
    if (!proxied.isNull())
        return proxied->match(start, role, value, hits, flags);

    return QModelIndexList();
}

QSize QPyQmlModelProxy::span(const QModelIndex &index) const
{
    if (!proxied.isNull())
        return proxied->span(index);

    return QSize();
}

QHash<int, QByteArray> QPyQmlModelProxy::roleNames() const
{
    if (!proxied.isNull())
        return proxied->roleNames();

    return QHash<int, QByteArray>();
}
