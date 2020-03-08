/*
 * Simple dwmstatus, display:
 *   - mpd status
 *   - volume (by PulseAudio, dectect bluetooth headset)
 *   - time
 */

#define _BSD_SOURCE
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <strings.h>
#include <sys/time.h>
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <X11/Xlib.h>
#include <mpd/client.h>
#include <alsa/asoundlib.h>
#include <alsa/control.h>

#include "pulsevol.c"

char *tzshanghai = "Asia/Shanghai";

static Display *dpy;

int
runevery(time_t *ltime, int sec){
    /* return 1 if sec elapsed since last run
     * else return 0
    */
    time_t now = time(NULL);

    if ( difftime(now, *ltime ) >= sec)
    {
        *ltime = now;
        return(1);
    }
    else
        return(0);
}

void
settz(char *tzname)
{
	setenv("TZ", tzname, 1);
}

char *
mktimes(char *fmt, char *tzname)
{
	char buf[129];
	time_t tim;
	struct tm *timtm;
    char * icon="";

	settz(tzname);
	tim = time(NULL);
	timtm = localtime(&tim);
	if (timtm == NULL)
		return smprintf("");

	if (!strftime(buf, sizeof(buf)-1, fmt, timtm)) {
		fprintf(stderr, "strftime == 0\n");
		return smprintf("");
	}

	return smprintf("%s %s", icon, buf);
}

void
setstatus(char *str)
{
	XStoreName(dpy, DefaultRootWindow(dpy), str);
	XSync(dpy, False);
}

char *
readfile(char *base, char *file)
{
	char *path, line[513];
	FILE *fd;

	memset(line, 0, sizeof(line));

	path = smprintf("%s/%s", base, file);
	fd = fopen(path, "r");
	free(path);
	if (fd == NULL)
		return NULL;

	if (fgets(line, sizeof(line)-1, fd) == NULL)
		return NULL;
	fclose(fd);

	return smprintf("%s", line);
}

char *
get_mpdstat() {
    struct mpd_song * song = NULL;
	const char * title = NULL;
	const char * artist = NULL;
	char * retstr = NULL;
	int elapsed = 0, total = 0;
    struct mpd_connection * conn ;
    if (!(conn = mpd_connection_new("localhost", 0, 30000)) ||
        mpd_connection_get_error(conn)){
            return smprintf("");
    }
    char * icon = "🎵";

    mpd_command_list_begin(conn, true);
    mpd_send_status(conn);
    mpd_send_current_song(conn);
    mpd_command_list_end(conn);

    struct mpd_status* theStatus = mpd_recv_status(conn);
        if ((theStatus) && (mpd_status_get_state(theStatus) == MPD_STATE_PLAY)) {
                mpd_response_next(conn);
                song = mpd_recv_song(conn);
                title = smprintf("%s",mpd_song_get_tag(song, MPD_TAG_TITLE, 0));
                artist = smprintf("%s",mpd_song_get_tag(song, MPD_TAG_ARTIST, 0));

                elapsed = mpd_status_get_elapsed_time(theStatus);
                total = mpd_status_get_total_time(theStatus);
                mpd_song_free(song);
                retstr = smprintf("%s %.2d:%.2d/%.2d:%.2d %s - %s",
                                icon,
                                elapsed/60, elapsed%60,
                                total/60, total%60,
                                artist, title);
                free((char*)title);
                free((char*)artist);
        }
        else retstr = smprintf("");
		mpd_response_finish(conn);
		mpd_connection_free(conn);
		return retstr;
}

/* char * */
/* get_vol(void) */
/* { */
/*     int vol; */
/*     int maxvol = 64; */
/*     char * icon0 = "🔇"; */
/*     char * iconlow = "🔈"; */
/*     char * iconmed = "🔉"; */
/*     char * iconhig = "🔊"; */

/*     snd_hctl_t *hctl; */
/*     snd_ctl_elem_id_t *id; */
/*     snd_ctl_elem_value_t *control; */

/* // To find card and subdevice: /proc/asound/, aplay -L, amixer controls */
/*     snd_hctl_open(&hctl, "hw:0", 0); */
/*     snd_hctl_load(hctl); */

/*     snd_ctl_elem_id_alloca(&id); */
/*     snd_ctl_elem_id_set_interface(id, SND_CTL_ELEM_IFACE_MIXER); */

/* // amixer controls */
/*     snd_ctl_elem_id_set_name(id, "Master Playback Volume"); */

/*     snd_hctl_elem_t *elem = snd_hctl_find_elem(hctl, id); */

/*     snd_ctl_elem_value_alloca(&control); */
/*     snd_ctl_elem_value_set_id(control, id); */

/*     snd_hctl_elem_read(elem, control); */
/*     vol = (int)snd_ctl_elem_value_get_integer(control,0) * 100 / maxvol; */
/*     snd_hctl_close(hctl); */

/*     if (vol == 0) { */
/*         return smprintf("%s", icon0); */
/*     } else if (vol <= 25) { */
/*         return smprintf("%s %d", iconlow, vol); */
/*     } else if (vol <= 75) { */
/*         return smprintf("%s %d", iconmed, vol); */
/*     } else { */
/*         return smprintf("%s %d", iconhig, vol); */
/*     } */
/* } */


int
main(void)
{
	char *status = NULL;
	char *timesh = NULL;
	char *mpdstat = NULL;
    char *vol = NULL;

    time_t count1min = 0;

	if (!(dpy = XOpenDisplay(NULL))) {
		fprintf(stderr, "dwmstatus: cannot open display.\n");
		return 1;
	}

	for (;;sleep(1)) {

        if (runevery(&count1min, 60)) {
            free(timesh);
            free(vol);
            timesh = mktimes("%Y %b %d(%a) %H:%M", tzshanghai);
            vol = get_vol();
        }
        mpdstat = get_mpdstat();

        if (strcmp(mpdstat, "") == 0) {
            status = smprintf("| %s | %s ",
                    vol, timesh);
        } else {
            status = smprintf("| %s | %s | %s ",
                    mpdstat, vol, timesh);
        }

		setstatus(status);

		free(mpdstat);
		free(status);
	}

	XCloseDisplay(dpy);

	return 0;
}
