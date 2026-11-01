#include "aacdecoder.h"

// FFmpeg Includes
extern "C"
{
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswresample/swresample.h>
#include <libavutil/channel_layout.h>
#include <libavutil/error.h>
}

namespace nap::audio::utility
{
	// av_err2str() can't be used from C++
	static std::string ffmpegError(int errnum)
	{
		char buf[AV_ERROR_MAX_STRING_SIZE] = {};
		av_strerror(errnum, buf, sizeof(buf));
		return buf;
	}


	bool decodeAACFileToFloatBuffer(
		const std::string& path,
		int targetSampleRate,
		std::vector<float>& outBuffer,
		int& outSampleRate,
		int& outChannels,
		nap::utility::ErrorState& errorState)
	{
		outBuffer.clear();
		AVFormatContext* fmt = nullptr;
		AVCodecContext* ctx = nullptr;
		SwrContext* swr = nullptr;
		AVPacket* packet = av_packet_alloc();
		AVFrame* frame = av_frame_alloc();
		bool ok = false;

		// Formats and codecs are registered automatically in modern FFmpeg, av_register_all() no longer exists
		do
		{
			// Ensure packet and frame were allocated
			if (!errorState.check(packet != nullptr && frame != nullptr, "Out of memory"))
				break;

			// Open the input file and read the header
			int err = avformat_open_input(&fmt, path.c_str(), nullptr, nullptr);
			if (!errorState.check(err >= 0, "Failed to open audio file '%s': %s", path.c_str(), ffmpegError(err).c_str()))
				break;

			// Read packets of a media file to get stream information
			err = avformat_find_stream_info(fmt, nullptr);
			if (!errorState.check(err >= 0, "Failed to find stream info for audio file '%s': %s", path.c_str(), ffmpegError(err).c_str()))
				break;

			// Find the best audio stream and its decoder
			const AVCodec* codec = nullptr;
			int stream = av_find_best_stream(fmt, AVMEDIA_TYPE_AUDIO, -1, -1, &codec, 0);
			if (!errorState.check(stream >= 0 && codec != nullptr, "Failed to find decodable audio stream in file '%s'", path.c_str()))
				break;

			// Allocate a codec context for the decoder
			ctx = avcodec_alloc_context3(codec);
			if (!errorState.check(ctx != nullptr, "Failed to allocate decoder context for audio file '%s'", path.c_str()))
				break;

			// Copy codec parameters from input stream to output codec context
			err = avcodec_parameters_to_context(ctx, fmt->streams[stream]->codecpar);
			if (!errorState.check(err >= 0, "Failed to copy codec parameters for audio file '%s': %s", path.c_str(), ffmpegError(err).c_str()))
				break;

			// Initialize the decoder
			err = avcodec_open2(ctx, codec, nullptr);
			if (!errorState.check(err >= 0, "Failed to open decoder for audio file '%s': %s", path.c_str(), ffmpegError(err).c_str()))
				break;

			// Fall back to a default layout when the stream doesn't specify one
			if (ctx->ch_layout.order == AV_CHANNEL_ORDER_UNSPEC && ctx->ch_layout.nb_channels > 0)
				av_channel_layout_default(&ctx->ch_layout, ctx->ch_layout.nb_channels);

			// Set output sample rate and channel count
			outSampleRate = targetSampleRate > 0 ? targetSampleRate : ctx->sample_rate;
			outChannels = ctx->ch_layout.nb_channels;
			if (!errorState.check(outChannels > 0 && ctx->sample_rate > 0, "Invalid audio stream properties in file '%s'", path.c_str()))
				break;

			// Converts decoder output to interleaved float, resampling to the target rate
			err = swr_alloc_set_opts2(&swr,
									  &ctx->ch_layout, AV_SAMPLE_FMT_FLT, outSampleRate,
									  &ctx->ch_layout, ctx->sample_fmt, ctx->sample_rate, 0, nullptr);
			if (!errorState.check(err >= 0, "Failed to create resampler for audio file '%s': %s", path.c_str(), ffmpegError(err).c_str()))
				break;

			// Initialize the resampling context
			err = swr_init(swr);
			if (!errorState.check(err >= 0, "Failed to initialize resampler for audio file '%s': %s", path.c_str(), ffmpegError(err).c_str()))
				break;

			// Resamples one frame, or flushes the resampler when f is null
			auto convert = [&](const AVFrame* f)
			{
				int in_samples = f != nullptr ? f->nb_samples : 0;

				// Upper bound includes samples buffered inside the resampler
				int max_out = swr_get_out_samples(swr, in_samples);
				if (max_out <= 0)
					return true;

				// Resize output buffer and convert to interleaved float
				size_t offset = outBuffer.size();
				outBuffer.resize(offset + static_cast<size_t>(max_out) * outChannels);
				uint8_t* out[1] = { reinterpret_cast<uint8_t*>(outBuffer.data() + offset) };
				int n = swr_convert(swr, out, max_out,
									f != nullptr ? const_cast<const uint8_t**>(f->extended_data) : nullptr,
									in_samples);
				if (!errorState.check(n >= 0, "Failed to resample audio file '%s': %s", path.c_str(), ffmpegError(n).c_str()))
					return false;

				outBuffer.resize(offset + static_cast<size_t>(n) * outChannels);
				return true;
			};

			// Receives all frames currently available from the decoder
			auto drain = [&]()
			{
				int ret;
				while ((ret = avcodec_receive_frame(ctx, frame)) >= 0)
				{
					bool converted = convert(frame);
					av_frame_unref(frame);
					if (!converted)
						return false;
				}
				return errorState.check(ret == AVERROR(EAGAIN) || ret == AVERROR_EOF,
										"Failed to decode audio file '%s': %s", path.c_str(), ffmpegError(ret).c_str());
			};

			// Read packets from the input file and send them to the decoder
			bool failed = false;
			while (!failed && av_read_frame(fmt, packet) >= 0)
			{
				if (packet->stream_index == stream)
				{
					int ret = avcodec_send_packet(ctx, packet);

					// Skip corrupt packets
					if (ret == AVERROR_INVALIDDATA)
						ret = 0;

					failed = !errorState.check(ret >= 0 || ret == AVERROR(EAGAIN),
											   "Failed to decode audio file '%s': %s", path.c_str(), ffmpegError(ret).c_str()) || !drain();
				}
				av_packet_unref(packet);
			}
			if (failed)
				break;

			// Flush decoder, then resampler
			avcodec_send_packet(ctx, nullptr);
			if (!drain() || !convert(nullptr))
				break;

			ok = true;
		} while (false);

		// Cleanup
		av_frame_free(&frame);
		av_packet_free(&packet);
		swr_free(&swr);
		avcodec_free_context(&ctx);
		avformat_close_input(&fmt);

		return ok;
	}
}
