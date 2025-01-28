// This is the implementation of the Chimera class.
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
#include <QMetaType>

#include "qpycore_chimera.h"
#include "qpycore_misc.h"
#include "qpycore_pyqtpyobject.h"
#include "qpycore_types.h"

#include "sipAPIQtCore.h"


// The registered user-defined Python enum types.
QHash<PyObject *, QByteArray> Chimera::_py_enum_types;

// The cache of previously parsed argument type lists.
QHash<QByteArray, QList<const Chimera *> > Chimera::_previously_parsed;

// The registered QVariant to PyObject convertors.
Chimera::FromQVariantConvertors Chimera::registeredFromQVariantConvertors;

// The registered PyObject to QVariant convertors.
Chimera::ToQVariantConvertors Chimera::registeredToQVariantConvertors;

// The registered PyObject to QVariant data convertors.
Chimera::ToQVariantDataConvertors Chimera::registeredToQVariantDataConvertors;


// The base class for convenience wrappers around mapped type convertors.
class BaseMappedTypeConvertor
{
public:
    BaseMappedTypeConvertor(const char *name) : td(nullptr), _name(name) {}

protected:
    void ensureType();
    const sipTypeDef *td;

private:
    const char *_name;
};


// Ensure that the type definition is available.
void BaseMappedTypeConvertor::ensureType()
{
    if (!td)
    {
        td = sipFindType(_name);
        Q_ASSERT(td);
    }
}


// A convenience wrapper around a mapped type convertor.
template <class T>
class MappedTypeConvertor : public BaseMappedTypeConvertor
{
public:
    MappedTypeConvertor(const char *name) : BaseMappedTypeConvertor(name) {}

    PyObject *fromMappedType(void *cpp);
    int toMappedType(PyObject *py, T &cpp);
};


// Convert an instance of the mapped type to a Python object.  A NULL pointer
// is returned if there was an error.
template <class T>
PyObject *MappedTypeConvertor<T>::fromMappedType(void *cpp)
{
    ensureType();

    return sipConvertFromNewType(new T(*reinterpret_cast<T *>(cpp)), td, NULL);
}


// Convert a Python object to an instance of the mapped type.  A non-zero value
// is returned if there was an error.
template <class T>
int MappedTypeConvertor<T>::toMappedType(PyObject *py, T &cpp)
{
    ensureType();

    int state, iserr = 0;
    void *p = sipForceConvertToType(py, td, NULL, SIP_NOT_NONE, &state,
            &iserr);

    if (!iserr)
    {
        cpp = *reinterpret_cast<T *>(p);
        sipReleaseType(p, td, state);
    }

    return iserr;
}


// The different mapped type convertors.
static MappedTypeConvertor<QVariantHash> variant_hash(
        "QHash<QString,QVariant>");
static MappedTypeConvertor<QVariantList> variant_list("QList<QVariant>");
static MappedTypeConvertor<QVariantMap> variant_map("QMap<QString,QVariant>");
static MappedTypeConvertor<QVariantPair> variant_pair(
        "std::pair<QVariant,QVariant>");


// Construct an invalid type.
Chimera::Chimera()
    : _type(0), _py_type(0), _inexact(false), _is_qflags(false),
      _is_char_star(false)
{
}


// Construct a copy.
Chimera::Chimera(const Chimera &other)
{
    _type = other._type;

    _py_type = other._py_type;
    Py_XINCREF((PyObject *)_py_type);
    
    metatype = other.metatype;
    _inexact = other._inexact;
    _is_qflags = other._is_qflags;
    _name = other._name;
    _is_char_star = other._is_char_star;
}


// Destroy the type.
Chimera::~Chimera()
{
    Py_XDECREF((PyObject *)_py_type);
}


// Register the type and pseudo fully qualified C++ name of a user-defined
// Python enum.
void Chimera::registerPyEnum(PyObject *enum_type,
        const QByteArray &fq_cpp_name)
{
    Py_INCREF(enum_type);
    _py_enum_types.insert(enum_type, fq_cpp_name);
}


// Parse an object as a type.
const Chimera *Chimera::parse(PyObject *obj)
{
    Chimera *ct = new Chimera;
    bool parse_ok;

    if (PyType_Check(obj))
    {
        // Parse the type object.
        parse_ok = ct->parse_py_type((PyTypeObject *)obj);

        if (!parse_ok)
            raiseParseException(obj);
    }
    else
    {
        const char *cpp_type_name = sipString_AsASCIIString(&obj);

        if (cpp_type_name)
        {
            // Always use normalised type names so that we have a consistent
            // standard.
            QByteArray norm_name = QMetaObject::normalizedType(cpp_type_name);
            Py_DECREF(obj);

            // Parse the type name.
            parse_ok = ct->parse_cpp_type(norm_name);

            if (!parse_ok)
                raiseParseCppException(cpp_type_name);
        }
        else
        {
            parse_ok = false;
        }
    }

    if (!parse_ok)
    {
        delete ct;
        return 0;
    }

    return ct;
}


// Parse a C++ type name as a type.
const Chimera *Chimera::parse(const QByteArray &name)
{
    Chimera *ct = new Chimera;

    if (!ct->parse_cpp_type(name))
    {
        delete ct;

        raiseParseCppException(name.constData());

        return 0;
    }

    return ct;
}


// Parse a meta-property as a type.
const Chimera *Chimera::parse(const QMetaProperty &mprop)
{
    Chimera *ct = new Chimera;
    QByteArray type_name;

    // The type name of an enum property sometimes includes the scope in the
    // name and sometimes doesn't (maybe only within the scope itself).
    // Therefore get the name of the meta-enum.
    if (mprop.isEnumType())
    {
        QMetaEnum menum = mprop.enumerator();

        type_name = QByteArray(menum.name());

        if (menum.scope())
            type_name.prepend("::").prepend(menum.scope());

        // If it is a QFlag then we have the typedef name rather than the
        // underlying type which SIP knows about, therefore resolve the typedef
        // name.
        if (mprop.isFlagType())
        {
            const char *resolved = sipResolveTypedef(type_name.constData());

            if (resolved)
                type_name = QByteArray(resolved);
        }
    }
    else
    {
        type_name = QByteArray(mprop.typeName());
    }

    ct->_type = sipFindType(type_name.constData());
    ct->metatype = mprop.metaType();
    ct->_inexact = true;
    ct->_is_qflags = mprop.isFlagType();
    ct->_name = type_name;

    return ct;
}


