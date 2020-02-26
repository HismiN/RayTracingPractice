#include <Dxlib.h>
#include <math.h>
#include <vector>
#include <algorithm>

const int ScreenX = 600;			// �X�N���[��X�T�C�Y
const int ScreenY = 600;			// �X�N���[��Y�T�C�Y
const float tileSize = 25;			// ���̃^�C���T�C�Y
const float RayMaxDistance = 3000;	// ���C�̍Œ�����

/// �v�Z�p�x�N�g���\����
// float2�̉�i�F���Ƃ��j
struct float2
{
	float x;
	float y;
	float2() {
		x = 0; y = 0;
	};
	float2(float x, float y) {
		this->x = x;
		this->y = y;
	};
	float2(float xy) {
		this->x = xy;
		this->y = xy;
	};
};

// float3�̉�i�F���Ƃ��j
struct float3
{
	float x;
	float y;
	float z;
	float3() {
		x = 0; y = 0; z = 0;
	};
	float3(float x, float y, float z) {
		this->x = x;
		this->y = y;
		this->z = z;
	};
	float3(float xyz) {
		this->x = xyz;
		this->y = xyz;
		this->z = xyz;
	};
	float3 operator-()
	{
		return float3(x * -1, y * -1, z * -1);
	}
	void operator+=(float3 in)
	{
		x += in.x; y += in.y; z += in.z;
	}
	void operator*=(float in)
	{
		x *= in; y *= in; z *= in;
	}

	// �傫����Ԃ�
	float Magnitude()
	{
		return sqrt(x*x + y * y + z * z);
	}
	// ���K�������x�N�g����Ԃ�
	void Normalize() 
	{
		float mag = Magnitude();

		this->x /= mag;
		this->y /= mag;
		this->z /= mag;
	};
	// �͈͓��ɃN�����v����
	void Clamp(float _max = 1,float _min = 0)
	{
		this->x = max(min(this->x, _max), _min);
		this->y = max(min(this->y, _max), _min);
		this->z = max(min(this->z, _max), _min);
	}
};

// float4�̉�
struct float4
{
	float x;
	float y;
	float z;
	float w;
	float4() {
		x = 0; y = 0; z = 0; w = 0;
	};
	float4(float x, float y, float z, float w) {
		this->x = x; 
		this->y = y;
		this->z = z;
		this->w = w;
	};
	float4(float3 xyz, float w) {
		this->x = xyz.x;
		this->y = xyz.y;
		this->z = xyz.z;
		this->w = w;
	};
	float4(float xyzw) {
		this->x = xyzw;
		this->y = xyzw;
		this->z = xyzw;
		this->w = xyzw;
	};
};


// float2���m�̌v�Z
float2 operator+(const float2& a, const float2& b)
{
	float2 ret = float2(a.x + b.x, a.y + b.y);
	return ret;
};
float2 operator-(const float2& a, const float2& b)
{
	float2 ret = float2(a.x - b.x, a.y - b.y);
	return ret;
};

// float3���m�̌v�Z
float3 operator+(const float3& a, const float3& b)
{
	float3 ret = float3(a.x + b.x, a.y + b.y, a.z + b.z);
	return ret;
};
float3 operator-(const float3& a, const float3& b)
{
	float3 ret = float3(a.x - b.x, a.y - b.y, a.z - b.z);
	return ret;
};
float3 operator*(const float3& a, const float3& b)
{
	float3 ret = float3(a.x * b.x, a.y * b.y, a.z * b.z);
	return ret;
};
///���ς�Ԃ�
float Dot(const float3& a, const float3& b)
{
	return a.x*b.x + a.y*b.y + a.z*b.z;
};

// ���˃x�N�g���擾�֐�
float3 Reflect(float3 ray, float3 normal)
{
	return ray - normal * 2 * (Dot(ray, normal));
}

// �͈͓��ɒl�����߂�֐�
float Clamp(float val ,float _min = 0,float _max = 1)
{
	return max(min(val, _max),_min);
}

/// �I�u�W�F�N�g�\����
// ��
struct Sphere
{
	float rad;// ���a
	float3 pos;// ���W
	float3 albedo;// �{�[���̐F
	Sphere() { rad = 100; pos = float3(); albedo = float3(1, 1, 1); };
	Sphere(float rad) { this->rad = rad; pos = float3(); albedo = float3(1, 1, 1); };
	Sphere(float rad,float3 pos) { this->rad = rad; this->pos = pos; albedo = float3(1, 1, 1); };
	Sphere(float rad,float3 pos, float3 albedo) { this->rad = rad; this->pos = pos; this->albedo = albedo; };
};
// ��
struct Plane
{
	float offset;// �����ǂ̂��炢�����Ă���(����ł���)�̂�
	float3 normal;// �@��(���K���ς�)
	float3 albedo = float3(1);// ���̐F
	Plane() { offset = 0; normal = float3(0,1,0); };
	Plane(float offset) { this->offset = offset; normal = float3(0,1,0); };
	Plane(float offset, float3 normal) { this->offset = offset; this->normal = normal; };
	Plane(float offset, float3 normal,float3 albedo) { this->offset = offset; this->normal = normal; this->albedo = albedo;	};
};


