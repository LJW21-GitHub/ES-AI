// This contains the meta-type used by PyQt.
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
#include <QList>
#include <QMap>
#include <QMetaObject>
#include <QMetaType>
#include <QPair>

#include "qpycore_chimera.h"
#include "qpycore_classinfo.h"
#include "qpycore_enums_flags.h"
#include "qpycore_enums_flags_metatype.h"
#include "qpycore_misc.h"
#include "qpycore_objectified_strings.h"
#include "qpycore_pyqtproperty.h"
#include "qpycore_pyqtsignal.h"
#include "qpycore_pyqtslot.h"
#include "qpycore_qmetaobjectbuilder.h"
#include "qpycore_types.h"

#include "sipAPIQtCore.h"


// A tuple of the property name and definition.
typedef QPair<PyObject *, PyObject *> PropertyData;


// Forward declarations.
static int trawl_hierarchy(PyTypeObject *pytype, qpycore_metaobject *qo,
        QMetaObjectBuilder &builder, QList<EnumFlag> &penums,
        QList<const qpycore_pyqtSignal *> &psigs,
        QMap<uint, PropertyData> &pprops);
static int trawl_type(PyTypeObject *pytype, qpycore_metaobject *qo,
        QMetaObjectBuilder &builder, QList<EnumFlag> &penums,
        QList<const qpycore_pyqtSignal *> &psigs,
        QMap<uint, PropertyData> &pprops);
static const QMetaObject *get_scope_qmetaobject(const Chimera *ct);


