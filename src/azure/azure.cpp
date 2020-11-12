#include "azure.h"

esp_err_t __azureespret__;
#define NVS_CHECK(x)          \
    __azureespret__ = x;           \
    if (__azureespret__ != ESP_OK) \
    {                         \
        nvs_close(nvs_arg);   \
        log_e("Check Err");   \
        return __azureespret__;    \
    }

#define ESP_CHECK(x)          \
    __azureespret__ = x;           \
    if (__azureespret__ != ESP_OK) \
    {                         \
        log_e("Check Err");   \
        return __azureespret__;    \
    }

Azure::Azure(String clientid)
{
    _json_doc = new JsonDoc(16384);
    _client_id = clientid;
}

Azure::~Azure()
{
    clearAllTask();
    _json_doc->clear();
}

std::vector<String> Azure::split(String src, String delim)
{
    std::vector<String> buf;
    int32_t flag = 0, pst = 0;
    int32_t len = delim.length();

    while (1)
    {
        flag = src.indexOf(delim, flag);
        if (flag < 0)
            break;
        buf.push_back(src.substring(pst, flag));
        pst = flag + len;
        flag += len;
    }

    if (pst < src.length())
    {
        buf.push_back(src.substring(pst, src.length()));
    }

    return buf;
}

String Azure::URLEncode(const char *msg)
{
    const char *hex = "0123456789abcdef";
    String encodedMsg = "";

    while (*msg != '\0')
    {
        if (('a' <= *msg && *msg <= 'z') || ('A' <= *msg && *msg <= 'Z') || ('0' <= *msg && *msg <= '9'))
        {
            encodedMsg += *msg;
        }
        else
        {
            encodedMsg += '%';
            encodedMsg += hex[*msg >> 4];
            encodedMsg += hex[*msg & 15];
        }
        msg++;
    }
    return encodedMsg;
}

esp_err_t Azure::begin(int8_t timezone)
{
    _timezone = timezone;
    for (int retry = 0; retry < 3; retry++)
    {
        if (syncTime(timezone) == ESP_OK)
        {
            break;
        }
    }

    ESP_CHECK(loadData());
    if (_is_login)
    {
        if (RefreshToken() == ESP_FAIL)
        {
            return ESP_FAIL;
        }
    }
    return ESP_OK;
}

bool Azure::isLogin()
{
    return _is_login;
}

String Azure::getLoginURL()
{
    return _login_url;
}

String Azure::getLoginCode()
{
    return _login_code;
}

String Azure::getErrInfo()
{
    return _err_info;
}

esp_err_t Azure::AuthorizationRequest()
{
    HTTPClient http;
    String url = "https://login.microsoftonline.com/common/oauth2/v2.0/devicecode";

    http.begin(url);
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");

    String data = "client_id=" + _client_id +
                  "&scope=openid offline_access tasks.readwrite";
    int httpResponseCode = http.POST(data);

    if (httpResponseCode != 200)
    {
        log_e("HTTP Request failed. code = %d. payload = %s", httpResponseCode, http.getString());
        _err_info = "HTTP request failed. code = " + String(httpResponseCode);
        return ESP_FAIL;
    }

    String result = http.getString();
    http.end();

    DeserializationError err = deserializeJson((*_json_doc), result);
    if (err)
    {
        log_e("DeserializeJson failed: %s", err.c_str());
        _err_info = "DeserializeJson failed: " + String(err.c_str());
        return ESP_FAIL;
    }
    log_d("json doc usage: %d/%d", (*_json_doc).memoryUsage(), (*_json_doc).capacity());

    if ((*_json_doc).containsKey("error"))
    {
        log_e("Error occurred. Description: %s", (*_json_doc)["error_description"].as<String>().c_str());
        _err_info = "Error occurred. Description: " + (*_json_doc)["error_description"].as<String>();
        return ESP_FAIL;
    }

    log_d("User code: %s", (*_json_doc)["user_code"].as<String>().c_str());
    log_d("Device code: %s", (*_json_doc)["device_code"].as<String>().c_str());
    log_d("Verification url: %s", (*_json_doc)["verification_uri"].as<String>().c_str());
    log_d("Message: %s", (*_json_doc)["message"].as<String>().c_str());

    _device_code = (*_json_doc)["device_code"].as<String>();
    _login_url = (*_json_doc)["verification_uri"].as<String>();
    _login_code = (*_json_doc)["user_code"].as<String>();

    return ESP_OK;
}