// ���Ƃ̓����蔻��A���������ꏊ�̐F��Ԃ��B
bool isHitSphere(float3& retCol, float3& hitPos, float3& normal, const float3& eye, const float3& ray, float3& light, const Sphere& sph)
{
	// ���̂̒��S�_�܂ł̃x�N�g��
	float3 toCenter(sph.pos - eye);

	// ���_���琂���܂ł̃x�N�g��
	float3 RayVec(ray*Dot(toCenter,ray));

	// ����
	float line = (toCenter - RayVec).Magnitude();

	// �����̒��������a���Z����Γ������Ă���
	if (sph.rad >= line)
	{
		// ���̂ɓ��荞�񂾃��C�̒������擾����
		float sbRay = sqrt(sph.rad * sph.rad - line * line);

		// ���_���瓖�������ꏊ�܂ł̋������Z�o
		float distance = Dot(toCenter, ray) - sbRay;
		
		// �����x�N�g���Ǝ��_�Ƌ������瓖�����Ă�����W���擾
		hitPos = eye + ray * distance;

		// �@�����o���Đ��K������
		normal = hitPos - sph.pos;
		normal.Normalize();

		// ���C�g�Ɩ@������e�̋������擾
		float shadow = Dot(light, normal);
		shadow = Clamp(shadow);

		// ���C�g�̔��˃x�N�g�����쐬
		float3 refVec = Reflect(light,normal);

		// �����̋t�x�N�g�����쐬
		float3 reRay = -float3(ray.x, ray.y, ray.z);

		// ���C�g�̔��˃x�N�g���Ǝ����̋t�x�N�g�����狾�ʔ��˂̋������v�Z
		float specular = Dot(refVec, reRay);
		specular = pow(specular,20);
				
		// �{�[���̐F�����
		retCol = sph.albedo *shadow;
		if (shadow > 0)
		{
			retCol += specular;// ���ʔ��ːF�����Z
		}

		retCol.Clamp();// �l�O�̐F�ɂȂ�Ȃ��悤�N�����v����
		return true;
	}

	return false;
}

// ���Ƃ̓����蔻��A���������ꏊ�̐F��Ԃ��B
bool isHitPlane(float3& retCol,float3 & hitPos, float3 & normal, const float3& eye, float3& ray, float3& light, const Plane& plane)
{
	// �����x�N�g���Ə��̖@���x�N�g���̓��ς��Ƃ�
	float d = Dot(ray, plane.normal);

	// ���_�Ɩ@���x�N�g���̓��ς��Ƃ鎖�ō������擾����
	float hight = Dot(eye, plane.normal);

	// �������Z�o
	float distance = ((plane.offset - hight) / d);
	
	// ������0���傫����Γ������Ă���
	if (distance >= 0)
	{
		//���������ꏊ�̍��W���擾
		hitPos = eye + distance * ray;

		// ���̒��F
		if (cos(hitPos.x / tileSize) * sin(hitPos.z / tileSize) >= 0)
		{
			retCol = plane.albedo;
		}
		else
		{
			retCol = 1-plane.albedo;
		}

		// ������3000�ȏ㗣��Ă��鏰�͕\�����Ȃ�
		if (distance >= RayMaxDistance)
		{
			return false;
		}
		return true;
	}
	return false;
}

