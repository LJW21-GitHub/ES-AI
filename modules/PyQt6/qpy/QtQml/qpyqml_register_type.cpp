// This contains the main implementation of qmlRegisterType.
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

#include <qqmlprivate.h>
#include <QByteArray>
#include <QString>
#include <QQmlListProperty>
#include <QQmlParserStatus>
#include <QQmlPropertyValueSource>

#include "qpyqml_api.h"
#include "qpyqmlmodel.h"
#include "qpyqmlobject.h"
#include "qpyqmlvalidator.h"

#include "sipAPIQtQml.h"


class QQmlPropertyValueInterceptor;


// Forward declarations.
static QQmlPrivate::RegisterType *init_type(PyTypeObject *py_type, bool ctor,
        PyTypeObject *attached);
static void complete_init(QQmlPrivate::RegisterType *rt);
static int register_type(QQmlPrivate::RegisterType *rt);


// The registration data for the QAbstractItemModel proxy types.
const int NrOfModelTypes = 60;
static QQmlPrivate::RegisterType model_proxy_types[NrOfModelTypes];

// The registration data for the QObject proxy types.
const int NrOfObjectTypes = 60;
static QQmlPrivate::RegisterType object_proxy_types[NrOfObjectTypes];

// The registration data for the QValidator proxy types.
const int NrOfValidatorTypes = 10;
static QQmlPrivate::RegisterType validator_proxy_types[NrOfValidatorTypes];


// Register an anonymous Python type.
int qpyqml_register_anonymous_type(PyTypeObject *py_type, const char *uri,
        int major)
{
    // Initialise the registration data structure.
    QQmlPrivate::RegisterType *rt = init_type(py_type, false, nullptr);

    if (!rt)
        return -1;

    rt->uri = uri;
    rt->version = QTypeRevision::fromVersion(major, 0);
    rt->elementName = nullptr;

    return register_type(rt);
}


// Register a library Python type.
int qpyqml_register_library_type(PyTypeObject *py_type, const char *uri,
        int major, int minor, const char *qml_name, PyTypeObject *attached)
{
    // Initialise the registration data structure.
    QQmlPrivate::RegisterType *rt = init_type(py_type, true, attached);

    if (!rt)
        return -1;

    if (!qml_name)
        qml_name = sipPyTypeName(py_type);

    rt->uri = uri;
    rt->version = QTypeRevision::fromVersion(major, minor);
    rt->elementName = qml_name;

    return register_type(rt);
}


// Register an uncreatable library Python type.
int qpyqml_register_uncreatable_type(PyTypeObject *py_type, const char *uri,
        int major, int minor, const char *qml_name, const QString &reason)
{
    // Initialise the registration data structure.
    QQmlPrivate::RegisterType *rt = init_type(py_type, false, nullptr);

    if (!rt)
        return -1;

    if (!qml_name)
        qml_name = sipPyTypeName(py_type);

    rt->noCreationReason = reason;
    rt->uri = uri;
    rt->version = QTypeRevision::fromVersion(major, minor);
    rt->elementName = qml_name;

    return register_type(rt);
}


// Register the proxy type with QML.
static int register_type(QQmlPrivate::RegisterType *rt)
{
    int type_id = QQmlPrivate::qmlregister(QQmlPrivate::TypeRegistration, rt);

    if (type_id < 0)
    {
        PyErr_SetString(PyExc_RuntimeError,
                "unable to register type with QML");
        return -1;
    }

    return type_id;
}