esp_err_t Azure::getAuthorizationStatus()
{
    HTTPClient http;
    String url = "https://login.microsoftonline.com/common/oauth2/v2.0/token";

    // http.begin(url);
    // http.addHeader("Content-Type", "application/x-www-form-urlencoded");

    String data = "grant_type=urn\%3Aietf\%3Aparams\%3Aoauth\%3Agrant-type\%3Adevice_code"
                  "&device_code=" +
                  _device_code +
                  "&client_id=" + _client_id;

    http.begin(url);
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");

    int httpResponseCode = http.POST(data);
    // if(httpResponseCode != 200)
    // {
    //     log_e("HTTP Request failed. code = %d", httpResponseCode);
            _err_info = "HTTP request failed. code = " + String(httpResponseCode);
    //     return AUTH_STATUS_FAILED;
    // }

    String result = http.getString();
    // http.end();
    DeserializationError err = deserializeJson((*_json_doc), result);
    if (err)
    {
        if (err.code() == DeserializationError::IncompleteInput)
        {
            return AUTH_STATUS_PENDING;
        }
        else
        {
            log_e("DeserializeJson failed: [%d] %s", err.code(), err.c_str());
            _err_info = "DeserializeJson failed: " + String(err.c_str());
            return AUTH_STATUS_FAILED;
        }
    }
    log_d("json doc usage: %d/%d", (*_json_doc).memoryUsage(), (*_json_doc).capacity());

    if ((*_json_doc).containsKey("error"))
    {
        String err = (*_json_doc)["error"].as<String>();
        if (err.indexOf("authorization_pending") >= 0)
        {
            return AUTH_STATUS_PENDING;
        }

        if (err.indexOf("authorization_declined") >= 0)
        {
            _err_info = "User declined";
            log_e("User declined.");
            return AUTH_STATUS_DECLINED;
        }
        else if (err.indexOf("expired_token") >= 0)
        {
            _err_info = "Timeout";
            log_e("Timeout.");
            return AUTH_STATUS_TIMEOUT;
        }
        else
        {
            _err_info = "Other errors, please check the log";
            log_e("Error occurred. Description: %s", (*_json_doc)["error_description"].as<String>().c_str());
            return AUTH_STATUS_FAILED;
        }
    }

    log_d("expires_in: %s", (*_json_doc)["expires_in"].as<String>().c_str());
    log_d("access_token: %s", (*_json_doc)["access_token"].as<String>().c_str());
    log_d("refresh_token: %s", (*_json_doc)["refresh_token"].as<String>().c_str());
    log_d("id_token: %s", (*_json_doc)["id_token"].as<String>().c_str());

    _refresh_token = (*_json_doc)["refresh_token"].as<String>();
    _access_token = (*_json_doc)["access_token"].as<String>();
    _expires_in = (*_json_doc)["expires_in"].as<int>();
    _id_token = (*_json_doc)["id_token"].as<String>();

    _is_login = true;
    saveData();

    return AUTH_STATUS_OK;
}

