#pragma once

#include <string>
#include <windows.h>
#include <wininet.h>

struct HttpRequestInfo
{
    std::string agent, host, path;
    const char* method = "GET";
    const char** acceptTypes = defaultAcceptTypes;
    bool https = true;

    static PCTSTR defaultAcceptTypes[];
};

class HttpRequest
{
private:
    HINTERNET m_inet, m_conn, m_req;

public:
    HttpRequest(const HttpRequestInfo& info);
    ~HttpRequest();
    void addHeaders(const char* headers);
    void begin(size_t totalBodySize = 0);
    void write(const char* data, size_t len);
    void end();

private:
    void close();
};
