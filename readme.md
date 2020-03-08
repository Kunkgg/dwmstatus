# dwmstatus

Fork from [suckless.org](git://git.suckless.org/dwmstatus)

Simplest dwmstatus in C, inculde:

*   mpd status (The playing informations, title, artist, progress)
*   volume (by PulseAudio, dectect bluetooth headset)
*   time and date

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
systemctl --user enable mpd.service
systemctl --user enable pulseaudio.service
```

Because this script uses C-API of mpd and PulseAudio.
The PulseAudio C-API is Async.
This script use a loop send requests to query sink informations.
If PulseAudio hasn't worked, this script couldn't stuck in.
