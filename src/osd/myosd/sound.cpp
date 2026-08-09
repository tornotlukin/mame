// license:BSD-3-Clause
//============================================================
//
//  sound.cpp -  osd sound handling
//
//  MAME4DROID by David Valdeita (Seleuco)
//
//============================================================

// MAME headers
#include "emu.h"

// DROID headers
#include "myosd.h"

static void myosd_sound_init(int rate, int stereo);
static void myosd_sound_play(void *buff, int len);
static void myosd_sound_exit(void);

//============================================================
//  sound_init
//============================================================

void my_osd_interface::sound_init()
{
    osd_printf_verbose("droid_osd_interface::sound_init\n");

    // if the host does not want to handle audio, do a default
    if (m_callbacks.sound_play == NULL)
    {
        m_callbacks.sound_init = myosd_sound_init;
        m_callbacks.sound_play = myosd_sound_play;
        m_callbacks.sound_exit = myosd_sound_exit;
    }

    m_sample_rate = options().sample_rate();

    if (strcmp(options().value(OPTION_SOUND), "none") == 0)
        m_sample_rate = 0;

    if (m_sample_rate != 0)
    {
        m_callbacks.sound_init(m_sample_rate, 1);
    }

    m_current_stream_id = 0;
    m_next_stream_id = 1;
}

//============================================================
//  sound_exit
//============================================================

void my_osd_interface::sound_exit()
{
    osd_printf_verbose("droid_osd_interface::sound_exit\n");
    if (m_sample_rate != 0)
    {
        m_callbacks.sound_exit();
    }
    m_current_stream_id = 0;
}

osd::audio_info my_osd_interface::sound_get_information()
{
    osd::audio_info result;
    result.m_generation = 1;
    result.m_default_sink = 1;
    result.m_default_source = 0;
    result.m_nodes.resize(1);
    result.m_nodes[0].m_name = "myosdsound";
    result.m_nodes[0].m_display_name = "myosd sound";
    result.m_nodes[0].m_id = 1;
    result.m_nodes[0].m_rate.m_default_rate = 0; // Magic value meaning "use configured sample rate"
    result.m_nodes[0].m_rate.m_min_rate = 0;
    result.m_nodes[0].m_rate.m_max_rate = 0;
    result.m_nodes[0].m_sinks = 2;
    result.m_nodes[0].m_sources = 0;
    result.m_nodes[0].m_port_names.reserve(2);
    result.m_nodes[0].m_port_names.emplace_back("L");
    result.m_nodes[0].m_port_names.emplace_back("R");
    result.m_nodes[0].m_port_positions.reserve(2);
    result.m_nodes[0].m_port_positions.emplace_back(osd::channel_position::FL());
    result.m_nodes[0].m_port_positions.emplace_back(osd::channel_position::FR());
    if (m_current_stream_id) {
        result.m_streams.resize(1);
        result.m_streams[0].m_id = m_current_stream_id;
        result.m_streams[0].m_node = 1;
    }
    return result;
}

bool my_osd_interface::no_sound()
{
    return m_sample_rate == 0;
}

uint32_t my_osd_interface::sound_stream_sink_open(uint32_t node, std::string name, uint32_t rate){
    osd_printf_verbose("my_osd_interface::sound_stream_sink_open");
    //sound_init();
    if (m_current_stream_id)
        return 0;

    m_current_stream_id = m_next_stream_id++;
    return m_current_stream_id;
}

void my_osd_interface::sound_stream_close(uint32_t id){
    osd_printf_verbose("my_osd_interface::sound_stream_close");
    //sound_exit();
    if (id == m_current_stream_id)
        m_current_stream_id = 0;
}

void my_osd_interface::sound_stream_sink_update(
        uint32_t id,
        int16_t const *buffer,
        int samples_this_frame)
{
    osd_printf_verbose("my_osd_interface::update_audio_stream: samples=%d \n", samples_this_frame);

    if (m_sample_rate == 0 || m_callbacks.sound_play==NULL  || buffer==NULL || id != m_current_stream_id)
        return;

    //if(machine().video().fastforward())
        //return;

    m_callbacks.sound_play((void*)buffer,samples_this_frame * sizeof(int16_t) * 2);
}


//============================================================
//  default sound impl
//============================================================


static void myosd_sound_init(int rate, int stereo)
{

}

static void myosd_sound_exit(void)
{

}

static void myosd_sound_play(void *buff, int len)
{

}
