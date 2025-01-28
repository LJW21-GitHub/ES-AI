// This is the implementation of dataCache.
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

#include "qpyopengl_data_cache.h"


// The type object.
PyTypeObject *qpyopengl_dataCache_TypeObject;


// Forward declarations.
extern "C" {
static int dataCache_clear(PyObject *self);
static void dataCache_dealloc(PyObject *self);
static int dataCache_traverse(PyObject *self, visitproc visit, void *arg);
}


// Define the slots.
static PyType_Slot qpyopengl_dataCache_Slots[] = {
    {Py_tp_dealloc,     (void *)dataCache_dealloc},
    {Py_tp_clear,       (void *)dataCache_clear},
    {Py_tp_traverse,    (void *)dataCache_traverse},
    {0,                 0}
};


// Define the type.
static PyType_Spec qpyopengl_dataCache_Spec = {
    "PyQt6.QtGui.dataCache",
    sizeof (qpyopengl_dataCache),
    0,
    Py_TPFLAGS_DEFAULT|Py_TPFLAGS_HAVE_GC,
    qpyopengl_dataCache_Slots
};


// The dataCache clear function.
static int dataCache_clear(PyObject *self)
{
    qpyopengl_dataCache *data_cache = (qpyopengl_dataCache *)self;

    Array *uncached = data_cache->uncached;

    if (uncached)
    {
        data_cache->uncached = 0;

        uncached->clear();
        delete uncached;
    }

    PrimaryCache *pcache = data_cache->pcache;

    if (pcache)
    {
        data_cache->pcache = 0;

        PrimaryCache::const_iterator pit = pcache->constBegin();

        while (pit != pcache->constEnd())
        {
            delete *pit;
            ++pit;
        }

        delete pcache;
    }

    return 0;
}


// The dataCache dealloc function.
static void dataCache_dealloc(PyObject *self)
{
    PyObject_GC_UnTrack(self);

    dataCache_clear(self);

    PyObject_GC_Del(self);
}


// The dataCache traverse function.
static int dataCache_traverse(PyObject *self, visitproc visit, void *arg)
{
    qpyopengl_dataCache *data_cache = (qpyopengl_dataCache *)self;
    int vret;

    Array *uncached = data_cache->uncached;

    if (uncached)
    {
        vret = uncached->traverse(visit, arg);

        if (vret != 0)
            return vret;
    }

    PrimaryCache *pcache = data_cache->pcache;

    if (pcache)
    {
        PrimaryCache::const_iterator pit = pcache->constBegin();

        while (pit != pcache->constEnd())
        {
            PrimaryCacheEntry *pce = *pit;

            vret = pce->skey_0.traverse(visit, arg);

            if (vret != 0)
                return vret;

            SecondaryCache *scache = pce->skey_n;

            if (scache)
            {
                SecondaryCache::const_iterator sit = scache->constBegin();

                while (sit != scache->constEnd())
                {
                    vret = (*sit)->traverse(visit, arg);

                    if (vret != 0)
                        return vret;

                    ++sit;
                }
            }

            ++pit;
        }
    }

    return 0;
}


// Initialise the type and return true if there was no error.
bool qpyopengl_dataCache_init_type()
{
    qpyopengl_dataCache_TypeObject = (PyTypeObject *)PyType_FromSpec(
            &qpyopengl_dataCache_Spec);

    return qpyopengl_dataCache_TypeObject;
}


// Create a new data cache.
qpyopengl_dataCache *qpyopengl_dataCache_New()
{
    return (qpyopengl_dataCache *)PyType_GenericAlloc(
            qpyopengl_dataCache_TypeObject, 0);
}


// PrimaryCacheEntry ctor.
PrimaryCacheEntry::PrimaryCacheEntry() : skey_n(0)
{
}


// PrimaryCacheEntry dtor.
PrimaryCacheEntry::~PrimaryCacheEntry()
{
    if (skey_n)
    {
        SecondaryCache::const_iterator sit = skey_n->constBegin();

        while (sit != skey_n->constEnd())
        {
            delete (*sit);
            ++sit;
        }

        delete skey_n;

        skey_n = 0;
    }
}


// Array ctor.
Array::Array() : data(0)
{
    buffer.bi_obj = 0;
}


// Array dtor.
Array::~Array()
{
    clear();
}


// Clear the array.
void Array::clear()
{
    if (data)
    {
        sipFree(data);
        data = 0;
    }
    else if (buffer.bi_obj)
    {
        sipReleaseBufferInfo(&buffer);
    }
}


// Traverse the array.
int Array::traverse(visitproc visit, void *arg)
{
    PyObject *obj = buffer.bi_obj;

    return obj ? visit(obj, arg) : 0;
}
