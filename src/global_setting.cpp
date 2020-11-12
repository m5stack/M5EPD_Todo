#include "global_setting.h"
#include "./resources/ImageResource.h"
#include "esp32-hal-log.h"
#include <WiFi.h>

#define DEFAULT_WALLPAPER 2
SemaphoreHandle_t _xSemaphore_LoadingAnime = NULL;
static uint8_t _loading_anime_eixt_flag = false;
esp_err_t __espret__;
#define NVS_CHECK(x)              \
    __espret__ = x;           \
    if (__espret__ != ESP_OK) \
    {                         \
        nvs_close(nvs_arg);   \
        log_e("Check Err");   \
        return __espret__;    \
    }

const uint8_t *kIMGLoading[16] = {
    ImageResource_item_loading_01_32x32,
    ImageResource_item_loading_02_32x32,
    ImageResource_item_loading_03_32x32,
    ImageResource_item_loading_04_32x32,
    ImageResource_item_loading_05_32x32,
    ImageResource_item_loading_06_32x32,
    ImageResource_item_loading_07_32x32,
    ImageResource_item_loading_08_32x32,
    ImageResource_item_loading_09_32x32,
    ImageResource_item_loading_10_32x32,
    ImageResource_item_loading_11_32x32,
    ImageResource_item_loading_12_32x32,
    ImageResource_item_loading_13_32x32,
    ImageResource_item_loading_14_32x32,
    ImageResource_item_loading_15_32x32,
    ImageResource_item_loading_16_32x32
};

uint8_t global_language = LANGUAGE_EN;
String global_wifi_ssid;
String global_wifi_password;
uint8_t global_wifi_configed = false;
uint8_t global_time_synced = false;
uint8_t global_ttf_file_loaded = false;
uint32_t global_boot_count = 0;
int8_t global_timezone = 8;
M5EPD_Canvas loadinginfo(&M5.EPD);
M5EPD_Canvas loading(&M5.EPD);

int8_t GetTimeZone(void)
{
    return global_timezone;
}

void SetTimeZone(int8_t time_zone)
{
    global_timezone = time_zone;
}

void SetTTFLoaded(uint8_t val)
{
    global_ttf_file_loaded = val;
}

uint8_t isTTFLoaded()
{
    return global_ttf_file_loaded;
}

uint8_t isTimeSynced(void)
{
    return global_time_synced;
}

void SetTimeSynced(uint8_t val)
{
    global_time_synced = val;
    SaveSetting();
}

void SetLanguage(uint8_t language)
{
    if (language >= LANGUAGE_EN && language <= LANGUAGE_ZH)
    {
        global_language = language;
    }
}

uint8_t GetLanguage(void)
{
    return global_language;
}

uint32_t LoadBootCount(void)
{
    nvs_handle nvs_arg;
    nvs_open("boot", NVS_READWRITE, &nvs_arg);
    nvs_get_u32(nvs_arg, "count", &global_boot_count);
    global_boot_count++;
    nvs_set_u32(nvs_arg, "count", global_boot_count);
    nvs_commit(nvs_arg);
    nvs_close(nvs_arg);

    return global_boot_count;
}


esp_err_t LoadSetting(void)
{
    nvs_handle nvs_arg;
    NVS_CHECK(nvs_open("system", NVS_READONLY, &nvs_arg));
    NVS_CHECK(nvs_get_u8(nvs_arg, "Language", &global_language));
    NVS_CHECK(nvs_get_u8(nvs_arg, "Timesync", &global_time_synced));

    size_t length = 128;
    char buf[128];
    NVS_CHECK(nvs_get_str(nvs_arg, "ssid", buf, &length));
    global_wifi_ssid = String(buf);
    length = 128;
    NVS_CHECK(nvs_get_str(nvs_arg, "pswd", buf, &length));
    global_wifi_password = String(buf);
    // log_d("ssid = %s, pswd = %s", global_wifi_ssid.c_str(), global_wifi_password.c_str());
    global_wifi_configed = true;

    nvs_get_i8(nvs_arg, "timezone", &global_timezone);
    log_d("Success");

    nvs_close(nvs_arg);
    return ESP_OK;
}

