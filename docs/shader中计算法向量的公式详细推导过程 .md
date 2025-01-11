#      Shader 中计算法向量的算法推导 

##                               刘文庆整理

​        在计算机图形学中，法向量（垂直于某个面的向量）有着广泛的应用，比如处理光照需要用到法向量来计算光线的入射角，最终渲染出逼真的场景。

​       图形系统编程过程中，在阅读现代Shader代码过程中，很对同学会迷惑下面一句代码：Normal = mat3(transpose(inverse(model))) * aNormal;有经验的同学可能基本明白该语句首先通过inverse(model)求取模型变换矩阵的逆矩阵，然后对该逆矩阵通过transpose()函数求取该矩阵的转置矩阵，最后通过将mat4X4矩阵变换为mat3*3矩阵，然后乘以传入的顶点法向量aNormal得到新的法向量Normal，送给光照模型进行计算表面颜色。但是为何直接使用aNormal，而需要进行计算得到变换后的Normal值，本位试图给你解决迷惑。


![](../images/2025_01_06_d87563d9f3a57a6078eag-1.jpg)

​         3D 建模工具设计出的模型文件一般都是包含了顶点坐标和每个面的法向量信息，也就是说法向量是已知的，模型一旦确定，每个面的法向量就确定了。但是，事情没有那么简单，场景要动起来才会丰富多彩，完全静止的东西往往缺乏生气，这就需要用到模型变换，比如平移，缩放（等比例／不等比例），旋转等，而不管如何变换，法向量和对应面的垂直关系必须成立，否则场景渲染的效果就会失真甚至完全不正常。
​        $3 D$ 模型是由大量的顶点坐标组成的，只要模型变换矩阵乘以顶点坐标就完成了模型的变换，但是法向量却不能直接与模型变换矩阵相乘，因为向量是有向的线段，而顶点坐标仅仅表示一个点，这个本质的不同导致变换矩阵与法向量相乘后很可能不再和变换后的面垂直。

​         举个简单的例子，参见下图，假设针对 3 D 模型做沿 y 轴向上平移 1 个单位的变换，向量 p 为垂直于其中某个面的法向量。如果向量 $\mathrm{p}=(1,0)$ 执行同样的平移操作，则变成了向量 $\mathrm{p}^{\prime}=(1,1), \mathrm{p}$ 和 $\mathrm{p}^{\prime}$ 并不是平行关系，显然针对法向量做相同的平移变换之后， p ＇不可能再垂直于平移后的平面，因为平面沿 y 轴向上平移 1 个单位之后仍然与变换前平面平行。
<img src="../images/2025_01_06_d87563d9f3a57a6078eag-2.jpg" style="zoom: 33%;" />