// Create a dynamic meta-object for a Python type by introspecting its
// attributes.  Note that it leaks if the type is deleted.
const qpycore_metaobject *qpycore_get_dynamic_metaobject(sipWrapperType *wt)
{
    qpycore_metaobject *qo = reinterpret_cast<qpycore_metaobject *>(
            sipGetTypeUserData(wt));

    // Return any QMetaObject from a previous call.
    if (qo)
        return qo;

    PyTypeObject *pytype = (PyTypeObject *)wt;

    const pyqt6ClassPluginDef *plugin = reinterpret_cast<const pyqt6ClassPluginDef *>(sipTypePluginData(sipTypeFromPyTypeObject(pytype)));

    // Only create a dynamic meta-object if its base wrapped type has a static
    // Qt meta-object.
    if (!plugin || !plugin->static_metaobject)
        return 0;

    QMetaObjectBuilder builder;

    // Get the super-type's meta-object.
    PyTypeObject *tp_base = reinterpret_cast<PyTypeObject *>(
            PyType_GetSlot(pytype, Py_tp_base));

    builder.setSuperClass(qpycore_get_qmetaobject((sipWrapperType *)tp_base));

    // Get the name of the type.  Dynamic types have simple names.
    builder.setClassName(sipPyTypeName(pytype));

    // Go through the class hierarchy getting all PyQt enums, properties, slots
    // and signals.

    QList<EnumFlag> penums;
    QList<const qpycore_pyqtSignal *> psigs;
    QMap<uint, PropertyData> pprops;

    qo = new qpycore_metaobject;

    if (trawl_hierarchy(pytype, qo, builder, penums, psigs, pprops) < 0)
        return 0;

    qo->nr_signals = psigs.count();

    // Initialise the header section of the data table.  Note that Qt v4.5
    // introduced revision 2 which added constructors.  However the design is
    // broken in that the static meta-call function doesn't provide enough
    // information to determine which Python sub-class of a Qt class is to be
    // created.  So we stick with revision 1 (and don't allow pyqtSlot() to
    // decorate __init__).

    // Set up any class information.
    QList<ClassInfo> class_info_list = qpycore_pop_class_info_list(
            (PyObject *)pytype);

    for (int i = 0; i < class_info_list.length(); ++i)
    {
        const ClassInfo &ci = class_info_list.at(i);

        builder.addClassInfo(ci.first, ci.second);
    }

    // Set up any enums/flags.
    for (int i = 0; i < penums.count(); ++i)
    {
        const EnumFlag &ef = penums.at(i);

        QMetaEnumBuilder enum_builder = builder.addEnumerator(ef.name);
        enum_builder.setMetaType(ef.metaType);
        enum_builder.setIsFlag(ef.isFlag);
        enum_builder.setIsScoped(true);

        QList<QPair<QByteArray, int> >::const_iterator it = ef.keys.constBegin();

        while (it != ef.keys.constEnd())
        {
            enum_builder.addKey(it->first, it->second);
            ++it;
        }
    }

    // Add the signals to the meta-object.
    for (int g = 0; g < qo->nr_signals; ++g)
    {
        const qpycore_pyqtSignal *ps = psigs.at(g);

        QMetaMethodBuilder signal_builder = builder.addSignal(
                ps->parsed_signature->signature.mid(1));

        if (ps->parameter_names)
            signal_builder.setParameterNames(*ps->parameter_names);

        signal_builder.setRevision(ps->revision);
    }

    // Add the slots to the meta-object.
    for (int s = 0; s < qo->pslots.count(); ++s)
    {
        const Chimera::Signature *slot_signature = qo->pslots.at(s)->slotSignature();
        const QByteArray &sig = slot_signature->signature;

        QMetaMethodBuilder slot_builder = builder.addSlot(sig);

        // Add any type.
        if (slot_signature->result)
            slot_builder.setReturnType(slot_signature->result->name());

        slot_builder.setRevision(slot_signature->revision);
    }

    // Add the properties to the meta-object.
    QMapIterator<uint, PropertyData> it(pprops);

    while (it.hasNext())
    {
        it.next();

        const PropertyData &pprop = it.value();
        const char *prop_name = PyBytes_AsString(pprop.first);
        qpycore_pyqtProperty *pp = (qpycore_pyqtProperty *)pprop.second;
        int notifier_id;

        if (pp->pyqtprop_notify)
        {
            qpycore_pyqtSignal *ps = (qpycore_pyqtSignal *)pp->pyqtprop_notify;
            const QByteArray &sig = ps->parsed_signature->signature;

            notifier_id = builder.indexOfSignal(sig.mid(1));

            if (notifier_id < 0)
            {
                PyErr_Format(PyExc_TypeError,
                        "the notify signal '%s' was not defined in this class",
                        sig.constData() + 1);

                // Note that we leak the property name.
                return 0;
            }
        }
        else
        {
            notifier_id = -1;
        }

        // A Qt v5 revision 7 meta-object holds the QMetaType::Type of the type
        // or its name if it is unresolved (ie. not known to the type system).
        // In Qt v4 both are held.  For QObject sub-classes Chimera will fall
        // back to the QMetaType::QObjectStar if there is no specific meta-type
        // for the sub-class.  This means that, for Qt v4,
        // QMetaProperty::read() can handle the type.  However, Qt v5 doesn't
        // know that the unresolved type is a QObject sub-class.  Therefore we
        // have to tell it that the property is a QObject, rather than the
        // sub-class.  This means that QMetaProperty.typeName() will always
        // return "QObject*".
        QByteArray prop_type;

        if (pp->pyqtprop_parsed_type->metatype.id() == QMetaType::QObjectStar)
        {
            // However, if the type is a Python sub-class of QObject then we
            // use the name of the Python type.  This anticipates that the type
            // is one that will be proxied by QML at some point.
            if (pp->pyqtprop_parsed_type->typeDef() == sipType_QObject)
            {
                prop_type = sipPyTypeName(
                        (PyTypeObject *)pp->pyqtprop_parsed_type->py_type());
                prop_type.append('*');
            }
            else
            {
                prop_type = "QObject*";
            }
        }
        else
        {
            prop_type = pp->pyqtprop_parsed_type->name();
        }

        // Note that there is an addProperty() overload that also takes a
        // QMetaType argument (rather than call QMetaType::fromName() as this
        // overload does) so we could pass the QMetaType that the parser
        // created.  However this breaks properties that are application Python
        // enums which have a parsed QMetaType that describes an int whereas Qt
        // seems to need an invalid QMetaType for these to work.
        QMetaPropertyBuilder prop_builder = builder.addProperty(
                QByteArray(prop_name), prop_type, notifier_id);

        // Reset the defaults.
        prop_builder.setReadable(false);
        prop_builder.setWritable(false);

        // Enum or flag.
        if (pp->pyqtprop_parsed_type->isEnumOrFlag())
        {
            prop_builder.setEnumOrFlag(true);
        }

        if (pp->pyqtprop_get && PyCallable_Check(pp->pyqtprop_get))
        {
            // Readable.
            prop_builder.setReadable(true);
        }

        if (pp->pyqtprop_set && PyCallable_Check(pp->pyqtprop_set))
        {
            // Writable.
            prop_builder.setWritable(true);

            // See if the name of the setter follows the Designer convention.
            // If so tell the UI compilers not to use setProperty().
            PyObject *setter_name_obj = PyObject_GetAttr(pp->pyqtprop_set,
                    qpycore_dunder_name);

            if (setter_name_obj)
            {
                QByteArray ascii = qpycore_convert_ASCII(setter_name_obj);
                Py_DECREF(setter_name_obj);

                if (ascii.startsWith("set") && ascii[3] == (char)toupper(prop_name[0]) && ascii.mid(4) == prop_name + 1)
                    prop_builder.setStdCppSet(true);
            }

            PyErr_Clear();
        }

        if (pp->pyqtprop_reset && PyCallable_Check(pp->pyqtprop_reset))
        {
            // Resettable.
            prop_builder.setResettable(true);
        }

        // Add the property flags.
        prop_builder.setDesignable(pp->pyqtprop_flags & 0x00001000);
        prop_builder.setScriptable(pp->pyqtprop_flags & 0x00004000);
        prop_builder.setStored(pp->pyqtprop_flags & 0x00010000);
        prop_builder.setUser(pp->pyqtprop_flags & 0x00100000);
        prop_builder.setConstant(pp->pyqtprop_flags & 0x00000400);
        prop_builder.setFinal(pp->pyqtprop_flags & 0x00000800);

        prop_builder.setRevision(pp->pyqtprop_revision);

        // Save the property data for qt_metacall().  (We already have a
        // reference.)
        qo->pprops.append(pp);

        // We've finished with the property name.
        Py_DECREF(pprop.first);
    }

    // Build the meta-object.
    qo->mo = builder.toMetaObject();

    // Save the meta-object in any enums/flags.
    for (int i = 0; i < penums.count(); ++i)
    {
        const EnumFlag &ef = penums.at(i);

        static_cast<EnumFlagMetaTypeInterface *>(
                const_cast<QtPrivate::QMetaTypeInterface *>(
                        ef.metaType.iface()))->metaObject = qo->mo;
    }

    // Save for next time.
    sipSetTypeUserData(wt, qo);

    return qo;
}