esp_err_t Azure::RefreshToken()
{
    HTTPClient http;
    String url = "https://login.microsoftonline.com/common/oauth2/v2.0/token";

    http.begin(url);
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");

    String data = "client_id=" + _client_id +
                  "&scope=openid offline_access tasks.readwrite"
                  "&grant_type=refresh_token"
                  "&refresh_token=" +
                  _refresh_token;
    int httpResponseCode = http.POST(data);

    if (httpResponseCode != 200)
    {
        log_e("HTTP Request failed. code = %d. payload = %s", httpResponseCode, http.getString());
        _err_info = "HTTP request failed. code = " + String(httpResponseCode);
        return ESP_FAIL;
    }

    String result = http.getString();
    http.end();

    DeserializationError err = deserializeJson((*_json_doc), result);
    if (err)
    {
        log_e("DeserializeJson failed: %s", err.c_str());
        _err_info = "DeserializeJson failed: " + String(err.c_str());
        return ESP_FAIL;
    }
    log_d("json doc usage: %d/%d", (*_json_doc).memoryUsage(), (*_json_doc).capacity());

    if ((*_json_doc).containsKey("error"))
    {
        log_e("Error occurred. Description: %s", (*_json_doc)["error_description"].as<String>().c_str());
        _err_info = "Error occurred. Description: " + (*_json_doc)["error_description"].as<String>();
        return ESP_FAIL;
    }

    log_d("expires_in: %s", (*_json_doc)["expires_in"].as<String>().c_str());
    log_d("access_token: %s", (*_json_doc)["access_token"].as<String>().c_str());
    log_d("refresh_token: %s", (*_json_doc)["refresh_token"].as<String>().c_str());
    log_d("id_token: %s", (*_json_doc)["id_token"].as<String>().c_str());

    _refresh_token = (*_json_doc)["refresh_token"].as<String>();
    _access_token = (*_json_doc)["access_token"].as<String>();
    _expires_in = (*_json_doc)["expires_in"].as<int>();
    _id_token = (*_json_doc)["id_token"].as<String>();

    ESP_CHECK(saveData());

    _token_update_time = millis();
    _next_update_time = _token_update_time + (_expires_in - 300) * 1000;

    return ESP_OK;
}

bool Azure::isNeedRefreshToken()
{
    if(millis() > _next_update_time)
    {
        return true;
    }
    return false;
}

// Get all the todoTaskList in the user's mailbox.
esp_err_t Azure::getAllTodoTaskList()
{
    HTTPClient http;
    String url = "https://graph.microsoft.com/beta/me/todo/lists";

    http.begin(url);
    http.addHeader("Authorization", _access_token);

    int httpResponseCode = http.GET();

    if (httpResponseCode != 200)
    {
        log_e("HTTP Request failed. code = %d. payload = %s", httpResponseCode, http.getString());
        _err_info = "HTTP request failed. code = " + String(httpResponseCode);
        return ESP_FAIL;
    }

    String payload = http.getString();
    http.end();

    DynamicJsonDocument filter(512);
    filter["value"][0]["displayName"] = true;
    filter["value"][0]["id"] = true;
    log_d("filter mem usage: %d/%d\n", filter.memoryUsage(), filter.capacity());
    DeserializationError err = deserializeJson((*_json_doc), payload, DeserializationOption::Filter(filter));
    if (err)
    {
        log_e("DeserializeJson failed: %s", err.c_str());
        _err_info = "DeserializeJson failed: " + String(err.c_str());
        return ESP_FAIL;
    }
    log_d("json doc usage: %d/%d", (*_json_doc).memoryUsage(), (*_json_doc).capacity());

    if ((*_json_doc).containsKey("error"))
    {
        log_e("Error occurred. Description: %s", (*_json_doc)["error_description"].as<String>().c_str());
        _err_info = "Error occurred. Description: " + (*_json_doc)["error_description"].as<String>();
        return ESP_FAIL;
    }

    JsonArray array = (*_json_doc)["value"].as<JsonArray>();
    
    for (int i = 0; i < array.size(); i++)
    {
        azure_tasklist_t list;
        list.display_name = array[i]["displayName"].as<String>();
        list.id = array[i]["id"].as<String>();
        tasklistmap.insert(std::pair<String, azure_tasklist_t>(list.id, list));
    }

    return ESP_OK;
}

