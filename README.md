## ♾️🚆

Listen to [トレイン to トレイン](https://www.youtube.com/watch?v=LJ9vkt7BHYI) forever.

Track list for 鉄道むすめ キャラクターソング Vol.1~12 included [here](tracks.yml).

### Options

```
  infinitetrain input output [-n <count>] [-r] [-v] [-h]

  OPTIONS:

      input                             path to track list yml file
      output                            path to output file (- for stdout)
      -n, --iterations=[count]          amount of tracks to play before exiting
      -r, --regular                     disable randomization of track order
      -v, --verbose                     enable debugging messages
      -h, --help                        display help text and exit
```

### Usage

Play with [mpv](https://mpv.io/) in regular order:
```bash
./infinitetrain --regular tracks.yml - | mpv - --force-window
```

Combine all 12 tracks to a single file:
```bash
./infinitetrain --regular --iterations=12 tracks.yml combined.flac
```

Play in a random order and serve over HTTP port 8080 with [FFmpeg](https://ffmpeg.org/):
```bash
./infinitetrain tracks.yml - | ffmpeg -re -f flac -i - -c copy -listen 1 -f flac http://localhost:8080/
```