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

    //ɾ���ļ�·����n��ǰ����־�ļ�������ɾ����־�ļ����µĿ��ļ��л�����һ��ɾ��
    //isRootΪtrueʱ��ֻ������յ����ļ���
    inline void DeleteOldFiles(std::string path, int oldDays, bool isRoot)
    {
        // ���ڵ�ǰϵͳ�ĵ�ǰ����/ʱ��
        time_t nowTime = time(0);
        //�ļ����
        intptr_t hFile = 0;
        //�ļ���Ϣ
        struct _finddata_t fileinfo;
        //�ļ���չ��
        std::string extName = ".log";
        std::string str;
        //�Ƿ��ǿ��ļ���
        bool isEmptyFolder = true;
        if ((hFile = _findfirst(str.assign(path).append("/*").c_str(), &fileinfo)) != -1)
        {
            do
            {
                //�����Ŀ¼,����֮
                //�������,����ļ�
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
                        //����־�ļ�
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
/// ��  ��	:  ����ļ����Ƿ���ڣ���������ھʹ���
//  ����ֵ	:  void --  { �� }
//  ��  ��	:  [in] std::string folder  --  { ��Ҫ������·�� }
// ************************************************************************
    inline void MkDir(std::string folder)
    {
        if (0 == ::_access(folder.c_str(), 0)) //�ж�·���Ƿ���ڣ�������ڲ���Ҫ�ٴ�����
        {
            return;
        }
        std::replace(folder.begin(), folder.end(), '/', '\\');	 //��·���е� ��б�� ͳһ�滻�� ��б��
        std::string folder_builder; //���ļ���·����������һ����·��
        std::string sub;		//Ҫ�������ļ�������
        sub.reserve(folder.size());
        for (auto it = folder.begin(); it != folder.end(); ++it) //����·��
        {
            const char c = *it;
            sub.push_back(c);
            if (c == '\\' || it == folder.end() - 1) //���������б�� ���� ��β�ˣ��Ϳ����ж��Ƿ�����ˣ�������ʱҪ����
            {
                folder_builder.append(sub);//��һ��·�� + ���ڵ��ļ�������
                if (0 != ::_access(folder_builder.c_str(), 0)) //������ڵ��ļ����Ƿ����
                {
                    if (0 != ::_mkdir(folder_builder.c_str())) //������ʱ��Ҫ����
                    {
                        return;//����ʧ��
                    }
                    else
                    {
                        //�����ļ��гɹ���
                    }
                }
                sub.clear();//����ļ������ƣ�Ȼ����ܴ���һ�����ļ�������
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
        //����ʹ��setGlobally
        defaultConf.setGlobally(el::ConfigurationType::Format, "%datetime %msg");
        defaultConf.setGlobally(el::ConfigurationType::Enabled, "true");
        defaultConf.setGlobally(el::ConfigurationType::ToFile, "true");
        defaultConf.setGlobally(el::ConfigurationType::ToStandardOutput, "true");
        //defaultConf.setGlobally(el::ConfigurationType::SubsecondPrecision, "6");
        defaultConf.setGlobally(el::ConfigurationType::MillisecondsWidth, "6");
        defaultConf.setGlobally(el::ConfigurationType::PerformanceTracking, "true");
        defaultConf.setGlobally(el::ConfigurationType::LogFlushThreshold, "1");

        //�����ļ���Сʱ����
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

        //�����ļ���Сʱ����
        el::Loggers::addFlag(el::LoggingFlag::StrictLogFileSizeCheck);
    }

    class LogDispatcher : public el::LogDispatchCallback
    {
    protected:
        void handle(const el::LogDispatchData* data) noexcept override {
            m_data = data;
            // ʹ�ü�¼����Ĭ����־���������е���
            dispatch(m_data->logMessage()->logger()->logBuilder()->build(m_data->logMessage(),
                m_data->dispatchAction() == el::base::DispatchAction::NormalLog));

            //�˴�Ҳ����д�����ݿ�
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
    static inline      void  SetEasyLoggingSafeThread()  //�ú�������������winmain()������ʹ�ã��ʲ�������ͷ�ļ���
    {
          int argc = 0;

          LPWSTR* lpszArgv = NULL;
          LPWSTR szCmdLine = (LPWSTR)::GetCommandLineW(); //��ȡ�����в�����
          lpszArgv = ::CommandLineToArgvW((LPWSTR)szCmdLine, &argc); //��������в����ַ�����
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
