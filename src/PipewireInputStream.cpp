/*
 * MIT License
 *
 * Copyright (c) 2025 Robin E. R. Davies
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is furnished to do
 * so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "PipewireInputStream.hpp"
#include <thread>
#include <atomic>
extern "C"
{
#include <pipewire/pipewire.h>
#include <spa/param/audio/format-utils.h>
#include <spa/param/props.h>
}
#include <stdexcept>
#include <string>

using namespace pipedal;

namespace pipedal::impl
{

    class PipeWireInputStreamImpl : public PipeWireInputStream
    {
    private:
        pw_main_loop *loop = nullptr;
        pw_stream *stream = nullptr;
        Callback callback;

        std::atomic<bool> is_running_{false};

        static void on_process(void *userData)
        {
            PipeWireInputStreamImpl *this_ = (PipeWireInputStreamImpl *)userData;
            this_->on_process();
        }

        void on_process()
        {
            pw_buffer *buf;

            if ((buf = pw_stream_dequeue_buffer(stream)) == NULL)
                return;

            // Process buffer data
            spa_buffer *sbuf = buf->buffer;
            for (uint32_t i = 0; i < sbuf->n_datas; i++)
            {
                // Access audio data through sbuf->datas[i].data
                // Size is sbuf->datas[i].chunk->size
            }

            pw_stream_queue_buffer(stream, buf);
        }

        static void on_stream_state_changed(void *userData, enum pw_stream_state old,
                                            enum pw_stream_state state, const char *error)
        {
            PipeWireInputStreamImpl *this_ = (PipeWireInputStreamImpl *)userData;
            this_->on_stream_state_changed(
                        old, state, error);
        }

        void on_stream_state_changed(enum pw_stream_state old,
                                     enum pw_stream_state state, const char *error)
        {
            if (state == PW_STREAM_STATE_ERROR)
            {
                pw_main_loop_quit(loop);
                is_running_ = false;
            }
        }

        static const struct pw_stream_events stream_events_;

    public:
        PipeWireInputStreamImpl(const std::string &stream_name,
                                uint32_t channels,
                                uint32_t rate )
            : is_running_(false)
        {

            // Initialize PipeWire
            pw_init(nullptr, nullptr);

            // Create main loop
            loop = pw_main_loop_new(nullptr);
            if (!loop)
            {
                throw std::runtime_error("Failed to create main loop");
            }

            // Create stream
            stream = pw_stream_new_simple(
                pw_main_loop_get_loop(loop),
                stream_name.c_str(),
                pw_properties_new(
                    PW_KEY_MEDIA_TYPE, "Audio",
                    PW_KEY_MEDIA_CATEGORY, "Playback",
                    PW_KEY_MEDIA_CLASS, "Audio/Sink",
                    PW_KEY_NODE_NAME, stream_name.c_str(),
                    PW_KEY_NODE_DESCRIPTION, stream_name.c_str(),
                    PW_KEY_APP_NAME, "PiPedal Test",
                    PW_KEY_APP_PROCESS_BINARY, "pipedaltest",
                    PW_KEY_MEDIA_ROLE, "DSP",
                    nullptr),
                &stream_events_,
                this);

            if (!stream)
            {
                pw_main_loop_destroy(loop);
                loop = nullptr;
                throw std::runtime_error("Failed to create stream");
            }

            // Setup audio format
            uint8_t buffer[1024];
            spa_pod_builder b = {0};
            spa_pod_builder_init(&b, buffer, sizeof(buffer));

            const spa_pod *params[1];
            spa_audio_info_raw format = {
                .format = SPA_AUDIO_FORMAT_S16,
                .flags = SPA_AUDIO_FLAG_NONE,
                .rate = rate,
                .channels = channels};
            if (channels == 1) 
            {
                format.position[0] = SPA_AUDIO_CHANNEL_MONO; // Mono channel
            }
            else if (channels == 2)
            {
                format.position[0] = SPA_AUDIO_CHANNEL_FL; // Front Left
                format.position[1] = SPA_AUDIO_CHANNEL_FR; // Front Right
            }
            else if (channels == 4)
            {
                // 3.1
                format.position[0] = SPA_AUDIO_CHANNEL_FL; 
                format.position[1] = SPA_AUDIO_CHANNEL_FR; 
                format.position[2] = SPA_AUDIO_CHANNEL_FC; 
                format.position[3] = SPA_AUDIO_CHANNEL_LFE; 
            }
            else
            {
                throw std::runtime_error("Unsupported number of channels: " + std::to_string(channels));    
            }
            params[0] = spa_format_audio_raw_build(&b, SPA_PARAM_EnumFormat, &format);

            // Connect stream
            int res = pw_stream_connect(
                stream,
                PW_DIRECTION_INPUT,
                PW_ID_ANY,
                pw_stream_flags(PW_STREAM_FLAG_AUTOCONNECT |
                                PW_STREAM_FLAG_RT_PROCESS),
                params, 1);

            if (res < 0)
            {
                pw_stream_destroy(stream);
                stream = nullptr;
                pw_main_loop_destroy(loop);
                loop = nullptr;
                throw std::runtime_error("Failed to connect stream: " + std::string(strerror(-res)));
            }

            // xxx: delete me
            pw_main_loop_run(loop);
        }

        ~PipeWireInputStreamImpl()
        {
            if (stream)
            {
                pw_stream_destroy(stream);
            }
            if (loop)
            {
                pw_main_loop_destroy(loop);
            }
            pw_deinit();
        }

        std::unique_ptr<std::jthread> serviceThread;


        virtual void Activate(Callback &&callback) override
        {
            serviceThread = std::make_unique<std::jthread>([this]() {
                if (!is_running_)
            {
                    is_running_ = true;
                    pw_main_loop_run(loop);
                    is_running_ = false;
                }
            }); 
        }

        virtual void Deactivate() override
        {
            if (is_running_)
            {
                pw_main_loop_quit(loop);
            }
            serviceThread = nullptr; // and join.
        }

        bool IsActive() const { return is_running_; }
    };

    const struct pw_stream_events PipeWireInputStreamImpl::stream_events_ = {
        .version = PW_VERSION_STREAM_EVENTS,
        .state_changed = on_stream_state_changed,
        .process = on_process,
    };
}

using namespace pipedal::impl;

PipeWireInputStream::ptr PipeWireInputStream::Create(const std::string &streamName, int sampleRate, int channels)
{
    return std::make_shared<PipeWireInputStreamImpl>(streamName, channels, sampleRate);
}