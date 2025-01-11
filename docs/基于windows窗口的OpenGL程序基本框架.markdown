<h1 align="Center">基于windows窗口的OpenGL程序基本框架</h1>

<h3 align="Center">作者：刘文庆</h3>



​    网络上关于OpenGL应用程序很多，主要采用freeGlut和glfw两种结构，近些年以glfw封装的程序结构为主。这两种程序结构均是对窗口程序进行了封装，屏蔽了创建OpenGL程序过程中创建窗口Content的细节，通过几行代码就生成的OpenGL窗口应用程序，使得OpenGL程序开发简洁化，使得初学者可以集中精力学习OpenGL编程的主要基础性原理。这种封装也存在一定的不足之处，就是生成的OpenGL窗口程序占用windows窗口的整个部分，不便于通过窗口的可视化控件（<font color="Red">按钮、编辑框</font>）控制OpenGL内部的物体。为此我在WindowAPI函数基础上封装了windows窗口的OpenGL程序类框架GLContext，可以很容易的将OpenGL窗口内嵌到windows窗口中的任意子控件上。

​    为了使用OpenGL 1.1以上的API函数，GLContext内嵌的Glad.h和glad.c文件，关于采用Glad方案对OpenGL扩展函数的引入模式可以参阅[OpenGL-GLAD配置_opengl glad-CSDN博客](https://blog.csdn.net/qq_51355375/article/details/140570607)的文章，如果GLContext框架内嵌的glad.c和glad.c文件不适合你的机器配置可以直接去https://glad.dav1d.de/网站下载适合你当前机器的文件替换GLContext框架目录中的文件即可。

​    通过GLContext类和Win::WinForm的封装，我们可以很容易的创建windows窗口的OpenGL应用程序：

```c++
class  WindowsForm :public Win::WinForm, public GLContext
{
private:
    Win::Button  m_AboutButton;
    Win::Button  m_CloseButton;
public:
    WindowsForm() :Win::WinForm(), GLContext()
    {

    };
    virtual void Init_Event(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
    {
        HWND hWnd = hDlg;// ::GetDlgItem(hDlg, IDC_OPENGLVIEW);
        m_AboutButton.set(hDlg, IDD_ABOUTBOX);
        m_CloseButton.set(hDlg, IDM_EXIT);
        
        HWND hOpenGLWnd =  ::GetDlgItem(hDlg, IDC_OPENGLVIEW);
       
     //如果我们希望OpenGL窗口占用整个windows窗口，可以直接将hWnd传给CreateOpenGLContext函数
        bool succ = CreateOpenGLContext(hOpenGLWnd, TRUE);
        if (!succ)
        {
            ::MessageBox(hWnd, L"初始化OpenGL窗口发生错误", L"系统信息", MB_OK);
            return;
        }
        return;
    };
    virtual void Resize_Event(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
    {
        WindowResize(LOWORD(lParam), HIWORD(lParam));
    }
    virtual void Destroy_Event(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
    {
        shutdown();
    };
    virtual BOOL  OnIdle(LONG lCount)
    {
        GLDrawProcess();

        GLContext::swapBuffer();
        return TRUE;
    }
    virtual void Command_Event(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
    {
        int wmId = LOWORD(wParam);
        if (wmId == IDD_ABOUTBOX)
        {
            Win::WinForm::ShowModelDialog(IDD_ABOUTBOX, hDlg, About);// Win::WinForm::DialogWindowProc);
        }
        else if (wmId == IDM_EXIT)
        {
            ::SendMessageW(hDlg, WM_DESTROY, 0, 0);
        }
    }

    // virtual void  OpenGL_Init(void);
    //virtual void  GLDrawProcess();
    // virtual void  GLWndResize(GLsizei width, GLsizei height);

};
```



```
WindowsForm  g_WinForm;

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: 在此处放置代码。

    // 初始化全局字符串
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_WINMAINAPP, szWindowClass, MAX_LOADSTRING);
   
    g_WinForm.CreateWindowFromReSource(hInstance, IDD_MAINDIALOG);
    return g_WinForm.Run(); 
}
```

​    程序运行结果如下：

![image-20250109111035492](../images/image-20250109111035492.png)

​    我们已经将工程封装为Microsoft Visual Studio 的项目模板[WinFormOpenGL.zip](./WinFormOpenGL.zip),下载后将WinFormOpenGL.zip文件拷贝到Microsoft Visual Studio的项目模板目录下<font color="Red">C:\Users\Administrator\Documents\Visual Studio 2022\Templates\ProjectTemplates</font>就可以使用了。





<font color="Red">附录：GLContext.hpp文件源码</font>

```
///////////////////////////////////////////////////////////////////////////////
//  GLContext.hpp  用于构建Window窗口的OpenGL应用程序框架
//  本头文件是一个独立的头文件，实现了单个头文件构建OpenGL窗口程序，
//  OpenGL窗口仅通过CreateOpenGLContext函数创建OpenGL视图，依赖传入的参数决定视图位置
//  默认情况下创建的应用程序基于doublebuffer和深度缓冲区，自动绘制一个正方体和一个三角椎体，并进行选装
//  如果需要创建平行投影的二维视图程序需要重载 OpenGL_Init  WindowResize  GLDrawProcess三个函数  
//  版权所有        刘文庆    2023.11 廊坊      
///////////////////////////////////////////////////////////////////////////////

#ifndef  __GLCONTEXT_HPP__
#define  __GLCONTEXT_HPP__

#include <windows.h>
#include "glad.h"
#include <gl/GL.h>
#include <gl/glu.h>			// Header File For The GLu32 Library
//#include "wglext.h"



//#define   FREEGLUT_STATIC
//#include "freeglut.h"

#pragma comment(lib,"opengl32.lib")
#pragma comment(lib,"glu32.lib")

#ifndef WGL_ARB_create_context_profile
#define WGL_ARB_create_context_profile 1

#define WGL_CONTEXT_CORE_PROFILE_BIT_ARB 0x00000001
#define WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB 0x00000002
#define WGL_CONTEXT_PROFILE_MASK_ARB 0x9126


#define WGL_CONTEXT_MAJOR_VERSION_ARB       0x2091
#define WGL_CONTEXT_MINOR_VERSION_ARB       0x2092
#define WGL_CONTEXT_LAYER_PLANE_ARB     0x2093
#define WGL_CONTEXT_FLAGS_ARB           0x2094
#define WGL_CONTEXT_PROFILE_MASK_ARB        0x9126

#define WGLEW_ARB_create_context_profile WGLEW_GET_VAR(__WGLEW_ARB_create_context_profile)



#endif /* WGL_ARB_create_context_profile */



typedef HGLRC(WINAPI* PFNWGLCREATECONTEXTATTRIBSARBPROC) (HDC hDC, HGLRC hShareContext, const int* attribList);

//#ifdef WGL_WGLEXT_PROTOTYPES
//HGLRC WINAPI wglCreateContextAttribsARB(HDC hDC, HGLRC hShareContext, const int* attribList);
//#endif

//PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB = NULL;// 创建一个空的函数指针
// 通过wglGetProcAddress对函数寻址, 然后赋值
//wglCreateContextAttribsARB = (PFNWGLCREATECONTEXTATTRIBSARBPROC)wglGetProcAddress("wglCreateContextAttribsARB");

class GLContext 
{
protected:
	int		_format;
	HWND    _hWnd;			//窗口句柄
	HDC		_hDC;			//绘制设备上下文
	HGLRC   _hRC;			//opengl设备上下文  可以理解为opengl对象

	BOOL    _IsDoubleBuffer = TRUE;
	
	float   _AOV = 120.0f;
	float   _Near = 0.1f;
	float   _Far = 100.0f;
    
	float _GLVersion = 3.2f;
	
public:
	GLContext();
	GLContext(const float c_GLVersion) { 
		_GLVersion = c_GLVersion; 
		_format = 0;
	    _hWnd = 0;
	    _hDC = 0;
	    _hRC = 0;
	}
	~GLContext();

	//初始化 GL 建立opengl   GLView附着的句柄   是否使用双缓冲区           视角的张开角度    进平面距离         远平面距离
	bool   CreateOpenGLContext(HWND hWnd,      BOOL cIsDoubleBuffer = TRUE,float AOV=45,    float cNear=0.1f,  float cFar=100.0f,const float c_GLVersion = 3.2f);
	//销毁 EGL
	void shutdown();

	//交换缓冲区
	void swapBuffer();

	
	virtual void OpenGL_Init();
	virtual void WindowResize(GLsizei width, GLsizei height);
	virtual void GLDrawProcess();
private:

	static HMODULE _InsideHModleInstance(HMODULE _HModule=NULL,int _Release=false)
	{
		static HMODULE s_lModleInst = NULL;
		if (_HModule != NULL)
		{
			s_lModleInst = _HModule;
		}
		else
		{
			if (_Release == true)
			{
				FreeLibrary(s_lModleInst);
				s_lModleInst = NULL;
			}
		}
		return s_lModleInst;
	}
	
	static void* cWGLGetProcAddr(const char* name)
	{
		auto ret = wglGetProcAddress(name);
		if (ret == NULL)
		{
			ret = GetProcAddress(_InsideHModleInstance(), name);
		}
		return ret;
	}
	static bool initgladFuncAddr() noexcept
	{
		//assert(wglGetCurrentContext() != NULL);     // 保证wgl有合适的上下文
		HMODULE glInst = LoadLibraryA("opengl32.dll");
		_InsideHModleInstance(glInst);
		if (gladLoadGLLoader(cWGLGetProcAddr) == 0)
		{
			return false;
		}
		if (_InsideHModleInstance() != NULL)
		{
			_InsideHModleInstance(NULL, true);
			//FreeLibrary(glModleInst);
			//glModleInst = NULL;
		}
		return true;
	}
};

inline GLContext::GLContext() {
	_format = 0;
	_hWnd = 0;
	_hDC = 0;
	_hRC = 0;
}

GLContext::~GLContext() {
	shutdown();
}


inline bool GLContext::CreateOpenGLContext(HWND hWnd, BOOL cIsDoubleBuffer, float AOV, float cNear, float cFar, const float c_GLVersion)
{
	_hWnd = hWnd;
	_GLVersion = c_GLVersion;
	DWORD  tempPFD = PFD_DRAW_TO_WINDOW |            // 像素格式支持windows窗口
		PFD_SUPPORT_OPENGL;						// 像素格式支持OpenGL
	if (cIsDoubleBuffer == TRUE)
	{
		_IsDoubleBuffer = cIsDoubleBuffer;
		tempPFD = tempPFD | PFD_DOUBLEBUFFER;  // 像素格式支持双缓冲

	}
	//声明像素格式描述符
	PIXELFORMATDESCRIPTOR pfd = {
					sizeof(PIXELFORMATDESCRIPTOR),				// Size Of This Pixel Format Descriptor
					1,											// 版本号
				   //PFD_DRAW_TO_WINDOW |						// 像素格式支持windows窗口
				  // PFD_SUPPORT_OPENGL |						// 像素格式支持OpenGL
				   //PFD_DOUBLEBUFFER,							// 像素格式支持双缓冲
				   tempPFD,
		           PFD_TYPE_RGBA,								// 像素格式颜色值为RGBA
				   32,										// 像素颜色深度
				   0, 0, 0, 0, 0, 0,							// Color Bits Ignored
				   0,										// No Alpha Buffer
				   0,										// Shift Bit Ignored
				   0,										// No Accumulation Buffer
				   0, 0, 0, 0,								// Accumulation Bits Ignored
				   16,											// 16Bit Z-Buffer (Depth Buffer)  
				   0,											// No Stencil Buffer
				   0,											// No Auxiliary Buffer
				   PFD_MAIN_PLANE,								// Main Drawing Layer
				   0,											// Reserved
				   0, 0, 0										// Layer Masks Ignored
	};


	_hDC = GetDC(_hWnd);  //获取设备DC

	unsigned PixelFormat = ChoosePixelFormat(_hDC, &pfd); //从_hDC 中查找有没有匹配 pfd像素格式的索引


	if (!SetPixelFormat(_hDC, PixelFormat, &pfd))
	{  //将当前_hDC 建立成pfd 这种像素格式和索引，建立后，windows内部将建立各种缓冲区 颜色缓冲区 模板缓冲区 深度缓冲区
		return false;
	}
	////////////////////////////////////////////////////////////////////////////
	// 	   这段代码描述了如何创建OpenGL核心模式的事例---如果不使用该行代码，系统会更具定义的shade中的版本设定系统的OpenGL工作模式
		// 根据hdc, 创建一个临时的OpenGL context using wglCreateContext
	HGLRC tempRC = wglCreateContext(_hDC);// 创建Render Context
	wglMakeCurrent(_hDC, tempRC);// 绑定到device上
	
	// PFNWGLCREATECONTEXTATTRIBSARBPROC代表了wglCreateContextAttribsARB的函数签名
	PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB = NULL;// 创建一个空的函数指针
	// 通过wglGetProcAddress对函数寻址, 然后赋值
	wglCreateContextAttribsARB = (PFNWGLCREATECONTEXTATTRIBSARBPROC)wglGetProcAddress("wglCreateContextAttribsARB");

	int  InsideGLVersion = (int)(_GLVersion * 10);
	int  tempGLMajorVersion = InsideGLVersion * 0.1;
	int  tempGLMinorVersion = InsideGLVersion - tempGLMajorVersion * 10;
	
	int attribList[] = { WGL_CONTEXT_MAJOR_VERSION_ARB, tempGLMajorVersion,//3,
	WGL_CONTEXT_MINOR_VERSION_ARB, tempGLMinorVersion,//3,
	WGL_CONTEXT_FLAGS_ARB, 0,
	WGL_CONTEXT_PROFILE_MASK_ARB,
		//WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB,0, };//定义OpenGL兼容模式
		WGL_CONTEXT_CORE_PROFILE_BIT_ARB,0, };//WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB

	//if (OpenGL_Model == 2)

	if(InsideGLVersion<33)
	{
		attribList[7] = WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB; //兼容模式
	}
	//else
	{
		//attribList[7] = WGL_CONTEXT_CORE_PROFILE_BIT_ARB;//核心模式
	}
	// 调用获取来的函数指针
	HGLRC hglrc = wglCreateContextAttribsARB(_hDC, 0, attribList);
	if (hglrc == NULL)
	{
		// MessageBox(NULL, L"不能创建OGL3.3 context\n尝试创建3.2？", L"提示", MB_OK);
		attribList[3] = 2;
		hglrc = wglCreateContextAttribsARB(_hDC, 0, attribList);
		if (hglrc == NULL)
		{
			// MessageBox(NULL, L"opengl3.2 create failed!", L"提示", MB_OK);
			// return false;
		}
	}

	if (hglrc == NULL)
	{
		hglrc = wglCreateContext(_hDC); //wgl 是window扩展 opengl的函数  wglCreateContext意思是在给定的dc 上建立opengl的设备上下文 并返回句柄
	}

	// 重新绑定到Null的Render Context上
	wglMakeCurrent(NULL, NULL);
	// 删除遗留的临时Render Context
	// 问题, 这里创建一个临时的RC, 然后创建真正的RC之后, 又删掉这个临时的RC, 这是何必?
	wglDeleteContext(tempRC);

	_hRC = hglrc;

	//////////////////////////////////////////////////////////////////////////	

	//_hRC = wglCreateContext(_hDC); //wgl 是window扩展 opengl的函数  wglCreateContext意思是在给定的dc 上建立opengl的设备上下文 并返回句柄
	if (!wglMakeCurrent(_hDC, _hRC)) { //建立_hDC _hRC 关联 当用opengl进行绘制的时候 实际绘制到dc上  在当前的线程中建立opengl对象 该opengl对象和线程绑定  所以opengl只能在该线程下绘制 在其他线程中绘制要切换设备上下文
		return false;
	}

	initgladFuncAddr();
	//gladLoadGL();
	this->_AOV = AOV;
	this->_Near = cNear;
	this->_Far = cFar;
	

	OpenGL_Init();

	return true;
}

inline void GLContext::shutdown() {
	if (_hRC != NULL) {
		wglMakeCurrent(NULL, NULL);
		wglDeleteContext(_hRC);
		_hRC = NULL;
	}

	if (_hDC != NULL) {
		ReleaseDC(_hWnd, _hDC);
		_hDC = NULL;
	}
}


inline void GLContext::GLDrawProcess()
{
	static GLfloat s_angle = 0.0; // 设置旋转的角度
	//gluLookAt(0.0, 0.0, 5.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0);
	static float  s_Camera_z = 1.0f;

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // 清理颜色缓存和深度缓存
	glLoadIdentity(); // 重置矩阵
	gluLookAt(0.0, 0.0, s_Camera_z, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0);

	glTranslatef(0.0f, 0.0f, -8.0f);
	// 设置旋转的角度，这里glRotatef 第1个参数是角度，2～4 参数是指定旋转轴
	// 这里我们设置于y轴作旋转轴
	glRotatef(s_angle, 0.0, 1.0f, 0.0f);
	s_angle += 0.01f;

	// 绘制三角锥
	// 这里是通过绘制4个三角形来拼接起来的
	glTranslatef(-1.5f, 0.0f, 0.0f);
	glBegin(GL_TRIANGLES);
	// 第1个三角形 前面
	glColor3f(1.0f, 0.0f, 0.0f);
	glVertex3f(0.0f, 1.0f, 0.0f);
	glColor3f(0.0f, 1.0f, 0.0f);
	glVertex3f(-1.0f, -1.0f, 1.0f);
	glColor3f(0.0f, 0.0f, 1.0f);
	glVertex3f(1.0f, -1.0f, 1.0f);

	// 第2个三角形 左面
	glColor3f(1.0f, 0.0f, 0.0f);
	glVertex3f(0.0f, 1.0f, 0.0f);
	glColor3f(0.0f, 1.0f, 0.0f);
	glVertex3f(-1.0f, -1.0f, -1.0f);
	glColor3f(0.0f, 0.0f, 1.0f);
	glVertex3f(-1.0f, -1.0f, 1.0f);

	// 第3个三角形 右边    
	glColor3f(1.0f, 0.0f, 0.0f);
	glVertex3f(0.0f, 1.0f, 0.0f);
	glColor3f(0.0f, 1.0f, 0.0f);
	glVertex3f(1.0f, -1.0f, 1.0f);
	glColor3f(0.0f, 0.0f, 1.0f);
	glVertex3f(1.0f, -1.0f, -1.0f);

	// 第4个三角形 后边    
	glColor3f(1.0f, 0.0f, 0.0f);
	glVertex3f(0.0f, 1.0f, 0.0f);
	glColor3f(0.0f, 1.0f, 0.0f);
	glVertex3f(1.0f, -1.0f, -1.0f);
	glColor3f(0.0f, 0.0f, 1.0f);
	glVertex3f(-1.0f, -1.0f, -1.0f);

	glEnd();


	// 绘制一个正方体
	glTranslatef(3.0f, 0.0f, 0.0f);
	glRotatef(s_angle, 1.0, 0.0f, 0.0f);
	glBegin(GL_QUADS);
	// 前面
	glColor3f(1.0f, 0.0f, 0.0f);
	glVertex3f(-1.0f, 1.0f, 1.0f);
	glVertex3f(1.0f, 1.0f, 1.0f);
	glVertex3f(1.0f, -1.0f, 1.0f);
	glVertex3f(-1.0f, -1.0f, 1.0f);

	// 左面
	glColor3f(0.0f, 1.0f, 0.0f);
	glVertex3f(-1.0f, 1.0f, -1.0f);
	glVertex3f(-1.0f, -1.0f, -1.0f);
	glVertex3f(-1.0f, -1.0f, 1.0f);
	glVertex3f(-1.0f, 1.0f, 1.0f);

	// 右面
	glColor3f(0.0f, 0.0f, 1.0f);
	glVertex3f(1.0f, 1.0f, 1.0f);
	glVertex3f(1.0f, -1.0f, 1.0f);
	glVertex3f(1.0f, -1.0f, -1.0f);
	glVertex3f(1.0f, 1.0f, -1.0f);

	// 后面
	glColor3f(0.5f, 0.5f, 1.0f);
	glVertex3f(1.0f, 1.0f, -1.0f);
	glVertex3f(1.0f, -1.0f, -1.0f);
	glVertex3f(-1.0f, -1.0f, -1.0f);
	glVertex3f(-1.0f, 1.0f, -1.0f);

	// 上面
	glColor3f(1.0f, 0.5f, 0.0f);
	glVertex3f(-1.0f, 1.0f, -1.0f);
	glVertex3f(-1.0f, 1.0f, 1.0f);
	glVertex3f(1.0f, 1.0f, 1.0f);
	glVertex3f(1.0f, 1.0f, -1.0f);

	// 下面
	glColor3f(0.5f, 0.5f, 0.5f);
	glVertex3f(1.0f, -1.0f, 1.0f);
	glVertex3f(-1.0f, -1.0f, 1.0f);
	glVertex3f(-1.0f, -1.0f, -1.0f);
	glVertex3f(1.0f, -1.0f, -1.0f);

	glEnd();

}

//双缓冲 进行交换
inline void GLContext::swapBuffer()
{
	SwapBuffers(_hDC);
}

inline void GLContext::OpenGL_Init()
{
	glShadeModel(GL_SMOOTH);	// 启用阴影平滑
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);	// 黑色背景
	glClearDepth(1.0f);		// 设置深度缓存
	glEnable(GL_DEPTH_TEST);	// 启用深度测试
	glDepthFunc(GL_LEQUAL);	// 所作深度测试的类型

	RECT rect;
	::GetWindowRect(_hWnd, &rect);
	
	int height = rect.bottom - rect.top;
	int width = rect.right - rect.left;
	if (height == 0)	// 防止被零除
	{
		height = 1;	// 将Height设为1
	}
	glViewport(0, 0, width, height);	// 重置当前的视口
	
	glMatrixMode(GL_PROJECTION);	// 选择投影矩阵
	glLoadIdentity();				// 重置投影矩阵

	// 设置视口的大小
	gluPerspective(this->_AOV, (GLfloat)width / (GLfloat)height, this->_Near, this->_Far);

	glMatrixMode(GL_MODELVIEW);	// 选择模型观察矩阵
}

inline void GLContext::WindowResize(GLsizei width, GLsizei height)
{
	if (height == 0)	// 防止被零除
	{
		height = 1;	// 将Height设为1
	}
	glViewport(0, 0, width, height);	// 重置当前的视口
	glMatrixMode(GL_PROJECTION);	// 选择投影矩阵
	glLoadIdentity();				// 重置投影矩阵

	// 设置视口的大小
	gluPerspective(this->_AOV, (GLfloat)width / (GLfloat)height, this->_Near, this->_Far);

	glMatrixMode(GL_MODELVIEW);	// 选择模型观察矩阵
	glLoadIdentity();	// 重置模型观察矩阵
}

#endif
```

