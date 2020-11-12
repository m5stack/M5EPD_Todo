#include "frame_setting.h"

void key_timezone_plus_cb(epdgui_args_vector_t &args)
{
    int *tz = (int*)(args[0]);
    (*tz)++;
    if((*tz) > 12)
    {
        (*tz) = 12;
    }
    String str = String(*tz);
    if((*tz) > 0)
    {
        str = "+" + str;
    }
    ((EPDGUI_Button*)(args[1]))->setLabel(str);
    ((EPDGUI_Button*)(args[1]))->Draw(UPDATE_MODE_GL16);
}

void key_timezone_minus_cb(epdgui_args_vector_t &args)
{
    int *tz = (int*)(args[0]);
    (*tz)--;
    if((*tz) < -11)
    {
        (*tz) = -11;
    }
    String str = String(*tz);
    if((*tz) > 0)
    {
        str = "+" + str;
    }
    ((EPDGUI_Button*)(args[1]))->setLabel(str);
    ((EPDGUI_Button*)(args[1]))->Draw(UPDATE_MODE_GL16);
}

void key_timezone_reset_cb(epdgui_args_vector_t &args)
{
    int *tz = (int*)(args[0]);
    (*tz) = 0;
    ((EPDGUI_Button*)(args[1]))->setLabel(String(*tz));
    ((EPDGUI_Button*)(args[1]))->Draw(UPDATE_MODE_GL16);
}

void key_exit_setting_cb(epdgui_args_vector_t &args)
{
    int *tz = (int*)(args[0]);
    uint8_t *lang = (uint8_t*)(args[1]);
    SetTimeZone(*tz);
    SetLanguage(*lang);
    SaveSetting();
    LoadSetting();
    *((int*)(args[2])) = 0;
}

void sw_en_cb(epdgui_args_vector_t &args)
{
    uint8_t *lang = (uint8_t*)(args[0]);
    *lang = LANGUAGE_EN;
}

void sw_zh_cb(epdgui_args_vector_t &args)
{
    uint8_t *lang = (uint8_t*)(args[0]);
    *lang = LANGUAGE_ZH;
}

void sw_ja_cb(epdgui_args_vector_t &args)
{
    uint8_t *lang = (uint8_t*)(args[0]);
    *lang = LANGUAGE_JA;
}

const uint16_t kTimeZoneY = 380;

