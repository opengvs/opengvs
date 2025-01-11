<h3 align="Center">三维图形学中相机变换矩阵的推导过程(以OpenGL为例)</h3>

<h3 align="Center">刘文庆整理</h3>

   

​    经过前面几篇文章的学习，同学们应该明确了在三维图形学工程实践中的坐标变换均是通过矩阵运算的形式进行的(<font color="Red">无论是坐标变换还是坐标系间的变换</font>>)。本篇文章我们讨论<font color="Blue">世界坐标系</font>下的<font color="Red">物体坐标</font>到<font color="Blue">观察坐标系</font>下坐标的变换过程，我们称为<font color="Blue">视变换</font>。    
$$
\left( {\begin{array}{*{20}{c}}
{{{\rm{x}}_c}}\\
{{y_c}}\\
{_{{z_c}}}\\
1
\end{array}} \right) = {V_{w2c}}\left( {\begin{array}{*{20}{c}}
{{x_w}}\\
{{y_w}}\\
{{z_w}}\\
1
\end{array}} \right){{\rm{V}}_{w2c}} = \left( {\begin{array}{*{20}{c}}
{{a_{00}}}&{{a_{01}}}&{{a_{02}}}&{{a_{03}}}\\
{{a_{10}}}&{{a_{11}}}&{{a_{12}}}&{{a_{13}}}\\
{{a_{20}}}&{{a_{21}}}&{{a_{22}}}&{{a_{23}}}\\
{{a_{30}}}&{{a_{31}}}&{{a_{32}}}&{{a_{33}}}
\end{array}} \right) = \left( {\begin{array}{*{20}{c}}
{{a_{00}}}&{{a_{01}}}&{{a_{02}}}&0\\
{{a_{10}}}&{{a_{11}}}&{{a_{12}}}&0\\
{{a_{20}}}&{{a_{21}}}&{{a_{22}}}&0\\
0&0&0&1
\end{array}} \right) \cdot \left( {\begin{array}{*{20}{c}}
1&0&0&{ - {X_{eye}}}\\
0&1&0&{ - {Y_{eye}}}\\
0&0&1&{ - {Z_{eye}}}\\
0&0&0&1
\end{array}} \right)
$$
​    很显然<font color="Blue">视变换</font>为坐标系间的变换，为简单起见我们采用<font color="Blue">基变换</font>的方法推导变换矩阵。



​    在OpenGL和Direct3D图形系统中，均提过一个Lookat函数，用于定义相机的基本参数，起定义如下：

```c++
void gluLookAt(GLdouble eyex,GLdouble eyey,GLdouble eyez,
               GLdouble centerx,GLdouble centery,GLdouble centerz,
               GLdouble upx,GLdouble upy,GLdouble upz)
```

   其中：

​        eyex, eyey,eyez 指定视点的位置

　　     centerx,centery,centerz 指定参考点的位置

　　     upx,upy,upz 指定视点向上的方向

![image-20250108152801119](../images/image-20250108152801119.png)

​     OpenGL图形系统通过这9个参数就定一个相机坐标系，那么OpenGL内部是如何完成世界坐标系到相机坐标系的变换的呢？

​     依据坐标系间坐标变换的坐标基变换理论，我们需要求得<font color="Red">世界坐标系W</font>到<font color="Red">观察坐标系C</font>的变换矩阵，我们首先需要计算求取<font color="Red">世界坐标系W</font>中的三个轴在<font color="Red">观察坐标系C</font>的坐标基映射。然而Lookat函数中9个参数定义的三个坐标点均是在<font color="Red">世界坐标系W</font>下的坐标值，这样根据Lookat函数定义，我们可以很容易的推导出<font color="Red">观察坐标系C</font>中三个轴在<font color="Red">世界坐标系W</font>中的对应基坐标映射：

​      ![image-20250108153012785](../images/image-20250108153012785.png) 



​        我们定义矢量${V_z} =  - f =  - \frac{{Center - Eye}}{{\left| {Center - Eye} \right|}} = \left( {\begin{array}{*{20}{c}}
{ - {f_x}}\\
{ - {f_y}}\\
{ - {f_z}}
\end{array}} \right)$   为相机坐标系的$z$轴坐标基

