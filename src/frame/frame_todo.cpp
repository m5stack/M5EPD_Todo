#include "frame_todo.h"

#define MAX_ITEM_NUM 10

void key_todo_exit_cb(epdgui_args_vector_t &args)
{
    EPDGUI_PopFrame(true);
    *((int*)(args[0])) = 0;
}


Frame_Todo::Frame_Todo(String list_id)
{
    _frame_name = "Frame_Todo";
    _list_id = list_id;

    _language = GetLanguage();
    _canvas_title->drawString(azure.tasklistmap[_list_id].display_name, 270, 34);
    if(_language == LANGUAGE_JA)
    {
        exitbtn("戻る");
    }
    else if(_language == LANGUAGE_ZH)
    {
        exitbtn("返回");
    }
    else
    {
        exitbtn("Back");
    }

    time(&_time);

    _key_exit->AddArgs(EPDGUI_Button::EVENT_RELEASED, 0, (void*)(&_is_run));
    _key_exit->Bind(EPDGUI_Button::EVENT_RELEASED, key_todo_exit_cb);
}

Frame_Todo::~Frame_Todo()
{
    clearKeys();
}

void Frame_Todo::clearKeys()
{
    for(int i = 0; i < _keys.size(); i++)
    {
        if(_keys[i] != NULL)
        {
            delete _keys[i];
        }
    }
    _keys.clear();
}

void Frame_Todo::newTaskItem(azure_task_t *task)
{
    const uint16_t kHeight = 80;
    const uint16_t kHeightdiv2 = kHeight >> 1;

    if(task->status == "completed")
    {
        return;
    }

    EPDGUI_Button *key = new EPDGUI_Button(4, 72 + (_keys.size() % MAX_ITEM_NUM) * (kHeight + 10), 532, kHeight + 4);
    _keys.push_back(key);

    key->SetEnable(false);
    key->CanvasNormal()->fillCanvas(0);
    // key->CanvasNormal()->drawRect(0, 0, 532, kHeight, 15);
    key->CanvasNormal()->setTextSize(26);
    key->CanvasNormal()->setTextColor(15);
    key->CanvasNormal()->setTextDatum(CL_DATUM);

    uint16_t len = task->title.length();
    uint16_t n = 0;
    int charcnt = 0;
    char buf[len];
    memcpy(buf, task->title.c_str(), len);
    String title = task->title;
    while (n < len)
    {
        uint16_t unicode = key->CanvasNormal()->decodeUTF8((uint8_t*)buf, &n, len - n);
        if((unicode > 0) && (unicode < 0x7F))
        {
            charcnt++;
        }
        else
        {
            charcnt += 2;
        }
        if(charcnt >= 24)
        {
            title = title.substring(0, n);
            title += "...";
            break;
        }
    }

    if(task->dueStamp != 0)
    {
        key->CanvasNormal()->drawString(title, 72, kHeightdiv2 - 15);
        char strtime[100];
        struct tm t = task->dueDateTime;
        sprintf(strtime, "%d/%d/%d %d:%02d", t.tm_year, t.tm_mon, t.tm_mday, t.tm_hour, t.tm_min);
        key->CanvasNormal()->setTextSize(26);
        key->CanvasNormal()->setTextColor(6);
        key->CanvasNormal()->drawString(strtime, 72, kHeightdiv2 + 20);
    }
    else
    {
        key->CanvasNormal()->drawString(title, 72, kHeightdiv2 + 5);
    }
    
    if((task->dueStamp != 0) && (task->dueStamp < _time))
    {
        key->CanvasNormal()->pushImage(15, kHeightdiv2 - 16, 32, 32, ImageResource_item_icon_timeout_32x32);
    }
    else
    {
        key->CanvasNormal()->pushImage(15, kHeightdiv2 - 16, 32, 32, ImageResource_item_icon_task_32x32);
    }
    
    if(task->importance == Azure::TASK_IMPORTANCE_HIGH)
    {
        key->CanvasNormal()->pushImage(532 - 15 - 32, kHeightdiv2 - 16, 32, 32, ImageResource_item_icon_task_important_32x32);
    }
    else
    {
        key->CanvasNormal()->pushImage(532 - 15 - 32, kHeightdiv2 - 16, 32, 32, ImageResource_item_icon_task_normal_32x32);
    }

    *(key->CanvasPressed()) = *(key->CanvasNormal());
    key->CanvasPressed()->ReverseColor();
    key->SetCustomString(task->id);
}