// ���C�g���[�V���O�{��
void RayTracing(const float3& eye,float3& light, const std::vector<Sphere>& sph,const const std::vector<Plane>& plane)
{
	// �X�N���[���s�N�Z���������[�v����
	for (int y = 0; y < ScreenY; ++y)
	{
		for (int x = 0; x < ScreenX; ++x)
		{
			bool isHit = false;
			// ���S���_�̃X�N���[�����W���擾
			float3 screenPos(x- ScreenX/2, -y+ ScreenY/2, 0);

			// �Ԃ�l�p
			float3 color;// �ŏI�`��F
			float3 hitPos;// ���C�����������ꏊ�̍��W
			float3 normal;// �@��

			// ���C�x�N�g���̐���
			float3 ray = screenPos - eye;
			ray.Normalize();// ���K��
							
			// ���Ƃ̓�����
			for (auto& p : plane)
			{
				float3 checkPos = eye;
				float3 tmpCol = float3();
				if (isHitPlane(tmpCol, checkPos, normal, eye, ray, light, p))
				{
					hitPos = checkPos;
					color = tmpCol;
					/*if (checkPos.z <= hitPos.z)
					{
					}
					else
					{
						continue;
					}*/
					isHit = true;
					
					// �e�̕`��
					for (auto& s : sph)
					{
						auto c = float3(0, 0, 0);
						auto po = float3(0, 0, 0);

						if (isHitSphere(c, po, normal, hitPos, Reflect(ray,p.normal), light, s))
						{
							color = color * c;
						}
						// �����蔻��̎擾
						if (isHitSphere(c, po, normal, hitPos, -light, light, s))
						{
							color *= 0.5;
							break;
						}
					}
				}
			}

			// ���̂Ƃ̓�����
			for (auto& s: sph)
			{
				// �����蔻��̎擾
				if (isHitSphere(color, hitPos, normal, eye, ray, light, s))
				{
					isHit = true;
					auto addCol = float3(0, 0, 0);// ���Z����F������ϐ�
					auto RefVec = Reflect(ray, normal);// �����̔��˃x�N�g��

					/// �ŏ��ɓ����������C�̍��W���甽�˃x�N�g�����΂��ăI�u�W�F�N�g�Ƃ̓����蔻�������Ĕ��˂�����
					// ���Ƃ̔���
					for (auto& p : plane)
					{
						if (isHitPlane(addCol, hitPos, normal, hitPos, RefVec, light, p))
						{
							color = color * addCol;
						}
					}

					// ���̓��m�̔���
					for (auto& rs : sph)
					{
						float3 hp;
						float3 n;
						// �������g�͂͂���
						if ((rs.pos - s.pos).Magnitude() <= 0)
						{
							continue;
						}

						// ���ϒl���[���ȉ��̎��͔��˕������t�Ȃ̂ŕ`�悵�Ȃ�
						if (Dot(s.pos - hitPos, s.pos - rs.pos) > 0)
						{
							if (isHitSphere(addCol, hp, n, hitPos, RefVec, light, rs))
							{
								float3 hp2;
								float3 n2;
								auto RefVec2 = Reflect(RefVec, n);// �����̔��˃x�N�g��
								auto addCol2 = float3(0, 0, 0);// ���Z����F������ϐ�
								// ���Ƃ̓�����
								for (auto& p : plane)
								{
									if (isHitPlane(addCol2, hp2, n2, hp, RefVec2, light, p))
									{
										addCol = addCol * addCol2;
									}
								}
								color = color * addCol;
							}
						}
					}
				}				
			}

			if (!isHit)
			{
				// ���C���������ĂȂ������Ƃ��͍��F�ɂ���
				color = float3(0.8,1,0.8);
			}

			// �ŏI�I�ɓ����F�Ńs�N�Z���Ƀh�b�g��ł�
			DrawPixel(x, y, GetColor(color.x * 0xff, color.y * 0xff, color.z * 0xff));
		}
	}
}

// ���C�����[�v
int WINAPI WinMain(HINSTANCE hInstance,	HINSTANCE hPrevInstance,LPSTR lpCmdLine,int nCmdShow)
{
	// Dxlib�̏�����
	SetOutApplicationLogValidFlag(false);
	SetGraphMode(ScreenX, ScreenY, 32);
	ChangeWindowMode(true);
	SetMainWindowText("RayTracingPractice");
	DxLib_Init();

	// ���C�g�x�N�g���쐬
	float3 light = float3(-1,2,2);
	light.Normalize();
	// ���_
	float3 eye(0,0,500);

	// ���̂̔z��
	std::vector<Sphere> spheres = {
		Sphere(100,float3(140,50,-50),float3(0.8,0.5,0.5)),
		Sphere(100,float3(-140,30,0),float3(0.5,1,0.5)),
		Sphere(100,float3(0,0,-250),float3(1,1,0.5)),
	};

	// Z���W�Ń\�[�g���鎮
	auto sorting = [](Sphere a, Sphere b) {
		return (a.pos.z+a.rad) < (b.pos.z+b.rad);
	};

	// �\�[�g����
	std::sort(spheres.begin(), spheres.end(),sorting);

	// ���̔z��
	const std::vector<Plane> planes = {
		Plane(-100, float3(0, 1, 0),float3(1,0.75,0.25)),
	};

	// ���C�g���[�V���O
	RayTracing(eye, light, spheres, planes);

	// ���͑҂���Ԃɂ��Ă���
	WaitKey();

	// Dxlib�̏I��
	DxLib_End();
	return 0;
}