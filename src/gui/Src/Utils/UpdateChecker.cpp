#include "UpdateChecker.h"
#include <QMessageBox>
#include <QIcon>
#include <QDateTime>

#include "StringUtil.h"
#include "MiscUtil.h"

#ifdef _WIN32
#include <windows.h>
#include <Wininet.h>
#include <functional>

class Cleanup
{
    std::function<void()> fn;
public:
    explicit Cleanup(std::function<void()> fn) : fn(std::move(fn)) { }
    ~Cleanup() { fn(); }
};

static std::string httpGet(const char* url,
                           const char* useragent,
                           unsigned int timeout)
{
    HINTERNET hInternet = nullptr;
    HINTERNET hUrl = nullptr;
    DWORD dwBufferSize = 0;
    const char* headers = nullptr;
    DWORD dwHeaderslen = 0;
    const DWORD chunk_size = 8192;
    std::string result;

    Cleanup cleanup([&]()
    {
        DWORD dwLastError = GetLastError();
        if(hUrl != NULL)
            InternetCloseHandle(hUrl);
        if(hInternet != NULL)
            InternetCloseHandle(hInternet);
    });

    hInternet = InternetOpenA(useragent,
                              INTERNET_OPEN_TYPE_PRECONFIG,
                              NULL,
                              NULL,
                              0);

    if(!hInternet)
    {
        return {};
    }

    // Set a time-out value in milliseconds
    InternetSetOptionA(hInternet,
                       INTERNET_OPTION_RECEIVE_TIMEOUT,
                       &timeout,
                       sizeof(timeout));

    // Attempt to enable content decoding if the option is supported
    DWORD gzipDecoding = true;
    if(InternetSetOptionA(hInternet,
                          INTERNET_OPTION_HTTP_DECODING,
                          &gzipDecoding,
                          sizeof(gzipDecoding)))
    {
        // Add the required request headers
        headers = "Accept-Encoding: gzip\r\n";
        dwHeaderslen = static_cast<DWORD>(strlen(headers));
    }

    DWORD dwFlags = INTERNET_FLAG_RELOAD;
    if(strncmp(url, "https://", 8) == 0)
        dwFlags |= INTERNET_FLAG_SECURE;

    // Make the HTTP request
    hUrl = InternetOpenUrlA(hInternet, url, headers, dwHeaderslen, dwFlags, 0);
    if(!hUrl)
        return {};

    // Get HTTP status code
    DWORD dwStatusCode;
    dwBufferSize = sizeof(dwStatusCode);
    if(HttpQueryInfoA(hUrl, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER, &dwStatusCode, &dwBufferSize, 0))
    {
        if(dwStatusCode != 200)
        {
            return {};
        }
    }

    // Get HTTP content length
    unsigned long long contentLength = 0;
    dwBufferSize = sizeof(contentLength);
    HttpQueryInfoA(hUrl, HTTP_QUERY_CONTENT_LENGTH | HTTP_QUERY_FLAG_NUMBER, &contentLength, &dwBufferSize, 0);

    // Reserve space in the string if we know the content length
    if(contentLength > 0)
        result.reserve(static_cast<size_t>(contentLength));

    // Receive the data from the HINTERNET handle
    char buffer[chunk_size];
    DWORD dwRead = 0;
    unsigned long long readBytes = 0;
    unsigned long long totalBytes = contentLength;
    while(InternetReadFile(hUrl, buffer, sizeof(buffer), &dwRead))
    {
        readBytes += dwRead;

        // We are done if nothing more to read, so now we can report total size in our final cb call
        if(dwRead == 0)
            totalBytes = readBytes;

        // Exit if nothing more to read
        if(dwRead == 0)
            break;

        // Append buffer to result string
        result.append(buffer, dwRead);
    }

    if(totalBytes > 0 && readBytes != totalBytes)
    {
        return {};
    }

    return result;
}
#else
static std::string httpGet(const char* url,
                           const char* useragent,
                           unsigned int timeout)
{
    return {};
}
#endif // _WIN32

UpdateChecker::UpdateChecker(QWidget* parent)
    : QThread(parent),
      mParent(parent)
{
    connect(this, &UpdateChecker::updateCheckFinished, this, &UpdateChecker::finishedSlot);
}

void UpdateChecker::checkForUpdates()
{
    if(!isRunning())
    {
        GuiAddStatusBarMessage(tr("Checking for updates...\n").toUtf8().constData());
        start();
    }
}

void UpdateChecker::run()
{
    QString userAgent = "x64dbg " + ToDateString(GetCompileDate()) + " " __TIME__;
    std::string result = httpGet("https://update.x64dbg.com/releases.json",
                                 userAgent.toUtf8().constData(), 3000);
    emit updateCheckFinished(QString::fromStdString(result));
}

void UpdateChecker::finishedSlot(const QString & json)
{
    if(json.isEmpty())
    {
        SimpleErrorBox(mParent, tr("Network Error!"), tr("Failed to check for updates"));
        return;
    }

    QRegExp regExp("\"published_at\": ?\"([^\"]+)\"");
    QDateTime serverTime;
    if(regExp.indexIn(json) >= 0)
        serverTime = QDateTime::fromString(regExp.cap(1), Qt::ISODate);
    if(!serverTime.isValid())
    {
        SimpleErrorBox(mParent, tr("Error!"), tr("File on server could not be parsed..."));
        return;
    }
    QRegExp regUrl("\"browser_download_url\": ?\"([^\"]+)\"");
    auto url = regUrl.indexIn(json) >= 0 ? regUrl.cap(1) : "https://releases.x64dbg.com";
    auto server = serverTime.date();
    auto build = GetCompileDate();
    QString info;
    if(server > build)
        info = QString(tr("New build %1 available!<br>Download <a href=\"%2\">here</a><br><br>You are now on build %3")).arg(ToDateString(server)).arg(url).arg(ToDateString(build));
    else if(server < build)
        info = QString(tr("You have a development build (%1) of x64dbg!")).arg(ToDateString(build));
    else
        info = QString(tr("You have the latest build (%1) of x64dbg!")).arg(ToDateString(build));
    GuiAddStatusBarMessage((info + "\n").toUtf8().constData());
    SimpleInfoBox(mParent, tr("Information"), info);
}
