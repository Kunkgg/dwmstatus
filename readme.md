# dwmstatus

Fork from [suckless.org - status monitor](https://dwm.suckless.org/status_monitor/)

Simplest dwmstatus in C, inculde:

*   mpd status (The playing informations, title, artist, progress)
*   volume (by PulseAudio, dectect bluetooth headset)
*   time and date

![dwmstatus-screenshot](./dwmstatus-screenshot.png)

## Usage

### Step1

```sh
git clone git@github.com:Kunkgg/dwmstatus.git
cd dwmstatus
sudo make install
```

### Step2

add `dwmstatus 2>&1 >/dev/null &` to .xinitrc

### Step3

Make sure `mpd` and `PulseAudio` have started before `dwmstatus`, like this:

```sh
systemctl --user enable mpd
systemctl --user enable pulseaudio.service
```

Because this script uses C-API of mpd and PulseAudio.
The PulseAudio C-API is Async.
This script use a loop send requests to query sink informations.
If PulseAudio hasn't worked, this script couldn't stuck in.