// Parse a normalised C++ signature as a list of types.
Chimera::Signature *Chimera::parse(const QByteArray &sig, const char *context)
{
    // Extract the argument list.
    int start_idx = sig.indexOf('(');

    if (start_idx < 0)
        start_idx = 0;
    else
        ++start_idx;

    int end_idx = sig.lastIndexOf(')');

    int len;

    if (end_idx < start_idx)
        len = -1;
    else
        len = end_idx - start_idx;

    // Parse each argument type.
    Chimera::Signature *parsed_sig = new Chimera::Signature(sig, true);

    if (len > 0)
    {
        QByteArray args_str = sig.mid(start_idx, len);

        // Check we haven't already done it.
        QList<const Chimera *> parsed_args = _previously_parsed.value(args_str);

        if (parsed_args.isEmpty())
        {
            int i, arg_start, template_level;

            i = arg_start = template_level = 0;

            // Extract each argument allowing for commas in templates.
            for (;;)
            {
                char ch = (i < args_str.size() ? args_str.at(i) : '\0');
                QByteArray arg;

                switch (ch)
                {
                case '<':
                    ++template_level;
                    break;

                case '>':
                    --template_level;
                    break;

                case '\0':
                    arg = args_str.mid(arg_start, i - arg_start);
                    break;

                case ',':
                    if (template_level == 0)
                    {
                        arg = args_str.mid(arg_start, i - arg_start);
                        arg_start = i + 1;
                    }

                    break;
                }

                if (!arg.isEmpty())
                {
                    Chimera *ct = new Chimera;

                    if (!ct->parse_cpp_type(arg))
                    {
                        delete ct;
                        delete parsed_sig;
                        qDeleteAll(parsed_args.constBegin(),
                                parsed_args.constEnd());

                        raiseParseCppException(arg.constData(), context);

                        return 0;
                    }

                    parsed_args.append(ct);

                    if (ch == '\0')
                        break;
                }

                ++i;
            }

            // Only parse once.
            _previously_parsed.insert(args_str, parsed_args);
        }

        parsed_sig->parsed_arguments = parsed_args;
    }

    return parsed_sig;
}


// Parses a C++ signature given as a Python tuple of types and an optional
// name.  Return 0 if there was an error.
Chimera::Signature *Chimera::parse(PyObject *types, const char *name,
        const char *context)
{
    if (!name)
        name = "";

    Chimera::Signature *parsed_sig = new Chimera::Signature(name, false);

    parsed_sig->signature.append('(');
    parsed_sig->py_signature.append('[');

    for (Py_ssize_t i = 0; i < PyTuple_Size(types); ++i)
    {
        PyObject *type = PyTuple_GetItem(types, i);
        const Chimera *parsed_type = parse(type);

        if (!parsed_type)
        {
            delete parsed_sig;

            raiseParseException(type, context);

            return 0;
        }

        parsed_sig->parsed_arguments.append(parsed_type);

        if (i > 0)
        {
            parsed_sig->signature.append(',');
            parsed_sig->py_signature.append(", ");
        }

        parsed_sig->signature.append(parsed_type->name());

        if (parsed_type->_py_type)
            parsed_sig->py_signature.append(sipPyTypeName(parsed_type->_py_type));
        else
            parsed_sig->py_signature.append(parsed_type->name());
    }

    parsed_sig->signature.append(')');
    parsed_sig->py_signature.append(']');

    return parsed_sig;
}


// Raise an exception after parse() of a Python type has failed.
void Chimera::raiseParseException(PyObject *type, const char *context)
{
    if (PyType_Check(type))
    {
        if (context)
            PyErr_Format(PyExc_TypeError,
                    "Python type '%s' is not supported as %s type",
                    sipPyTypeName((PyTypeObject *)type), context);
        else
            PyErr_Format(PyExc_TypeError, "unknown Python type '%s'",
                    sipPyTypeName((PyTypeObject *)type));
    }
    else
    {
        const char *cpp_type_name = sipString_AsASCIIString(&type);

        if (cpp_type_name)
        {
            raiseParseCppException(cpp_type_name, context);
            Py_DECREF(type);
        }
    }
}


// Raise an exception after parse() of a C++ type has failed.
void Chimera::raiseParseCppException(const char *type, const char *context)
{
    if (context)
        PyErr_Format(PyExc_TypeError,
                "C++ type '%s' is not supported as %s type", type, context);
    else
        PyErr_Format(PyExc_TypeError, "unknown C++ type '%s'", type);
}


