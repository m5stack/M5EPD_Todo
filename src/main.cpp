#include <M5EPD.h>
#include "epdgui/epdgui.h"
#include "frame/frame.h"
#include "azure/azure.h"
#include "resources/binaryttf.h"

static const char kPrenderGlyph[77] = {
    'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', //10
       'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l',   //9
            'z', 'x', 'c', 'v', 'b', 'n', 'm',   //7
    'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', //10
       'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L',   //9
            'Z', 'X', 'C', 'V', 'B', 'N', 'M',   //7
    '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', //10
       '-', '/', ':', ';', '(', ')', '$', '&', '@',   //9
            '_', '\'', '.', ',', '?', '!',   //7
};


esp_err_t connectWiFi()
{
    if((GetWifiSSID().length() < 2) || (GetWifiPassword().length() < 8))
    {
        return ESP_FAIL;
    }
    WiFi.mode(WIFI_STA);
    WiFi.begin(GetWifiSSID().c_str(), GetWifiPassword().c_str());
    log_d("Connect to %s", GetWifiSSID().c_str());
    uint32_t t = millis();
    while (1)
    {
        if(millis() - t > 8000)
        {
            WiFi.disconnect();
            return ESP_FAIL;
        }

        if(WiFi.status() == WL_CONNECTED)
        {
            return ESP_OK;
        }
    }
}

void waitUser()
{
    M5.TP.flush();
    while(1)
    {
        if(M5.TP.avaliable())
        {
            M5.TP.update();
            if(M5.TP.getFingerNum() != 0)
            {
                return;
            }
        }
    }
}

bool is_enter_setting_mode = false;

void BootPages()
{
    // logo
    M5EPD_Canvas canvas(&M5.EPD);
    canvas.createCanvas(256, 256);
    canvas.pushImage(0, 0, 256, 256, ImageResource_todo_logo_p2_256x256);
    canvas.pushCanvas(142, 352, UPDATE_MODE_GL16);
    delay(2000);

    uint32_t boot_count = LoadBootCount();
    log_d("boot count = %lu", boot_count);
    if(boot_count > 1)
    {
        M5.EPD.Clear();
        M5.EPD.UpdateFull(UPDATE_MODE_GC16);
        return;
    }

    M5.EPD.WriteFullGram4bpp(ImageResource_instructions_540x960);
    M5.EPD.UpdateFull(UPDATE_MODE_GL16);
    waitUser();
    M5.EPD.Clear();
    M5.EPD.UpdateFull(UPDATE_MODE_GC16);

    is_enter_setting_mode = true;
}

void setup()
{
    disableCore0WDT();
    M5.begin();

    M5.update();
    if(M5.BtnL.isPressed() || M5.BtnR.isPressed() || M5.BtnP.isPressed())
    {
        is_enter_setting_mode = true;
    }

    M5.EPD.Clear(true);

    M5.EPD.SetRotation(90);
    M5.TP.SetRotation(90);

    esp_err_t load_err = LoadSetting();

    BootPages();

    M5.TP.flush();
    if(M5.TP.avaliable())
    {
        M5.TP.update();
        if(M5.TP.getFingerNum() != 0)
        {
            is_enter_setting_mode = true;
        }
    }

    log_d("M5Todo Beta");

    M5EPD_Canvas canvas(&M5.EPD);
    if(SD.exists("/font.ttf"))
    {
        canvas.loadFont("/font.ttf", SD);
        SetTTFLoaded(true);
    }
    else
    {
        canvas.loadFont(binaryttf, sizeof(binaryttf));
        SetTTFLoaded(false);
        SetLanguage(LANGUAGE_EN);
    }
    canvas.createRender(26, 128);
    canvas.createRender(36, 64);

    if(is_enter_setting_mode || (load_err != ESP_OK))
    {
        M5EPD_Canvas loadinginfo(&M5.EPD);
        loadinginfo.createCanvas(400, 32);
        loadinginfo.fillCanvas(0);
        loadinginfo.setTextSize(26);
        loadinginfo.setTextDatum(CC_DATUM);
        loadinginfo.setTextColor(15);
        loadinginfo.drawString("Loading...", 200, 16);
        loadinginfo.pushCanvas(70, 464, UPDATE_MODE_GL16);
        Frame_Base *frame = new Frame_Setting();
        EPDGUI_PushFrame(frame);
        EPDGUI_MainLoop();
        M5.EPD.Clear();
        M5.EPD.UpdateFull(UPDATE_MODE_GC16);
    }
    
    LoadingAnime_32x32_Start(254, 424);
    esp_err_t wifi_err = ESP_FAIL;
    if(load_err == ESP_OK)
    {
        wifi_err = connectWiFi();
        if(wifi_err == ESP_OK)
        {
            
            azure.begin(GetTimeZone());
        }
    }    

    if(wifi_err == ESP_FAIL)
    {
        Frame_Base *frame = new Frame_WifiPassword();
        EPDGUI_AddFrame("Frame_WifiPassword", frame);
        for(int i = 0; i < 77; i++)
        {
            canvas.preRender(kPrenderGlyph[i]);
        }
        frame = new Frame_WifiScan();
        EPDGUI_AddFrame("Frame_WifiScan", frame);
        EPDGUI_PushFrame(frame);
    }
    else if(!azure.isLogin())
    {
        Frame_Base *frame = new Frame_TodoLogin();
        EPDGUI_AddFrame("Frame_TodoLogin", frame);
        EPDGUI_PushFrame(frame);
    }
    else
    {
        Frame_Base *frame = new Frame_TodoList();
        EPDGUI_AddFrame("Frame_TodoList", frame);
        EPDGUI_PushFrame(frame);
    }

    LoadingAnime_32x32_Stop();
}


void loop()
{
    EPDGUI_MainLoop();
}
