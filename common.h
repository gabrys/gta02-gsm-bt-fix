/*
 * Copyright 2007 Wolfson Microelectronics PLC.
 * Author: Graeme Gregory
 *         graeme.gregory@wolfsonmicro.com or linux@wolfsonmicro.com
 *
 * Copyright 2010 Piotr Gabryjeluk
 * Author: Piotr Gabryjeluk
 *         piotr@gabryjeluk.pl
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 *
 * Revision history:
 *    20th Jan 2007   Initial version.
 *     9th Apr 2010   Description, instructions, daemonizing code, cleaning up.
 *
 * Description:
 *
 *  This program is meant to be run on Neo FreeRunner GTA02 phone.
 *
 *  It opens hw:0,1 ALSA audio device (that is wired directly to the
 *  Bluetooth chip audio connector) and sets the parameters of the device
 *  such as bitrate, frequency and number of channels that match audio
 *  output/input of GSM chip of the phone. This makes the Bluetooth
 *  chip ready to exchange audio frames with GSM chip.
 *
 *  The program just opens the device and does not write or read anything
 *  to/from it, because all audio routing is done in hardware. In order to
 *  keep the settings applied, program needs to keep the device open.
 *
 *  If you want to route the GSM audio to Bluetooth headset, in
 *  addition to having this program running, you need to do several
 *  other things:
 *
 *   o have the Bluetooth device paired and concted
 *   o load appropriate alsa state file (scenario)
 *   o work around a buggy kernel, by running this once:
 *     (this somehow fixes buggy power management of audio chip)
 *   # amixer sset "Capture Left Mixer" "Analogue Mix Right"
 *   # amixer sset "Capture Left Mixer" "Analogue Mix Left"
 *
 * based on pcm_bluetooth.c from Wolfson Microelectronics
 * which was based on pcm_mic.c from alsa-lib
 *
 */

#ifndef _COMMON_H
#define _COMMON_H

#include <alsa/asoundlib.h>
#include <sys/types.h>
#include <unistd.h>


static char *device = "hw:0,1";	 		/* playback device */
                                 		/* this is the one wired to the Bluetooth chip */

static int err, pid;
static snd_pcm_t *phandle,*chandle;

void gsm_bt_workaround(int value) {
 
    /* Thanks http://alsa.opensrc.org/index.php/HowTo_access_a_mixer_control */
 
    snd_hctl_t *hctl;
    snd_ctl_elem_id_t *id;
    snd_hctl_elem_t *elem;
    snd_ctl_elem_value_t *control;

    err = snd_hctl_open(&hctl, "hw:0", 0);
    err = snd_hctl_load(hctl);
 
    snd_ctl_elem_id_alloca(&id);
    snd_ctl_elem_id_set_interface(id, SND_CTL_ELEM_IFACE_MIXER);
    snd_ctl_elem_id_set_name(id, "Capture Left Mixer");
    elem = snd_hctl_find_elem(hctl, id);
 
    snd_ctl_elem_value_alloca(&control);
    snd_ctl_elem_value_set_id(control, id);
 
    snd_ctl_elem_value_set_enumerated(control, 0, value);
    err = snd_hctl_elem_write(elem, control);
 
    snd_hctl_close(hctl);
}

int daemonize() {
    pid = fork();
    if (pid < 0) {
        fprintf(stderr, "Could not fork\n");
        return -1;
    }
    if (pid > 0) {
        /* Parent code. Let's output the child pid, so that init script can kill it when needed */
        return pid;
    }

    /* deamonize */
    chdir("/");
    setsid();
    close(0);
    close(1);
    close(2);
    return 0;
}

int gsm_bt_enable(void) {

    /* First apply kernel bug workaround */
    gsm_bt_workaround(2); /* amixer sset "Capture Left Mixer" "Analogue Mix Right" */
    gsm_bt_workaround(1); /* amixer sset "Capture Left Mixer" "Analogue Mix Left" */
    
    /* Then open and set up the audio device */
    
    if ((err = snd_pcm_open(&phandle, device, SND_PCM_STREAM_PLAYBACK, 0)) < 0) {
    	fprintf(stderr, "Playback open error: %s\n", snd_strerror(err));
    	return -1;
    }
    
    if ((err = snd_pcm_open(&chandle, device, SND_PCM_STREAM_CAPTURE, 0)) < 0) {
    	fprintf(stderr, "Capture open error: %s\n", snd_strerror(err));
	    return -1;
    }

    if ((err = snd_pcm_set_params(phandle,
				  SND_PCM_FORMAT_S16_LE,
				  SND_PCM_ACCESS_RW_INTERLEAVED,
				  1,
				  8000,
				  1,
				  500000)) < 0) {	/* 0.5sec */
    	fprintf(stderr, "Playback open error: %s\n", snd_strerror(err));
    	return -1;
    }

    if ((err = snd_pcm_set_params(chandle,
				  SND_PCM_FORMAT_S16_LE,
				  SND_PCM_ACCESS_RW_INTERLEAVED,
				  1,
				  8000,
				  1,
				  500000)) < 0) { /* 0.5sec */
        fprintf(stderr, "Capture open error: %s\n", snd_strerror(err));
        return -1;
    }

    /* The device is open and it's set up properly */
    /* Now we need to keep the device open */
    
    /* daemonize */
    pid = daemonize();
    if (pid != 0) {
        return pid;
    }

    /* And now wait forever */
    while (1) {
        sleep(9999);
    }
    return 0;
}

void gsm_bt_cleanup(void) {
    snd_pcm_close(phandle);
    snd_pcm_close(chandle);
}

#endif
