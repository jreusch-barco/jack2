/* -*- mode: c; c-file-style: "linux"; -*- */
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


#include <math.h>
#include <stdio.h>
#include <memory.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <stdarg.h>
#include <signal.h>
#include <sys/types.h>
#include <regex.h>
#include <string.h>

#include "alsa_driver.h"


static int port_names_load_portfile(alsa_driver_t *driver, const char *filename, char **buf, const unsigned int offset, const unsigned int num) {
	int fh, i, ret, lineno, id, res=0;
	char line[256];

	printf("Trying to load portnames from %s\n", filename);
	fh = open(filename, O_RDONLY);
	if (-1!=fh) {
		res = 1;
		i = 0;
		lineno = 1;
		for (;;) {
			ret = read(fh, &line[i], 1);
			if (0==ret) {
				break;
			} else if (-1==ret) {
				sprintf(stderr, "Error while reading \"%s\": %s", filename, strerror(errno));
				break;
			}
			if (0x0A==line[i]) {
				/* new line, parse input */
				line[i] = 0;

				if ('#' != line[0]) {
					i=0;
					while ((i<255) && (line[i]!='=')) i++;
					if (255==i) {
						sprintf(stderr, "Error while reading \"%s\": Line %d has no key=value syntax!", filename, lineno);
					} else {
						line[i] = 0;
						id = atoi(line);
						if ((id>=1) && (id<=num)) {
							if (NULL==buf[id-1+offset]) {
								/* don't overwrite existing names */
								buf[id-1+offset] = strdup(&line[i+1]);
							}
						} else {
							sprintf(stderr, "Error while reading \"%s\": Key %d out of range in line %d (1..%d)", filename, id, lineno, num);
						}
					}
				}

				i = 0;
				lineno++;
			} else {
				i++;
				if (i==255) {
					sprintf(stderr, "Error while reading \"%s\": Line %d is too long", filename, lineno);
					break;
				}
			}
		}

		(void) close(fh);
	}

	return res;
}


static void port_names_default_portnames(char **buf, const unsigned int offset, const unsigned int num, const char *defaultname) {
	unsigned int i;
	char line[256];

	/* Fill in default names */
	for (i=0; i<num; i++) {
		if (NULL==buf[i+offset]) {
			snprintf(line, 255, defaultname, i+1);
			buf[i+offset] = strdup(line);
		}
	}
}


char** port_names_get_portnames(alsa_driver_t *driver) {
	snd_ctl_card_info_t *card_info;
	int err;
	const char *card_name = NULL;
	char filename[256], *speed;
	char **buf;

	printf("Using port names patch v0.1 (07.04.2010)\n");

	if (driver->frame_rate > 96000) {
		speed="qs";
	} else if (driver->frame_rate > 48000) {
		speed="ds";
	} else {
		speed="ss";
	}

	snd_ctl_card_info_alloca(&card_info);
	err = snd_ctl_card_info(driver->ctl_handle, card_info);
	if (err >= 0) {
		card_name = snd_ctl_card_info_get_name(card_info);
	} else {
		card_name = "noname";
	}

	buf = malloc(sizeof(char *)*(driver->capture_nchannels + driver->playback_nchannels));
	if (NULL==buf) {
		sprintf(stderr, "ALSA: Not enough memory for %d port names", driver->capture_nchannels + driver->playback_nchannels);
		return NULL;
	}
	bzero(buf, sizeof(char *)*(driver->capture_nchannels + driver->playback_nchannels));

	/* Read port names from special to general:
	 * Begin with user and speed specific port names */
	snprintf(filename, 255, "%s/.config/jack/cards/%s.%s.ports.in", getenv("HOME"), card_name, speed);
	(void) port_names_load_portfile(driver, filename, buf, 0, driver->capture_nchannels);

	/* Now user general */
	snprintf(filename, 255, "%s/.config/jack/cards/%s.ports.in", getenv("HOME"), card_name);
	(void) port_names_load_portfile(driver, filename, buf, 0, driver->capture_nchannels);

	/* System speed specific */
	snprintf(filename, 255, "/etc/jack/cards/%s.%s.ports.in", card_name, speed);
	(void) port_names_load_portfile(driver, filename, buf, 0, driver->capture_nchannels);

	/* System general */
	snprintf(filename, 255, "/etc/jack/cards/%s.ports.in", card_name);
	(void) port_names_load_portfile(driver, filename, buf, 0, driver->capture_nchannels);

	/* Fill all still unnamed ports with default names */
	port_names_default_portnames(buf, 0, driver->capture_nchannels, "capture_%lu");


	/* Same procedure for the playback channels */
	snprintf(filename, 255, "%s/.config/jack/cards/%s.%s.ports.out", getenv("HOME"), card_name, speed);
	(void) port_names_load_portfile(driver, filename, buf, driver->capture_nchannels, driver->playback_nchannels);

	snprintf(filename, 255, "%s/.config/jack/cards/%s.ports.out", getenv("HOME"), card_name);
	(void) port_names_load_portfile(driver, filename, buf, driver->capture_nchannels, driver->playback_nchannels);

	snprintf(filename, 255, "/etc/jack/cards/%s.%s.ports.out", card_name, speed);
	(void) port_names_load_portfile(driver, filename, buf, driver->capture_nchannels, driver->playback_nchannels);

	snprintf(filename, 255, "/etc/jack/cards/%s.ports.out", card_name);
	(void) port_names_load_portfile(driver, filename, buf, driver->capture_nchannels, driver->playback_nchannels);

	port_names_default_portnames(buf, driver->capture_nchannels, driver->playback_nchannels, "playback_%lu");

	return buf;
}
