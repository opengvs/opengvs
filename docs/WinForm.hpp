///////////////////////////////////////////////////////////////////////////////
//  WinForm.hpp  ���ڹ���Windows API������ʽ��Ӧ�ó�������
//  ��ͷ�ļ���һ��������ͷ�ļ���ʵ���˵���ͷ�ļ�����API��ʽӦ�ó������
//  CreateWindowFromCode        ���ڹ�����API������ʽ����
//  CreateWindowFromReSource    ���ڹ�����Դ�ļ���ʽ�Ĵ���
//  FormRun                     ����window�ϴ����¼��ַ������������˸��죬�����û�ʵ��Idle�¼�
//  ���η�װ���Controls.hpp ͷ�ļ�������ɻ����Ļ���API������ʽWindows���ڿ��������ݽ���
//  ע�⣺���η�װδ�����¼������ɡ������û����Բ����麯����ʽ����ָ����ʽ��ɻ����¼�����ļ���          
//  ��Ȩ����        ������    2023.11 �ȷ�      
///////////////////////////////////////////////////////////////////////////////

#ifndef  WINFORM_HPP
#define  WINFORM_HPP

#include "targetver.h"
#define WIN32_LEAN_AND_MEAN             // �� Windows ͷ�ļ����ų�����ʹ�õ�����
// Windows ͷ�ļ�
#include <windows.h>
#include <Mmsystem.h>
// C ����ʱͷ�ļ�
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include "resource.h"
#include "Controls.hpp"
#include <string>
//#include "StringToNumber.hpp"

//#include <winnt.h>
#pragma comment( lib,"winmm.lib" )


// ȫ�ֱ���:
#ifndef MAX_LOADSTRING

#define MAX_LOADSTRING 100
extern HINSTANCE hInst;                                // ��ǰʵ��
extern WCHAR szTitle[MAX_LOADSTRING];                  // �������ı�
extern WCHAR szWindowClass[MAX_LOADSTRING];            // ����������

#else

#endif

namespace Win
{

    typedef  BOOL(APP_OnIdle)(LONG lCount);
 
    class WinForm
    {
    protected:
        //static HINSTANCE m_hInstance;
       

        HWND     m_hWnd = NULL;
        int     m_InsideUseRecource=0;
    public:
        static HINSTANCE get_hInstance(HINSTANCE hInstancs = 0);
        static WinForm* getWinForm(WinForm* pForm = NULL);
    public:

        APP_OnIdle* m_pOnIdle = NULL;
      public:
        WinForm();// { g_pWInform = this; };
        virtual ~WinForm() {};