// Parse the given Python type object.
bool Chimera::parse_py_type(PyTypeObject *type_obj)
{
    const sipTypeDef *td = sipTypeFromPyTypeObject(type_obj);

    if (td)
    {
        if (sipTypeIsNamespace(td))
            return false;

        _type = td;
        _name = sipTypeName(td);

        set_qflags(td);

        if (sipTypeIsEnum(td))
        {
            metatype = QMetaType::fromName(_name);

            // Unregistered enums are handled as integers.
            if (!metatype.isValid())
                metatype = QMetaType(QMetaType::Int);
        }
        else if (_is_qflags)
        {
            metatype = QMetaType(QMetaType::Int);
        }
        else
        {
            // If there is no assignment helper then assume it is a
            // pointer-type.
            if (!get_assign_helper())
                _name.append('*');

            metatype = QMetaType::fromName(_name);

            // If it is a user type then it must be a type that SIP knows
            // about but was registered by Qt.
            if (metatype.id() < QMetaType::User)
            {
                if (PyType_IsSubtype(type_obj, sipTypeAsPyTypeObject(sipType_QObject)))
                {
                    metatype = QMetaType(QMetaType::QObjectStar);
                }
                else if (sipIsUserType((sipWrapperType *)type_obj))
                {
                    // It is a non-QObject Python sub-class so make sure it
                    // gets wrapped in a PyQt_PyObject.
                    _type = 0;
                    metatype = QMetaType::fromType<PyQt_PyObject>();
                    _name.clear();
                }
            }
        }
    }
    else if (_py_enum_types.contains((PyObject *)type_obj))
    {
        // Note that we use the fully qualified name.
        _name = _py_enum_types.value((PyObject *)type_obj);

        metatype = QMetaType::fromName(_name);
    }
    else if (type_obj == &PyList_Type)
    {
        metatype = QMetaType(QMetaType::QVariantList);
    }
    else if (type_obj == &PyUnicode_Type)
    {
        _type = sipType_QString;
        metatype = QMetaType(QMetaType::QString);
    }
    else if (type_obj == &PyBool_Type)
    {
        metatype = QMetaType(QMetaType::Bool);
    }
    else if (type_obj == &PyLong_Type)
    {
        // We choose to map to a C++ int for the same reasons as above and to
        // be consistent with Python3 where everything is a long object.  If
        // this isn't appropriate the user can always use a string to specify
        // the exact C++ type they want.
        metatype = QMetaType(QMetaType::Int);
        _inexact = true;
    }
    else if (type_obj == &PyFloat_Type)
    {
        metatype = QMetaType(QMetaType::Double);
    }
    else if (type_obj == sipVoidPtr_Type)
    {
        metatype = QMetaType(QMetaType::VoidStar);
        _name = "void*";
    }

    // Fallback to using a PyQt_PyObject.
    if (!metatype.isValid())
        metatype = QMetaType::fromType<PyQt_PyObject>();

    // If there is no name so far then use the meta-type name.
    if (_name.isEmpty())
        _name = metatype.name();

    _py_type = type_obj;
    Py_INCREF((PyObject *)_py_type);

    return true;
}


// Set the internal QFlags flag.
void Chimera::set_qflags(const sipTypeDef *td)
{
    if (sipTypeIsMapped(td) && qpycore_is_pyqt_type(_type))
    {
        const pyqt6MappedTypePluginDef *plugin = reinterpret_cast<const pyqt6MappedTypePluginDef *>(sipTypePluginData(_type));

        if (plugin)
            _is_qflags = plugin->flags & 0x01;
    }
}


// Update a C++ type so that any typedefs are resolved.
QByteArray Chimera::resolve_types(const QByteArray &type)
{
    // Split into a base type and a possible list of template arguments.
    QByteArray resolved = type.simplified();

    // Get the raw type, ie. without any "const", "&" or "*".
    QByteArray raw_type;
    int original_raw_start;

    if (resolved.startsWith("const "))
        original_raw_start = 6;
    else
        original_raw_start = 0;

    raw_type = resolved.mid(original_raw_start);

    while (raw_type.endsWith('&') || raw_type.endsWith('*') || raw_type.endsWith(' '))
        raw_type.chop(1);

    int original_raw_len = raw_type.size();

    if (original_raw_len == 0)
        return QByteArray();

    // Get any template arguments.
    QList<QByteArray> args;
    int tstart = raw_type.indexOf('<');

    if (tstart >= 0)
    {
        // Make sure the template arguments are terminated.
        if (!raw_type.endsWith('>'))
            return QByteArray();

        // Split the template arguments taking nested templates into account.
        int depth = 1, arg_start = tstart + 1;

        for (int i = arg_start; i < raw_type.size(); ++i)
        {
            int arg_end = -1;
            char ch = raw_type.at(i);

            if (ch == '<')
            {
                ++depth;
            }
            else if (ch == '>')
            {
                --depth;

                if (depth < 0)
                    return QByteArray();

                if (depth == 0)
                    arg_end = i;
            }
            else if (ch == ',' && depth == 1)
            {
                arg_end = i;
            }

            if (arg_end >= 0)
            {
                QByteArray arg = resolve_types(raw_type.mid(arg_start, arg_end - arg_start));

                if (arg.isEmpty())
                    return QByteArray();

                args.append(arg);

                arg_start = arg_end + 1;
            }
        }

        if (depth != 0)
            return QByteArray();

        // Remove the template arguments.
        raw_type.truncate(tstart);
    }

    // Expand any typedef.
    const char *base_type = sipResolveTypedef(raw_type.constData());

    if (base_type)
        raw_type = base_type;

    // Add any (now resolved) template arguments.
    if (args.count() > 0)
    {
        raw_type.append('<');

        for (QList<QByteArray>::const_iterator it = args.begin();;)
        {
            raw_type.append(*it);

            if (++it == args.end())
                break;

            raw_type.append(',');
        }

        if (raw_type.endsWith('>'))
            raw_type.append(' ');

        raw_type.append('>');
    }

    // Replace the original raw type with the resolved one.
    resolved.replace(original_raw_start, original_raw_len, raw_type);

    return resolved;
}


