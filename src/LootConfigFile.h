#pragma once
#include <Windows.h>
#include <atomic>
#include <filesystem>
#include <string_view>
namespace PL {
enum class DefaultConfigStatus { Existing,Created,Failed };
struct DefaultConfigResult { DefaultConfigStatus status;DWORD error=0; };
// Publish a complete default atomically. Never replace an existing custom INI,
// including an intentionally empty, malformed or read-only one.
inline DefaultConfigResult ensureDefaultConfig(const std::filesystem::path& path,std::string_view text){
    const auto attributes=GetFileAttributesW(path.c_str());
    if(attributes!=INVALID_FILE_ATTRIBUTES)return {(attributes&FILE_ATTRIBUTE_DIRECTORY)?DefaultConfigStatus::Failed:DefaultConfigStatus::Existing,(attributes&FILE_ATTRIBUTE_DIRECTORY)?ERROR_DIRECTORY:0u};
    if(text.size()>1024*1024)return {DefaultConfigStatus::Failed,ERROR_FILE_TOO_LARGE};
    std::error_code ec;std::filesystem::create_directories(path.parent_path(),ec);
    if(ec)return {DefaultConfigStatus::Failed,static_cast<DWORD>(ec.value())};
    static std::atomic<unsigned> serial=0;
    std::filesystem::path temp;HANDLE file=INVALID_HANDLE_VALUE;
    for(unsigned i=0;i<16&&file==INVALID_HANDLE_VALUE;++i){
        temp=path;temp+=L".default-"+std::to_wstring(GetCurrentProcessId())+L"-"+std::to_wstring(++serial)+L".tmp";
        file=CreateFileW(temp.c_str(),GENERIC_WRITE,FILE_SHARE_READ,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);
        if(file==INVALID_HANDLE_VALUE&&GetLastError()!=ERROR_FILE_EXISTS&&GetLastError()!=ERROR_ALREADY_EXISTS)return {DefaultConfigStatus::Failed,GetLastError()};
    }
    if(file==INVALID_HANDLE_VALUE)return {DefaultConfigStatus::Failed,ERROR_FILE_EXISTS};
    DWORD error=0,written=0;
    if(!WriteFile(file,text.data(),static_cast<DWORD>(text.size()),&written,nullptr))error=GetLastError();
    else if(written!=text.size())error=ERROR_WRITE_FAULT;
    if(!error&&!FlushFileBuffers(file))error=GetLastError();
    CloseHandle(file);
    if(error){DeleteFileW(temp.c_str());return {DefaultConfigStatus::Failed,error};}
    if(MoveFileExW(temp.c_str(),path.c_str(),MOVEFILE_WRITE_THROUGH))return {DefaultConfigStatus::Created};
    error=GetLastError();DeleteFileW(temp.c_str());
    if(error==ERROR_ALREADY_EXISTS||error==ERROR_FILE_EXISTS)return {DefaultConfigStatus::Existing};
    return {DefaultConfigStatus::Failed,error};
}
}
