#include "frame_todo_list.h"
#include "frame_todo.h"

#define MAX_ITEM_NUM 12


void key_todolist_select_cb(epdgui_args_vector_t &args)
{
    EPDGUI_Button *key = (EPDGUI_Button*)(args[1]);
    Frame_Base *frame = new Frame_Todo(key->GetCustomString());
    EPDGUI_PushFrame(frame);
    *((int*)(args[0])) = 0;
}

Frame_TodoList::Frame_TodoList()
{
    _frame_name = "Frame_TodoList";

    _is_first = true;
}

Frame_TodoList::~Frame_TodoList()
{

}

void Frame_TodoList::newListItem(azure_tasklist_t *list)
{
    EPDGUI_Button *key = new EPDGUI_Button(4, 4 + (_keys.size() % MAX_ITEM_NUM) * 80, 532, 80);
    _keys.push_back(key);

    uint16_t len = list->display_name.length();
    uint16_t n = 0;
    int charcnt = 0;
    char buf[len];
    memcpy(buf, list->display_name.c_str(), len);
    String title = list->display_name;
    while (n < len)
    {
        uint16_t unicode = key->CanvasNormal()->decodeUTF8((uint8_t*)buf, &n, len - n);
        if((unicode > 0) && (unicode < 0x7F))
        {
            charcnt += 1;
        }
        else
        {
            charcnt += 2;
        }
        if(charcnt > 24)
        {
            title = title.substring(0, n);
            title += "...";
            break;
        }
    }

    key->setBMPButton(title, String(list->taskmap.size()), ImageResource_item_icon_tasklist_32x32);
    key->SetCustomString(list->id);
    key->AddArgs(EPDGUI_Button::EVENT_RELEASED, 0, &_is_run);
    key->AddArgs(EPDGUI_Button::EVENT_RELEASED, 1, key);
    key->Bind(EPDGUI_Button::EVENT_RELEASED, key_todolist_select_cb);
    if(list->taskmap.size() == 0)
    {
        key->SetEnable(false);
    }
}


int Frame_TodoList::run()
{
    if(_is_first)
    {
        _current_page = 1;
        log_d("heap = %d, psram = %d", ESP.getFreeHeap(), ESP.getFreePsram());
        _is_first = false;
        LoadingAnime_32x32_Start(254, 424);
        
        for(int i = 0; i < _keys.size(); i++)
        {
            delete _keys[i];
        }
        _keys.clear();
        EPDGUI_Clear();
        esp_err_t err = azure.updateAllTask();
        if(err != ESP_OK)
        {
            LoadingAnime_32x32_Stop();
            UserMessage(MESSAGE_ERR, 20000, "ERROR", azure.getErrInfo(), "Touch the screen to retry");
            _is_first = true;
            M5.EPD.Clear();
            M5.EPD.UpdateFull(UPDATE_MODE_GC16);
            return 1;
        }
        _page_num = (azure.tasklistmap.size() - 1) / MAX_ITEM_NUM + 1;
        for(auto iter = azure.tasklistmap.begin(); iter != azure.tasklistmap.end(); iter++)
        {
            newListItem(&(iter->second));
        }
        
        int size = _keys.size() > MAX_ITEM_NUM ? MAX_ITEM_NUM : _keys.size();
        for(int i = 0; i < size; i++)
        {
            EPDGUI_AddObject(_keys[i]);
        }

        LoadingAnime_32x32_Stop();
        M5.EPD.Clear();
        EPDGUI_Draw(UPDATE_MODE_NONE);
        M5.EPD.UpdateFull(UPDATE_MODE_GC16);
    }

    M5.update();

    if(M5.BtnR.wasReleased())
    {
        if(_current_page != _page_num)
        {
            _current_page++;
            EPDGUI_Clear();
            int start = (_current_page - 1) * MAX_ITEM_NUM;
            int end = _keys.size() > (_current_page * MAX_ITEM_NUM) ? (_current_page * MAX_ITEM_NUM) : _keys.size();
            for(int i = start; i < end; i++)
            {
                EPDGUI_AddObject(_keys[i]);
            }
            M5.EPD.Clear();
            EPDGUI_Draw(UPDATE_MODE_NONE);
            M5.EPD.UpdateFull(UPDATE_MODE_GC16);
        }
    }
    if(M5.BtnL.wasReleased())
    {
        if(_current_page != 1)
        {
            _current_page--;
            EPDGUI_Clear();
            int start = (_current_page - 1) * MAX_ITEM_NUM;
            int end = _keys.size() > (_current_page * MAX_ITEM_NUM) ? (_current_page * MAX_ITEM_NUM) : _keys.size();
            for(int i = start; i < end; i++)
            {
                EPDGUI_AddObject(_keys[i]);
            }
            M5.EPD.Clear();
            EPDGUI_Draw(UPDATE_MODE_NONE);
            M5.EPD.UpdateFull(UPDATE_MODE_GC16);
        }
    }
    if(M5.BtnP.wasReleased())
    {
        M5.EPD.Clear();
        M5.EPD.UpdateFull(UPDATE_MODE_GL16);
        _is_first = true;
    }

    if(azure.isNeedRefreshToken())
    {
        azure.RefreshToken();
    }
    
    return 1;
}

int Frame_TodoList::init(epdgui_args_vector_t &args)
{
    _is_run = 1;
    M5.EPD.Clear();

    _current_page = 1;
    int size = _keys.size() > MAX_ITEM_NUM ? MAX_ITEM_NUM : _keys.size();
    for(int i = 0; i < size; i++)
    {
        EPDGUI_AddObject(_keys[i]);
    }
    return 0;
}
