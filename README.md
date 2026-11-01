# napaacdecode

NAP module that decodes AAC audio files (raw ADTS `.aac` or MP4/M4A containers) into an interleaved 32 bit floating point sample buffer, optionally resampled to a target sample rate.

## Usage

Add `napaacdecode` to the `RequiredModules` of your app or module `.json` and include the header:

```cpp
#include <aacdecoder.h>

std::vector<float> samples;
int rate = 0, channels = 0;
nap::utility::ErrorState error;
if (!nap::audio::utility::decodeAACFileToFloatBuffer("test.aac", 48000, samples, rate, channels, error))
{
    nap::Logger::error(error.toString());
    return false;
}
// samples.size() / channels = number of frames
```

### `decodeAACFileToFloatBuffer`

| Parameter | Description |
|---|---|
| `path` | File to decode |
| `targetSampleRate` | Sample rate of the output in Hz. The audio is resampled when it differs from the source. `0` keeps the source rate |
| `outBuffer` | Interleaved samples in the range [-1, 1] |
| `outSampleRate` | Sample rate of `outBuffer` |
| `outChannels` | Number of interleaved channels in `outBuffer` |
| `error` | Contains a description of the failure when `false` is returned |

Returns `true` on success.

## FFmpeg

The module links against its own FFmpeg 9.x (libavcodec 63) in `thirdparty/ffmpeg`, independent of the FFmpeg used by `napvideo`. Only libavformat, libavcodec, libswresample and libavutil are required.

The bundled libraries are a stripped build that contains just the AAC decoder, the `mov` and `aac` demuxers and the `file` protocol:

```bash
./configure --prefix=$PWD/install --disable-everything --disable-programs --disable-doc \
  --disable-avdevice --disable-avfilter --disable-swscale \
  --enable-demuxer=mov,aac --enable-decoder=aac --enable-parser=aac,aac_latm \
  --enable-protocol=file --enable-shared --disable-static --enable-pic
make -j$(nproc) && make install
```
