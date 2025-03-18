
#ifndef  __OSG_LINE_DEF_H__
#define  __OSG_LINE_DEF_H__

#include "g_consts.h"

#define __USE_OWNED_VECTOR

#ifdef __USE_OWNED_VECTOR
namespace OSG
{
	class vec3 {
	public:
		double x, y, z;

		double dot(const vec3& b) {
			return vec3::x * b.x + vec3::y * b.y + vec3::z * b.z;
		}

		vec3 cross(const vec3& b) {
			return OSG::vec3(
				vec3::y * b.z - vec3::z * b.y,
				vec3::z * b.x - vec3::x * b.z,
				vec3::x * b.y - vec3::y * b.x
			);
		}

		vec3 normalize() {
			const float s = 1.0f / sqrtf(vec3::x * vec3::x + vec3::y * vec3::y + vec3::z * vec3::z);
			return OSG::vec3(vec3::x * s, vec3::y * s, vec3::z * s);
		}
		
		inline const vec3 operator+(const vec3& b) const
		{
			return OSG::vec3(
				vec3::x + b.x,
				vec3::y + b.y,
				vec3::z + b.z
			);
		}
		inline  vec3& operator+=(const vec3& b)  
		{
			*this = vec3::operator+(b);
			return *this;
		}

		inline const vec3 operator-(const vec3& b) const
		{
			return OSG::vec3(
				vec3::x - b.x,
				vec3::y - b.y,
				vec3::z - b.z
			);
		}
		inline vec3& operator-=(const vec3& b) {
			*this = vec3::operator-(b);
			return *this;
		}

		inline const vec3 operator*(const vec3& b) const {
			return OSG::vec3(
				vec3::x * b.x,
				vec3::y * b.y,
				vec3::z * b.z
			);
		}
		inline vec3& operator*=(const vec3& b) {
			*this = vec3::operator*(b);
			return *this;
		}
		inline const vec3 operator*(float b) const {
			return OSG::vec3(
				vec3::x * b,
				vec3::y * b,
				vec3::z * b
			);
		}
		inline vec3& operator*=(float b) {
			*this = vec3::operator*(b);
			return *this;
		}

		inline const vec3 operator/(const vec3& b) const {
			return OSG::vec3(
				vec3::x / b.x,
				vec3::y / b.y,
				vec3::z / b.z
			);
		}
		inline vec3& operator/=(const vec3& b) {
			*this = vec3::operator/(b);
			return *this;
		}
		inline const vec3 operator/(float b) const {
			return OSG::vec3(
				vec3::x * b,
				vec3::y * b,
				vec3::z * b
			);
		}
		inline vec3& operator/=(float b) {
			*this = vec3::operator/(b);
			return *this;
		}
		double length()
		{
			return sqrt(x * x + y * y + z * z);
		}
		inline double& operator [] (int i) 
		{
		   if(i==0)
			  return x; 
		   else if (i == 1)
			   return y;
		   else  
			   return z;
		}
		inline double operator [] (int i) const 
		{
			if (i == 0)
				return x;
			else if (i == 1)
				return y;
			else
				return z;
		}
		vec3(float x, float y, float z) {
			vec3::x = x;
			vec3::y = y;
			vec3::z = z;
		}
		vec3(float x) {
			vec3::x = x;
			vec3::y = x;
			vec3::z = x;
		}
		vec3() {
			//
		}
		~vec3() {
			//
		}
	};
	inline double dot(const vec3& a,const vec3& b) {
		return a.x * b.x + a.y * b.y + a.z * b.z;
	}

	inline const vec3 cross(const vec3& a, const vec3& b) {
		return OSG::vec3(
			a.y * b.z - a.z * b.y,
			a.z * b.x - a.x * b.z,
			a.x * b.y - a.y * b.x
		);
	}

	inline const vec3 normalize(const vec3& a) {
		const float s = 1.0f / sqrtf(a.x * a.x + a.y * a.y + a.z * a.z);
		return OSG::vec3(a.x * s, a.y * s, a.z * s);
	}
}
#endif
#ifndef __USE_OWNED_VECTOR
#include "glm/glm.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/quaternion.hpp"
#define GLM_ENABLE_EXPERIMENTAL
#include "glm/gtx/euler_angles.hpp"
#else
#define glm  OSG
#define dvec3  vec3
#endif

