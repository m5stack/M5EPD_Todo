#ifndef _GLOBAL_SETTING_H_
#define _GLOBAL_SETTING_H_

#include <M5EPD.h>
#include <nvs.h>


enum
{
    LANGUAGE_EN = 0,    // default, English
    LANGUAGE_JA, // Japanese
    LANGUAGE_ZH // Simplified Chinese
};

enum
{
    MESSAGE_INFO = 0,
    MESSAGE_WARN,
    MESSAGE_ERR,
    MESSAGE_OK
};

void SetLanguage(uint8_t language);
uint8_t GetLanguage(void);
esp_err_t LoadSetting(void);
esp_err_t SaveSetting(void);
void SetWifi(String ssid, String password);
String GetWifiSSID(void);
String GetWifiPassword(void);
uint8_t isWiFiConfiged(void);
bool SyncNTPTime(void);

int8_t GetTimeZone(void);
void SetTimeZone(int8_t time_zone);

const uint8_t* GetLoadingIMG_32x32(uint8_t id);
void LoadingAnime_32x32_Start(uint16_t x, uint16_t y);
void LoadingAnime_32x32_Stop();

uint8_t isTimeSynced(void);
void SetTimeSynced(uint8_t val);

void SetTTFLoaded(uint8_t val);
uint8_t isTTFLoaded(void);

void UserMessage(int level, uint32_t timeout, String title, String msg = "", String end = "");
uint32_t LoadBootCount(void);

#endif //_GLOBAL_SETTING_H_