// This defines the interfaces to the Qt meta-type support for enums/flags.
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


#ifndef _QPYCORE_ENUMS_FLAGS_METATYPE_H
#define _QPYCORE_ENUMS_FLAGS_METATYPE_H


#include <QByteArray>
#include <QMetaType>


// Encapsulate the definition of a Python enum as required by QMetaEnum.
struct EnumFlagMetaTypeInterface : QtPrivate::QMetaTypeInterface
{
    const QMetaObject *metaObject;
};


QMetaType qpycore_register_enum_metatype(const QByteArray &fq_cpp_name,
        bool unsigned_enum);

#endif
