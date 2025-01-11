<h2 align="center">三维图形学 openGL/Diret3D中变换原理</h1>

<h4 align="Center">刘文庆整理</h4>

​        计算机三维图形的原理本质是依据数学原理，采用计算机程序设计的方法将三维世界场景中的物体投影显示到计算机平面显示器上理论、技术和方法，其整个计算过程可参阅照相机拍摄照片的过程进行理解：

![image-20250105104059557](../images/image-20250105104059557.png)

​         为了描述的清楚，我们需要首先定义系统工作过程中需要的三维空间和二维平面坐标系，三维图形学中定义的坐标系如下：

​        <font color="Red">注意：数学中三维坐标系分为右手坐标系和左手坐标系，右手坐标系也称为笛卡尔坐标系，openGL使用右手坐标系，Direct3D使用左手坐标系，当然他们均可以通过设置进行更改。</font>

<font color="Red"><font size="5">●</font>世界坐标系： </font> 坐标系统主要用于计算机图形场景中的所有图形对象的空间定位和定义。世界坐标系是其他坐标系的全局位置基础，只有通过世界坐标系才能够描述清楚其他坐标系的位置和方位定义。通常三维图形软件将世界坐标系定义在屏幕中心点，x 轴向右，y 轴向上，z 轴依据使用左/右手定则(openGL向外/direct3D  向内)。当然也可以定义在其他位置，如OSG(OpenSceneGraph)中定义世界坐标系有时为地球中心。

<font color="Red"><font size="5">●</font>物体局部坐标系： </font> 为了描述物体空间结构，需要根据物体本身的结构形状，定义描述物体本身的局部坐标系。局部坐标系可以是三维直角坐标系，也可以为球面坐标系或其他坐标系类型。

<font color="Red"><font size="5">●</font>观察坐标系： </font>观察坐标系就是照相机或人眼的位置和方位。观察坐标系通常是以视点的位置为原点，通过用户指定的一个向上的观察向量来定义整个坐标系统，观察坐标系主要用于从观察者的角度对整个世界坐标系内的对象进行重新定位和描述，从而简化几何物体在投影面的成像的数学推导和计算。

关于投影变换我们会在后续描述中进行详细描述。

<font color="Red"><font size="5">●</font>规格化设备坐标系： </font> 是为了避免设备相关性而定义的一种虚拟的设备坐标系。规格化坐标系的坐标范围一般从 0 到 1，也有的是从-1 到＋1。采用规格化设备坐标系的好处是屏蔽了具体设备的分辨率，使得图形处理能够尽量避开对具体设备坐标的考虑。实际图形处理时，先将世界坐标转换成对应的规格化设备坐标，然后再将规格化设备坐标映射到具体的设备坐标上去。

<font color="Red"><font size="5">●</font>视口坐标系  (窗口坐标系)： </font> 是图形窗口上显示三维场景的位置、大小、方位描述。视口坐标系一般采用整数坐标，其坐标范围由具体窗口系统决定。视口坐标系上的一个点一般对窗口上的一个像素。

<font color="Red"><font size="5">●</font>屏幕坐标系统：</font> 也称设备坐标系统，它主要用于某一特殊的计算机图形显示设备(如光栅显示器)的表面的点的定义，在多数情况下，对于每一个具体的显示设备，都有一个单独的坐标系统(视口坐标到设备坐标的变换一般有操作系统完成)。

​        有了坐标系统，就可以描述三维图形学进行三维物体显示计算机的过程原理了，通常其计算过程如下：

![image-20250105153825212](../images/image-20250105153825212.png)

用文字描述 OpenGL  数学变换过程为：

<font color="Red">第一步：</font>数据点集(包括颜色、材质、纹理等)局部坐标描述的物体，经过世界坐标变换，将数据集的坐标信息统一变换为世界坐标系下的数据。变换过程即为乘以世界变换矩阵，这一点的推导我们在上一章中已经推到过。关于世界坐标变换在程序设计中大量用到，在后续的章节中我们还会大量介绍相关函数。

<font color="Red">第二步：</font>将世界坐标系下的物体集数据，通过视变换 (Viewing transformation)将数据变换为照相机坐标下。openGL  提供的变换函数为：

```
Void  gluLookAt(GLdouble eyex, GLdouble eyey, GLdouble eyez,
              GLdouble centerx, GLdouble centery, GLdouble centerz,
              GLdouble upx, GLdouble upy, GLdouble upz);
```

​        函数参数中, eye=(eyex, eyey, eyez)是视点(原点)的位置.center = (centerx, centery, centerz)是视口中心点的位置，参数定义相机正对着的世界坐标系中的点的位置坐标，成像后这一点会位于画板的中心位置，代表眼睛看向的方向，一般设置垂直计算机屏幕并指向计算机屏幕方向；$center - eye$是  z轴负方向，$z = (eye - center) / |eye - center|$。$up = (upx, upy, upz) - eye$表示上方，向量$up(upx,  upy, upz)$代表视线向上方向, x轴正方向  $x = up× z /  |up×  z|$。y轴正方向(就是正上方)$y = z×  x$。只需控制这三个量,便可定义新的视点。

<font color="Red">第三步：</font>视坐标下的数据需要经过透视变换 Projection Transformation，将三维空间数据变换为透视剪切平面上的二维平面数据。OpenGL中把三维模型投影到二维屏幕上的过程，即OpenGL的投影变换（Projection Transformation）。