​         假设 p 0 与 p 1 是变换前某平面上的两个点，切向量 $\mathrm{s}=\mathrm{p} 1-\mathrm{p} 0$ ，与之垂直的法向量为 $n ; p 0 '$ 和 $p 1$＇为 $p 0$ 和 $p 1$ 变换后的点，变换后的切向量 $s^{\prime}=p 1^{\prime}-p 0^{\prime}$ ，变换后法向量为 $n^{\prime}$ ，其中模型变换矩阵为 $M$ ，法向量变换矩阵为 $\mathrm{M}^{\prime}$ ，变换后 $\mathrm{n}^{\prime}$ 与 $\mathrm{s}^{\prime}$ 需要互相垂直。
![](https://cdn.mathpix.com/cropped/2025_01_06_d87563d9f3a57a6078eag-2.jpg?height=587&width=1473&top_left_y=1756&top_left_x=362)

使用公式可以表示为：

$$
n^{\prime}=M^{\prime} * n
$$

$$
s^{\prime}=p_{1}^{\prime}-p_{0}^{\prime}=M * p_{1}-M * p_{0}=M *\left(p_{1}-p_{0}\right)=M * s
$$

$s$＇和 $n$＇互相垂直，可以得知 $n$＇和 $s$＇的点积为 0 ，所以有：

$$
\begin{aligned}
& n^{\prime} \cdot s^{\prime}=0 \\
& \left(M^{\prime} * n\right) \cdot(M * s)=0
\end{aligned}
$$

由公式：$\left(A \cdot B=A^{T} \times B\right)$ 得：

$$
\begin{aligned}
& \quad\left(M^{\prime} * n\right)^{T} \times(M * s)=0 \\
& \text { 由于: }(A * B)^{T}=B^{T} * A_{\text {得: }}^{T} \\
& n^{T} * M^{\prime T} * M * s=0 \\
& \text { 我们观察上式, 已知: } n^{T} * s=0
\end{aligned}
$$

$$
M^{\prime T} * M=I
$$

要使上式成立，必定：$M$ ，即：

$$
M^{\prime T}=I * M^{-1}
$$

从而：

$$
M^{\prime}=\left(M^{-1}\right)^{T}
$$

## 点乘和叉乘转换成矩阵乘法

向量 $\vec{u}=\left(u_{x}, u_{y}, u_{z}\right)$ 和向量 $\vec{v}=\left(v_{x}, v_{y}, v_{z}\right)$ ．

## 点乘转换成矩阵乘法

$\vec{u} \cdot \vec{v}=\vec{v} \cdot \vec{u}=\left(u_{x}, u_{y}, u_{z}\right) \cdot\left(v_{x}, v_{y}, v_{z}\right)=\left(v_{x}, v_{y}, v_{z}\right) \cdot\left(u_{x}, u_{y}, u_{z}\right)=u_{x} v_{x}+u_{y} v_{y}$

$$
+y_{z} v_{z}
$$

转换成矩阵乘法：

$$
\vec{u} \cdot \vec{v}=\vec{u} \vec{v}^{T}=\left[\begin{array}{lll}
u_{x} & u_{y} & u_{z}
\end{array}\right]\left[\begin{array}{c}
v_{x} \\
v_{y} \\
v_{z}
\end{array}\right]=u_{x} v_{x}+u_{y} v_{y}+y_{z} v_{z}
$$

## 叉乘转换成矩阵乘法

叉乘：

$$
\vec{u} \times \vec{v}=\left(u_{x}, u_{y}, u_{z}\right) \times\left(v_{x}, v_{y}, v_{z}\right)=\left(u_{y} v_{z}-u_{z} v_{y}, u_{z} v_{x}-u_{x} v_{z}, u_{x} v_{y}-u_{y} v_{x}\right)
$$

转换成矩阵乘法（列向量表示形式）：

$$
\begin{aligned}
\vec{u} \times \vec{v} & =\left(u_{x}, u_{y}, u_{z}\right) \times\left(v_{x}, v_{y}, v_{z}\right) \\
& =\left[\begin{array}{ccc}
0 & -u_{z} & u_{y} \\
u_{z} & 0 & -u_{x} \\
-u_{y} & u_{x} & 0
\end{array}\right]\left[\begin{array}{l}
v_{x} \\
v_{y} \\
v_{z}
\end{array}\right] \\
& =\left[\begin{array}{l}
u_{y} v_{z}-u_{z} v_{y} \\
u_{z} v_{x}-u_{x} v_{z} \\
u_{x} v_{y}-u_{y} v_{x}
\end{array}\right]
\end{aligned}
$$

转换成矩阵乘法（行向量表示形式）：

$$
\begin{aligned}
\vec{u} \times \vec{v} & =\left(u_{x}, u_{y}, u_{z}\right) \times\left(v_{x}, v_{y}, v_{z}\right) \\
& =\left[\begin{array}{lll}
u_{x} & u_{y} & u_{z}
\end{array}\right]\left[\begin{array}{ccc}
0 & -v_{z} & v_{y} \\
v_{z} & 0 & -v_{x} \\
-v_{y} & v_{x} & 0
\end{array}\right] \\
& =\left[\begin{array}{lll}
u_{y} v_{z}-u_{z} v_{y} & u_{z} v_{x}-u_{x} v_{z} & u_{x} v_{y}-u_{y} v_{x}
\end{array}\right]
\end{aligned}
$$