// Parse the given C++ type name.
bool Chimera::parse_cpp_type(const QByteArray &type)
{
    _name = type;
    QByteArray nonconst_name = type.mid(type.startsWith("const ") ? 6 : 0);

    // Resolve any types.
    QByteArray resolved = resolve_types(nonconst_name);

    if (resolved.isEmpty())
        return false;

    // See if the type is known to Qt.
    metatype = QMetaType::fromName(resolved);

    // If not then use the PyQt_PyObject wrapper for now.
    bool valid = metatype.isValid();

    if (!valid)
        metatype = QMetaType::fromType<PyQt_PyObject>();

    // See if the type (without a pointer) is known to SIP.
    bool is_ptr = resolved.endsWith('*');

    if (is_ptr)
    {
        resolved.chop(1);

        if (resolved.endsWith('*'))
            return false;
    }

    _type = sipFindType(resolved.constData());

    // If we didn't find the type and we have resolved some typedefs then try
    // again with the original type.  This means that QVector<qreal> will work
    // as a signal argument type.  It may be that we should always lookup the
    // original type - but we don't want to risk breaking things.
    if (!_type && nonconst_name != resolved)
        _type = sipFindType(nonconst_name.constData());

    if (!_type)
    {
        // This is the only fundamental pointer type recognised by Qt.
        if (metatype.id() == QMetaType::VoidStar)
            return true;

        // This is 'int', 'bool', etc.
        if (metatype != QMetaType::fromType<PyQt_PyObject>() && !is_ptr)
            return true;

        if (resolved == "char" && is_ptr)
        {
            // This is (hopefully) '\0' terminated string.
            _is_char_star = true;

            return true;
        }

        // This is an explicit 'PyQt_PyObject'.
        if (resolved == "PyQt_PyObject" && !is_ptr)
            return true;

        return false;
    }

    if (sipTypeIsNamespace(_type))
        return false;

    set_qflags(_type);

    // We always pass these as integers.
    if ((!valid && sipTypeIsEnum(_type)) || _is_qflags)
        metatype = QMetaType(QMetaType::Int);

    if (is_ptr)
    {
        // We don't support pointers to enums.
        if (sipTypeIsEnum(_type))
        {
            _type = 0;
        }
        else if (sipTypeIsClass(_type))
        {
            PyTypeObject *type_obj = sipTypeAsPyTypeObject(_type);

            if (PyType_IsSubtype(type_obj, sipTypeAsPyTypeObject(sipType_QObject)))
                metatype = QMetaType(QMetaType::QObjectStar);
        }
    }

    return true;
}


// Convert a Python object to C++ at a given address.  This has a lot in common
// with the method that converts to a QVariant.  However, unlike that method,
// we have no control over the size of the destination storage and so must
// convert exactly as requested.
bool Chimera::fromPyObject(PyObject *py, void *cpp) const
{
    // Let any registered convertors have a go first.
    for (int i = 0; i < registeredToQVariantDataConvertors.count(); ++i)
    {
        bool ok;

        if (registeredToQVariantDataConvertors.at(i)(py, cpp, metatype, &ok))
            return ok;
    }

    int iserr = 0;

    PyErr_Clear();

    switch (typeId())
    {
    case QMetaType::Nullptr:
        *reinterpret_cast<std::nullptr_t *>(cpp) = nullptr;
        break;

    case QMetaType::Bool:
        *reinterpret_cast<bool *>(cpp) = PyLong_AsLong(py);
        break;

    case QMetaType::Int:
        *reinterpret_cast<int *>(cpp) = PyLong_AsLong(py);
        break;

    case QMetaType::UInt:
        *reinterpret_cast<unsigned int *>(cpp) = sipLong_AsUnsignedLong(py);
        break;

    case QMetaType::Double:
        *reinterpret_cast<double *>(cpp) = PyFloat_AsDouble(py);
        break;

    case QMetaType::VoidStar:
        *reinterpret_cast<void **>(cpp) = sipConvertToVoidPtr(py);
        break;

    case QMetaType::Long:
        *reinterpret_cast<long *>(cpp) = PyLong_AsLong(py);
        break;

    case QMetaType::LongLong:
        *reinterpret_cast<qlonglong *>(cpp) = PyLong_AsLongLong(py);
        break;

    case QMetaType::Short:
        *reinterpret_cast<short *>(cpp) = PyLong_AsLong(py);
        break;

    case QMetaType::Char:
        if (PyBytes_Check(py) && PyBytes_Size(py) == 1)
            *reinterpret_cast<char *>(cpp) = *PyBytes_AsString(py);
        else
            iserr = 1;
        break;

    case QMetaType::Char16:
        {
            char16_t ch;

            if (to_char16_t(py, ch))
                *reinterpret_cast<char16_t *>(cpp) = ch;
            else
                iserr = 1;

            break;
        }

    case QMetaType::Char32:
        {
            char32_t ch;

            if (to_char32_t(py, ch))
                *reinterpret_cast<char32_t *>(cpp) = ch;
            else
                iserr = 1;

            break;
        }

    case QMetaType::ULong:
        *reinterpret_cast<unsigned long *>(cpp) = sipLong_AsUnsignedLong(py);
        break;

    case QMetaType::ULongLong:
        *reinterpret_cast<qulonglong *>(cpp) = static_cast<qulonglong>(PyLong_AsUnsignedLongLong(py));
        break;

    case QMetaType::UShort:
        *reinterpret_cast<unsigned short *>(cpp) = sipLong_AsUnsignedLong(py);
        break;

    case QMetaType::UChar:
        if (PyBytes_Check(py) && PyBytes_Size(py) == 1)
            *reinterpret_cast<unsigned char *>(cpp) = *PyBytes_AsString(py);
        else
            iserr = 1;
        break;

    case QMetaType::Float:
        *reinterpret_cast<float *>(cpp) = PyFloat_AsDouble(py);
        break;

    case QMetaType::QObjectStar:
        *reinterpret_cast<void **>(cpp) = sipForceConvertToType(py,
                sipType_QObject, 0, SIP_NO_CONVERTORS, 0, &iserr);
        break;

    case QMetaType::QVariantHash:
        {
            QVariantHash qvh;

            iserr = variant_hash.toMappedType(py, qvh);

            if (!iserr)
                *reinterpret_cast<QVariantHash *>(cpp) = qvh;

            break;
        }

    case QMetaType::QVariantList:
        {
            QVariantList qvl;

            iserr = variant_list.toMappedType(py, qvl);

            if (!iserr)
                *reinterpret_cast<QVariantList *>(cpp) = qvl;

            break;
        }

    case QMetaType::QVariantMap:
        {
            QVariantMap qvm;

            iserr = variant_map.toMappedType(py, qvm);

            if (!iserr)
                *reinterpret_cast<QVariantMap *>(cpp) = qvm;

            break;
        }

    case QMetaType::QVariantPair:
        {
            QVariantPair qvp;

            iserr = variant_pair.toMappedType(py, qvp);

            if (!iserr)
                *reinterpret_cast<QVariantPair *>(cpp) = qvp;

            break;
        }

    case -1:
        {
            const char **ptr = reinterpret_cast<const char **>(cpp);

            if (PyBytes_Check(py))
                *ptr = PyBytes_AsString(py);
            else if (py == Py_None)
                *ptr = 0;
            else
                iserr = 1;

            break;
        }

    default:
        if (_type)
        {
            if (sipTypeIsEnum(_type))
            {
                *reinterpret_cast<int *>(cpp) = sipConvertToEnum(py, _type);
            }
            else if (_name.endsWith('*'))
            {
                // This must be a pointer-type.

                *reinterpret_cast<void **>(cpp) = sipForceConvertToType(py,
                        _type, 0, SIP_NO_CONVERTORS, 0, &iserr);
            }
            else
            {
                // This must be a value-type.

                sipAssignFunc assign = get_assign_helper();

                if (assign)
                {
                    int state;
                    void *value_class;

                    value_class = sipForceConvertToType(py, _type, 0,
                            SIP_NOT_NONE, &state, &iserr);

                    if (!iserr)
                        assign(cpp, 0, value_class);

                    sipReleaseType(value_class, _type, state);
                }
                else
                {
                    iserr = 1;
                }
            }
        }
        else if (metatype.flags() & QMetaType::IsEnumeration)
        {
            *reinterpret_cast<int *>(cpp) = get_enum_value(py);
        }
        else
        {
            iserr = 1;
        }
    }

    if (iserr || PyErr_Occurred())
    {
        PyErr_Format(PyExc_TypeError,
                "unable to convert a Python '%s' object to a C++ '%s' instance",
                sipPyTypeName(Py_TYPE(py)), _name.constData());

        return false;
    }

    return true;
}