int Frame_Todo::run()
{
    if(_is_first || _is_update)
    {
        _current_page = 1;
        log_d("heap = %d, psram = %d", ESP.getFreeHeap(), ESP.getFreePsram());
        _is_first = false;
        LoadingAnime_32x32_Start(254, 424);
        clearKeys();
        EPDGUI_Clear();
        if(_is_update)
        {
            _is_update = false;
            esp_err_t err = azure.getListTask(&(azure.tasklistmap[_list_id]));
            if(err != ESP_OK)
            {
                LoadingAnime_32x32_Stop();
                delay(500);
                UserMessage(MESSAGE_ERR, 20000, "ERROR", azure.getErrInfo(), "Touch the screen to retry");
                _is_update = true;
                M5.EPD.FillPartGram4bpp(0, 72, 540, 888, 0xFFFF);
                M5.EPD.UpdateFull(UPDATE_MODE_GC16);
                return 1;
            }
        }
        _page_num = (azure.tasklistmap[_list_id].taskmap.size() - 1) / MAX_ITEM_NUM + 1;
        for(auto iter = azure.tasklistmap[_list_id].taskmap.begin(); iter != azure.tasklistmap[_list_id].taskmap.end(); iter++)
        {
            newTaskItem(&(iter->second));
        }
        LoadingAnime_32x32_Stop();
        delay(500);
        M5.EPD.Clear();
        log_d("_keys.size() = %d", _keys.size());
        int size = _keys.size() > MAX_ITEM_NUM ? MAX_ITEM_NUM : _keys.size();
        for(int i = 0; i < size; i++)
        {
            EPDGUI_AddObject(_keys[i]);
            log_d("add %d, %d", i, _keys[i]->GetID());
        }
        EPDGUI_AddObject(_key_exit);
        _canvas_title->pushCanvas(0, 8, UPDATE_MODE_NONE);
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
            EPDGUI_AddObject(_key_exit);
            int start = (_current_page - 1) * MAX_ITEM_NUM;
            int end = _keys.size() > (_current_page * MAX_ITEM_NUM) ? (_current_page * MAX_ITEM_NUM) : _keys.size();
            for(int i = start; i < end; i++)
            {
                EPDGUI_AddObject(_keys[i]);
            }
            M5.EPD.FillPartGram4bpp(0, 72, 540, 888, 0xFFFF);
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
            EPDGUI_AddObject(_key_exit);
            int start = (_current_page - 1) * MAX_ITEM_NUM;
            int end = _keys.size() > (_current_page * MAX_ITEM_NUM) ? (_current_page * MAX_ITEM_NUM) : _keys.size();
            for(int i = start; i < end; i++)
            {
                EPDGUI_AddObject(_keys[i]);
            }
            M5.EPD.FillPartGram4bpp(0, 72, 540, 888, 0xFFFF);
            EPDGUI_Draw(UPDATE_MODE_NONE);
            M5.EPD.UpdateFull(UPDATE_MODE_GC16);
        }
    }
    if(M5.BtnP.wasReleased())
    {
        M5.EPD.FillPartGram4bpp(0, 72, 540, 888, 0xFFFF);
        M5.EPD.UpdateFull(UPDATE_MODE_GL16);
        _is_update = true;
    }

    if(azure.isNeedRefreshToken())
    {
        azure.RefreshToken();
        M5.EPD.FillPartGram4bpp(0, 72, 540, 888, 0xFFFF);
        M5.EPD.UpdateFull(UPDATE_MODE_GL16);
        _is_update = true;
    }

    return 1;
}

int Frame_Todo::init(epdgui_args_vector_t &args)
{
    _is_first = true;
    _is_update = false;
    _is_run = 1;
    M5.EPD.Clear();
    _canvas_title->pushCanvas(0, 8, UPDATE_MODE_NONE);
    EPDGUI_AddObject(_key_exit);
    _keys.clear();
    
    return 1;
}