esp_err_t SaveSetting(void)
{
    log_d("%d", global_timezone);
    nvs_handle nvs_arg;
    NVS_CHECK(nvs_open("system", NVS_READWRITE, &nvs_arg));
    NVS_CHECK(nvs_set_u8(nvs_arg, "Language", global_language));
    NVS_CHECK(nvs_set_u8(nvs_arg, "Timesync", global_time_synced));
    NVS_CHECK(nvs_set_i8(nvs_arg, "timezone", global_timezone));
    NVS_CHECK(nvs_set_str(nvs_arg, "ssid", global_wifi_ssid.c_str()));
    NVS_CHECK(nvs_set_str(nvs_arg, "pswd", global_wifi_password.c_str()));
    // log_d("ssid = %s, pswd = %s", global_wifi_ssid.c_str(), global_wifi_password.c_str());
    NVS_CHECK(nvs_commit(nvs_arg));
    log_d("Success");
    nvs_close(nvs_arg);
    return ESP_OK;
}

void SetWifi(String ssid, String password)
{
    global_wifi_ssid = ssid;
    global_wifi_password = password;
    SaveSetting();
}

uint8_t isWiFiConfiged(void)
{
    return global_wifi_configed;
}

String GetWifiSSID(void)
{
    return global_wifi_ssid;
}

String GetWifiPassword(void)
{
    return global_wifi_password;
}

bool SyncNTPTime(void)
{
    const char *ntpServer = "time.cloudflare.com";
    configTime(8 * 3600, 0, ntpServer);

    struct tm timeInfo;
    if (getLocalTime(&timeInfo))
    {
        RTC_TimeTypeDef time_struct;
        time_struct.Hours = timeInfo.tm_hour;
        time_struct.Minutes = timeInfo.tm_min;
        time_struct.Seconds = timeInfo.tm_sec;
        M5.RTC.SetTime(&time_struct);
        RTC_DateTypeDef date_struct;
        date_struct.WeekDay = timeInfo.tm_wday;
        date_struct.Month = timeInfo.tm_mon + 1;
        date_struct.Date = timeInfo.tm_mday;
        date_struct.Year = timeInfo.tm_year + 1900;
        M5.RTC.SetDate(&date_struct);
        SetTimeSynced(1);
        log_d("Time Synced %d-%d-%d [%d]  %d:%02d:%02d", date_struct.Year, date_struct.Month, date_struct.Date, date_struct.WeekDay, time_struct.Hours, time_struct.Minutes, time_struct.Seconds);
        return 1;
    }
    log_d("Time Sync failed");
    return 0;
}

const uint8_t* GetLoadingIMG_32x32(uint8_t id)
{
    return kIMGLoading[id];
}

void __LoadingAnime_32x32(void *pargs)
{
    uint16_t *args = (uint16_t *)pargs;
    uint16_t x = args[0];
    uint16_t y = args[1];
    free(pargs);

    loading.fillCanvas(0);
    loading.pushCanvas(x, y, UPDATE_MODE_GL16);
    int anime_cnt = 0;
    uint32_t time = 0;
    uint32_t timeout = millis();
    bool updated = false;
    loadinginfo.pushCanvas(70, y + 45, UPDATE_MODE_GL16);
    while (1)
    {
        if(millis() - time > 250)
        {
            time = millis();
            loading.pushImage(0, 0, 32, 32, GetLoadingIMG_32x32(anime_cnt));
            loading.pushCanvas(x, y, UPDATE_MODE_DU4);
            anime_cnt++;
            if(anime_cnt == 16)
            {
                anime_cnt = 0;
            }
        }

        if((updated == false) && (millis() - timeout > 60 * 1000))
        {
            updated = true;
            log_e("Loading timeout, system restart.");
            delay(1000);
            ESP.restart();
        }

        // xSemaphoreTake(_xSemaphore_LoadingAnime, portMAX_DELAY);
        if(_loading_anime_eixt_flag == true)
        {
            // xSemaphoreGive(_xSemaphore_LoadingAnime);
            break;
        }
        // xSemaphoreGive(_xSemaphore_LoadingAnime);
    }
    vTaskDelete(NULL);
}

