#include "aacdecoder.h"

// FFmpeg Includes
extern "C"
{
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswresample/swresample.h>
#include <libavutil/opt.h>
#include <libavutil/channel_layout.h>
}

namespace nap::audio::utility
{
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

		// Since this module is linked against a stripped down version of FFmpeg that only contains the decoders we need, we need to register all the codecs and formats here.
		av_register_all();

		while (true)
		{
			// Open the input file and read the header
			if (avformat_open_input(&fmt, path.c_str(), nullptr, nullptr) < 0)
			{
				errorState.fail("Failed to open audio file '%s'", path.c_str());
	            break;
			}

			// Read the stream info from the file
			if (avformat_find_stream_info(fmt, nullptr) < 0)
			{
				errorState.fail("Failed to find stream info for audio file '%s'", path.c_str());
	            break;
			}

			// Find the best audio stream in the file
			int stream = av_find_best_stream(fmt, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
			if (stream < 0)
			{
				errorState.fail("Failed to find audio stream in file '%s'", path.c_str());
                break;
			}

			// Find the decoder for the audio stream
			const AVCodec* codec = avcodec_find_decoder(fmt->streams[stream]->codecpar->codec_id);
			if (codec == nullptr)
			{
				errorState.fail("Failed to find decoder for audio file '%s'", path.c_str());
                break;
			}

			// Allocate a codec context for the decoder
			ctx = avcodec_alloc_context3(codec);
			if (ctx == nullptr)
			{
				errorState.fail("Failed to allocate decoder context for audio file '%s'", path.c_str());
                break;
			}

			// Copy codec parameters from input stream to output codec context
			if (avcodec_parameters_to_context(ctx, fmt->streams[stream]->codecpar) < 0)
			{
				errorState.fail("Failed to copy codec parameters to decoder context for audio file '%s'", path.c_str());
                break;
			}

			// Open the codec
			if (avcodec_open2(ctx, codec, nullptr) < 0)
			{
				errorState.fail("Failed to open decoder for audio file '%s'", path.c_str());
                break;
			}

			// Set output sample rate and channel count
			outSampleRate = targetSampleRate > 0 ? targetSampleRate : ctx->sample_rate;
			outChannels = ctx->channels;
			uint64_t layout = ctx->channel_layout != 0 ? ctx->channel_layout : av_get_default_channel_layout(ctx->channels);

			// Converts decoder output (usually planar float) to interleaved float, resampling to the target rate
			swr = swr_alloc_set_opts(nullptr, layout, AV_SAMPLE_FMT_FLT, outSampleRate,
									 layout, ctx->sample_fmt, ctx->sample_rate, 0, nullptr);
			if (swr == nullptr || swr_init(swr) < 0)
			{
				errorState.fail("Failed to initialize resampler for audio file '%s'", path.c_str());
                break;
			}

			auto convert = [&](AVFrame* f)
			{
				// Upper bound includes samples buffered inside the resampler
				int max_out = swr_get_out_samples(swr, f != nullptr ? f->nb_samples : 0);
				if (max_out <= 0) return true;
				size_t offset = outBuffer.size();
				outBuffer.resize(offset + static_cast<size_t>(max_out) * outChannels);
				uint8_t* out[1] = { reinterpret_cast<uint8_t*>(outBuffer.data() + offset) };
				int n = swr_convert(swr, out, max_out,
									f != nullptr ? const_cast<const uint8_t**>(f->extended_data) : nullptr,
									f != nullptr ? f->nb_samples : 0);
				if (n < 0) return false;
				outBuffer.resize(offset + static_cast<size_t>(n) * outChannels);
				return true;
			};

			bool error = false;
			while (!error && av_read_frame(fmt, packet) >= 0)
			{
				if (packet->stream_index == stream && avcodec_send_packet(ctx, packet) >= 0)
				{
					while (avcodec_receive_frame(ctx, frame) >= 0)
					{
						if (!convert(frame))
						{
							error = true;
							break;
						}
					}
				}
				av_packet_unref(packet);
			}
			if (error)
			{
				errorState.fail("Failed to convert audio frame for file '%s'", path.c_str());
				break;
			}

			// Flush decoder, then resampler
			avcodec_send_packet(ctx, nullptr);
			while (avcodec_receive_frame(ctx, frame) >= 0)
				convert(frame);
			convert(nullptr);

			ok = true;
		}

		// Cleanup
		av_frame_free(&frame);
		av_packet_free(&packet);
		swr_free(&swr);
		avcodec_free_context(&ctx);
		avformat_close_input(&fmt);

		return ok;
	}
}
