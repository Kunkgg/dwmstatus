#include <stdio.h>
#include <string.h>
#include <pulse/pulseaudio.h>

#include "utils.c"

// Field list is here: http://0pointer.de/lennart/projects/pulseaudio/doxygen/structpa__sink__info.html
typedef struct pa_devicelist {
	uint8_t initialized;
	char name[512];
	uint32_t index;
	char description[256];
    struct pa_cvolume volume;
    pa_sink_state_t state;
    int mute;
} pa_devicelist_t;

void pa_state_cb(pa_context *c, void *userdata);
void pa_sinklist_cb(pa_context *c, const pa_sink_info *l, int eol, void *userdata);
int pa_get_devicelist(pa_devicelist_t *output);
char * get_vol();
char * parse_vol(char *vol_str);

// This callback gets called when our context changes state.  We really only
// care about when it's ready or if it has failed
void pa_state_cb(pa_context *c, void *userdata) {
	pa_context_state_t state;
	int *pa_ready = userdata;

	state = pa_context_get_state(c);
	switch  (state) {
		// There are just here for reference
		case PA_CONTEXT_UNCONNECTED:
		case PA_CONTEXT_CONNECTING:
		case PA_CONTEXT_AUTHORIZING:
		case PA_CONTEXT_SETTING_NAME:
		default:
			break;
		case PA_CONTEXT_FAILED:
		case PA_CONTEXT_TERMINATED:
			*pa_ready = 2;
			break;
		case PA_CONTEXT_READY:
			*pa_ready = 1;
			break;
	}
}

// pa_mainloop will call this function when it's ready to tell us about a sink.
// Since we're not threading, there's no need for mutexes on the devicelist
// structure
void pa_sinklist_cb(pa_context *c, const pa_sink_info *l, int eol, void *userdata) {
    pa_devicelist_t *pa_devicelist = userdata;
    int ctr = 0;

    // If eol is set to a positive number, you're at the end of the list
    if (eol > 0) {
	return;
    }

    // We know we've allocated 16 slots to hold devices.  Loop through our
    // structure and find the first one that's "uninitialized."  Copy the
    // contents into it and we're done.  If we receive more than 16 devices,
    // they're going to get dropped.  You could make this dynamically allocate
    // space for the device list, but this is a simple example.
    for (ctr = 0; ctr < 16; ctr++) {
	if (! pa_devicelist[ctr].initialized) {
	    strncpy(pa_devicelist[ctr].name, l->name, 511);
	    strncpy(pa_devicelist[ctr].description, l->description, 255);
	    pa_devicelist[ctr].index = l->index;
	    pa_devicelist[ctr].initialized = 1;
	    pa_devicelist[ctr].volume = l->volume;
	    pa_devicelist[ctr].state = l->state;
	    pa_devicelist[ctr].mute = l->mute;
	    break;
	}
    }
}


