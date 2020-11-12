#ifndef _FRAME_TODO_H_
#define _FRAME_TODO_H_

#include "frame_base.h"
#include "../epdgui/epdgui.h"
#include "frame_todo_login.h"
#include "../azure/azure.h"

class Frame_Todo : public Frame_Base
{
public:
    Frame_Todo(String list_id);
    ~Frame_Todo();
    int init(epdgui_args_vector_t &args);
    int run();

private:
    void newTaskItem(azure_task_t *task);
    void clearKeys();

private:
    bool _is_first;
    bool _is_update;
    std::vector<EPDGUI_Button*> _keys;
    uint8_t _language;
    String _list_id;
    time_t _time;
    int _page_num = 1;
    int _current_page = 1;
};

#endif //_FRAME_TODO_H_

