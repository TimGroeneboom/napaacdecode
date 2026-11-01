#pragma once

// External Includes
#include <string>
#include <vector>
#include <utility/errorstate.h>
#include <utility/dllexport.h>

namespace nap::audio::utility
{
	/**
	 * Decodes an audio file (e.g. aac) into an interleaved 32 bit floating point sample buffer.
	 * Written against the FFmpeg 9.x API (libavcodec 63).
	 * @param path file to decode, for example 'test.aac'
	 * @param targetSampleRate sample rate of outBuffer; the stream is resampled when it differs from the source. 0 keeps the source rate
	 * @param outBuffer interleaved samples in range [-1, 1]
	 * @param outSampleRate sample rate of outBuffer
	 * @param outChannels number of interleaved channels in outBuffer
	 * @param errorState error state containing error messages if decoding fails
	 * @return true on success
	 */
	NAPAPI bool decodeAACFileToFloatBuffer(
		const std::string& path,
		int targetSampleRate,
		std::vector<float>& outBuffer,
		int& outSampleRate,
		int& outChannels,
		nap::utility::ErrorState& errorState);

}
