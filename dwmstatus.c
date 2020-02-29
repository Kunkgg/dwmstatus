/*
 * Copy me if you can.
 * by 20h
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

char *tzshanghai = "Asia/Shanghai";

static Display *dpy;

char *
smprintf(char *fmt, ...)
{
	va_list fmtargs;
	char *ret;
	int len;

	va_start(fmtargs, fmt);
	len = vsnprintf(NULL, 0, fmt, fmtargs);
	va_end(fmtargs);

	ret = malloc(++len);
	if (ret == NULL) {
		perror("malloc");
		exit(1);
	}

	va_start(fmtargs, fmt);
	vsnprintf(ret, len, fmt, fmtargs);
	va_end(fmtargs);

	return ret;
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

	settz(tzname);
	tim = time(NULL);
	timtm = localtime(&tim);
	if (timtm == NULL)
		return smprintf("");

	if (!strftime(buf, sizeof(buf)-1, fmt, timtm)) {
		fprintf(stderr, "strftime == 0\n");
		return smprintf("");
	}

	return smprintf("%s", buf);
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
                retstr = smprintf("%.2d:%.2d/%.2d:%.2d %s - %s",
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

char *
get_vol(void)
{
    int vol;
    snd_hctl_t *hctl;
    snd_ctl_elem_id_t *id;
    snd_ctl_elem_value_t *control;

// To find card and subdevice: /proc/asound/, aplay -L, amixer controls
    snd_hctl_open(&hctl, "hw:0", 0);
    snd_hctl_load(hctl);

    snd_ctl_elem_id_alloca(&id);
    snd_ctl_elem_id_set_interface(id, SND_CTL_ELEM_IFACE_MIXER);

// amixer controls
    snd_ctl_elem_id_set_name(id, "Master Playback Volume");

    snd_hctl_elem_t *elem = snd_hctl_find_elem(hctl, id);

    snd_ctl_elem_value_alloca(&control);
    snd_ctl_elem_value_set_id(control, id);

    snd_hctl_elem_read(elem, control);
    vol = (int)snd_ctl_elem_value_get_integer(control,0);

    snd_hctl_close(hctl);
    return smprintf("vol:%d", vol);
}

int
main(void)
{
	char *status;
	char *timesh;
	char *mpdstat;
    char *vol;

	if (!(dpy = XOpenDisplay(NULL))) {
		fprintf(stderr, "dwmstatus: cannot open display.\n");
		return 1;
	}

	for (;;sleep(60)) {
		timesh = mktimes("%Y %b %d(%a) %H:%M", tzshanghai);
        mpdstat = get_mpdstat();
        vol = get_vol();

		status = smprintf("| %s | %s | %s ",
				mpdstat, vol, timesh);
		setstatus(status);

		free(mpdstat);
		free(timesh);
		free(vol);
		free(status);
	}

	XCloseDisplay(dpy);

	return 0;
}