​                 $
{V_y} = u = \left( {\begin{array}{*{20}{c}}
{{u_x}}\\
{{u_y}}\\
{{u_z}}
\end{array}} \right)
$                      为相机坐标系的y轴坐标基



​        依据右手定则：  $
{V_x} = {V_y} \times {V_z} = \left( {\begin{array}{*{20}{c}}
{{u_x}}\\
{{u_y}}\\
{{u_z}}
\end{array}} \right) \times \left( {\begin{array}{*{20}{c}}
{ - {f_x}}\\
{ - {f_y}}\\
{ - {f_z}}
\end{array}} \right) = \left( {\begin{array}{*{20}{c}}
{{s_x}}\\
{{s_y}}\\
{{s_z}}
\end{array}} \right)
$ 为相机坐标系的x轴坐标基



​        这样我们就很容易得到<font color="Red">观察坐标系C</font>到<font color="Red">世界坐标系W</font>变换的基矩阵：       
$$
{V_{c2w}} = \left( {\begin{array}{*{20}{c}}
{{s_x}}&{{u_x}}&{ - {f_x}}\\
{{s_y}}&{{u_y}}&{ - {f_y}}\\
{{s_z}}&{{u_z}}&{ - {f_z}}
\end{array}} \right) = {V_{w2c}}^{ - 1}
$$
​        由于${V_x、V_y、V_z}$均为单位向量，且两两相互垂直，所以${V_{c2w}}$矩阵为正交单位矩阵，根据正交单位矩阵的性质(<font color="Blue">其逆矩阵等于其转置矩阵</font>),因此<font color="Red">世界坐标系W</font>到<font color="Red">观察坐标系C</font>的变换基矩阵为:
$$
{V_{w2c}} = \left( {\begin{array}{*{20}{c}}
{{s_x}}&{{s_y}}&{{s_z}}\\
{{u_x}}&{{u_y}}&{{u_z}}\\
{ - {f_x}}&{ - {f_y}}&{ - {f_z}}
\end{array}} \right)
$$
​        有了变换基矩阵，我们就很容易写出相机变换过程的矩阵计算公式：
$
\left( {\begin{array}{*{20}{c}}
{{{\rm{x}}_c}}\\
{{y_c}}\\
{_{{z_c}}}\\
1
\end{array}} \right) = {V_{w2c}}\left( {\begin{array}{*{20}{c}}
{{x_w}}\\
{{y_w}}\\
{{z_w}}\\
1
\end{array}} \right)
$ 其中：
$$
{{\rm{V}}_{w2c}} = \left( {\begin{array}{*{20}{c}}
{{{\rm{s}}_{_X}}}&{{s_y}}&{{s_z}}&0\\
{{u_x}}&{{u_y}}&{{u_z}}&0\\
{ - {f_x}}&{ - {f_y}}&{ - {f_z}}&0\\
0&0&0&1
\end{array}} \right) \cdot \left( {\begin{array}{*{20}{c}}
1&0&0&{ - {X_{eye}}}\\
0&1&0&{ - {Y_{eye}}}\\
0&0&1&{ - {Z_{eye}}}\\
0&0&0&1
\end{array}} \right) = \left( {\begin{array}{*{20}{c}}
{{s_x}}&{{s_y}}&{{s_z}}&{ - (sEye)}\\
{{u_x}}&{{u_y}}&{{u_z}}&{ - (uEye)}\\
{ - {f_x}}&{ - {f_y}}&{ - {f_z}}&{fEye}\\
0&0&0&1
\end{array}} \right)
$$
​     上述计算公式就是OpenGL系统内部使用的视变换矩阵公式，有经验的同学可以通过glm源码解析出该公式：

```c++
GLM_FUNC_QUALIFIER mat<4, 4, T, Q> lookAtRH(vec<3, T, Q> const& eye, vec<3, T, Q> const& center, vec<3, T, Q> const& up)
	{
		vec<3, T, Q> const f(normalize(center - eye));
		vec<3, T, Q> const s(normalize(cross(f, up)));
		vec<3, T, Q> const u(cross(s, f));


		mat<4, 4, T, Q> Result(1);
		Result[0][0] = s.x;//0列0行
		Result[1][0] = s.y;//1列0行
		Result[2][0] = s.z;//2列0行
		
		Result[0][1] = u.x;//0列1行
		Result[1][1] = u.y;//1列1行
		Result[2][1] = u.z;//2列1行
		
		Result[0][2] =-f.x;//0列2行
		Result[1][2] =-f.y;//1列2行
		Result[2][2] =-f.z;//2列2行
		
		Result[3][0] =-dot(s, eye);//3列0行
		Result[3][1] =-dot(u, eye);//3列1行
		Result[3][2] = dot(f, eye);//3列2行
		return Result;
	}
```