void LoadingAnime_32x32_Start(uint16_t x, uint16_t y)
{
    if(_xSemaphore_LoadingAnime == NULL)
    {
        _xSemaphore_LoadingAnime = xSemaphoreCreateMutex();
        loadinginfo.createCanvas(400, 32);
        loadinginfo.fillCanvas(0);
        loadinginfo.setTextSize(26);
        loadinginfo.setTextDatum(CC_DATUM);
        loadinginfo.setTextColor(15);
        loadinginfo.drawString("Loading...", 200, 16);
        loading.createCanvas(32, 32);
        loading.fillCanvas(0);
    }
    _loading_anime_eixt_flag = false;
    uint16_t *pos = (uint16_t*)calloc(2, sizeof(uint16_t));
    pos[0] = x;
    pos[1] = y;
    xTaskCreatePinnedToCore(__LoadingAnime_32x32, "__LoadingAnime_32x32", 16 * 1024, pos, 1, NULL, 0);
}

void LoadingAnime_32x32_Stop()
{
    // xSemaphoreTake(_xSemaphore_LoadingAnime, portMAX_DELAY);
    _loading_anime_eixt_flag = true;
    // xSemaphoreGive(_xSemaphore_LoadingAnime);
    delay(200);
}

void UserMessage(int level, uint32_t timeout, String title, String msg, String end)
{
    M5.EPD.Clear();
    M5EPD_Canvas canvas(&M5.EPD);
    canvas.createCanvas(460, 460);
    canvas.fillCanvas(0);

    switch(level)
    {
        case MESSAGE_INFO:
        {
            canvas.pushImage(182, 0, 96, 96, ImageResource_message_level_info_96x96);
            break;
        }
        case MESSAGE_WARN:
        {
            canvas.pushImage(182, 0, 96, 96, ImageResource_message_level_warn_96x96);
            break;
        }
        case MESSAGE_ERR:
        {
            canvas.pushImage(182, 0, 96, 96, ImageResource_message_level_error_96x96);
            break;
        }
        case MESSAGE_OK:
        {
            canvas.pushImage(182, 0, 96, 96, ImageResource_message_level_success_96x96);
            break;
        }
    }

    const uint16_t kTitleY = 96 + 35;
    const uint16_t kMsgY = kTitleY + 50;
    const uint16_t kEndY = 430;

    canvas.setTextDatum(TC_DATUM);
    canvas.setTextSize(36);
    canvas.drawString(title, 230, kTitleY);

    if(msg.length())
    {
        canvas.setTextSize(26);
        canvas.setTextColor(8);
        if(msg.length() < 40)
        {
            canvas.setTextDatum(TC_DATUM);
            canvas.drawString(msg, 230, kMsgY);
        }
        else
        {
            canvas.setTextArea(0, kMsgY, 460, 200);
            canvas.print(msg);
        }
    }
    
    if(end.length())
    {
        canvas.setTextDatum(TC_DATUM);
        canvas.setTextSize(26);
        canvas.setTextColor(8);
        canvas.drawString(end, 230, kEndY);
    }
    
    canvas.pushCanvas(40, 250, UPDATE_MODE_NONE);
    M5.EPD.UpdateFull(UPDATE_MODE_GC16);

    while(M5.TP.avaliable());
    M5.TP.flush();
    uint32_t t = millis();
    while(1)
    {
        if(M5.TP.avaliable())
        {
            M5.TP.update();
            if(M5.TP.getFingerNum() != 0)
            {
                break;
            }
        }
        if(millis() - t > timeout)
        {
            break;
        }
    }
}