int pa_get_devicelist(pa_devicelist_t *output) {
    // Define our pulse audio loop and connection variables
    pa_mainloop *pa_ml;
    pa_mainloop_api *pa_mlapi;
    pa_operation *pa_op;
    pa_context *pa_ctx;


    // We'll need these state variables to keep track of our requests
    int state = 0;
    int pa_ready = 0;

    // Initialize our device lists
    memset(output, 0, sizeof(pa_devicelist_t) * 16);

    // Create a mainloop API and connection to the default server
    pa_ml = pa_mainloop_new();
    pa_mlapi = pa_mainloop_get_api(pa_ml);
    pa_ctx = pa_context_new(pa_mlapi, "test");

    // This function connects to the pulse server
    pa_context_connect(pa_ctx, NULL, 0, NULL);


    // This function defines a callback so the server will tell us it's state.
    // Our callback will wait for the state to be ready.  The callback will
    // modify the variable to 1 so we know when we have a connection and it's
    // ready.
    // If there's an error, the callback will set pa_ready to 2
    pa_context_set_state_callback(pa_ctx, pa_state_cb, &pa_ready);

    // Now we'll enter into an infinite loop until we get the data we receive
    // or if there's an error
    for (;;) {
	// We can't do anything until PA is ready, so just iterate the mainloop
	// and continue
	if (pa_ready == 0) {
	    pa_mainloop_iterate(pa_ml, 1, NULL);
	    continue;
	}
	// We couldn't get a connection to the server, so exit out
	if (pa_ready == 2) {
	    pa_context_disconnect(pa_ctx);
	    pa_context_unref(pa_ctx);
	    pa_mainloop_free(pa_ml);
	    return -1;
	}
	// At this point, we're connected to the server and ready to make
	// requests
	switch (state) {
	    // State 0: we haven't done anything yet
	    case 0:
		// This sends an operation to the server.  pa_sinklist_info is
		// our callback function and a pointer to our devicelist will
		// be passed to the callback The operation ID is stored in the
		// pa_op variable
		pa_op = pa_context_get_sink_info_list(pa_ctx,
			pa_sinklist_cb,
			output
			);

		// Update state for next iteration through the loop
		state++;
		break;
	    case 1:
		if (pa_operation_get_state(pa_op) == PA_OPERATION_DONE) {
		    // Now we're done, clean up and disconnect and return
		    pa_operation_unref(pa_op);
		    pa_context_disconnect(pa_ctx);
		    pa_context_unref(pa_ctx);
		    pa_mainloop_free(pa_ml);
		    return 0;
		}
		break;
	    default:
		// We should never see this state
		fprintf(stderr, "in state %d\n", state);
		return -1;
	}
	// Iterate the main loop and go again.  The second argument is whether
	// or not the iteration should block until something is ready to be
	// done.  Set it to zero for non-blocking.
	pa_mainloop_iterate(pa_ml, 1, NULL);
    }
}


char *
parse_vol(char *vol_str) {
    int i=0;
    char * token = strtok(vol_str, " ");
    // loop through the string to extract all other tokens
    while( token != NULL ) {
        if (i == 1){
            return smprintf("%s", token);
            }
        token = strtok(NULL, " ");
        i++;
    }
    return smprintf("%s", "error");
}

char *
get_vol() {
    int ctr;
    int run_sink_n = -1;
    int target_sink_n = 0;
    char vol_str[PA_CVOLUME_SNPRINT_MAX];
    struct pa_cvolume *volume_tmp;
	pa_sink_state_t state;
    char * icon_vol = "";
    char * icon_bluetooth = "";
    char * icon_headset = "";
    char * icon;
    char * vol;

    // This is where we'll store the output device list
    pa_devicelist_t pa_output_devicelist[16];

    if (pa_get_devicelist(pa_output_devicelist) < 0) {
        return smprintf("%s", "vol_failed");
    }

    // Decide the target sink's index in devicelist array
    // Invaild, the server does not support sink state introspection.
    // Running, sink is playing and used by at least one non-corked sink-input.
    // Idle, the sink is playing but there is no non-corked sink-input attached to it.
    // Suspended, actual sink access can be closed, for instance. No sounds are playing.
    for (ctr = 0; ctr < 16; ctr++) {
        if (! pa_output_devicelist[ctr].initialized) {
            break;
        }
        state = pa_output_devicelist[ctr].state;
        if (state == PA_SINK_RUNNING  || state == PA_SINK_IDLE) {
            run_sink_n = ctr;
            break;
        }
    }

    // First find Running or Idle sink as target,
    // If there are not Running or Idle sinks, use the last initialized sink.
    if (run_sink_n >= 0) {
        target_sink_n = run_sink_n;
    } else {
        target_sink_n = ctr - 1;
    }
    if (strstr(pa_output_devicelist[target_sink_n].name, "blue")) {
        icon = smprintf("%s %s", icon_bluetooth, icon_headset);
    } else {
        icon = smprintf("%s", icon_vol);
    }
    volume_tmp = &pa_output_devicelist[target_sink_n].volume;
    pa_cvolume_snprint(vol_str, sizeof(vol_str), volume_tmp);
    vol = parse_vol(vol_str);

    return smprintf("%s %s", icon, vol);
}
