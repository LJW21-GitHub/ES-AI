// This contains the support for QOpenGLVersionFunctionsFactory.get().
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
#include <QOpenGLContext>
#include <QOpenGLVersionProfile>
#include <QOpenGLVersionFunctionsFactory>

#include "qpyopengl_api.h"


// Forward declarations.
int qpyopengl_add_constants(PyObject *obj);


// Create the Python object that wraps the requested OpenGL functions.
PyObject *qpyopengl_version_functions(
        const QOpenGLVersionProfile &version_profile, QOpenGLContext *context,
        PyObject *py_context)
{
    // Get a valid version profile if possible.
    QOpenGLVersionProfile vp(version_profile);

    if (!vp.isValid() && context)
        vp = QOpenGLVersionProfile(context->format());

    // Create the functions.
    QAbstractOpenGLFunctions *funcs = QOpenGLVersionFunctionsFactory::get(vp,
            context);

    if (!funcs)
    {
        Py_INCREF(Py_None);
        return Py_None;
    }

    // Qt doesn't allow us to introspect the functions to find which set we
    // have, so work out what it should be based on the version profile.
    const sipTypeDef *td;

#if !defined(SIP_FEATURE_PyQt_OpenGL_ES2)
    QByteArray name("QOpenGLFunctions_");

    QPair<int, int> version = vp.version();
    name.append(QByteArray::number(version.first));
    name.append('_');
    name.append(QByteArray::number(version.second));

    if (vp.hasProfiles())
    {
        switch (vp.profile())
        {
        case QSurfaceFormat::CoreProfile:
            name.append("_Core");
            break;

        case QSurfaceFormat::CompatibilityProfile:
            name.append("_Compatibility");
            break;

        default:
            ;
        }
    }

    td = sipFindType(name.constData());
    if (!td)
    {
        PyErr_Format(PyExc_TypeError, "%s is not supported by PyQt6",
                name.constData());

        return SIP_NULLPTR;
    }
#else
    td = sipType_QOpenGLFunctions_ES2;
#endif

    // Ownership is with the context.
    PyObject *funcs_obj = sipConvertFromType(funcs, td, py_context);

    if (funcs_obj)
    {
        if (qpyopengl_add_constants(funcs_obj) < 0)
        {
            Py_DECREF(funcs_obj);
            funcs_obj = SIP_NULLPTR;
        }
    }

    return funcs_obj;
}