<font color="Red">注意！这是一个比较重要的小细节：</font>

​         <font color="Red">glm库和Open内部储存矩阵元素采用的是列优先的储存方式</font>

​        <font color="Red">所以mat[i][j]表示的是第i列，第j行元素</font>

这块代码的元素位置以及本文推导的矩阵内的元素位置并没有问题，是完全一样的。

附件：开源一个自己写么Camera类（C#代码）

```C#
 public class Camera
 {
     public Matrix4 _InsideMat = Matrix4.LookAt(new Vector3(0.0f, 0.0f, 1.0f),
                           new Vector3(0, 0, 0), Vector3.UnitY);

     private Vector3 _position = new Vector3(0.0f, 1.0f, 0.0f);

     //相机内部不维护center点，观察点通过_position+_front计算得到 ,这样的好处是可以随意移动相机的位置
     private Vector3 _front = -Vector3.UnitZ;

     private Vector3 _up = Vector3.UnitY;
 

     // The field of view of the camera (radians)
     private float _fov = MathHelper.PiOver2;//PiOver2
     private float _near = 0.1f;
     private float _far = 10000.0f;
     public Camera(Vector3 eye, Vector3 center, Vector3 up, float fov = 165.0f, float aspect = 1.0f, float near = 0.1f, float far = 10000.0f)
     {
         _position = eye;
         //_center = center;
         _front = Vector3.Normalize(center - eye);
          _up = up;

         Fov = fov;
         AspectRatio = aspect;
         _near = near;
         _far = far;
         _InsideMat = Matrix4.LookAt(eye, center, up);

     }
     // The position of the camera
     public Vector3 Position
     {
         get { return _position; } // set { _position = value; } }
         set { setPosition(value); }
     }

     public Vector3 Front { get { return _front; } }
     public Vector3 Up => _up;


     // This is simply the aspect ratio of the viewport, used for the projection matrix
     public float AspectRatio { private get; set; }

     public Vector3 Right  { get { return Vector3.Normalize(Vector3.Cross(Front, _up)); } }

     public Vector3 Center { get { return Position+Front; } }

     public float Fov
     {
         get => MathHelper.RadiansToDegrees(_fov);
         set
         {
             var angle = Clamp(value, 1f, 160f);
             _fov = MathHelper.DegreesToRadians(angle);
         }
     }

     public float Near { get { return _near; } set { _near = value; } }
     public float Far { get { return _far; } set { _far = value; } }


     public void setPosition(Vector3 value)
     {
         _position  = value;
         _InsideMat = Matrix4.LookAt(_position, _position+_front, _up);
     }
     public void setPosition(float x,float y,float z)
     {
         setPosition(new Vector3(x, y, z));
     }
     public void moveFront(float distance)
     {
         Position += distance * Front;
     }
     public void moveUp(float distance)
     {
         _up.Normalize();
         Position += distance * _up;
     }
     public void moveRight(float distance)
     {

         Position += distance * Right;
     }
     public Vector3 getRotation()
     {
         Quaternion quat = _InsideMat.ExtractRotation(false);
         Vector3 retRotate = quat.ToEulerAngles();
         retRotate.X = MathHelper.RadiansToDegrees(retRotate.X);
         retRotate.Y = MathHelper.RadiansToDegrees(retRotate.Y);
         retRotate.Z = MathHelper.RadiansToDegrees(retRotate.Z);

         return retRotate;
     }
     public void setRotation(Vector3 EulorAngle)
     {
         Quaternion quat = Quaternion.FromEulerAngles(MathHelper.DegreesToRadians(EulorAngle.X),
                                                      MathHelper.DegreesToRadians(EulorAngle.Y),
                                                      MathHelper.DegreesToRadians(EulorAngle.Z));
         Matrix4 mat = Matrix4.CreateFromQuaternion(quat);
 
         _front.X = -mat.Row0.Z;  
         _front.Y = -mat.Row1.Z;
         _front.Z = -mat.Row2.Z;

         _up.X = mat.Row0.Y;
         _up.Y = mat.Row1.Y;
         _up.Z = mat.Row2.Y;
       

         _InsideMat = Matrix4.LookAt(Position, Position + _front, _up);          
     }
     public void rotateY(float angle)
     {
         Vector3 tCenter = new Vector3(Position + Front);

         Matrix3 RotateMatrix = Matrix3.CreateFromAxisAngle(_up, MathHelper.DegreesToRadians(angle));
        
         tCenter = RotateMatrix * tCenter;
         _front = tCenter - _position;

         _InsideMat = Matrix4.LookAt(Position, tCenter, _up);
     }
     public void rotateX(float angle)
     {
         Vector3 tCenter = new Vector3(Position + Front);
         Vector3 tUp = new Vector3(Position + _up);
         Vector3 tRight = Right;

         Matrix3 RotateMatrix = Matrix3.CreateFromAxisAngle(tRight, MathHelper.DegreesToRadians(angle));

         tCenter = RotateMatrix * tCenter;
         _front = tCenter - _position;
         tUp     = RotateMatrix * tUp;
         _up = Vector3.Normalize(tUp - _position);

       
         _InsideMat = Matrix4.LookAt(Position, tCenter, _up);
     }
     public void rotateZ(float angle)
     {
         Vector3 tCenter = new Vector3(Position + Front);
         Vector3 tUp = new Vector3(Position + _up);
         Matrix3 RotateMatrix = Matrix3.CreateFromAxisAngle(_front, MathHelper.DegreesToRadians(angle));
         tUp = RotateMatrix * tUp;
         _up = Vector3.Normalize(tUp - Position);
         _InsideMat = Matrix4.LookAt(Position, Position + _front, _up);
     }


     // Get the view matrix using the amazing LookAt function described more in depth on the web tutorials
     public Matrix4 GetViewMatrix()
     {
         return _InsideMat;
       //  return Matrix4.LookAt(Position, Center, _up);
     }

     // Get the projection matrix using the same method we have used up until this point
     public Matrix4 GetProjectionMatrix()
     {
         return Matrix4.CreatePerspectiveFieldOfView(_fov, AspectRatio, _near, _far);
     }


     float Clamp(float val, float min, float max)
     {
         return val > max ? max : val < min ? min : val;
     }

     //
     // 摘要:
     //     Build a world space to camera space matrix.
     //
     // 参数:
     //   eye:
     //     Eye (camera) position in world space.
     //
     //   target:
     //     Target position in world space.
     //
     //   up:
     //     Up vector in world space (should not be parallel to the camera direction, that
     //     is target - eye).
     //
     // 返回结果:
     //     A Matrix4 that transforms world space to camera space.
     public static Matrix4 LookAt(Vector3 eye, Vector3 target, Vector3 up)
     {
         Vector3 vector = Vector3.Normalize(eye - target);
         Vector3 right = Vector3.Normalize(Vector3.Cross(up, vector));
         Vector3 vector2 = Vector3.Normalize(Vector3.Cross(vector, right));
        
         Matrix4 result = default(Matrix4);
         result.Row0.X = right.X;
         result.Row0.Y = vector2.X;
         result.Row0.Z = vector.X;
         result.Row0.W = 0f;
         result.Row1.X = right.Y;
         result.Row1.Y = vector2.Y;
         result.Row1.Z = vector.Y;
         result.Row1.W = 0f;
         result.Row2.X = right.Z;
         result.Row2.Y = vector2.Z;
         result.Row2.Z = vector.Z;
         result.Row2.W = 0f;
         result.Row3.X = 0f - (right.X * eye.X + right.Y * eye.Y + right.Z * eye.Z);
         result.Row3.Y = 0f - (vector2.X * eye.X + vector2.Y * eye.Y + vector2.Z * eye.Z);
         result.Row3.Z = 0f - (vector.X * eye.X + vector.Y * eye.Y + vector.Z * eye.Z);
         result.Row3.W = 1f;
         return result;
     }
 }
```