Frame_Setting::Frame_Setting(void)
{
    _frame_name = "Frame_Setting";

    _canvas = new M5EPD_Canvas(&M5.EPD);
    _canvas->createCanvas(264, 60);
    _canvas->fillCanvas(0);
    _canvas->setTextSize(26);
    _canvas->setTextColor(15);
    _canvas->setTextDatum(CL_DATUM);

    _sw_en = new EPDGUI_Switch(2, 4, 100, 532, 61);
    _sw_zh = new EPDGUI_Switch(2, 4, 160, 532, 61);
    _sw_ja = new EPDGUI_Switch(2, 4, 220, 532, 61);

    key_timezone_plus = new EPDGUI_Button("+", 448, kTimeZoneY, 88, 52);
    String str = String(GetTimeZone());
    if(GetTimeZone() > 0)
    {
        str = "+" + str;
    }
    key_timezone_reset = new EPDGUI_Button(str, 360, kTimeZoneY, 88, 52);
    key_timezone_minus = new EPDGUI_Button("-", 272, kTimeZoneY, 88, 52);
    
    key_timezone_plus->AddArgs(EPDGUI_Button::EVENT_RELEASED, 0, &_timezone);
    key_timezone_plus->AddArgs(EPDGUI_Button::EVENT_RELEASED, 1, key_timezone_reset);
    key_timezone_plus->Bind(EPDGUI_Button::EVENT_RELEASED, key_timezone_plus_cb);

    key_timezone_reset->AddArgs(EPDGUI_Button::EVENT_RELEASED, 0, &_timezone);
    key_timezone_reset->AddArgs(EPDGUI_Button::EVENT_RELEASED, 1, key_timezone_reset);
    key_timezone_reset->Bind(EPDGUI_Button::EVENT_RELEASED, key_timezone_reset_cb);

    key_timezone_minus->AddArgs(EPDGUI_Button::EVENT_RELEASED, 0, &_timezone);
    key_timezone_minus->AddArgs(EPDGUI_Button::EVENT_RELEASED, 1, key_timezone_reset);
    key_timezone_minus->Bind(EPDGUI_Button::EVENT_RELEASED, key_timezone_minus_cb);

    if(isTTFLoaded())
    {
        _sw_en->SetLabel(0, "English");
        _sw_en->SetLabel(1, "English");
        _sw_zh->SetLabel(0, "中文");
        _sw_zh->SetLabel(1, "中文");
        _sw_ja->SetLabel(0, "日本語");
        _sw_ja->SetLabel(1, "日本語");
    }
    else
    {
        _sw_en->SetLabel(0, "English");
        _sw_en->SetLabel(1, "English");
        _sw_zh->SetLabel(0, "Chinese (Need .ttf file)");
        _sw_zh->SetLabel(1, "Chinese (Need .ttf file)");
        _sw_ja->SetLabel(0, "Japanese (Need .ttf file)");
        _sw_ja->SetLabel(1, "Japanese (Need .ttf file)");
    }
    _sw_en->Canvas(1)->ReverseColor();
    _sw_en->AddArgs(1, 0, &_language);
    _sw_en->Bind(1, &sw_en_cb);
    _sw_zh->Canvas(1)->ReverseColor();
    _sw_zh->AddArgs(1, 0, &_language);
    _sw_zh->Bind(1, &sw_zh_cb);
    _sw_ja->Canvas(1)->ReverseColor();
    _sw_ja->AddArgs(1, 0, &_language);
    _sw_ja->Bind(1, &sw_ja_cb);

    _sw_mutex_group = new EPDGUI_MutexSwitch();
    _sw_mutex_group->Add(_sw_en);
    _sw_mutex_group->Add(_sw_zh);
    _sw_mutex_group->Add(_sw_ja);
 
    uint8_t language = GetLanguage();
    if(language == LANGUAGE_JA)
    {
        exitbtn("エスケープ", 200);
        _canvas_title->drawString("設定", 270, 34);
        _canvas->drawString("時間帯 (UTC)", 11, 35);
        _sw_ja->setState(1);
    }
    else if(language == LANGUAGE_ZH)
    {
        exitbtn("退出");
        _canvas_title->drawString("设置", 270, 34);
        _canvas->drawString("时区 (UTC)", 11, 35);
        _sw_zh->setState(1);
    }
    else
    {
        exitbtn("Exit");
        _canvas_title->drawString("Setting", 270, 34);
        _canvas->drawString("Time zone (UTC)", 11, 35);
        _sw_en->setState(1);
    }
    
    _key_exit->AddArgs(EPDGUI_Button::EVENT_RELEASED, 0, &_timezone);
    _key_exit->AddArgs(EPDGUI_Button::EVENT_RELEASED, 1, &_language);
    _key_exit->AddArgs(EPDGUI_Button::EVENT_RELEASED, 2, &_is_run);
    _key_exit->Bind(EPDGUI_Button::EVENT_RELEASED, key_exit_setting_cb);

    _timezone = GetTimeZone();
    _language = language;
}

Frame_Setting::~Frame_Setting(void)
{
    delete _sw_en;
    delete _sw_zh;
    delete _sw_ja;
    delete _sw_mutex_group;
}

int Frame_Setting::init(epdgui_args_vector_t &args)
{
    _is_run = 1;
    M5.EPD.Clear();
    _canvas_title->pushCanvas(0, 8, UPDATE_MODE_NONE);
    _canvas->pushCanvas(4, kTimeZoneY, UPDATE_MODE_NONE);
    EPDGUI_AddObject(_sw_mutex_group);
    EPDGUI_AddObject(_key_exit);
    EPDGUI_AddObject(key_timezone_plus);
    EPDGUI_AddObject(key_timezone_reset);
    EPDGUI_AddObject(key_timezone_minus);
    return 2;
}