#define QPYQML_TYPE_INIT(t, n) \
    case n##U: \
        QPyQml##t##n::staticMetaObject = *mo; \
        QPyQml##t##n::attachedPyType = attached; \
        rt->typeId = QQmlPrivate::QmlMetaType<QPyQml##t##n>::self(); \
        rt->listId = QQmlPrivate::QmlMetaType<QPyQml##t##n>::list(); \
        rt->objectSize = ctor ? sizeof(QPyQml##t##n) : 0; \
        if (ctor) rt->create = QQmlPrivate::createInto<QPyQml##t##n>; else rt->create = 0; \
        rt->attachedPropertiesFunction = attached_mo ? QPyQml##t##n::attachedProperties : 0; \
        rt->parserStatusCast = is_parser_status ? QQmlPrivate::StaticCastSelector<QPyQml##t##n,QQmlParserStatus>::cast() : -1; \
        rt->valueSourceCast = is_value_source ? QQmlPrivate::StaticCastSelector<QPyQml##t##n,QQmlPropertyValueSource>::cast() : -1; \
        rt->valueInterceptorCast = QQmlPrivate::StaticCastSelector<QPyQml##t##n,QQmlPropertyValueInterceptor>::cast(); \
        break


// Return a pointer to the initialised registration structure for a type.
static QQmlPrivate::RegisterType *init_type(PyTypeObject *py_type, bool ctor,
        PyTypeObject *attached)
{
    PyTypeObject *qobject_type = sipTypeAsPyTypeObject(sipType_QObject);

    // Check the type is derived from QObject and get its meta-object.
    if (!PyType_IsSubtype(py_type, qobject_type))
    {
        PyErr_SetString(PyExc_TypeError,
                "type being registered must be a sub-type of QObject");
        return 0;
    }

    const QMetaObject *mo = pyqt6_qtqml_get_qmetaobject(py_type);

    // See if the type is a parser status.
    bool is_parser_status = PyType_IsSubtype(py_type,
            sipTypeAsPyTypeObject(sipType_QQmlParserStatus));

    // See if the type is a property value source.
    bool is_value_source = PyType_IsSubtype(py_type,
            sipTypeAsPyTypeObject(sipType_QQmlPropertyValueSource));


    // Check any attached type is derived from QObject and get its meta-object.
    const QMetaObject *attached_mo;

    if (attached)
    {
        if (!PyType_IsSubtype(attached, qobject_type))
        {
            PyErr_SetString(PyExc_TypeError,
                    "attached properties type must be a sub-type of QObject");
            return 0;
        }

        attached_mo = pyqt6_qtqml_get_qmetaobject(attached);

        Py_INCREF((PyObject *)attached);
    }
    else
    {
        attached_mo = nullptr;
    }

    QByteArray ptr_name(sipPyTypeName(py_type));
    ptr_name.append('*');

    QByteArray list_name(sipPyTypeName(py_type));
    list_name.prepend("QQmlListProperty<");
    list_name.append('>');

    QQmlPrivate::RegisterType *rt;

    // See if we have the QQuickItem registation helper from the QtQuick
    // module.  Check each time because it could be imported at any point.

    typedef sipErrorState (*QQuickItemRegisterFn)(PyTypeObject *, const QMetaObject *, const QByteArray &, const QByteArray &, QQmlPrivate::RegisterType **);

    static QQuickItemRegisterFn qquickitem_register = 0;

    if (!qquickitem_register)
        qquickitem_register = (QQuickItemRegisterFn)sipImportSymbol(
                "qtquick_register_item");

    if (qquickitem_register)
    {
        sipErrorState estate = qquickitem_register(py_type, mo, ptr_name,
                list_name, &rt);

        if (estate == sipErrorFail)
            return 0;

        if (estate == sipErrorNone)
        {
            complete_init(rt);
            return rt;
        }
    }

    // Initialise the specific type.

    static const sipTypeDef *model_td = 0;

    if (!model_td)
        model_td = sipFindType("QAbstractItemModel");

    static const sipTypeDef *validator_td = 0;

    if (!validator_td)
        validator_td = sipFindType("QValidator");

    if (validator_td && PyType_IsSubtype(py_type, sipTypeAsPyTypeObject(validator_td)))
    {
        int type_nr = QPyQmlValidatorProxy::addType(py_type);

        if (type_nr >= NrOfValidatorTypes)
        {
            PyErr_Format(PyExc_TypeError,
                    "a maximum of %d QValidator types may be registered with QML",
                    NrOfValidatorTypes);
            return 0;
        }

        rt = &validator_proxy_types[type_nr];

        // Initialise those members that depend on the C++ type.
        switch (type_nr)
        {
            QPYQML_TYPE_INIT(Validator, 0);
            QPYQML_TYPE_INIT(Validator, 1);
            QPYQML_TYPE_INIT(Validator, 2);
            QPYQML_TYPE_INIT(Validator, 3);
            QPYQML_TYPE_INIT(Validator, 4);
            QPYQML_TYPE_INIT(Validator, 5);
            QPYQML_TYPE_INIT(Validator, 6);
            QPYQML_TYPE_INIT(Validator, 7);
            QPYQML_TYPE_INIT(Validator, 8);
            QPYQML_TYPE_INIT(Validator, 9);
        }
    }
    else if (model_td && PyType_IsSubtype(py_type, sipTypeAsPyTypeObject(model_td)))
    {
        int type_nr = QPyQmlModelProxy::addType(py_type);

        if (type_nr >= NrOfModelTypes)
        {
            PyErr_Format(PyExc_TypeError,
                    "a maximum of %d QAbstractItemModel types may be registered with QML",
                    NrOfModelTypes);
            return 0;
        }

        rt = &model_proxy_types[type_nr];

        // Initialise those members that depend on the C++ type.
        switch (type_nr)
        {
            QPYQML_TYPE_INIT(Model, 0);
            QPYQML_TYPE_INIT(Model, 1);
            QPYQML_TYPE_INIT(Model, 2);
            QPYQML_TYPE_INIT(Model, 3);
            QPYQML_TYPE_INIT(Model, 4);
            QPYQML_TYPE_INIT(Model, 5);
            QPYQML_TYPE_INIT(Model, 6);
            QPYQML_TYPE_INIT(Model, 7);
            QPYQML_TYPE_INIT(Model, 8);
            QPYQML_TYPE_INIT(Model, 9);
            QPYQML_TYPE_INIT(Model, 10);
            QPYQML_TYPE_INIT(Model, 11);
            QPYQML_TYPE_INIT(Model, 12);
            QPYQML_TYPE_INIT(Model, 13);
            QPYQML_TYPE_INIT(Model, 14);
            QPYQML_TYPE_INIT(Model, 15);
            QPYQML_TYPE_INIT(Model, 16);
            QPYQML_TYPE_INIT(Model, 17);
            QPYQML_TYPE_INIT(Model, 18);
            QPYQML_TYPE_INIT(Model, 19);
            QPYQML_TYPE_INIT(Model, 20);
            QPYQML_TYPE_INIT(Model, 21);
            QPYQML_TYPE_INIT(Model, 22);
            QPYQML_TYPE_INIT(Model, 23);
            QPYQML_TYPE_INIT(Model, 24);
            QPYQML_TYPE_INIT(Model, 25);
            QPYQML_TYPE_INIT(Model, 26);
            QPYQML_TYPE_INIT(Model, 27);
            QPYQML_TYPE_INIT(Model, 28);
            QPYQML_TYPE_INIT(Model, 29);
            QPYQML_TYPE_INIT(Model, 30);
            QPYQML_TYPE_INIT(Model, 31);
            QPYQML_TYPE_INIT(Model, 32);
            QPYQML_TYPE_INIT(Model, 33);
            QPYQML_TYPE_INIT(Model, 34);
            QPYQML_TYPE_INIT(Model, 35);
            QPYQML_TYPE_INIT(Model, 36);
            QPYQML_TYPE_INIT(Model, 37);
            QPYQML_TYPE_INIT(Model, 38);
            QPYQML_TYPE_INIT(Model, 39);
            QPYQML_TYPE_INIT(Model, 40);
            QPYQML_TYPE_INIT(Model, 41);
            QPYQML_TYPE_INIT(Model, 42);
            QPYQML_TYPE_INIT(Model, 43);
            QPYQML_TYPE_INIT(Model, 44);
            QPYQML_TYPE_INIT(Model, 45);
            QPYQML_TYPE_INIT(Model, 46);
            QPYQML_TYPE_INIT(Model, 47);
            QPYQML_TYPE_INIT(Model, 48);
            QPYQML_TYPE_INIT(Model, 49);
            QPYQML_TYPE_INIT(Model, 50);
            QPYQML_TYPE_INIT(Model, 51);
            QPYQML_TYPE_INIT(Model, 52);
            QPYQML_TYPE_INIT(Model, 53);
            QPYQML_TYPE_INIT(Model, 54);
            QPYQML_TYPE_INIT(Model, 55);
            QPYQML_TYPE_INIT(Model, 56);
            QPYQML_TYPE_INIT(Model, 57);
            QPYQML_TYPE_INIT(Model, 58);
            QPYQML_TYPE_INIT(Model, 59);
        }
    }
    else
    {
        int type_nr = QPyQmlObjectProxy::addType(py_type);

        if (type_nr >= NrOfObjectTypes)
        {
            PyErr_Format(PyExc_TypeError,
                    "a maximum of %d QObject types may be registered with QML",
                    NrOfObjectTypes);
            return 0;
        }

        rt = &object_proxy_types[type_nr];

        // Initialise those members that depend on the C++ type.
        switch (type_nr)
        {
            QPYQML_TYPE_INIT(Object, 0);
            QPYQML_TYPE_INIT(Object, 1);
            QPYQML_TYPE_INIT(Object, 2);
            QPYQML_TYPE_INIT(Object, 3);
            QPYQML_TYPE_INIT(Object, 4);
            QPYQML_TYPE_INIT(Object, 5);
            QPYQML_TYPE_INIT(Object, 6);
            QPYQML_TYPE_INIT(Object, 7);
            QPYQML_TYPE_INIT(Object, 8);
            QPYQML_TYPE_INIT(Object, 9);
            QPYQML_TYPE_INIT(Object, 10);
            QPYQML_TYPE_INIT(Object, 11);
            QPYQML_TYPE_INIT(Object, 12);
            QPYQML_TYPE_INIT(Object, 13);
            QPYQML_TYPE_INIT(Object, 14);
            QPYQML_TYPE_INIT(Object, 15);
            QPYQML_TYPE_INIT(Object, 16);
            QPYQML_TYPE_INIT(Object, 17);
            QPYQML_TYPE_INIT(Object, 18);
            QPYQML_TYPE_INIT(Object, 19);
            QPYQML_TYPE_INIT(Object, 20);
            QPYQML_TYPE_INIT(Object, 21);
            QPYQML_TYPE_INIT(Object, 22);
            QPYQML_TYPE_INIT(Object, 23);
            QPYQML_TYPE_INIT(Object, 24);
            QPYQML_TYPE_INIT(Object, 25);
            QPYQML_TYPE_INIT(Object, 26);
            QPYQML_TYPE_INIT(Object, 27);
            QPYQML_TYPE_INIT(Object, 28);
            QPYQML_TYPE_INIT(Object, 29);
            QPYQML_TYPE_INIT(Object, 30);
            QPYQML_TYPE_INIT(Object, 31);
            QPYQML_TYPE_INIT(Object, 32);
            QPYQML_TYPE_INIT(Object, 33);
            QPYQML_TYPE_INIT(Object, 34);
            QPYQML_TYPE_INIT(Object, 35);
            QPYQML_TYPE_INIT(Object, 36);
            QPYQML_TYPE_INIT(Object, 37);
            QPYQML_TYPE_INIT(Object, 38);
            QPYQML_TYPE_INIT(Object, 39);
            QPYQML_TYPE_INIT(Object, 40);
            QPYQML_TYPE_INIT(Object, 41);
            QPYQML_TYPE_INIT(Object, 42);
            QPYQML_TYPE_INIT(Object, 43);
            QPYQML_TYPE_INIT(Object, 44);
            QPYQML_TYPE_INIT(Object, 45);
            QPYQML_TYPE_INIT(Object, 46);
            QPYQML_TYPE_INIT(Object, 47);
            QPYQML_TYPE_INIT(Object, 48);
            QPYQML_TYPE_INIT(Object, 49);
            QPYQML_TYPE_INIT(Object, 50);
            QPYQML_TYPE_INIT(Object, 51);
            QPYQML_TYPE_INIT(Object, 52);
            QPYQML_TYPE_INIT(Object, 53);
            QPYQML_TYPE_INIT(Object, 54);
            QPYQML_TYPE_INIT(Object, 55);
            QPYQML_TYPE_INIT(Object, 56);
            QPYQML_TYPE_INIT(Object, 57);
            QPYQML_TYPE_INIT(Object, 58);
            QPYQML_TYPE_INIT(Object, 59);
        }
    }

    rt->metaObject = mo;
    rt->attachedPropertiesMetaObject = attached_mo;

    complete_init(rt);

    return rt;
}


// Complete the initialisation of a type registration structure.
static void complete_init(QQmlPrivate::RegisterType *rt)
{
    rt->structVersion = 0;
    rt->uri = 0;
    rt->version = QTypeRevision::zero();
    rt->elementName = 0;
    rt->extensionObjectCreate = 0;
    rt->extensionMetaObject = 0;
    rt->customParser = 0;
    rt->revision = QTypeRevision::zero();
}


// Return the proxy that created an object.  This is called with the GIL.
QObject *qpyqml_find_proxy_for(QObject *proxied)
{
    QSetIterator<QObject *> oit(QPyQmlObjectProxy::proxies);

    while (oit.hasNext())
    {
        QPyQmlObjectProxy *proxy = static_cast<QPyQmlObjectProxy *>(oit.next());

        if (proxy->proxied.data() == proxied)
            return proxy;
    }

    QSetIterator<QObject *> mit(QPyQmlModelProxy::proxies);

    while (mit.hasNext())
    {
        QPyQmlModelProxy *proxy = static_cast<QPyQmlModelProxy *>(mit.next());

        if (proxy->proxied.data() == proxied)
            return proxy;
    }

    QSetIterator<QObject *> vit(QPyQmlValidatorProxy::proxies);

    while (vit.hasNext())
    {
        QPyQmlValidatorProxy *proxy = static_cast<QPyQmlValidatorProxy *>(vit.next());

        if (proxy->proxied.data() == proxied)
            return proxy;
    }

    PyErr_Format(PyExc_TypeError,
            "QObject instance at %p was not created from QML", proxied);

    return 0;
}