​       事实上，投影变换的目的就是定义一个视景体，使得视景体外多余的部分裁剪掉，最终进入图像的只是视景体内的有关部分。投影包括透视投影（PerspectiveProjection）和正视投影（Orthographic Projection）两种。

​        <font color="Red"><font size="5">⑴</font>透视投影：</font>透视投影符合人们心理习惯，即离视点近的物体大，离视点远的物体小，远到极点即为消失，成为灭点。它的视景体类似于一个顶部和底部都被进行切割过的棱椎，也就是棱台。这个投影通常用于动画、视觉仿真以及其它许多具有真实性反映的方面。

​       OpenGL透视投影函数有两个glFrustum()和gluPerspective（）。

​        <font color="Red"> 透视函数glFrustum()：</font>

```
void glFrustum(GLdouble left,GLdouble Right,//近剪切平面左上角坐标
GLdouble bottom,GLdouble top,//近剪切平面右下角坐标
GLdouble near,  //近剪切平面在视坐标系下的z轴的位置坐标
GLdouble far   //远将切面在视坐标系下的Z轴位置坐标
);
```

​        它创建一个透视视景体。其操作是创建一个透视投影矩阵，并且用这个矩阵乘以当前矩阵（三维物体的各个顶点坐标组成的矩阵）。这个函数的参数只定义近裁剪平面的左下角点和右上角点的三维空间坐标，即（left，bottom，-near）和（right，top，-near）；最后一个参数 far是远裁剪平面的 Z负值，其左下角点和右上角点空间坐标由函数根据透视投影原理自动生成。near和 far表示离视点的远近，它们总为正值。该函数形成的视景体如图三所示。

![image-20250105155947086](../images/image-20250105155947086.png)

  <font color="Red"> 透视函数gluPerspective（）</font>

```
void gluPerspective(GLdouble fovy,//视野张角
GLdouble aspect,//宽高比
GLdouble zNear, //近平面
GLdouble zFar);//远平面
```

​        它也创建一个对称透视视景体，但它的参数定义于前面的不同，参数fovy定义视野的角度，范围是[0.0,  180.0]，通常理解为水平面内观察点张开的角度；参数aspect是投影平面宽度与高度的比率；参数zNear和Far分别是远近裁剪面沿Z负轴到视点的距离，它们总为正值。

![image-20250105160314605](../images/image-20250105160314605.png)

以上两个函数缺省时，视点都在原点，视线沿Z轴指向负方向。

<font color="Red"><font size="5">⑵</font>透视投影：</font>正射投影，又叫平行投影。这种投影的视景体是一个矩形的平行管道，也就是一个长方体，如图五所示。正射投影的最大一个特点是无论物体距离相机多远，投影后的物体大小尺寸不变。这种投影通常用在建筑蓝图绘制和计算机辅助设计(CAD)等方面，这些行业要求投影后的物体尺寸及相互间的角度不变，以便施工或制造时物体比例大小正确。

![image-20250105160533678](../images/image-20250105160533678.png)

OpenGL正射投影函数也有两个，glOrtho（）和  gluOrtho2D（）

<font color="Red">glOrtho（）：</font>

```
void glOrtho(GLdouble left, 
GLdouble right, 
GLdouble bottom, 
GLdouble top,  
GLdouble near, 
GLdouble far)
```

它创建一个平行视景体。实际上这个函数的操作是创建一个正射投影矩阵，并且用这个矩阵乘以当前矩阵。其中近裁剪平面是一个矩形，矩形左下角点三维空间坐标是（left，bottom，-near），右上角点是（right，top，-near）；远裁剪平面也是一个矩形，左下角点空间坐标是（left，bottom，-far），右上角点是（right，top，-far）。所有的near和far值同时为正或同时为负。如果没有其他变换，正射投影的方向平行于Z轴，且视点朝向Z负轴。这意味着物体在视点前面时far和near都为负值，物体在视点后面时far和near都为正值。

<font color="Red">gluOrtho2D（）：</font>

```
void gluOrtho2D(GLdouble left, 
GLdouble right, 
GLdouble bottom, 
GLdouble top)
```

​       它是一个特殊的正射投影函数，主要用于二维图像到二维屏幕上的投影。它的near和far缺省值分别为-1.0和1.0，所有二维物体的Z坐标都为0.0。因此它的裁剪面是一个左下角点为（left，bottom）、右上角点为（right，top）的矩形。

​         经过投影变换后三维数据集中的三维坐标，均变为二维坐标，当然投影体外的物体就被剪切掉了，一般情况下 OpenGL会将二维坐标值进行归一化处理，即将二维平面坐标均变换为[-1,1]之间的值，其变换原理比较简单，这里不再叙述。

<font color="Red">第四步：</font>视口变换**——**从 **NDC**到屏幕坐标

视变换是将规范化设备坐标(NDC)转换为屏幕坐标的过程，视口变化通过函数:

```
glViewport(GLint  sx  , GLint  sy  , GLsizei  ws  , GLsizei  hs);
```

两个函数来指定。其中(sx,sy)表示窗口的左下角坐标，ws,hs为宽度与高度。

​         经过上述变换过程，计算机窗口上就绘制了对应的三维物体，这就是三维图形学基本原理。当然在实际编程过程中，由于 OpenGL或Direct3D对矩阵变换进行了函数封装，我们基本不用使用矩阵参数完成物体的位置变换。现代计算机的发展，三维视觉正在接近我们的生活，利用OpenCV进行双目识别或无人驾驶越来越受到人们的关注，这样就需要反向推到变换矩阵内部参数。