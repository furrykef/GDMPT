#include <fstream>
#include <memory>
#include <string>

#include <Godot.hpp>
#include <AudioStreamGenerator.hpp>
#include <AudioStreamGeneratorPlayback.hpp>
#include <AudioStreamPlayer.hpp>
#include <File.hpp>
#include <Node.hpp>

#include <libopenmpt/libopenmpt_ext.hpp>


const int SAMPLE_RATE = 44100;
const float BUFFER_LENGTH = 0.05;       // should be a bit longer than a frame


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
        register_property<ModPlayer, int>("tempo", &ModPlayer::set_tempo, &ModPlayer::get_tempo, 125);
        register_property<ModPlayer, int>("speed", &ModPlayer::set_speed, &ModPlayer::get_speed, 6);
        register_property<ModPlayer, double>("tempo_factor", &ModPlayer::set_tempo_factor, &ModPlayer::get_tempo_factor, 1.0);
    }

    void _ready()
    {
        m_gen.instance();
        m_gen->set_mix_rate(SAMPLE_RATE);   // TODO: use global mixer sampling rate?
        m_gen->set_buffer_length(BUFFER_LENGTH);
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
        m_module = std::make_unique<openmpt::module_ext>(read.ptr(), buf.size());
        // TODO: what if this fails?
        m_interactive = static_cast<openmpt::ext::interactive*>(m_module->get_interface(openmpt::ext::interactive_id));
    }

    void reset()
    {
        if (m_module) {
            m_module->set_position_order_row(0, 0);
            // TODO: reset tempo, speed, volume, channel muting, etc.
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

    int get_tempo()
    {
        if (m_module) {
            return m_module->get_current_tempo();
        } else {
            // TODO: better option?
            return -1;
        }
    }

    void set_tempo(int tempo)
    {
        // TODO: exception handling
        if (m_module && m_interactive) {
            m_interactive->set_current_tempo(tempo);
        }
    }

    int get_speed() const
    {
        if (m_module) {
            return m_module->get_current_speed();
        } else {
            // TODO: better option?
            return -1;
        }
    }

    void set_speed(int speed)
    {
        // TODO: exception handling
        if (m_module && m_interactive) {
            m_interactive->set_current_speed(speed);
        }
    }

    double get_tempo_factor()
    {
        if (m_module && m_interactive) {
            return m_interactive->get_tempo_factor();
        } else {
            // TODO: better option?
            return 0.0;
        }
    }

    void set_tempo_factor(double tempo_factor)
    {
        // TODO: exception handling
        if (m_module && m_interactive) {
            m_interactive->set_tempo_factor(tempo_factor);
        }
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

    // Other
    std::unique_ptr<openmpt::module_ext> m_module;
    openmpt::ext::interactive* m_interactive = nullptr;
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
