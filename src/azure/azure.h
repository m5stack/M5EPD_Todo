#pragma once
#define ARDUINOJSON_DECODE_UNICODE 1
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <vector>
#include <map>
#include <time.h>
#include "nvs.h"

#define DEFAULT_TIME_ZONE 8
#define AZURE_CLIENTID    "7cc4f108-17c1-408d-830a-f7e3d5e3a8f7"

struct DefaultAllocator
{
    void *allocate(size_t n)
    {
        if(psramFound())
        {
            return ps_malloc(n);
        }
        else
        {
            return malloc(n);
        }
    }

    void deallocate(void *p)
    {
        free(p);
    }
};

#define JsonDoc BasicJsonDocument<DefaultAllocator>

typedef struct
{
    uint8_t importance;
    String status;
    String title;
    struct tm createdDateTime;
    time_t createdStamp;
    struct tm dueDateTime;
    time_t dueStamp;
    String id;
}azure_task_t;

typedef struct
{
    String display_name;
    String id;
    std::map<String, azure_task_t> taskmap;
}azure_tasklist_t;

class Azure
{
public:
    enum
    {
        TASK_IMPORTANCE_NORMAL = 0,
        TASK_IMPORTANCE_HIGH
    };
    enum
    {
        AUTH_STATUS_FAILED = ESP_FAIL,
        AUTH_STATUS_OK = ESP_OK,
        AUTH_STATUS_PENDING = 1,
        AUTH_STATUS_DECLINED,
        AUTH_STATUS_TIMEOUT
    };
public:
    Azure(String clientid);
    ~Azure();
    esp_err_t begin(int8_t timezone);
    esp_err_t AuthorizationRequest();
    esp_err_t getAuthorizationStatus();
    esp_err_t RefreshToken();
    esp_err_t getAllTodoTaskList();
    esp_err_t creatTodoTaskList(String display_name);
    esp_err_t getListTask(azure_tasklist_t *list);
    esp_err_t updateAllTask();
    esp_err_t clearAllTask();

    std::vector<String> split(String src, String delim);
    String URLEncode(const char* msg);

    esp_err_t syncTime(int8_t timezone);
    String getLoginURL();
    String getLoginCode();
    String getErrInfo();
    bool isLogin();
    bool isNeedRefreshToken();
    esp_err_t saveData();
    esp_err_t loadData();

private:
    void convertTime(String datetime, struct tm *target_time, time_t *target_stamp);

public:
    std::map<String, azure_tasklist_t> tasklistmap;

private:
    int8_t _timezone;
    bool _is_login = false;
    struct tm _time_info;
    time_t _time_stamp;
    String _err_info;
    String _login_url;
    String _login_code;
    String _user_id;
    String _client_id;
    String _refresh_token;
    String _access_token;
    String _device_code;
    uint32_t _expires_in = 0;
    uint32_t _token_update_time = 0;
    uint32_t _next_update_time = 0;
    String _id_token;
    JsonDoc *_json_doc;
};

extern Azure azure;
