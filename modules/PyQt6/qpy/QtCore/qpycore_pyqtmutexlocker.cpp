// This is the implementation of the PyQtMutexLocker class.
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

#include <QMutex>

#include "qpycore_pyqtmutexlocker.h"


// Create a locker for a QMutex.
PyQtMutexLocker::PyQtMutexLocker(QMutex *mutex, PyObject *wrapper) :
        _wrapper(wrapper), _r_locker(nullptr)
{
    Py_INCREF(_wrapper);

    _locker = new QMutexLocker<QMutex>(mutex);
}


// Create a locker for a QRecursiveMutex.
PyQtMutexLocker::PyQtMutexLocker(QRecursiveMutex *mutex, PyObject *wrapper) :
        _wrapper(wrapper), _locker(nullptr)
{
    Py_INCREF(_wrapper);

    _r_locker = new QMutexLocker<QRecursiveMutex>(mutex);
}


// Destroy the locker.
PyQtMutexLocker::~PyQtMutexLocker()
{
    if (_locker)
        delete _locker;
    else
        delete _r_locker;

    Py_DECREF(_wrapper);
}


// Return the Python object that wraps the mutex.
PyObject *PyQtMutexLocker::mutex()
{
    Py_INCREF(_wrapper);

    return _wrapper;
}


// Explicitly unlock the mutex.
void PyQtMutexLocker::unlock()
{
    if (_locker)
        _locker->unlock();
    else
        _r_locker->unlock();
}


// Explicitly relock the mutex.
void PyQtMutexLocker::relock()
{
    if (_locker)
        _locker->relock();
    else
        _r_locker->relock();
}