// Trawl a type's hierarchy looking for any enums, slots, signals or
// properties.
static int trawl_hierarchy(PyTypeObject *pytype, qpycore_metaobject *qo,
        QMetaObjectBuilder &builder, QList<EnumFlag> &penums,
        QList<const qpycore_pyqtSignal *> &psigs,
        QMap<uint, PropertyData> &pprops)
{
    if (trawl_type(pytype, qo, builder, penums, psigs, pprops) < 0)
        return -1;

    PyObject *tp_bases;

    if (PyType_HasFeature(pytype, Py_TPFLAGS_HEAPTYPE))
        tp_bases = reinterpret_cast<PyObject *>(
                PyType_GetSlot(pytype, Py_tp_bases));
    else
        tp_bases = 0;

    if (!tp_bases)
        return 0;

    Q_ASSERT(PyTuple_Check(tp_bases));

    for (Py_ssize_t i = 0; i < PyTuple_Size(tp_bases); ++i)
    {
        PyTypeObject *sup = (PyTypeObject *)PyTuple_GetItem(tp_bases, i);

        if (PyType_IsSubtype(sup, sipTypeAsPyTypeObject(sipType_QObject)))
            continue;

        if (trawl_hierarchy(sup, qo, builder, penums, psigs, pprops) < 0)
            return -1;
    }

    return 0;
}