// Return the assignment helper for the type, if any.
sipAssignFunc Chimera::get_assign_helper() const
{
    if (sipTypeIsClass(_type))
        return ((sipClassTypeDef *)_type)->ctd_assign;

    if (sipTypeIsMapped(_type))
        return ((sipMappedTypeDef *)_type)->mtd_assign;

    return 0;
}


// Convert a Python object to a QVariant.
bool Chimera::fromPyObject(PyObject *py, QVariant *var, bool strict) const
{
    // Deal with the simple case of wrapping the Python object rather than
    // converting it.
    if (_type != sipType_QVariant && metatype == QMetaType::fromType<PyQt_PyObject>())
    {
        // If the type was specified by a Python type (as opposed to
        // 'PyQt_PyObject') then check the object is an instance of it.
        if (_py_type && !PyObject_TypeCheck(py, _py_type))
            return false;

        *var = keep_as_pyobject(py);
        return true;
    }

    // Let any registered convertors have a go first.  However don't invoke
    // then for None because that is effectively reserved for representing a
    // null QString.
    if (py != Py_None)
    {
        for (int i = 0; i < registeredToQVariantConvertors.count(); ++i)
        {
            bool ok;

            if (registeredToQVariantConvertors.at(i)(py, *var, &ok))
                return ok;
        }
    }

    int iserr = 0, value_class_state;
    void *ptr_class, *value_class = 0;

    // Temporary storage for different types.
    union {
        std::nullptr_t tmp_nullptr_t;
        bool tmp_bool;
        int tmp_int;
        unsigned int tmp_unsigned_int;
        double tmp_double;
        void *tmp_void_ptr;
        long tmp_long;
        qlonglong tmp_qlonglong;
        short tmp_short;
        char tmp_char;
        char16_t tmp_char16_t;
        char32_t tmp_char32_t;
        unsigned long tmp_unsigned_long;
        qulonglong tmp_qulonglong;
        unsigned short tmp_unsigned_short;
        unsigned char tmp_unsigned_char;
        float tmp_float;
    } tmp_storage;

    void *variant_data = &tmp_storage;

    PyErr_Clear();

    QVariant variant;
    int metatype_used = metatype.id();

    switch (typeId())
    {
    case QMetaType::Nullptr:
        tmp_storage.tmp_nullptr_t = nullptr;
        break;

    case QMetaType::Bool:
        tmp_storage.tmp_bool = PyLong_AsLong(py);
        break;

    case QMetaType::Int:
        if (_inexact)
        {
            // Fit it into the smallest C++ type we can.

            qlonglong qll = PyLong_AsLongLong(py);

            if (PyErr_Occurred())
            {
                // Try again in case the value is unsigned and will fit with
                // the extra bit.

                PyErr_Clear();

                qulonglong qull = static_cast<qulonglong>(PyLong_AsUnsignedLongLong(py));

                if (PyErr_Occurred())
                {
                    // It won't fit into any C++ type so pass it as a Python
                    // object.

                    PyErr_Clear();

                    *var = keep_as_pyobject(py);
                    metatype_used = QMetaType::UnknownType;
                }
                else
                {
                    tmp_storage.tmp_qulonglong = qull;
                    metatype_used = QMetaType::ULongLong;
                }
            }
            else if ((qlonglong)(int)qll == qll)
            {
                // It fits in a C++ int.
                tmp_storage.tmp_int = qll;
            }
            else if ((qulonglong)(unsigned int)qll == (qulonglong)qll)
            {
                // The extra bit is enough for it to fit.
                tmp_storage.tmp_unsigned_int = qll;
                metatype_used = QMetaType::UInt;
            }
            else
            {
                // This fits.
                tmp_storage.tmp_qlonglong = qll;
                metatype_used = QMetaType::LongLong;
            }
        }
        else
        {
            // It must fit into a C++ int.
            tmp_storage.tmp_int = PyLong_AsLong(py);
        }

        break;

    case QMetaType::UInt:
        tmp_storage.tmp_unsigned_int = sipLong_AsUnsignedLong(py);
        break;

    case QMetaType::Double:
        tmp_storage.tmp_double = PyFloat_AsDouble(py);
        break;

    case QMetaType::VoidStar:
        tmp_storage.tmp_void_ptr = sipConvertToVoidPtr(py);
        break;

    case QMetaType::Long:
        tmp_storage.tmp_long = PyLong_AsLong(py);
        break;

    case QMetaType::LongLong:
        tmp_storage.tmp_qlonglong = PyLong_AsLongLong(py);
        break;

    case QMetaType::Short:
        tmp_storage.tmp_short = PyLong_AsLong(py);
        break;

    case QMetaType::Char:
        if (PyBytes_Check(py) && PyBytes_Size(py) == 1)
            tmp_storage.tmp_char = *PyBytes_AsString(py);
        else
            iserr = 1;
        break;

    case QMetaType::Char16:
        if (!to_char16_t(py, tmp_storage.tmp_char16_t))
            iserr = 1;
        break;

    case QMetaType::Char32:
        if (!to_char32_t(py, tmp_storage.tmp_char32_t))
            iserr = 1;
        break;

    case QMetaType::ULong:
        tmp_storage.tmp_unsigned_long = sipLong_AsUnsignedLong(py);
        break;

    case QMetaType::ULongLong:
        tmp_storage.tmp_qulonglong = static_cast<qulonglong>(PyLong_AsUnsignedLongLong(py));
        break;

    case QMetaType::UShort:
        tmp_storage.tmp_unsigned_short = sipLong_AsUnsignedLong(py);
        break;

    case QMetaType::UChar:
        if (PyBytes_Check(py) && PyBytes_Size(py) == 1)
            tmp_storage.tmp_unsigned_char = *PyBytes_AsString(py);
        else
            iserr = 1;
        break;

    case QMetaType::Float:
        tmp_storage.tmp_float = PyFloat_AsDouble(py);
        break;

    case QMetaType::QObjectStar:
        tmp_storage.tmp_void_ptr = sipForceConvertToType(py, sipType_QObject,
                0, SIP_NO_CONVERTORS, 0, &iserr);
        break;

    case QMetaType::QVariantHash:
        {
            QVariantHash qvh;

            iserr = variant_hash.toMappedType(py, qvh);

            if (!iserr)
            {
                *var = QVariant(qvh);
                metatype_used = QMetaType::UnknownType;
            }
            else if (!strict)
            {
                // Assume the failure is because the key was the wrong type.
                iserr = 0;
                PyErr_Clear();

                *var = keep_as_pyobject(py);
                metatype_used = QMetaType::UnknownType;
            }

            break;
        }

    case QMetaType::QVariantList:
        {
            QVariantList qvl;

            iserr = variant_list.toMappedType(py, qvl);

            if (!iserr)
            {
                *var = QVariant(qvl);
                metatype_used = QMetaType::UnknownType;
            }

            break;
        }

    case QMetaType::QVariantMap:
        {
            QVariantMap qvm;

            iserr = variant_map.toMappedType(py, qvm);

            if (!iserr)
            {
                *var = QVariant(qvm);
                metatype_used = QMetaType::UnknownType;
            }
            else if (!strict)
            {
                // Assume the failure is because the key was the wrong type.
                iserr = 0;
                PyErr_Clear();

                *var = keep_as_pyobject(py);
                metatype_used = QMetaType::UnknownType;
            }

            break;
        }

    case QMetaType::QVariantPair:
        {
            QVariantPair qvp;

            iserr = variant_pair.toMappedType(py, qvp);

            if (!iserr)
            {
                *var = QVariant::fromValue(qvp);
                metatype_used = QMetaType::UnknownType;
            }

            break;
        }

    case -1:
        metatype_used = QMetaType::VoidStar;

        if (PyBytes_Check(py))
            tmp_storage.tmp_void_ptr = PyBytes_AsString(py);
        else if (py == Py_None)
            tmp_storage.tmp_void_ptr = 0;
        else
            iserr = 1;

        break;

    default:
        if (_type)
        {
            if (sipTypeIsEnum(_type))
            {
                // Note that this will be a registered enum.
                tmp_storage.tmp_int = sipConvertToEnum(py, _type);
            }
            else if (_name.endsWith('*'))
            {
                ptr_class = sipForceConvertToType(py, _type, 0,
                        SIP_NO_CONVERTORS, 0, &iserr);

                variant_data = &ptr_class;
            }
            else
            {
                value_class = sipForceConvertToType(py, _type, 0,
                    SIP_NOT_NONE, &value_class_state, &iserr);

                variant_data = value_class;
            }
        }
        else if (metatype.flags() & QMetaType::IsEnumeration)
        {
            tmp_storage.tmp_int = get_enum_value(py);
        }
        else
        {
            // This is a class we don't recognise.
            iserr = 1;
        }
    }

    if (iserr || PyErr_Occurred())
    {
        PyErr_Format(PyExc_TypeError,
                "unable to convert a Python '%s' object to a C++ '%s' instance",
                sipPyTypeName(Py_TYPE(py)), _name.constData());

        iserr = 1;
    }
    else if (_type == sipType_QVariant)
    {
        *var = QVariant(*reinterpret_cast<QVariant *>(variant_data));
    }
    else if (metatype_used != QMetaType::UnknownType)
    {
        *var = QVariant(QMetaType(metatype_used), variant_data);
    }

    // Release any temporary value-class instance now that QVariant will have
    // made a copy.
    if (value_class)
        sipReleaseType(value_class, _type, value_class_state);

    return (iserr == 0);
}


