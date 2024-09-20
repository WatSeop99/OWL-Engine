#include <Windows.h>
#include <stringapiset.h>
#include <locale>
#include <string>
#include "Util.h"

std::wstring UTF8ToWideString(const std::string& str)
{
    wchar_t wstr[MAX_PATH];
    if (!MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, str.c_str(), -1, wstr, MAX_PATH))
    {
        wstr[0] = L'\0';
    }
    return wstr;
}

std::string WideStringToUTF8(const std::wstring& wstr)
{
    char str[MAX_PATH];
    if (!WideCharToMultiByte(CP_ACP, MB_PRECOMPOSED, wstr.c_str(), -1, str, MAX_PATH, nullptr, nullptr))
    {
        str[0] = L'\0';
    }
    return str;
}

std::string ToLower(const std::string& str)
{
    std::string lower_case = str;
    std::locale loc;
    for (char& s : lower_case)
    {
        s = std::tolower(s, loc);
    }
    return lower_case;
}

std::wstring ToLower(const std::wstring& str)
{
    std::wstring lower_case = str;
    std::locale loc;
    for (wchar_t& s : lower_case)
    {
        s = std::tolower(s, loc);
    }
    return lower_case;
}

std::string GetBasePath(const std::string& filePath)
{
    size_t lastSlash;
    if ((lastSlash = filePath.rfind('/')) != std::string::npos)
    {
        return filePath.substr(0, lastSlash + 1);
    }
    else if ((lastSlash = filePath.rfind('\\')) != std::string::npos)
    {
        return filePath.substr(0, lastSlash + 1);
    }
    else
    {
        return "";
    }
}

std::wstring GetBasePath(const std::wstring& filePath)
{
    size_t lastSlash;
    if ((lastSlash = filePath.rfind(L'/')) != std::wstring::npos)
    {
        return filePath.substr(0, lastSlash + 1);
    }
    else if ((lastSlash = filePath.rfind(L'\\')) != std::wstring::npos)
    {
        return filePath.substr(0, lastSlash + 1);
    }
    else
    {
        return L"";
    }
}

std::string RemoveBasePath(const std::string& filePath)
{
    size_t lastSlash;
    if ((lastSlash = filePath.rfind('/')) != std::string::npos)
    {
        return filePath.substr(lastSlash + 1, std::string::npos);
    }
    else if ((lastSlash = filePath.rfind('\\')) != std::string::npos)
    {
        return filePath.substr(lastSlash + 1, std::string::npos);
    }
    else
    {
        return filePath;
    }
}

std::wstring RemoveBasePath(const std::wstring& filePath)
{
    size_t lastSlash;
    if ((lastSlash = filePath.rfind(L'/')) != std::string::npos)
    {
        return filePath.substr(lastSlash + 1, std::string::npos);
    }
    else if ((lastSlash = filePath.rfind(L'\\')) != std::string::npos)
    {
        return filePath.substr(lastSlash + 1, std::string::npos);
    }
    else
    {
        return filePath;
    }
}

std::string GetFileExtension(const std::string& filePath)
{
    std::string fileName = RemoveBasePath(filePath);
    size_t extOffset = fileName.rfind('.');
    if (extOffset == std::wstring::npos)
    {
        return "";
    }

    return fileName.substr(extOffset + 1);
}

std::wstring GetFileExtension(const std::wstring& filePath)
{
    std::wstring fileName = RemoveBasePath(filePath);
    size_t extOffset = fileName.rfind(L'.');
    if (extOffset == std::wstring::npos)
    {
        return L"";
    }

    return fileName.substr(extOffset + 1);
}

std::string RemoveExtension(const std::string& filePath)
{
    return filePath.substr(0, filePath.rfind("."));
}

std::wstring RemoveExtension(const std::wstring& filePath)
{
    return filePath.substr(0, filePath.rfind(L"."));
}

int Min(const int X, const int Y)
{
    return (X < Y ? X : Y);
}

float Min(const float X, const float Y)
{
    return (X < Y ? X : Y);
}

Vector3 Min(const Vector3& V1, const Vector3& V2)
{
    DirectX::XMVECTOR v1 = DirectX::XMLoadFloat3(&V1);
    DirectX::XMVECTOR v2 = DirectX::XMLoadFloat3(&V2);
    DirectX::XMVECTOR ret = DirectX::XMVectorMin(v1, v2);

    return Vector3(ret);
}

int Max(const int X, const int Y)
{
    return (X > Y ? X : Y);
}

float Max(const float X, const float Y)
{
    return (X > Y ? X : Y);
}

Vector3 Max(const Vector3& V1, const Vector3& V2)
{
    DirectX::XMVECTOR v1 = DirectX::XMLoadFloat3(&V1);
    DirectX::XMVECTOR v2 = DirectX::XMLoadFloat3(&V2);
    DirectX::XMVECTOR ret = DirectX::XMVectorMax(v1, v2);

    return Vector3(ret);
}

float Clamp(const float VAL, const float LOWER, const float UPPER)
{
    return Min(Max(VAL, LOWER), UPPER);
}

float Lerp(const float A, const float B, const float F)
{
    return A + F * (B - A);
}

float DegreeToRadian(const float ANGLE)
{
    return ANGLE / 180.0f * DirectX::XM_PI;
}
