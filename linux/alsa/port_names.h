/*
    Copyright (C) 2010 Florian Faber, faber@faberman.de

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/

#ifndef __jack_port_names_h__
#define __jack_port_names_h__

#ifdef __cplusplus
extern "C"
{
#endif

char** port_names_get_portnames(alsa_driver_t *driver);

#ifdef __cplusplus
}
#endif

#endif /* __jack_port_names_h__ */