// Convert a Python object to a QVariant based on the type of the object.
QVariant Chimera::fromAnyPyObject(PyObject *py, int *is_err)
{
    QVariant variant;

    if (py != Py_None)
    {
        // Let any registered convertors have a go first.
        for (int i = 0; i < registeredToQVariantConvertors.count(); ++i)
        {
            QVariant var;
            bool ok;

            if (registeredToQVariantConvertors.at(i)(py, var, &ok))
            {
                *is_err = !ok;

                return var;
            }
        }

        Chimera ct;

        if (ct.parse_py_type(Py_TYPE(py)))
        {
            // If the type is a dict then try and convert it to a QVariantMap
            // if possible.
            if (Py_TYPE(py) == &PyDict_Type)
                ct.metatype = QMetaType(QMetaType::QVariantMap);

            // The conversion is non-strict in case the type was a dict and we
            // can't convert it to a QVariantMap.
            if (!ct.fromPyObject(py, &variant, false))
            {
                *is_err = 1;
            }
        }
        else
        {
            *is_err = 1;
        }
    }

    return variant;
}


// Convert a QVariant to Python.
PyObject *Chimera::toPyObject(const QVariant &var) const
{
    if (_type != sipType_QVariant)
    {
        // For some reason (see qvariant_p.h) an invalid QVariant can be
        // returned when a QMetaType::Void is expected.
        if (!var.isValid() && metatype.id() == QMetaType::Void)
        {
            Py_INCREF(Py_None);
            return Py_None;
        }

        // Handle the reverse of non-strict conversions of dict to QVariantMap,
        // ie. we want a dict but we have a QVariantMap.
        if (metatype == QMetaType::fromType<PyQt_PyObject>() && _py_type == &PyDict_Type && var.metaType().id() == QMetaType::QVariantMap)
        {
            QVariantMap qvm = var.toMap();

            return variant_map.fromMappedType(&qvm);
        }

        // A sanity check.
        if (metatype != var.metaType())
        {
            // However having an Int QVariant value and having an invalid
            // QMetaType is the case that we are dealing with a user-defined
            // enum and so is valid.
            if (!(var.metaType().id() == QMetaType::Int && metatype.id() == QMetaType::UnknownType))
            {
                PyErr_Format(PyExc_TypeError,
                        "unable to convert a QVariant of type %d to a QMetaType of type %d",
                        var.metaType().id(), metatype.id());

                return 0;
            }
        }

        // Deal with the simple case of unwrapping a Python object rather than
        // converting it.
        if (metatype == QMetaType::fromType<PyQt_PyObject>())
        {
            PyQt_PyObject pyobj_wrapper = var.value<PyQt_PyObject>();

            if (!pyobj_wrapper.pyobject)
            {
                PyErr_SetString(PyExc_TypeError,
                        "unable to convert a QVariant back to a Python object");

                return 0;
            }

            Py_INCREF(pyobj_wrapper.pyobject);

            return pyobj_wrapper.pyobject;
        }
    }

    // Let any registered convertors have a go first.
    for (int i = 0; i < registeredFromQVariantConvertors.count(); ++i)
    {
        PyObject *py;

        if (registeredFromQVariantConvertors.at(i)(var, &py))
            return py;
    }

    return toPyObject(const_cast<void *>(var.constData()));
}