        HWND CreateWindowFromCode(HINSTANCE hInstance, 
            WCHAR* pszTitle,DWORD IDS_App_Title, 
            WCHAR* pszWindowClass, 
            DWORD IDC_SourceDefineWinForm,
            DWORD IDI_hIconID, WNDPROC     lpfnWndProc = NULL)
        {
            LoadStringW(hInstance, IDS_App_Title, pszTitle, MAX_LOADSTRING);
            LoadStringW(hInstance, IDC_SourceDefineWinForm, (LPWSTR)pszWindowClass, MAX_LOADSTRING);
            hInst = hInstance;
            // TODO: �ڴ˴����ô��롣
            WNDCLASSEXW wcex;

            wcex.cbSize = sizeof(WNDCLASSEX);

            wcex.style = CS_HREDRAW | CS_VREDRAW;
            if (lpfnWndProc == NULL)
                wcex.lpfnWndProc = this->WindowProc;
            else
                wcex.lpfnWndProc = lpfnWndProc;

            wcex.cbClsExtra = 0;
            wcex.cbWndExtra = 0;
            wcex.hInstance = hInstance;
            wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_hIconID));
            wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
            wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
            wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_SourceDefineWinForm);
            wcex.lpszClassName = szWindowClass;
            wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

            RegisterClassExW(&wcex);

            m_hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
                CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

            if (!m_hWnd)
            {
                return NULL;
            }
            m_hWnd = m_hWnd;
            //m_hInstance = hInstance;
            get_hInstance(hInstance);
            m_InsideUseRecource = 1;
            ShowWindow(m_hWnd, SW_NORMAL);
            UpdateWindow(m_hWnd);

            return m_hWnd;
        }

        HWND CreateWindowFromReSource(HINSTANCE hInstance, int ResourceIDD, DLGPROC lpDialogFunc = NULL, HWND parent = NULL)
        {
            hInst = hInstance;
            //HWND hWnd = NULL;
            // ֱ�Ӵ���
           // if (lpDialogFunc == NULL)
            //    hWnd = ::CreateDialog(hInstance, MAKEINTRESOURCE(ResourceIDD), parent, DialogWindowProc);
           // else
           //     hWnd = ::CreateDialog(hInstance, MAKEINTRESOURCE(ResourceIDD), parent, lpDialogFunc);
          

          // ��Ӵ���
            HRSRC hRs = FindResource(hInstance, MAKEINTRESOURCE(ResourceIDD), RT_DIALOG); // ����Դ���ص��ڴ�
            HGLOBAL hGl = LoadResource(hInstance, hRs);                                   // �ҵ��Ի������������
            LPCDLGTEMPLATE pTemplate = (LPCDLGTEMPLATE)LockResource(hGl);                  // ���ڴ������ݷ���ṹ��
            if (lpDialogFunc == NULL)
                m_hWnd = CreateDialogIndirect(hInstance, pTemplate, parent, DialogWindowProc);
            else
                m_hWnd = CreateDialogIndirect(hInstance, pTemplate, parent, lpDialogFunc);

            if (!m_hWnd)
            {
                return FALSE;
            }
            else
            {
                m_hWnd = m_hWnd;
                //m_hInstance = hInstance;
                get_hInstance(hInstance);
            }
            m_InsideUseRecource = 2;

            RECT rtDlg;
            GetWindowRect(m_hWnd, &rtDlg);

            int nScreenX = GetSystemMetrics(SM_CXSCREEN);
            int nScreenY = GetSystemMetrics(SM_CYSCREEN);

            SetWindowPos(m_hWnd,
                HWND_TOP,
                nScreenX / 2 - rtDlg.right / 2,
                nScreenY / 2 - rtDlg.bottom / 2,
                0,
                0,
                SWP_NOSIZE | SWP_SHOWWINDOW); //SWP_NOSIZE)

            ShowWindow(m_hWnd, SW_NORMAL);
            UpdateWindow(m_hWnd);



            return m_hWnd;
        }


        int APIENTRY  Run(APP_OnIdle*  pOnIdle=NULL)
        {
           // HACCEL hAccelTable = LoadAccelerators(hInst, MAKEINTRESOURCE(IDC_WINDOWSFORM));

            MSG msg = { 0 };
            //4. ��Ϣѭ��
            while (WM_QUIT != msg.message)
            {
                if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
                {     //����Ϣ�����в�����û����Ϣ �����ȡ û����ֱ�ӷ���  �������� openglһ��ʹ�����ַ�ʽ��else��ʵ�ֻ��ƺ��߼�����
                    TranslateMessage(&msg);			//������Ϣ
                    DispatchMessage(&msg);			//�ַ���Ϣ   ����� wndProc����������ֲ�ͬ����Ϣ
                }
                else
                {

                        DWORD  d = ::timeGetTime();
                        if (pOnIdle != NULL)
                        {
                            m_pOnIdle = pOnIdle;
                            m_pOnIdle(d);
                        }
                        else
                        {
                            OnIdle(d);
                        }

                }
            }

            // ����Ϣѭ��:
          /*  while (GetMessage(&msg, nullptr, 0, 0))
            {
                if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
                {
                    TranslateMessage(&msg);
                    DispatchMessage(&msg);
                }
            }
            */
            return (int)msg.wParam;
        }

         
        static INT_PTR ShowModelDialog(DWORD DialogSource_ID, HWND Parent, DLGPROC lpDialogFunc=nullptr);

        static   LRESULT CALLBACK  WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
        

        virtual LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
        
        static  INT_PTR CALLBACK DialogWindowProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
       
        virtual INT_PTR CALLBACK DialogFormProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
        
        virtual BOOL  OnIdle(LONG lCount)
        {
            
            return TRUE;
        }

        virtual void Init_Event(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {};
        virtual void Command_Event(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {};
        virtual void Notify_Event(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {};
        virtual void Paint_Event(HDC hdc, WPARAM wParam, LPARAM lParam) {  };
        virtual void Resize_Event(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {};
        virtual void Close_Event(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {};
        virtual void Destroy_Event(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {};

        virtual void KeyDown_Event(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {};
        virtual void KeyUp_Event(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {};
        virtual void KeyChar_Event(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {};
        
        virtual void MouseLButtonDown_Event(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {};
        virtual void MouseLButtonUp_Event(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {};
        virtual void MouseLButtonDBClick_Event(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {};

        virtual void MouseMButtonDown_Event(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {};
        virtual void MouseMButtonUp_Event(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {};
        virtual void MouseMButtonDBClick_Event(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {};

        virtual void MouseRButtonDown_Event(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {};
        virtual void MouseRButtonUp_Event(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {};
        virtual void MouseRButtonDBClick_Event(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {};

        virtual void MouseMove_Event(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {};     
        virtual void MouseWheel_Event(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {};

        virtual void ExternEvent_Proc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
        {
            if (m_InsideUseRecource == 1)
            {
                DefWindowProc(hDlg, message, wParam, lParam);
            }
        };
    };

   // class WinForm;
    inline WinForm* WinForm::getWinForm(WinForm* pForm)
    {
        static WinForm* _pInsideWinForm = NULL;
        if (pForm != NULL)
            _pInsideWinForm = pForm;

        return  _pInsideWinForm;
    }
    inline HINSTANCE WinForm::get_hInstance(HINSTANCE hInstancs)
    {
        static HINSTANCE _hInstance = NULL;
        if (hInstancs != NULL)
        {
            _hInstance = hInstancs;
        }
        return _hInstance;
    }
    inline INT_PTR WinForm::ShowModelDialog(DWORD DialogSource_ID, HWND Parent, DLGPROC lpDialogFunc)
    {
        if(lpDialogFunc==nullptr)
            return DialogBox(WinForm::get_hInstance(), MAKEINTRESOURCE(DialogSource_ID), Parent, WinForm::DialogWindowProc);
        return DialogBox(WinForm::get_hInstance(), MAKEINTRESOURCE(DialogSource_ID), Parent, lpDialogFunc);
    }
    /* 2024��1��26��   Ϊ�˼���WINFORM_USER�궨�壬���ò��ִ��붨�嵽��Ա������getWinForm��get_hInstance
    *   �����ڳ�Ա�����ж���Ϊ��̬�ĳ�Ա�����������ͽ����ϵͳ����ȫ�ֱ������ѵ㡣
#ifdef WINFORM_USER

    HINSTANCE WinForm::m_hInstance = NULL;
    WinForm* g_pWInform = NULL;
    inline INT_PTR WinForm::ShowModelDialog(DWORD DialogSource_ID, HWND Parent, DLGPROC lpDialogFunc)
    {
        return DialogBox(WinForm::m_hInstance, MAKEINTRESOURCE(DialogSource_ID), Parent, lpDialogFunc);
    }
#else
    extern  WinForm* g_pWInform;
#endif
*/

    inline WinForm::WinForm()
    {
        getWinForm(this);

       // g_pWInform = this;
    };
    inline LRESULT CALLBACK  WinForm::WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
    {
        
        return getWinForm()->WndProc(hWnd, message, wParam, lParam);
        /*if (g_pWInform == NULL)
            return 0;
        return g_pWInform->WndProc(hWnd, message, wParam, lParam);
        */
    }
    inline INT_PTR CALLBACK WinForm::DialogWindowProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
    {
        return getWinForm()->DialogFormProc(hDlg, message, wParam, lParam);
        /*
        if (g_pWInform == NULL)
            return 0;
        return g_pWInform->DialogFormProc(hDlg, message, wParam, lParam);
        */
    }
    inline LRESULT CALLBACK WinForm::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
    {
        if (this->m_hWnd == NULL)
        {
            this->m_hWnd = hWnd;
        }
        switch (message)
        {
          case  WM_CREATE:
          {
              Init_Event(hWnd, message, wParam, lParam);
          }
              break;
          case WM_COMMAND:
          {
              Command_Event(hWnd, message, wParam, lParam);
              /*
              int wmId = LOWORD(wParam);
              // �����˵�ѡ��:
              switch (wmId)
              {
                  // case IDC_BUTTON1:
                  //     ::MessageBox(hWnd, L"���", L"����", MB_OK);
                  //         break;
                  case IDM_ABOUT:
                    ::MessageBox(hWnd, L"���", L"����", MB_OK);
                  break;
                  case IDM_EXIT:
                    DestroyWindow(hWnd);
                  break;
                  default:
                      return DefWindowProc(hWnd, message, wParam, lParam);
              }
              */
          }
          break;
          case WM_NOTIFY:
          {
              Notify_Event(hWnd, message, wParam, lParam);
              // NMHDR* p = (NMHDR*)(void*)lParam;

              // LONG iID = wParam;
              // if (iID == IDC_SLIDER1)
               //{
               //   HWND hWnd = GetDlgItem(hDlg, IDOK);
               // }
               // else if (wParam == IDC_SPIN1)
               // {
               //     LPNMUPDOWN np;
               //    np = (LPNMUPDOWN)lParam;
               // SetDlgItemInt(hwndDlg, IDC_EDIT, np->iDelta + np->iPos, FALSE);
               //}
          }
          break;
          case WM_KEYDOWN:
          {
              KeyDown_Event(hWnd, message, wParam, lParam);
          }
          break;
          case WM_KEYUP:
          {
              KeyUp_Event(hWnd, message, wParam, lParam);
          }
          break;
          case WM_CHAR:
          {
              KeyChar_Event(hWnd, message, wParam, lParam);
          }
          break;
          case WM_MOUSEMOVE:
          {
              MouseMove_Event(hWnd, message, wParam, lParam);
          }
          break;
          case WM_MOUSEHWHEEL:
          {
              MouseWheel_Event(hWnd, message, wParam, lParam);
          }
          break;
          case WM_LBUTTONDOWN:
          {
              MouseLButtonDown_Event(hWnd, message, wParam, lParam);
          }
          break;
          case WM_LBUTTONUP:
          {
              MouseLButtonUp_Event(hWnd, message, wParam, lParam);
          }
          break;
          case WM_LBUTTONDBLCLK:
          {
              MouseLButtonDBClick_Event(hWnd, message, wParam, lParam);
          }
          break;
          case WM_MBUTTONDOWN:
          {
              MouseMButtonDown_Event(hWnd, message, wParam, lParam);
          }
          break;
          case WM_MBUTTONUP:
          {
              MouseMButtonUp_Event(hWnd, message, wParam, lParam);
          }
          break;
          case WM_MBUTTONDBLCLK:
          {
              MouseMButtonDBClick_Event(hWnd, message, wParam, lParam);
          }
          break;
          case WM_RBUTTONDOWN:
          {
              MouseRButtonDown_Event(hWnd, message, wParam, lParam);
          }
          break;
          case WM_RBUTTONUP:
          {
              MouseRButtonUp_Event(hWnd, message, wParam, lParam);
          }
          break;
          case WM_RBUTTONDBLCLK:
          {
              MouseRButtonDBClick_Event(hWnd, message, wParam, lParam);
          }
          break;

          case WM_PAINT:
          {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            // TODO: �ڴ˴����ʹ�� hdc ���κλ�ͼ����...
                Paint_Event(hdc, wParam, lParam);
            EndPaint(hWnd, &ps);
          }
          break;
          case WM_SIZE:
          {
              Resize_Event(hWnd, message, wParam, lParam);
          }
          break;
          case WM_CLOSE:
          {
              Close_Event(hWnd, message, wParam, lParam);
              ::SendMessageW(hWnd, WM_DESTROY, 0, 0);
          }
          break;
          case WM_DESTROY:
              Destroy_Event(hWnd, message, wParam, lParam);
              PostQuitMessage(0);
            break;
          default:
          {
              ExternEvent_Proc(hWnd, message, wParam, lParam);
          }
            //return DefWindowProc(hWnd, message, wParam, lParam);
        }
        return 0;
    }

    inline INT_PTR CALLBACK WinForm::DialogFormProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
    {
        if (this->m_hWnd == NULL)
        {
            this->m_hWnd = hDlg;
        }
        UNREFERENCED_PARAMETER(lParam);
        switch (message)
        {
            case WM_INITDIALOG:
            {
                Init_Event(hDlg,message,wParam,lParam);
                // Win::Button buton(hDlg, IDOK);
                // buton.disable();
                //  Win::ListBox listbox(hDlg, IDC_LIST1);
                //  listbox.addString(L"aasss");
                //  listbox.addString(L"fdsfdsfds");

                //  Win::ComboBox  combobox(hDlg, IDC_COMBO1);
                //  combobox.addString(L"sdfdsfsdf");
                //  combobox.addString(L"yuyuyuyu");

                //  Trackbar  trackbar(hDlg, IDC_SLIDER1);
                // trackbar.setRange(0, 100);

                return (INT_PTR)TRUE;
            }
            break;
            /* WM_COMMAND��Ϣ��������ť������Button��CheckBox��RadioButton����Щ�ؼ�ͨ��BN_CLICKED��Ϣ�������¼�
            ��WM_COMMAND�У�lParam����������������Ϣ���ǿؼ�֪ͨ��Ϣ�����lParamΪNULL�������Ǹ�������Ϣ��
            ����lParam����ŵı�Ȼ���ǿؼ��ľ������һ���ؼ�֪ͨ��Ϣ������wParam���ǵ�λ�ŵ��ǿؼ�ID����λ�ŵ�����Ӧ����Ϣ�¼�
            */
             case WM_COMMAND:
            {
                 Command_Event(hDlg,message, wParam, lParam);
                 //�ؼ�ID
                 //WORD wControlID = LOWORD(wParam);
                 //��Ϣ��
                 // WORD wCode = HIWORD(wParam);
                 //if (wCode == BN_CLICKED) //���򵯳�����
                 // {
                // �����˵�ѡ��:
                       /*  switch (wControlID)
                       {
                            case IDOK:
                            {
                              ::MessageBox(hDlg, L"���", L"����", MB_OK);
                             //  Win::Button buton(hDlg, IDOK);
                             //  buton.disable();

                             //  Win::EditBox edit1(hDlg, IDC_EDIT1);
                             // wchar_t tempStr[256];
                             // edit1.getText(tempStr,256);
                             //  std::wstring wstr = tempStr;
                             //  unsigned long  A = std::stol(tempStr);//toNumber(wstr);// toNumber(wstr);
                           }
                           break;
                           case IDM_ABOUT:
                             ::MessageBox(hDlg, L"���", L"����", MB_OK);
                             //  DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                             // WinForm::ShowModelDialog(IDD_ABOUTBOX, hDlg, About);
                             break;
                           case IDM_EXIT:
                               DestroyWindow(hDlg);
                               break;
                           }
                       */
                       //}
                       //else if (wCode == EN_CHANGE)//�༭�����ݸı䷢����Ϣ
                        //{
                               //  int a = 12;
                              //  int b = 0;
                               //}
                       //else  if (wCode == LBN_SELCHANGE)//listBox
                       //{
                       //   switch (wControlID)
                         // {
                                //case IDC_LIST2:
                              //   Win::ListBox listbox(hDlg, IDC_LIST2);
                              //   listbox.addString(L"aasss");
                               //   break;
                         // }
                           // }
                        // else if (wCode == CBN_SELCHANGE)//ComboBox
                         //{

                         //}

            }
            break;
            /* WM_NOTIFY��Ϣ������window3.*�����Ŀؼ���֧��WM_NOTIFY������
             * winows32���ӵĸ���Ϣ����Ҫ��Ϊ��Ϊ�����Ŀؼ�������Ϣ����ʹ�á�
             * ������Controls.hpp�д���Ŀؼ�֧��WM_NOTIFY�ؼ��У�UpDownBox��TreeView��Trackbar
               �����ؼ�֧��Command��Ϣ���ؼ��Ĵ�����ϢӦ����COMMAND�н���
               ��WM_NOTIFY�У�lParam�зŵ���һ����ΪNMHDR�ṹ��ָ�룬wParam�зŵ����ǿؼ���ID
               NMHDR
               {
                   HWnd hWndFrom; �൱��ԭWM_COMMAND���ݷ�ʽ��lParam������Ϣ�������ڵľ��
                   UINT idFrom;    �൱��ԭWM_COMMAND���ݷ�ʽ��wParam��low - order��������Ϣ�����ؼ���ID
                   UINT code;      �൱��ԭWM_COMMAND���ݷ�ʽ��Notify Code(wParam"s high-order)������Ϣ�¼���
               }��
             */
            case WM_NOTIFY:
            {
                 Notify_Event(hDlg, message, wParam, lParam);
                 // NMHDR* p = (NMHDR*)(void*)lParam;

                 // LONG iID = wParam;
                 // if (iID == IDC_SLIDER1)
                  //{
                  //   HWND hWnd = GetDlgItem(hDlg, IDOK);
                  // }
                  // else if (wParam == IDC_SPIN1)
                  // {
                  //     LPNMUPDOWN np;
                  //    np = (LPNMUPDOWN)lParam;
                  // SetDlgItemInt(hwndDlg, IDC_EDIT, np->iDelta + np->iPos, FALSE);
                  //}
            }
            break;
            case WM_KEYDOWN:
            {
                KeyDown_Event(hDlg, message, wParam, lParam);
            }
            break;
            case WM_KEYUP:
            {
                KeyUp_Event(hDlg, message, wParam, lParam);
            }
            break;
            case WM_CHAR:
            {
                KeyChar_Event(hDlg, message, wParam, lParam);
            }
            break;
            case WM_MOUSEMOVE:
            {
                MouseMove_Event(hDlg, message, wParam, lParam);
            }
            break;
            case WM_MOUSEHWHEEL:
            {
                MouseWheel_Event(hDlg, message, wParam, lParam);
            }
            break;
            case WM_LBUTTONDOWN:
            {
                MouseLButtonDown_Event(hDlg, message, wParam, lParam);
            }
            break;
            case WM_LBUTTONUP:
            {
                MouseLButtonUp_Event(hDlg, message, wParam, lParam);
            }
            break;
            case WM_LBUTTONDBLCLK:
            {
                MouseLButtonDBClick_Event(hDlg, message, wParam, lParam);
            }
            break;
            case WM_MBUTTONDOWN:
            {
                MouseMButtonDown_Event(hDlg, message, wParam, lParam);
            }
            break;
            case WM_MBUTTONUP:
            {
                MouseMButtonUp_Event(hDlg, message, wParam, lParam);
            }
            break;
            case WM_MBUTTONDBLCLK:
            {
                MouseMButtonDBClick_Event(hDlg, message, wParam, lParam);
            }
            break;
            case WM_RBUTTONDOWN:
            {
                MouseRButtonDown_Event(hDlg, message, wParam, lParam);
            }
            break;
            case WM_RBUTTONUP:
            {
                MouseRButtonUp_Event(hDlg, message, wParam, lParam);
            }
            break;
            case WM_RBUTTONDBLCLK:
            {
                MouseRButtonDBClick_Event(hDlg, message, wParam, lParam);
            }
            break;

            
            case WM_PAINT:
            {
                PAINTSTRUCT ps;
                HDC hdc = BeginPaint(hDlg, &ps);
                // TODO: �ڴ˴����ʹ�� hdc ���κλ�ͼ����...
                      Paint_Event(hdc, wParam, lParam);
                 EndPaint(hDlg, &ps);
           
            }
            break;
            case WM_SIZE:
            {
                Resize_Event(hDlg, message, wParam, lParam);
            }
            break;
            case  WM_CLOSE:
            {
                Close_Event(hDlg, message, wParam, lParam);

                ::SendMessageW(hDlg, WM_DESTROY, 0, 0);
            }
            break;
            case WM_DESTROY:
            {
               Destroy_Event(hDlg, message, wParam, lParam);
               PostQuitMessage(0);
            }
            break;
            default:
            {
                ExternEvent_Proc(hDlg, message, wParam, lParam);
            }
        }
        return (INT_PTR)FALSE;
    }
}


#endif

