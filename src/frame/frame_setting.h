#ifndef _FRAME_LANGUAGE_H_
#define _FRAME_LANGUAGE_H_

#include "frame_base.h"
#include "../epdgui/epdgui.h"

class Frame_Setting : public Frame_Base
{
public:
    Frame_Setting();
    ~Frame_Setting();
    int init(epdgui_args_vector_t &args);

private:
    EPDGUI_Switch *_sw_en;
    EPDGUI_Switch *_sw_zh;
    EPDGUI_Switch *_sw_ja;
    EPDGUI_MutexSwitch *_sw_mutex_group;
    EPDGUI_Button *key_timezone_plus;
    EPDGUI_Button *key_timezone_reset;
    EPDGUI_Button *key_timezone_minus;
    int _timezone;
    M5EPD_Canvas *_canvas;
    uint8_t _language;
};

#endif //_FRAME_LANGUAGE_H_