// Convert a C++ object at an arbitary address to Python.
PyObject *Chimera::toPyObject(void *cpp) const
{
    if (metatype == QMetaType::fromType<PyQt_PyObject>())
    {
        if (_is_char_star)
        {
            char *s = *reinterpret_cast<char **>(cpp);

            if (s)
                return PyBytes_FromString(s);

            Py_INCREF(Py_None);
            return Py_None;
        }

        if (_type && !sipTypeIsEnum(_type))
        {
            if (_name.endsWith('*'))
                cpp = *reinterpret_cast<void **>(cpp);

            return sipConvertFromType(cpp, _type, 0);
        }

        // Unwrap a Python object.
        PyQt_PyObject *pyobj_wrapper = reinterpret_cast<PyQt_PyObject *>(cpp);

        if (!pyobj_wrapper->pyobject)
        {
            PyErr_SetString(PyExc_TypeError,
                    "unable to convert a QVariant back to a Python object");

            return 0;
        }

        Py_INCREF(pyobj_wrapper->pyobject);

        return pyobj_wrapper->pyobject;
    }

    PyObject *py = 0;

    switch (metatype.id())
    {
    case QMetaType::Nullptr:
        py = Py_None;
        Py_INCREF(py);
        break;

    case QMetaType::Bool:
        py = PyBool_FromLong(*reinterpret_cast<bool *>(cpp));
        break;

    case QMetaType::Int:
        if (_is_qflags)
        {
            py = sipConvertFromType(cpp, _type, 0);
        }
        else
        {
            py = PyLong_FromLong(*reinterpret_cast<int *>(cpp));
        }

        break;

    case QMetaType::UInt:
        {
            long ui = *reinterpret_cast<unsigned int *>(cpp);

            if (ui < 0)
                py = PyLong_FromUnsignedLong((unsigned long)ui);
            else
                py = PyLong_FromLong(ui);

            break;
        }

    case QMetaType::Double:
        py = PyFloat_FromDouble(*reinterpret_cast<double *>(cpp));
        break;

    case QMetaType::VoidStar:
        py = sipConvertFromVoidPtr(*reinterpret_cast<void **>(cpp));
        break;

    case QMetaType::Long:
        py = PyLong_FromLong(*reinterpret_cast<long *>(cpp));
        break;

    case QMetaType::LongLong:
        py = PyLong_FromLongLong(*reinterpret_cast<qlonglong *>(cpp));
        break;

    case QMetaType::Short:
        py = PyLong_FromLong(*reinterpret_cast<short *>(cpp));
        break;

    case QMetaType::Char:
    case QMetaType::UChar:
        py = PyBytes_FromStringAndSize(reinterpret_cast<char *>(cpp), 1);
        break;

    case QMetaType::Char16:
        py = PyUnicode_DecodeUTF16(reinterpret_cast<char *>(cpp), 1, NULL,
                NULL);
        break;

    case QMetaType::Char32:
        py = PyUnicode_DecodeUTF32(reinterpret_cast<char *>(cpp), 1, NULL,
                NULL);
        break;

    case QMetaType::ULong:
        py = PyLong_FromUnsignedLong(*reinterpret_cast<unsigned long *>(cpp));
        break;

    case QMetaType::ULongLong:
        py = PyLong_FromUnsignedLongLong(*reinterpret_cast<qulonglong *>(cpp));
        break;

    case QMetaType::UShort:
        py = PyLong_FromLong(*reinterpret_cast<unsigned short *>(cpp));
        break;

    case QMetaType::Float:
        py = PyFloat_FromDouble(*reinterpret_cast<float *>(cpp));
        break;

    case QMetaType::QObjectStar:
        py = sipConvertFromType(*reinterpret_cast<void **>(cpp),
                sipType_QObject, 0);
        break;

    case QMetaType::QVariantHash:
        py = variant_hash.fromMappedType(cpp);
        break;

    case QMetaType::QVariantList:
        py = variant_list.fromMappedType(cpp);
        break;

    case QMetaType::QVariantMap:
        py = variant_map.fromMappedType(cpp);
        break;

    case QMetaType::QVariantPair:
        py = variant_pair.fromMappedType(cpp);
        break;

    default:
        if (_type)
        {
            if (sipTypeIsEnum(_type))
            {
                // Note that this will be a registered enum.
                py = sipConvertFromEnum(*reinterpret_cast<int *>(cpp), _type);
            }
            else if (_name.endsWith('*'))
            {
                py = sipConvertFromType(*reinterpret_cast<void **>(cpp),
                        _type, 0);
            }
            else
            {
                // Make a copy as it is a value type.
                void *copy = metatype.create(cpp);

                py = sipConvertFromNewType(copy, _type, 0);

                if (!py)
                    metatype.destroy(copy);
            }
        }
        else if (metatype.flags() & QMetaType::IsEnumeration)
        {
            py = get_enum_key(*reinterpret_cast<int *>(cpp));
        }
        else if (_name.contains("_QMLTYPE_"))
        {
            // These correspond to objects defined in QML.  We assume that they
            // are all sub-classes of QObject.  If this proves not to be the
            // case then we will have to look at the first part of _name (and
            // possibly move this code the the QtQml module).
            py = sipConvertFromType(*reinterpret_cast<void **>(cpp),
                    sipType_QObject, 0);
        }
        else if (_name.endsWith('*'))
        {
            // It's a pointer to an unknown type so convert it to a voidptr in
            // case that can be used.
            py = sipConvertFromVoidPtr(cpp);
        }
    }

    if (!py)
        PyErr_Format(PyExc_TypeError,
                "unable to convert a C++ '%s' instance to a Python object",
                _name.constData());

    return py;
}


