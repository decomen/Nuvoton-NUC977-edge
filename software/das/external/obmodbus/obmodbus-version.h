/*
 * Copyright © 2010-2014 Stéphane Raimbault <stephane.raimbault@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef OBMODBUS_VERSION_H
#define OBMODBUS_VERSION_H

/* The major version, (1, if %LIBOBMODBUS_VERSION is 1.2.3) */
#define LIBOBMODBUS_VERSION_MAJOR (3)

/* The minor version (2, if %LIBOBMODBUS_VERSION is 1.2.3) */
#define LIBOBMODBUS_VERSION_MINOR (1)

/* The micro version (3, if %LIBOBMODBUS_VERSION is 1.2.3) */
#define LIBOBMODBUS_VERSION_MICRO (4)

/* The full version, like 1.2.3 */
#define LIBOBMODBUS_VERSION        3.1.4

/* The full version, in string form (suited for string concatenation)
 */
#define LIBOBMODBUS_VERSION_STRING "3.1.4"

/* Numerically encoded version, like 0x010203 */
#define LIBOBMODBUS_VERSION_HEX ((LIBOBMODBUS_VERSION_MAJOR << 24) |  \
                               (LIBOBMODBUS_VERSION_MINOR << 16) |  \
                               (LIBOBMODBUS_VERSION_MICRO << 8))

/* Evaluates to True if the version is greater than @major, @minor and @micro
 */
#define LIBOBMODBUS_VERSION_CHECK(major,minor,micro)      \
    (LIBOBMODBUS_VERSION_MAJOR > (major) ||               \
     (LIBOBMODBUS_VERSION_MAJOR == (major) &&             \
      LIBOBMODBUS_VERSION_MINOR > (minor)) ||             \
     (LIBOBMODBUS_VERSION_MAJOR == (major) &&             \
      LIBOBMODBUS_VERSION_MINOR == (minor) &&             \
      LIBOBMODBUS_VERSION_MICRO >= (micro)))

#endif /* OBMODBUS_VERSION_H */
