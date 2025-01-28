// This is the implementation of the Qt meta-type support for enums/flags.
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


#include <QByteArray>
#include <QMetaType>

#include "qpycore_enums_flags_metatype.h"


// Forward declarations.
static const QMetaObject *get_enum_metaobject(
        const QtPrivate::QMetaTypeInterface *mti);


// Register an enum/flag with Qt and return the QMetaType.
QMetaType qpycore_register_enum_metatype(const QByteArray &fq_cpp_name,
        bool unsigned_enum)
{
    EnumFlagMetaTypeInterface *mti = new EnumFlagMetaTypeInterface;

    // Define the meta-type interface.
    mti->revision = QtPrivate::QMetaTypeInterface::CurrentRevision;
    mti->typeId = 0;
    mti->metaObjectFn = get_enum_metaobject;
    mti->name = qstrdup(fq_cpp_name.constData());

    if (unsigned_enum)
    {
        mti->alignment = alignof(unsigned);
        mti->size = sizeof(unsigned);
        mti->flags = QtPrivate::QMetaTypeForType<unsigned>::Flags | QMetaType::IsEnumeration | QMetaType::IsUnsignedEnumeration;
        mti->defaultCtr = QtPrivate::QMetaTypeForType<unsigned>::getDefaultCtr();
        mti->copyCtr = QtPrivate::QMetaTypeForType<unsigned>::getCopyCtr();
        mti->moveCtr = QtPrivate::QMetaTypeForType<unsigned>::getMoveCtr();
        mti->dtor = QtPrivate::QMetaTypeForType<unsigned>::getDtor();
        mti->equals = QtPrivate::QEqualityOperatorForType<unsigned>::equals;
        mti->lessThan = QtPrivate::QLessThanOperatorForType<unsigned>::lessThan;
        mti->debugStream = QtPrivate::QDebugStreamOperatorForType<unsigned>::debugStream;
        mti->dataStreamOut = QtPrivate::QDataStreamOperatorForType<unsigned>::dataStreamOut;
        mti->dataStreamIn = QtPrivate::QDataStreamOperatorForType<unsigned>::dataStreamIn;
        mti->legacyRegisterOp = QtPrivate::QMetaTypeForType<unsigned>::getLegacyRegister();
    }
    else
    {
        mti->alignment = alignof(int);
        mti->size = sizeof(int);
        mti->flags = QtPrivate::QMetaTypeForType<int>::Flags | QMetaType::IsEnumeration;
        mti->defaultCtr = QtPrivate::QMetaTypeForType<int>::getDefaultCtr();
        mti->copyCtr = QtPrivate::QMetaTypeForType<int>::getCopyCtr();
        mti->moveCtr = QtPrivate::QMetaTypeForType<int>::getMoveCtr();
        mti->dtor = QtPrivate::QMetaTypeForType<int>::getDtor();
        mti->equals = QtPrivate::QEqualityOperatorForType<int>::equals;
        mti->lessThan = QtPrivate::QLessThanOperatorForType<int>::lessThan;
        mti->debugStream = QtPrivate::QDebugStreamOperatorForType<int>::debugStream;
        mti->dataStreamOut = QtPrivate::QDataStreamOperatorForType<int>::dataStreamOut;
        mti->dataStreamIn = QtPrivate::QDataStreamOperatorForType<int>::dataStreamIn;
        mti->legacyRegisterOp = QtPrivate::QMetaTypeForType<int>::getLegacyRegister();
    }

    mti->metaObject = nullptr;

    // Create the meta-type.
    QMetaType mt = QMetaType(mti);

    // Register the meta-type.
    mt.registerType();

    return mt;
}


// Return the containing meta-object for a registered enum.
static const QMetaObject *get_enum_metaobject(
        const QtPrivate::QMetaTypeInterface *mti)
{
    return static_cast<const EnumFlagMetaTypeInterface *>(mti)->metaObject;
}