// Convert a QVariant to a Python object based on the type of the object.
PyObject *Chimera::toAnyPyObject(const QVariant &var)
{
    if (!var.isValid())
    {
        Py_INCREF(Py_None);
        return Py_None;
    }

    const char *type_name = var.typeName();

    // Qt v5.8.0 changed the way it was handling null in QML.  We treat it as a
    // special case though there may be other implications still to be
    // discovered.
    if (qstrcmp(type_name, "std::nullptr_t") == 0)
    {
        Py_INCREF(Py_None);
        return Py_None;
    }

    const sipTypeDef *td = sipFindType(type_name);
    Chimera *ct = new Chimera;

    ct->_type = td;
    ct->_name = type_name;
    ct->metatype = var.metaType();

    if (td)
        ct->set_qflags(td);

    PyObject *py = ct->toPyObject(var);
    delete ct;

    return py;
}


// Wrap a Python object in a QVariant without any conversion.
QVariant Chimera::keep_as_pyobject(PyObject *py)
{
    PyQt_PyObject pyobj_wrapper(py);

    return QVariant(QMetaType::fromType<PyQt_PyObject>(), &pyobj_wrapper);
}


// Convert a single character Python string to a char16_t and return true if
// there was no error.
bool Chimera::to_char16_t(PyObject *py, char16_t &cpp)
{
    if (!PyUnicode_Check(py) || PyUnicode_GetLength(py) != 1)
        return false;

    PyObject *utf16 = PyUnicode_AsUTF16String(py);

    if (!utf16)
        return false;

    // Skip the BOM.
    cpp = reinterpret_cast<char16_t *>(PyBytes_AsString(utf16))[1];

    Py_DECREF(utf16);

    return true;
}


// Convert a single character Python string to a char32_t and return true if
// there was no error.
bool Chimera::to_char32_t(PyObject *py, char32_t &cpp)
{
    if (!PyUnicode_Check(py) || PyUnicode_GetLength(py) != 1)
        return false;

    PyObject *utf32 = PyUnicode_AsUTF32String(py);

    if (!utf32)
        return false;

    // Skip the BOM.
    cpp = reinterpret_cast<char32_t *>(PyBytes_AsString(utf32))[1];

    Py_DECREF(utf32);

    return true;
}


// Return true if the type is either a C++ or Python enum or flag.
bool Chimera::isEnumOrFlag() const
{
    if (_type && sipTypeIsEnum(_type))
        return true;

    if (_is_qflags)
        return true;

    return (_py_type ? _py_enum_types.contains((PyObject *)_py_type) : false);
}


// Return the value of a user-defined enum.
int Chimera::get_enum_value(PyObject *py) const
{
    PyObject *value_obj = PyObject_GetAttrString(py, "value");

    if (!value_obj)
        return 0;

    int value = PyLong_AsLong(value_obj);

    Py_DECREF(value_obj);

    return value;
}


// Return the key of a user-defined enum.
PyObject *Chimera::get_enum_key(int cpp) const
{
    PyObject *py = NULL;
    QHashIterator<PyObject *, QByteArray> it(_py_enum_types);

    while (it.hasNext())
    {
        it.next();

        if (it.value() == _name)
        {
            py = PyObject_CallFunction(it.key(), "(i)", cpp);
            break;
        }
    }

    return py;
}


// Convert a Python object to C++, allocating storage as necessary.
Chimera::Storage *Chimera::fromPyObjectToStorage(PyObject *py) const
{
    Chimera::Storage *st = new Chimera::Storage(this, py);

    if (!st->isValid())
    {
        delete st;
        st = 0;
    }

    return st;
}


// Create the storage for a type.
Chimera::Storage *Chimera::storageFactory() const
{
    return new Chimera::Storage(this);
}