// Trawl a type's dict looking for any enums, slots, signals or properties.
static int trawl_type(PyTypeObject *pytype, qpycore_metaobject *qo,
        QMetaObjectBuilder &builder, QList<EnumFlag> &penums,
        QList<const qpycore_pyqtSignal *> &psigs,
        QMap<uint, PropertyData> &pprops)
{
    int rc = 0;
    Py_ssize_t pos = 0;
    PyObject *key, *value, *dict;

    if ((dict = sipPyTypeDictRef(pytype)) == NULL)
        return -1;

    while (PyDict_Next(dict, &pos, &key, &value))
    {
        // See if it is an enum.
        EnumFlag enum_flag = qpycore_pop_enum_flag(value);
        if (!enum_flag.name.isNull())
        {
            penums.append(enum_flag);
            continue;
        }

        // See if it is a slot, ie. it has been decorated with pyqtSlot().
        PyObject *sig_obj = PyObject_GetAttr(value,
                qpycore_dunder_pyqtsignature);

        if (sig_obj)
        {
            // Make sure it is a list and not some legitimate attribute that
            // happens to use our special name.
            if (PyList_Check(sig_obj))
            {
                for (Py_ssize_t i = 0; i < PyList_Size(sig_obj); ++i)
                {
                    // Set up the skeleton slot.
                    PyObject *decoration = PyList_GetItem(sig_obj, i);
                    Chimera::Signature *slot_signature = Chimera::Signature::fromPyObject(decoration);

                    // Check if a slot of the same signature has already been
                    // defined.  This typically happens with sub-classed
                    // mixins.
                    bool overridden = false;

                    for (int i = 0; i < qo->pslots.size(); ++i)
                        if (qo->pslots.at(i)->slotSignature()->signature == slot_signature->signature)
                        {
                            overridden = true;
                            break;
                        }

                    if (!overridden)
                        qo->pslots.append(
                                new PyQtSlot(value, true, slot_signature));
                }
            }

            Py_DECREF(sig_obj);
        }
        else
        {
            PyErr_Clear();

            // Make sure the key is an ASCII string.  Delay the error checking
            // until we know we actually need it.
            const char *ascii_key = sipString_AsASCIIString(&key);

            // See if the value is of interest.
            if (PyObject_TypeCheck(value, qpycore_pyqtProperty_TypeObject))
            {
                // It is a property.

                if (!ascii_key)
                {
                    rc = -1;
                    break;
                }

                Py_INCREF(value);

                qpycore_pyqtProperty *pp = (qpycore_pyqtProperty *)value;

                pprops.insert(pp->pyqtprop_sequence, PropertyData(key, value));

                // See if the property has a scope.  If so, collect all
                // QMetaObject pointers that are not in the super-class
                // hierarchy.
                const QMetaObject *mo = get_scope_qmetaobject(pp->pyqtprop_parsed_type);

                if (mo)
                    builder.addRelatedMetaObject(mo);
            }
            else if (PyObject_TypeCheck(value, qpycore_pyqtSignal_TypeObject))
            {
                // It is a signal.

                if (!ascii_key)
                {
                    rc = -1;
                    break;
                }

                qpycore_pyqtSignal *ps = (qpycore_pyqtSignal *)value;

                // Make sure the signal has a name.
                qpycore_set_signal_name(ps, sipPyTypeName(pytype), ascii_key);

                // Add all the overloads.
                do
                {
                    psigs.append(ps);
                    ps = ps->next;
                }
                while (ps);

                Py_DECREF(key);
            }
            else
            {
                PyErr_Clear();
            }
        }
    }

    Py_DECREF(dict);

    return rc;
}


// Return the QMetaObject for an enum type's scope.
static const QMetaObject *get_scope_qmetaobject(const Chimera *ct)
{
    // Check it is an enum.  Note that (starting with PyQt v6.8) we can return
    // a value for all enums/flags.
    if (!ct->isEnumOrFlag())
        return 0;

    // Check it has a scope.
    if (!ct->typeDef())
        return 0;

    const sipTypeDef *td = sipTypeScope(ct->typeDef());

    if (!td)
        return 0;

    // Check the scope is wrapped by PyQt.
    if (!qpycore_is_pyqt_type(td))
        return 0;

    return qpycore_get_qmetaobject((sipWrapperType *)sipTypeAsPyTypeObject(td));
}


// Return the QMetaObject for a type.
const QMetaObject *qpycore_get_qmetaobject(sipWrapperType *wt,
        const sipTypeDef *base_td)
{
    if (wt && sipIsUserType(wt))
    {
        const qpycore_metaobject *qo = qpycore_get_dynamic_metaobject(wt);

        if (qo)
            return qo->mo;
    }

    // Get the static meta-object if there is one.
    if (!base_td)
    {
        if (!wt)
            return 0;

        base_td = sipTypeFromPyTypeObject((PyTypeObject *)wt);

        if (!base_td)
            return 0;
    }

    const pyqt6ClassPluginDef *plugin = reinterpret_cast<const pyqt6ClassPluginDef *>(sipTypePluginData(base_td));

    if (!plugin)
        return 0;

    return reinterpret_cast<const QMetaObject *>(plugin->static_metaobject);
}
