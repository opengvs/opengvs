#ifndef EASYLOGGINGHELPER_H
#define EASYLOGGINGHELPER_H

#include "easylogging++.h"
#include <io.h>
#include <thread>
#include <shellapi.h>
#include <Windows.h>
#include <io.h>
#include <direct.h>

//INITIALIZE_EASYLOGGINGPP

namespace el
{

    static inline int LogCleanDays = 30;
    static inline std::string LogRootPath = "D:/EasyLog";
   // static el::base::SubsecondPrecision LogSsPrec(3);
    static inline el::base::MillisecondsWidth LogSsPrec(3);
    static inline std::string LoggerToday = el::base::utils::DateTime::getDateTime("%Y%M%d", &LogSsPrec);

    //删除文件路径下n天前的日志文件，由于删除日志文件导致的空文件夹会在下一次删除
    //isRoot为true时，只会清理空的子文件夹
    inline void DeleteOldFiles(std::string path, int oldDays, bool isRoot)
    {
        // 基于当前系统的当前日期/时间
        time_t nowTime = time(0);
        //文件句柄
        intptr_t hFile = 0;
        //文件信息
        struct _finddata_t fileinfo;
        //文件扩展名
        std::string extName = ".log";
        std::string str;
        //是否是空文件夹
        bool isEmptyFolder = true;
        if ((hFile = _findfirst(str.assign(path).append("/*").c_str(), &fileinfo)) != -1)
        {
            do
            {
                //如果是目录,迭代之
                //如果不是,检查文件
                if ((fileinfo.attrib & _A_SUBDIR))
                {
                    if (strcmp(fileinfo.name, ".") != 0 && strcmp(fileinfo.name, "..") != 0)
                    {
                        isEmptyFolder = false;
                        DeleteOldFiles(str.assign(path).append("/").append(fileinfo.name), oldDays, false);
                    }
                }
                else
                {
                    isEmptyFolder = false;
                    str.assign(fileinfo.name);
                    if ((str.size() > extName.size()) && (str.substr(str.size() - extName.size()) == extName))
                    {
                        //是日志文件
                        if ((nowTime - fileinfo.time_write) / (24 * 3600) > oldDays)
                        {
                            str.assign(path).append("/").append(fileinfo.name);
                                system(("attrib -H -R  " + str).c_str());
                            system(("del/q " + str).c_str());
                        }

                    }
                }
            } while (_findnext(hFile, &fileinfo) == 0);
            _findclose(hFile);

            if (isEmptyFolder && (!isRoot))
            {
                system(("attrib -H -R  " + path).c_str());
                system(("rd/q " + path).c_str());
            }
        }
    }
    inline bool isFileExists_access(const std::string& name)
    {
        return (access(name.c_str(), 0) == 0);
    }
    inline void ConfigureLogger(const std::string& fileName)
    {
        if (isFileExists_access(fileName) == true)
        {
            el::Configurations conf(fileName);
            el::Loggers::reconfigureAllLoggers(conf);
        }
    }
    // ************************************************************************
/// 描  述	:  检测文件夹是否存在，如果不存在就创建
//  返回值	:  void --  { 无 }
//  参  数	:  [in] std::string folder  --  { 需要创建的路径 }
// ************************************************************************
    inline void MkDir(std::string folder)
    {
        if (0 == ::_access(folder.c_str(), 0)) //判断路径是否存在，如果存在不需要再创建了
        {
            return;
        }
        std::replace(folder.begin(), folder.end(), '/', '\\');	 //将路径中的 正斜杠 统一替换成 反斜杠
        std::string folder_builder; //子文件夹路径，包含上一级的路径
        std::string sub;		//要检测的子文件夹名字
        sub.reserve(folder.size());
        for (auto it = folder.begin(); it != folder.end(); ++it) //遍历路径
        {
            const char c = *it;
            sub.push_back(c);
            if (c == '\\' || it == folder.end() - 1) //如果遇到反斜杠 或者 结尾了，就可以判断是否存在了，不存在时要创建
            {
                folder_builder.append(sub);//上一级路径 + 现在的文件夹名称
                if (0 != ::_access(folder_builder.c_str(), 0)) //检查现在的文件夹是否存在
                {
                    if (0 != ::_mkdir(folder_builder.c_str())) //不存在时需要创建
                    {
                        return;//创建失败
                    }
                    else
                    {
                        //创建文件夹成功了
                    }
                }
                sub.clear();//清空文件夹名称，然后才能存下一级的文件夹名称
            }
        }
    }
    static inline void ConfigureLogger()
    {
        std::string datetimeY = el::base::utils::DateTime::getDateTime("%Y", &LogSsPrec);
        std::string datetimeYM = el::base::utils::DateTime::getDateTime("%Y%M", &LogSsPrec);
        std::string datetimeYMd = el::base::utils::DateTime::getDateTime("%Y%M%d", &LogSsPrec);

        std::string filePath = LogRootPath + "/" + datetimeY + "/" + datetimeYM + "/";
        MkDir(filePath);
        std::string filename;

        el::Configurations defaultConf;
        defaultConf.setToDefault();
        //建议使用setGlobally
        defaultConf.setGlobally(el::ConfigurationType::Format, "%datetime %msg");
        defaultConf.setGlobally(el::ConfigurationType::Enabled, "true");
        defaultConf.setGlobally(el::ConfigurationType::ToFile, "true");
        defaultConf.setGlobally(el::ConfigurationType::ToStandardOutput, "true");
        //defaultConf.setGlobally(el::ConfigurationType::SubsecondPrecision, "6");
        defaultConf.setGlobally(el::ConfigurationType::MillisecondsWidth, "6");
        defaultConf.setGlobally(el::ConfigurationType::PerformanceTracking, "true");
        defaultConf.setGlobally(el::ConfigurationType::LogFlushThreshold, "1");

        //限制文件大小时配置
        defaultConf.setGlobally(el::ConfigurationType::MaxLogFileSize, "2097152");

        filename = datetimeYMd + "_" + el::LevelHelper::convertToString(el::Level::Global) + ".log";
        defaultConf.set(el::Level::Global, el::ConfigurationType::Filename, filePath + filename);

        filename = datetimeYMd + "_" + el::LevelHelper::convertToString(el::Level::Debug) + ".log";
        defaultConf.set(el::Level::Debug, el::ConfigurationType::Filename, filePath + filename);

        filename = datetimeYMd + "_" + el::LevelHelper::convertToString(el::Level::Error) + ".log";
        defaultConf.set(el::Level::Error, el::ConfigurationType::Filename, filePath + filename);

        filename = datetimeYMd + "_" + el::LevelHelper::convertToString(el::Level::Fatal) + ".log";
        defaultConf.set(el::Level::Fatal, el::ConfigurationType::Filename, filePath + filename);

        filename = datetimeYMd + "_" + el::LevelHelper::convertToString(el::Level::Info) + ".log";
        defaultConf.set(el::Level::Info, el::ConfigurationType::Filename, filePath + filename);

        filename = datetimeYMd + "_" + el::LevelHelper::convertToString(el::Level::Trace) + ".log";
        defaultConf.set(el::Level::Trace, el::ConfigurationType::Filename, filePath + filename);

        filename = datetimeYMd + "_" + el::LevelHelper::convertToString(el::Level::Warning) + ".log";
        defaultConf.set(el::Level::Warning, el::ConfigurationType::Filename, filePath + filename);

        el::Loggers::reconfigureLogger("default", defaultConf);

        //限制文件大小时启用
        el::Loggers::addFlag(el::LoggingFlag::StrictLogFileSizeCheck);
    }