esp_err_t Azure::getListTask(azure_tasklist_t *list)
{
    HTTPClient http;
    String url = "https://graph.microsoft.com/beta/me/todo/lists/" + list->id + "/tasks";

    http.begin(url);
    http.addHeader("Authorization", _access_token);

    int httpResponseCode = http.GET();

    if (httpResponseCode != 200)
    {
        log_e("HTTP Request failed. code = %d. payload = %s", httpResponseCode, http.getString());
        _err_info = "HTTP request failed. code = " + String(httpResponseCode);
        return ESP_FAIL;
    }

    String payload = http.getString();
    http.end();
    DynamicJsonDocument filter(512);
    filter["value"][0]["importance"] = true;
    filter["value"][0]["status"] = true;
    filter["value"][0]["title"] = true;
    filter["value"][0]["createdDateTime"] = true;
    filter["value"][0]["dueDateTime"] = true;
    filter["value"][0]["id"] = true;
    log_d("filter mem usage: %d/%d\n", filter.memoryUsage(), filter.capacity());
    DeserializationError err = deserializeJson((*_json_doc), payload, DeserializationOption::Filter(filter));
    if (err)
    {
        log_e("DeserializeJson failed: %s", err.c_str());
        _err_info = "DeserializeJson failed: " + String(err.c_str());
        return ESP_FAIL;
    }
    log_d("json doc usage: %d/%d", (*_json_doc).memoryUsage(), (*_json_doc).capacity());

    if ((*_json_doc).containsKey("error"))
    {
        log_e("Error occurred. Description: %s", (*_json_doc)["error_description"].as<String>().c_str());
        _err_info = "Error occurred. Description: " + (*_json_doc)["error_description"].as<String>();
        return ESP_FAIL;
    }

    JsonArray array = (*_json_doc)["value"].as<JsonArray>();
    list->taskmap.clear();
    for (int i = 0; i < array.size(); i++)
    {
        if(array[i]["status"].as<String>() == "completed")
        {
            continue;
        }
        azure_task_t task;
        String importance = array[i]["importance"].as<String>();
        if(importance == "high")
        {
            task.importance = TASK_IMPORTANCE_HIGH;
        }
        else
        {
            task.importance = TASK_IMPORTANCE_NORMAL;
        }

        task.status = array[i]["status"].as<String>();
        task.title = array[i]["title"].as<String>();

        if(array[i].containsKey("createdDateTime"))
        {
            convertTime(array[i]["createdDateTime"].as<String>(), &task.createdDateTime, &task.createdStamp);
        }
        else
        {
            task.createdStamp = 0;
        }
        if(array[i].containsKey("dueDateTime"))
        {
            convertTime(array[i]["dueDateTime"]["dateTime"].as<String>(), &task.dueDateTime, &task.dueStamp);
        }
        else
        {
            task.dueStamp = 0;
        }
        task.id = array[i]["id"].as<String>();
        list->taskmap.insert(std::pair<String, azure_task_t>(task.id, task));
    }

    return ESP_OK;
}

void Azure::convertTime(String datetime, struct tm *target_time, time_t *target_stamp)
{
    // "2020-11-04T08:48:00.0940839Z"
    struct tm time;
    std::vector<String> buf;
    String temp;

    temp = datetime.substring(0, 10);
    buf = split(temp, "-");
    time.tm_year = buf[0].toInt() - 1900;
    time.tm_mon = buf[1].toInt() - 1;
    time.tm_mday = buf[2].toInt();
    temp = datetime.substring(11, 19);
    buf = split(temp, ":");
    time.tm_hour = buf[0].toInt();
    time.tm_min = buf[1].toInt();
    time.tm_sec = buf[2].toInt();

    time_t stamp = mktime(&time);
    stamp += _timezone * 3600;
    *target_stamp = stamp;

    time = *localtime(&stamp);
    time.tm_year += 1900;
    time.tm_mon += 1;
    
    *target_time = time;
}

