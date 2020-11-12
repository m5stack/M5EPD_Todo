#include "frame_todo_login.h"
#include "frame_todo_list.h"

void key_todologin_exit_cb(epdgui_args_vector_t &args)
{
    EPDGUI_PopFrame(true);
    EPDGUI_PopFrame();
    *((int*)(args[0])) = 0;
}


Frame_TodoLogin::Frame_TodoLogin()
{
    _frame_name = "Frame_TodoLogin";

    _canvas = new M5EPD_Canvas(&M5.EPD);
    _canvas->createCanvas(540, 888);

    _language = GetLanguage();
}

Frame_TodoLogin::~Frame_TodoLogin()
{
    delete _canvas;
}


int Frame_TodoLogin::run()
{
    const uint16_t kQrCodeWidth = 300;
    const uint16_t kQrCodeX = (540 - kQrCodeWidth) / 2;
    const uint16_t kQrCodeY = kQrCodeX - 30;
    const uint16_t kCodeY = kQrCodeY + kQrCodeWidth + 30;
    const uint16_t kTextLineSpace = 35;
    const uint16_t kDescriptionY = kCodeY + 150;

    if(_is_first)
    {
        _is_first = false;
        LoadingAnime_32x32_Start(254, 424);

        esp_err_t err;
        err = azure.AuthorizationRequest();
        if(err != ESP_OK)
        {
            UserMessage(MESSAGE_ERR, 20000, "ERROR", azure.getErrInfo(), "Touch the screen to retry");
            _is_first = true;
            _canvas->fillCanvas(0);
            M5.EPD.Clear();
            M5.EPD.UpdateFull(UPDATE_MODE_GC16);
            return 1;
        }
        String url = azure.getLoginURL();

        _canvas->setTextSize(26);
        _canvas->setTextDatum(TC_DATUM);
        _canvas->qrcode(url, kQrCodeX, kQrCodeY, kQrCodeWidth);
        url.replace("https://", "");
        if(_language == LANGUAGE_JA)
        {
            _canvas->setTextSize(36);
            _canvas->drawString("コード: " + azure.getLoginCode(), 270, kCodeY);
            _canvas->setTextSize(26);
            _canvas->setTextLineSpace(6);
            _canvas->setTextArea(40, kDescriptionY, 460, 300);
            _canvas->print("ブラウザまたはQRコードをスキャンして " + url + " にアクセスし、許可を得るためにコードを入力してください。");
        }
        else if(_language == LANGUAGE_ZH)
        {
            _canvas->setTextSize(36);
            _canvas->drawString("代码: " + azure.getLoginCode(), 270, kCodeY);
            _canvas->setTextSize(26);
            _canvas->setTextLineSpace(6);
            _canvas->setTextArea(40, kDescriptionY, 460, 300);
            _canvas->print("请使用浏览器或扫描二维码访问 " + url + " ，并输入代码以取得授权。");
        }
        else
        {
            _canvas->setTextSize(36);
            _canvas->drawString("Code: " + azure.getLoginCode(), 270, kCodeY);
            _canvas->setTextSize(26);
            _canvas->setTextLineSpace(6);
            _canvas->setTextArea(40, kDescriptionY, 460, 300);
            _canvas->print("Use a browser or scan the QR code to open the page " + url + " and enter the code to authenticate.");
        }

        LoadingAnime_32x32_Stop();
        _canvas->pushCanvas(0, 72, UPDATE_MODE_GL16);
    }
    else
    {
        if(millis() - _last_update_time > 2000)
        {
            _last_update_time = millis();
            esp_err_t ret = azure.getAuthorizationStatus();
            if(ret == Azure::AUTH_STATUS_PENDING)
            {
                return 1;
            }

            if(ret == Azure::AUTH_STATUS_OK)
            {
                if(_language == LANGUAGE_JA)
                {
                    UserMessage(MESSAGE_OK, 2000, "成功", "", "画面をタッチしてください");
                }
                else if(_language == LANGUAGE_ZH)
                {
                    UserMessage(MESSAGE_OK, 2000, "成功", "", "轻触屏幕以继续");
                }
                else
                {
                    UserMessage(MESSAGE_OK, 2000, "Login successful.", "", "Touch the screen to continue");
                }
            }
            else
            {
                if(_language == LANGUAGE_JA)
                {
                    UserMessage(MESSAGE_ERR, 20000, "失敗", azure.getErrInfo(), "画面をタッチしてください");
                }
                else if(_language == LANGUAGE_ZH)
                {
                    UserMessage(MESSAGE_ERR, 20000, "失败", azure.getErrInfo(), "轻触屏幕以重试");
                }
                else
                {
                    UserMessage(MESSAGE_ERR, 20000, "Login failed.", azure.getErrInfo(), "Touch the screen to retry");
                }    
                _is_first = true;
                _canvas->fillCanvas(0);
                M5.EPD.Clear();
                M5.EPD.UpdateFull(UPDATE_MODE_GC16);
                return 1;
            }
            azure.begin(GetTimeZone());
            EPDGUI_PopFrame(true);
            Frame_Base *frame = new Frame_TodoList();
            EPDGUI_AddFrame("Frame_TodoList", frame);
            EPDGUI_PushFrame(frame);
            _is_run = 0;
            return 0;
        }
    }
    return 1;
}



int Frame_TodoLogin::init(epdgui_args_vector_t &args)
{
    _is_run = 1;
    
    _is_first = true;
    _canvas->fillCanvas(0);
    
    M5.EPD.Clear();

    return 0;
}

