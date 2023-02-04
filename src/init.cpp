#include <fstream>
#include <memory>
#include <string>

#include <Godot.hpp>
#include <AudioStreamGenerator.hpp>
#include <AudioStreamGeneratorPlayback.hpp>
#include <AudioStreamPlayer.hpp>
#include <File.hpp>
#include <Node.hpp>

#include <libopenmpt/libopenmpt.hpp>


const int SAMPLE_RATE = 44100;


class ModPlayer : public godot::Node
{
    GODOT_CLASS(ModPlayer, godot::Node);

public:
    ModPlayer() {}

    // Required even if empty
    void _init() {}

    // TODO: need some kind of destructor??

    static void _register_methods()
    {
        using godot::register_method;
        using godot::register_property;
        register_method("_ready", &ModPlayer::_ready);
        register_method("_process", &ModPlayer::_process);
        register_method("load", &ModPlayer::load);
        register_method("play", &ModPlayer::play);
        register_method("pause", &ModPlayer::pause);
        register_method("stop", &ModPlayer::stop);
        register_property<ModPlayer, godot::String>("filename", &ModPlayer::m_filename, "");
        register_property<ModPlayer, int>("repeat_count", &ModPlayer::m_repeat_count, 0);
        register_property<ModPlayer, int>("bpm", &ModPlayer::m_bpm, 32);
        register_property<ModPlayer, int>("speed", &ModPlayer::m_speed, 1);
    }

    void _ready()
    {
        m_gen.instance();
        m_gen->set_mix_rate(SAMPLE_RATE);   // TODO: use global mixer sampling rate?
        m_player = godot::AudioStreamPlayer::_new();
        m_player->set_stream(m_gen);
        add_child(m_player);
        m_player->play();
    }

    void _process(float delta)
    {
        fill_buffer();
    }

    void load()
    {
        stop();
        // TODO: exception safety!
        auto file = godot::File::_new();
        if (file->open(m_filename, godot::File::ModeFlags::READ) != godot::Error::OK) {
            // TODO: do something better here?
            godot::Godot::print("Couldn't open file");
            return;
        }
        auto buf = file->get_buffer(file->get_len());
        file->close();
        file->free();
        auto read = buf.read();
        // TODO: send in a log stream (probably an sstream); default is clog
        // TODO: what if module loading fails?
        m_module = std::make_unique<openmpt::module>(read.ptr(), buf.size());
    }

    void reset()
    {
        if (m_module) {
            m_module->set_position_order_row(0, 0);
        }
    }

    void play()
    {
        m_playing = true;
        fill_buffer();
    }

    void pause()
    {
        m_playing = false;
    }

    void stop()
    {
        pause();
        reset();
    }

    void fill_buffer()
    {
        if (!m_player || !m_module || !m_playing) {
            return;
        }
        godot::Ref<godot::AudioStreamGeneratorPlayback> playback = m_player->get_stream_playback();
        auto num_frames = playback->get_frames_available();
        if (num_frames > m_buf_left.size()) {
            // Buffers too small; embiggen them
            m_buf_left.resize(num_frames);
            m_buf_right.resize(num_frames);
        }
        if (num_frames > 0 && playback->can_push_buffer(num_frames)) {
            m_module->set_repeat_count(m_repeat_count);
            m_module->read(SAMPLE_RATE, num_frames, m_buf_left.data(), m_buf_right.data());
            // TODO: use push_buffer?
            for (int i = 0; i < num_frames; i++) {
                playback->push_frame({m_buf_left[i], m_buf_right[i]});
            }
        }
    }

private:
    // Properties
    godot::String m_filename = "";
    int m_repeat_count = 0;
    bool m_playing = false;
    int m_bpm = 32;
    int m_speed = 1;

    // Other
    std::unique_ptr<openmpt::module> m_module;
    godot::Ref<godot::AudioStreamGenerator> m_gen;
    godot::AudioStreamPlayer* m_player = nullptr;
    std::vector<float> m_buf_left;
    std::vector<float> m_buf_right;
};


extern "C" void GDN_EXPORT godot_gdnative_init(godot_gdnative_init_options* o)
{
    godot::Godot::gdnative_init(o);
}

extern "C" void GDN_EXPORT godot_gdnative_terminate(godot_gdnative_terminate_options* o)
{
    godot::Godot::gdnative_terminate(o);
}

extern "C" void GDN_EXPORT godot_nativescript_init(void* handle)
{
    godot::Godot::nativescript_init(handle);
    godot::register_class<ModPlayer>();
}