esp_err_t Azure::creatTodoTaskList(String display_name)
{
    HTTPClient http;
    String url = "https://graph.microsoft.com/beta/me/todo/lists";
    String data = "{\"displayName\": \"" + display_name + "\"}";

    http.begin(url);
    http.addHeader("Authorization", _access_token);
    http.addHeader("Content-Type", "application/json");
    int httpResponseCode = http.POST(data);

    if (httpResponseCode != 201)
    {
        log_e("HTTP Request failed. code = %d. payload = %s", httpResponseCode, http.getString());
        _err_info = "HTTP request failed. code = " + String(httpResponseCode);
        return ESP_FAIL;
    }

    String payload = http.getString();
    http.end();

    DeserializationError err = deserializeJson((*_json_doc), payload);
    if (err)
    {
        log_e("DeserializeJson failed: %s", err.c_str());
        _err_info = "DeserializeJson failed: " + String(err.c_str());
        return ESP_FAIL;
    }
    log_d("json doc usage: %d/%d", (*_json_doc).memoryUsage(), (*_json_doc).capacity());

    if ((*_json_doc).containsKey("error"))
    {
        log_e("Error occurred. Description: %s", (*_json_doc)["error_description"].as<String>().c_str());
        _err_info = "Error occurred. Description: " + (*_json_doc)["error_description"].as<String>();
        return ESP_FAIL;
    }

    return ESP_OK;
}

esp_err_t Azure::updateAllTask()
{
    ESP_CHECK(clearAllTask());
    ESP_CHECK(getAllTodoTaskList());
    log_d("num of list = %d", tasklistmap.size());
    for(auto iter = tasklistmap.begin(); iter != tasklistmap.end(); iter++)
    {
        ESP_CHECK(getListTask(&(iter->second)));
    }

    return ESP_OK;
}

esp_err_t Azure::clearAllTask()
{
    for(auto iter = tasklistmap.begin(); iter != tasklistmap.end(); iter++)
    {
        iter->second.taskmap.clear();
    }
    tasklistmap.clear();

    return ESP_OK;
}

esp_err_t Azure::syncTime(int8_t timezone)
{
    const char *ntpServer = "time.cloudflare.com";
    configTime(timezone * 3600, 0, ntpServer);

    if (getLocalTime(&_time_info))
    {
        time(&_time_stamp);
        _time_info.tm_year += 1900;
        _time_info.tm_mon += 1;
        log_d("time stamp = %d", _time_stamp);
        return ESP_OK;
    }
    log_e("Time sync failed");
    return ESP_FAIL;
}

esp_err_t Azure::saveData()
{
    log_d("saving data");
    nvs_handle nvs_arg;
    NVS_CHECK(nvs_open("azure data", NVS_READWRITE, &nvs_arg));
    time(&_time_stamp);
    NVS_CHECK(nvs_set_i64(nvs_arg, "expire time", _time_stamp + _expires_in - 60));
    NVS_CHECK(nvs_set_str(nvs_arg, "refresh token", _refresh_token.c_str()));
    // NVS_CHECK(nvs_set_str(nvs_arg, "ssid", global_wifi_ssid.c_str()));
    // NVS_CHECK(nvs_set_str(nvs_arg, "pswd", global_wifi_password.c_str()));
    NVS_CHECK(nvs_commit(nvs_arg));
    nvs_close(nvs_arg);
    log_d("done");
    return ESP_OK;
}

esp_err_t Azure::loadData()
{
    char databuf[2048];
    int64_t expire_time = 0;

    size_t length = 1024;
    nvs_handle nvs_arg;
    NVS_CHECK(nvs_open("azure data", NVS_READONLY, &nvs_arg));
    NVS_CHECK(nvs_get_str(nvs_arg, "refresh token", databuf, &length));
    _refresh_token = String(databuf);

    NVS_CHECK(nvs_get_i64(nvs_arg, "expire time", &expire_time));
    time(&_time_stamp);
    if (_time_stamp < expire_time)
    {
        _is_login = true;
    }
    else
    {
        log_d("exceed expire time");
    }

    // size_t length = 128;
    // char buf[128];
    // NVS_CHECK(nvs_get_str(nvs_arg, "ssid", buf, &length));
    // global_wifi_ssid = String(buf);
    // length = 128;
    // NVS_CHECK(nvs_get_str(nvs_arg, "pswd", buf, &length));
    // global_wifi_password = String(buf);
    nvs_close(nvs_arg);
    return ESP_OK;
}

Azure azure(AZURE_CLIENTID);