#ifdef __cplusplus
extern "C" {
#endif


#ifndef MyMin(a,b)

#define MyMin(a,b) ((a) < (b) ? (a) : (b))
#define MyMax(a,b) ((a) > (b) ? (a) : (b))

#define MyFABS(a) ((a) < (0) ? (-1*(a)):(a)  )

#endif
	//�ж�ʸ���Ƿ�ƽ��
	inline bool  IsParallel(const glm::dvec3& v1, const glm::dvec3& v2)
	{
		glm::dvec3 v3 = glm::cross(v1, v2);
		if ((fabs(v3.x) + fabs(v3.y) + fabs(v3.z)) < 0.000000001f) //v3Ϊ0ʸ��
			return  true;
		else
			return false;
	}
	//�ж�ʸ���Ƿ�ֱ
	inline bool IsVertical(const glm::dvec3& v1, const glm::dvec3& v2)
	{
		double value = glm::dot(v1, v2);
		if (fabs(value) < 0.00000001f)
			return true;
		else
			return false;
	}
	class  OSGPlane;
	class  OSGLine
	{
	public:
		inline OSGLine() {
			Pt_0 = glm::dvec3(0.0, 0.0, 0.0);
			vector = glm::vec3(0.0, 0.0, 0.0);
		};
		inline OSGLine(glm::dvec3 cA, glm::dvec3 cB)
		{
			Pt_0 = cA;
			vector = cB - cA;
			vector = glm::normalize(vector);
		}
		//������Ƿ���AB����
		bool  PointIsInLine(const glm::dvec3 cVec)
		{
			glm::dvec3   tVec = glm::normalize(cVec - Pt_0);
			

			glm::dvec3 tempV = glm::cross(tVec,  this->vector);
			if ((fabs(tempV.x) + fabs(tempV.y) + fabs(tempV.z)) < 0.00001)
				return true;
			else  return false;

			//return  tempV.isZero();
		}

		glm::dvec3  getPointInLine(double  cDistance)
		{
			glm::dvec3  tRetPt;
			tRetPt.x = Pt_0.x + vector.x * cDistance;
			tRetPt.y = Pt_0.y + vector.y * cDistance;
			tRetPt.z = Pt_0.z + vector.z * cDistance;
			return tRetPt;

		}
		//�����v��ֱ�ߵĴ���
		const glm::dvec3  GetVerticPointInLine(const glm::dvec3& p);


		//����cPt����ֱ����תangle�Ƕȵ�����
		const glm::dvec3 getPointRotateAroundLine(const glm::dvec3&  cPt, const double& angle);
		const glm::dvec3 getPointRotateAroundLineUseQuat(const glm::dvec3&  cPt, double& angle);

		glm::dvec3  GetPoint() { return Pt_0; };
		glm::dvec3  GetVector() { return vector; };

	private:  //���ǽ���ֱ�ߵĲ�������
		glm::dvec3  Pt_0;  //ֱ���ϵ�һ��
		glm::dvec3  vector; //ֱ�ߵ�����
	};

	class  OSGPlane
	{
	public:
		OSGPlane()
		{
			_fv[0] = _fv[1] = _fv[2] = _fv[3] = 0.0;
		}
		OSGPlane(double cA, double cB, double cC, double cD)
		{
			set(cA, cB, cC, cD);
		}
		OSGPlane(const glm::dvec3& v1, const glm::dvec3& v2, const glm::dvec3& v3)
		{
			set(v1, v2, v3);
		}
		//ƽ��ķ��ߺ�ƽ���ϵĵ㹹��ƽ��
		inline OSGPlane(const glm::dvec3& norm, const glm::dvec3& point)
		{
			set(norm, point);
		}

		inline void set(const glm::dvec3&  v1,const glm::dvec3&  v2, const glm::dvec3& v3)
		{
			glm::dvec3   norm =  glm::cross( (v2 - v1), (v3 - v2));
			double       length = norm.length();
			if (length > 1e-6)
				norm /= length;
			else
				norm = glm::vec3(0.0, 0.0, 0.0);

			set(norm[0], norm[1], norm[2], -glm::dot(v1 , norm) );
		}
		void set(double a, double b, double c, double d)
		{
			_fv[0] = a;
			_fv[1] = b;
			_fv[2] = c;
			_fv[3] = d;
		}

		inline void set(const glm::dvec3& norm, const glm::dvec3& point)
		{
			double d = -norm[0] * point[0] - norm[1] * point[1] - norm[2] * point[2];
			set(norm[0], norm[1], norm[2], d);
		}
		//������ķ���
		glm::dvec3  getNorm() 
		{ 
			glm::dvec3  tVec(_fv[0], _fv[1], _fv[2]);
			tVec = glm::normalize(tVec);
			return tVec;
		};

		const bool getLineIntersectionPoint(OSGLine& cLine, glm::dvec3& outPot)
		{
			//��ֱ��tLine��tPlaneXYƽ��Ľ���
				// x = PtX + Vx * t;
				// y = PtY + Vy * t;
				// z = PtZ + Vz * t;
				// 
				//Ax+By+Cz+D = 0��
				// ��A*PtX+B*PtY+C*PtZ+D��+��A*Vx+B*Vy+C*Vz��* t = 0��
				// t = -(A*PtX+B*PtY+C*PtZ+D)/��A*Vx+B*Vy+C*Vz��;

			glm::dvec3   Pt = cLine.GetPoint();
			glm::dvec3   VL = cLine.GetVector();
			glm::dvec3   _Vec = getNorm();

			if (IsParallel(_Vec, VL) == true)
				return false;//ֱ����ƽ�洹ֱ
			double t = -1.0 * (A() * Pt.x + B() * Pt.y + C() * Pt.z + D()) / (A() * VL.x + B() * VL.y + C() * VL.z);

			outPot = cLine.getPointInLine(t);
			return  true;
		}

		inline double A() const { return _fv[0]; };
		inline double B() const { return _fv[1]; };
		inline double C() const { return _fv[2]; };
		inline double D() const { return _fv[3]; };
		inline double& A() { return _fv[0]; };
		inline double& B() { return _fv[1]; };
		inline double& C() { return _fv[2]; };
		inline double& D() { return _fv[3]; };
		
	protected:
		/** Vec member variable. */
		double _fv[4];
		//glm::dvec3   norm;
	};
	//p����ֱ���ϵĴ���(ͶӰ��)�����Կ���tempP(p-Pt_0)ʸ����Vecttorʸ���ĵ���õ�
	//    ����ĳ��ȣ������ţ���Ȼ�����Pt_0��������Ǽ�����
	inline const glm::dvec3 OSGLine::GetVerticPointInLine(const glm::dvec3& p)
	{
		if (PointIsInLine(p) == true)
			return  p;
		glm::dvec3  tRtRetVetV;
		
		double  k = ( (p.x- Pt_0.x) * vector.x + (p.y- Pt_0.y) * vector.y + (p.z- Pt_0.z) * vector.z)  / (vector.x * vector.x + vector.y * vector.y + vector.z * vector.z);
		
		tRtRetVetV[0] = Pt_0.x+ vector.x * k;
		tRtRetVetV[1] = Pt_0.y + vector.y * k;
		tRtRetVetV[2] = Pt_0.z + vector.z * k;
		return  tRtRetVetV;
	}
	inline const glm::dvec3 OSGLine::getPointRotateAroundLine(const glm::dvec3& cPt, const double& angle)
	{
		if (PointIsInLine(cPt) == 1)
			return cPt;
		glm::dvec3  tVerticPt = GetVerticPointInLine(cPt);//���㴹��


		OSGPlane  plane(this->vector, cPt); //����ͨ��cPt���д�����ֱ�ߵ�ƽ��
		//�����ʵ�ʾ��Ǽ����cPt��ƽ��plane����tVerticPt����תangle�ǵ�����ֵ

		OSGLine  templine1(this->vector, cPt);

		glm::dvec3  tVerticPt1;
		bool bResult = plane.getLineIntersectionPoint((*this), tVerticPt1);//���㴹��


		if ((fabs(vector.x) < 0.0001) && (fabs(vector.y) < 0.0001))  //ƽ����xyƽ��
		{
			glm::dvec3  tPt1;
			tPt1[0] = (cPt.x - tVerticPt.x) * cos(angle) - (cPt.y - tVerticPt.y) * sin(angle) + tVerticPt1.x;
			tPt1[1] = (cPt.x - tVerticPt.x) * sin(angle) + (cPt.y - tVerticPt.y) * cos(angle) + tVerticPt1.y;
			tPt1[2] = tVerticPt.z;
			return tPt1;
		}
		else  if ((fabs(vector.y) < 0.0001) && (fabs(vector.z) < 0.0001))  //ƽ����yzƽ��
		{
			glm::dvec3  tPt1;
			tPt1[0] = tVerticPt.x;
			tPt1[1] = (cPt.y - tVerticPt.y) * cos(angle) - (cPt.z - tVerticPt.z) * sin(angle) + tVerticPt.y;
			tPt1[2] = (cPt.z - tVerticPt.z) * cos(angle) + (cPt.y - tVerticPt.y) * sin(angle) + tVerticPt.z;

			return tPt1;
		}
		else  if ((fabs(vector.z) < 0.0001) && (fabs(vector.x) < 0.0001))  //ƽ����zxƽ��
		{
			glm::dvec3  tPt1;

			tPt1[1] = tVerticPt.y;
			tPt1[2] = (cPt.z - tVerticPt.z) * cos(angle) - (cPt.x - tVerticPt.x) * sin(angle) + tVerticPt.z;
			tPt1[0] = (cPt.x - tVerticPt.x) * cos(angle) + (cPt.z - tVerticPt.z) * sin(angle) + tVerticPt.x;
			return tPt1;
		}
		else
		{
			OSGPlane  tPlaneXY(glm::dvec3(0, 0, 1), tVerticPt);//����

			OSGLine  tLine(vector, cPt);//����ͨ��cPt���д�ֱ��planeƽ���ֱ��

			//��ֱ��tLine��tPlaneXYƽ��Ľ���
			glm::dvec3  tPt;
			tPlaneXY.getLineIntersectionPoint(tLine, tPt);

			//tPt��ƽ��tPlaneXY����tVerticPt(x,y,z)����תangle�ǵ�������
			//x1 = xcos(��) - ysin(��);
			//y1 = ycos(��) + xsin(��)
			glm::dvec3  tPt1;
			tPt1[0] = (tPt.x - tVerticPt.x) * cos(angle) - (tPt.y - tVerticPt.y) * sin(angle) + tVerticPt.x;
			tPt1[1] = (tPt.y - tVerticPt.y) * cos(angle) + (tPt.x - tVerticPt.x) * sin(angle) + tVerticPt.y;
			tPt1[2] = tVerticPt.z;

			OSGLine  tLine1(vector, tPt1);//����ͨ��cPt1���д�ֱ��planeƽ���ֱ��
			//������Ҫ����tLine1��ƽ��plane���������ת�������


			glm::dvec3  tRetPt;
			plane.getLineIntersectionPoint(tLine1, tRetPt);
			return  tRetPt;
		}
	}
#ifndef __USE_OWNED_VECTOR
	const glm::dvec3 OSGLine::getPointRotateAroundLineUseQuat(const glm::dvec3& cPt, double& angle)
	{
		glm::dvec3   vecPt = cPt - GetPoint();
		glm::dvec3   vecLine = this->GetVector();

		//ע�⣺glm��Ĭ�Ϲ�����Ԫ��Ϊ��w,x,y,z)
		glm::quat quat(cos(angle * 0.5), sin(angle * 0.5) * vector.x, sin(angle * 0.5) * vector.y, sin(angle * 0.5) * vector.z);

		glm::dvec3  retPt = glm::dvec4(cPt,1.0) * //osg::Matrixd::translate(-this->Pt_0) * osg::Matrixd::rotate(tQuatQ) * osg::Matrixd::translate(this->Pt_0);
				            glm::translate(glm::mat4(1.0), glm::vec3(-Pt_0.x, -Pt_0.y, -Pt_0.z)) *
			                glm::mat4_cast(quat) *
				            glm::translate(glm::mat4(1.0), glm::vec3(Pt_0.x, Pt_0.y, Pt_0.z));
		return retPt;
	}
#endif
	
	// v1 = Cross(AB, AC)
    // v2 = Cross(AB, AP)
       // �ж�ʸ��v1��v2�Ƿ�ͬ��
	bool SameSide(const glm::dvec3& A, const glm::dvec3& B, const glm::dvec3& C, const glm::dvec3& P)
	{
		const glm::dvec3& AB = B - A;
		const glm::dvec3& AC = C - A;
		const glm::dvec3& AP = P - A;

		const glm::dvec3& v1 = glm::cross(AB,AC);
		const glm::dvec3& v2 = glm::cross(AB,AP);

		// v1 and v2 should point to the same direction
		return glm::dot( v1,v2 ) > 0;
	}
	inline bool IsInTrangle(const glm::dvec3& P, const glm::dvec3& A, const glm::dvec3& B, const glm::dvec3& C)
	{
		return SameSide(A, B, C, P) && SameSide(B, C, A, P) && SameSide(C, A, B, P);
	}
	// ���������Ƿ����������ཻ�������������
	// orig: ���ߵ��������
	// dir: ���ߵķ���Ҫ��Ϊ��λʸ��
	// A, B, C: ������������������
	// outPot(out):���������
	inline bool IntersectTriangle(const glm::dvec3& orig, const glm::dvec3& dir,
		           const glm::dvec3& A, const glm::dvec3& B, const glm::dvec3& C,
		           glm::dvec3& outPot)
	{
		// E1
		glm::dvec3   AB = B - A; //E1

		// E2
		glm::dvec3   AC = C - A;  //E2

		// ����S1 = dir X AC
		glm::dvec3   S1 = glm::cross(dir , AC);

		// ͨ������det,�ж������Ƿ���ƽ���ϻ���ƽ��ƽ��
		double  det =  glm::dot( AB , S1); //detΪ����t��v��u�ķ�ĸ

		glm::dvec3  S; //T;//����SΪ����A��ʸ��
		if (det > 0)
		{
			S = orig - A;
		}
		else
		{
			S = A - orig;
			det = -det;
		}

		// ���det�ӽ���0��˵��p������ֱ��������ƽ�棬p������dir������V2toV0�Ĳ�����
		//               ��������dir������������ƽ���ϣ��󽻵�û���κ�����
		if (det < 0.00001f)
			return false;
		
		double t = 0, v = 0,u = 0;

		// Calculate u and make sure u <= 1
		v = glm::dot(S , S1);
		if (v < 0.0f || v > det)
			return false;

		//  S2
		glm::dvec3  S2 = glm::cross(S , AB ); 

		// Calculate v and make sure u + v <= 1
		u = glm::dot(dir , S2);
		if (u < 0.0f || u + v > det)
			return false;

		// Calculate t, scale parameters, ray intersects triangle
		t = glm::dot(AC,  S2);

		float fInvDet = 1.0f / det;
		
		t *= fInvDet;
		u *= fInvDet;
		v *= fInvDet;

		outPot = orig + dir*t;  //����ֱ�߷��̼���
		//����  outPot = (1-u-v)*v0 + u*v1 + v*v2;//���������η��̼���

		return true;
	}

#ifdef __USE_OWNED_VECTOR
#undef glm    OSG
#undef dvec3  vec3
#endif
#ifdef __cplusplus
}
#endif

#endif
/* 
bool intersect(vec3 origin, vec3 direction, vec3 v0, vec3 v1, vec3 v2, out vec3 point)
{
	vec3 u, v, n;
	vec3 w0, w;
	float r, a, b;

	u = (v1 - v0);
	v = (v2 - v0);
	n = cross(u, v);

	w0 = origin - v0;
	a = -dot(n, w0);
	b = dot(n, direction);

	r = a / b;
	if (r < 0.0 || r > 1.0)
		return false;

	point = origin + r * direction;

	float uu, uv, vv, wu, wv, D;

	uu = dot(u, u);
	uv = dot(u, v);
	vv = dot(v, v);
	w = point - v0;
	wu = dot(w, u);
	wv = dot(w, v);
	D = uv * uv - uu * vv;

	float s, t;

	s = (uv * wv - vv * wu) / D;
	if (s < 0.0 || s > 1.0)
		return false;

	t = (uv * wu - uu * wv) / D;
	if (t < 0.0 || (s + t) > 1.0)
		return false;

	return true;
}
*/