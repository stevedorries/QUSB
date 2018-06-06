/******************************************************************************
**
** Copyright (C) 2014 BIMEtek Co. Ltd.
**
** This file is part of QUSB.
**
** QUSB is free software: you can redistribute it and/or modify it under the
** terms of the GNU Lesser General Public License as published by the Free
** Software Foundation, either version 3 of the License, or (at your option)
** any later version.
**
** QUSB is distributed in the hope that it will be useful, but WITHOUT ANY
** WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
** FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
** details.
**
** You should have received a copy of the GNU General Public License along with
** this file. If not, see <http://www.gnu.org/licenses/>.
**
******************************************************************************/

#ifndef QUSB_GLOBAL_H
#define QUSB_GLOBAL_H

#include <QtCore/qglobal.h>

#if defined(QUSB_LIBRARY)
#  define QUSB_SHARED_EXPORT Q_DECL_EXPORT
#else
#  define QUSB_SHARED_EXPORT Q_DECL_IMPORT
#endif

#ifdef QUSB_SRC
#define QUSB_SHARED_EXPORT
#endif

#endif // QUSB_GLOBAL_H
