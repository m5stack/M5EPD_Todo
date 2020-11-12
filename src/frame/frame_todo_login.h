#ifndef _FRAME_TODO_LOGIN_H_
#define _FRAME_TODO_LOGIN_H_

#include "frame_base.h"
#include "../epdgui/epdgui.h"
#include "../azure/azure.h"

class Frame_TodoLogin : public Frame_Base
{
public:
    Frame_TodoLogin();
    ~Frame_TodoLogin();
    int init(epdgui_args_vector_t &args);
    int run();

private:
    uint8_t _language;
    M5EPD_Canvas *_canvas;
    bool _is_first;
    uint32_t _last_update_time = 0;
};

#endif //_FRAME_TODO_LOGIN_H_

