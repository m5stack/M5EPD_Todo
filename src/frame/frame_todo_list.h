#ifndef _FRAME_TODOLIST_H_
#define _FRAME_TODOLIST_H_

#include "frame_base.h"
#include "../epdgui/epdgui.h"
#include "../azure/azure.h"

class Frame_TodoList : public Frame_Base
{
public:
    Frame_TodoList();
    ~Frame_TodoList();
    int init(epdgui_args_vector_t &args);
    int run();

private:
    void newListItem(azure_tasklist_t *list);

private:
    std::vector<EPDGUI_Button*> _keys;
    bool _is_first;
    int _page_num = 1;
    int _current_page = 1;
};

#endif //_FRAME_TODOLIST_H_