    class LogDispatcher : public el::LogDispatchCallback
    {
    protected:
        void handle(const el::LogDispatchData* data) noexcept override {
            m_data = data;
            // 使用记录器的默认日志生成器进行调度
            dispatch(m_data->logMessage()->logger()->logBuilder()->build(m_data->logMessage(),
                m_data->dispatchAction() == el::base::DispatchAction::NormalLog));

            //此处也可以写入数据库
        }
    private:
        const el::LogDispatchData* m_data;
        void dispatch(el::base::type::string_t&& logLine) noexcept
        {
           // el::base::SubsecondPrecision ssPrec(3);
            el::base::MillisecondsWidth     ssPrec(3);
            static std::string now = el::base::utils::DateTime::getDateTime("%Y%M%d", &ssPrec);
            if (now != LoggerToday)
            {
                LoggerToday = now;
                ConfigureLogger();
                std::thread task(el::DeleteOldFiles, LogRootPath, LogCleanDays, true);
            }
        }
    };
    static inline      void  SetEasyLoggingSafeThread()  //该函数仅仅限制在winmain()函数中使用，故不包含在头文件中
    {
          int argc = 0;

          LPWSTR* lpszArgv = NULL;
          LPWSTR szCmdLine = (LPWSTR)::GetCommandLineW(); //获取命令行参数；
          lpszArgv = ::CommandLineToArgvW((LPWSTR)szCmdLine, &argc); //拆分命令行参数字符串；
          std::wstring  wstr = *lpszArgv;
          std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
          std::string str = converter.to_bytes(wstr);
          START_EASYLOGGINGPP(argc, (char**)str.c_str());
    }

    static inline void InitEasylogging()
    {
        ConfigureLogger();

        el::Helpers::installLogDispatchCallback<LogDispatcher>("LogDispatcher");
        LogDispatcher* dispatcher = el::Helpers::logDispatchCallback<LogDispatcher>("LogDispatcher");
        dispatcher->setEnabled(true);
    }


}

#endif 
