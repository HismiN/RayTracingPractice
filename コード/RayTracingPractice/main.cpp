#include <Dxlib.h>
#include <math.h>
#include <vector>
#include <algorithm>

const int ScreenX = 600;			// スクリーンXサイズ
const int ScreenY = 600;			// スクリーンYサイズ
const float tileSize = 25;			// 床のタイルサイズ
const float RayMaxDistance = 3000;	// レイの最長距離

/// 計算用ベクトル構造体
// float2つの塊（色情報とか）
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

// float3つの塊（色情報とか）
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

	// 大きさを返す
	float Magnitude()
	{
		return sqrt(x*x + y * y + z * z);
	}
	// 正規化したベクトルを返す
	void Normalize() 
	{
		float mag = Magnitude();

		this->x /= mag;
		this->y /= mag;
		this->z /= mag;
	};
	// 範囲内にクランプする
	void Clamp(float _max = 1,float _min = 0)
	{
		this->x = max(min(this->x, _max), _min);
		this->y = max(min(this->y, _max), _min);
		this->z = max(min(this->z, _max), _min);
	}
};

// float4つの塊
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


// float2同士の計算
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

// float3同士の計算
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
///内積を返す
float Dot(const float3& a, const float3& b)
{
	return a.x*b.x + a.y*b.y + a.z*b.z;
};

// 反射ベクトル取得関数
float3 Reflect(float3 ray, float3 normal)
{
	return ray - normal * 2 * (Dot(ray, normal));
}

// 範囲内に値を収める関数
float Clamp(float val ,float _min = 0,float _max = 1)
{
	return max(min(val, _max),_min);
}

/// オブジェクト構造体
// 球
struct Sphere
{
	float rad;// 半径
	float3 pos;// 座標
	float3 albedo;// ボールの色
	Sphere() { rad = 100; pos = float3(); albedo = float3(1, 1, 1); };
	Sphere(float rad) { this->rad = rad; pos = float3(); albedo = float3(1, 1, 1); };
	Sphere(float rad,float3 pos) { this->rad = rad; this->pos = pos; albedo = float3(1, 1, 1); };
	Sphere(float rad,float3 pos, float3 albedo) { this->rad = rad; this->pos = pos; this->albedo = albedo; };
};
// 床
struct Plane
{
	float offset;// 床がどのくらい浮いている(沈んでいる)のか
	float3 normal;// 法線(正規化済み)
	float3 albedo = float3(1);// 床の色
	Plane() { offset = 0; normal = float3(0,1,0); };
	Plane(float offset) { this->offset = offset; normal = float3(0,1,0); };
	Plane(float offset, float3 normal) { this->offset = offset; this->normal = normal; };
	Plane(float offset, float3 normal,float3 albedo) { this->offset = offset; this->normal = normal; this->albedo = albedo;	};
};


// 球との当たり判定、当たった場所の色を返す。
bool isHitSphere(float3& retCol, float3& hitPos, float3& normal, const float3& eye, const float3& ray, float3& light, const Sphere& sph)
{
	// 球体の中心点までのベクトル
	float3 toCenter(sph.pos - eye);

	// 視点から垂線までのベクトル
	float3 RayVec(ray*Dot(toCenter,ray));

	// 垂線
	float line = (toCenter - RayVec).Magnitude();

	// 垂線の長さが半径より短ければ当たっている
	if (sph.rad >= line)
	{
		// 球体に入り込んだレイの長さを取得する
		float sbRay = sqrt(sph.rad * sph.rad - line * line);

		// 視点から当たった場所までの距離を算出
		float distance = Dot(toCenter, ray) - sbRay;
		
		// 視線ベクトルと視点と距離から当たっている座標を取得
		hitPos = eye + ray * distance;

		// 法線を出して正規化する
		normal = hitPos - sph.pos;
		normal.Normalize();

		// ライトと法線から影の強さを取得
		float shadow = Dot(light, normal);
		shadow = Clamp(shadow);

		// ライトの反射ベクトルを作成
		float3 refVec = Reflect(light,normal);

		// 視線の逆ベクトルを作成
		float3 reRay = -float3(ray.x, ray.y, ray.z);

		// ライトの反射ベクトルと視線の逆ベクトルから鏡面反射の強さを計算
		float specular = Dot(refVec, reRay);
		specular = pow(specular,20);
				
		// ボールの色を入魂
		retCol = sph.albedo *shadow;
		if (shadow > 0)
		{
			retCol += specular;// 鏡面反射色を加算
		}

		retCol.Clamp();// 値外の色にならないようクランプする
		return true;
	}

	return false;
}

// 床との当たり判定、当たった場所の色を返す。
bool isHitPlane(float3& retCol,float3 & hitPos, float3 & normal, const float3& eye, float3& ray, float3& light, const Plane& plane)
{
	// 視線ベクトルと床の法線ベクトルの内積をとる
	float d = Dot(ray, plane.normal);

	// 視点と法線ベクトルの内積をとる事で高さを取得する
	float hight = Dot(eye, plane.normal);

	// 距離を算出
	float distance = ((plane.offset - hight) / d);
	
	// 距離が0より大きければ当たっている
	if (distance >= 0)
	{
		//当たった場所の座標を取得
		hitPos = eye + distance * ray;

		// 床の着色
		if (cos(hitPos.x / tileSize) * sin(hitPos.z / tileSize) >= 0)
		{
			retCol = plane.albedo;
		}
		else
		{
			retCol = 1-plane.albedo;
		}

		// 距離が3000以上離れている床は表示しない
		if (distance >= RayMaxDistance)
		{
			return false;
		}
		return true;
	}
	return false;
}

// レイトレーシング本体
void RayTracing(const float3& eye,float3& light, const std::vector<Sphere>& sph,const const std::vector<Plane>& plane)
{
	// スクリーンピクセル数分ループを回す
	for (int y = 0; y < ScreenY; ++y)
	{
		for (int x = 0; x < ScreenX; ++x)
		{
			bool isHit = false;
			// 中心原点のスクリーン座標を取得
			float3 screenPos(x- ScreenX/2, -y+ ScreenY/2, 0);

			// 返り値用
			float3 color;// 最終描画色
			float3 hitPos;// レイが当たった場所の座標
			float3 normal;// 法線

			// レイベクトルの生成
			float3 ray = screenPos - eye;
			ray.Normalize();// 正規化
							
			// 床との当たり
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
					
					// 影の描画
					for (auto& s : sph)
					{
						auto c = float3(0, 0, 0);
						auto po = float3(0, 0, 0);

						if (isHitSphere(c, po, normal, hitPos, Reflect(ray,p.normal), light, s))
						{
							color = color * c;
						}
						// 当たり判定の取得
						if (isHitSphere(c, po, normal, hitPos, -light, light, s))
						{
							color *= 0.5;
							break;
						}
					}
				}
			}

			// 球体との当たり
			for (auto& s: sph)
			{
				// 当たり判定の取得
				if (isHitSphere(color, hitPos, normal, eye, ray, light, s))
				{
					isHit = true;
					auto addCol = float3(0, 0, 0);// 加算する色を入れる変数
					auto RefVec = Reflect(ray, normal);// 視線の反射ベクトル

					/// 最初に当たったレイの座標から反射ベクトルを飛ばしてオブジェクトとの当たり判定を取って反射させる
					// 床との反射
					for (auto& p : plane)
					{
						if (isHitPlane(addCol, hitPos, normal, hitPos, RefVec, light, p))
						{
							color = color * addCol;
						}
					}

					// 球体同士の反射
					for (auto& rs : sph)
					{
						float3 hp;
						float3 n;
						// 自分自身ははじく
						if ((rs.pos - s.pos).Magnitude() <= 0)
						{
							continue;
						}

						// 内積値がゼロ以下の時は反射方向が逆なので描画しない
						if (Dot(s.pos - hitPos, s.pos - rs.pos) > 0)
						{
							if (isHitSphere(addCol, hp, n, hitPos, RefVec, light, rs))
							{
								float3 hp2;
								float3 n2;
								auto RefVec2 = Reflect(RefVec, n);// 視線の反射ベクトル
								auto addCol2 = float3(0, 0, 0);// 加算する色を入れる変数
								// 床との当たり
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
				// レイが当たってなかったときは黒色にする
				color = float3(0.8,1,0.8);
			}

			// 最終的に得た色でピクセルにドットを打つ
			DrawPixel(x, y, GetColor(color.x * 0xff, color.y * 0xff, color.z * 0xff));
		}
	}
}

// メインループ
int WINAPI WinMain(HINSTANCE hInstance,	HINSTANCE hPrevInstance,LPSTR lpCmdLine,int nCmdShow)
{
	// Dxlibの初期化
	SetOutApplicationLogValidFlag(false);
	SetGraphMode(ScreenX, ScreenY, 32);
	ChangeWindowMode(true);
	SetMainWindowText("RayTracingPractice");
	DxLib_Init();

	// ライトベクトル作成
	float3 light = float3(-1,2,2);
	light.Normalize();
	// 視点
	float3 eye(0,0,500);

	// 球体の配列
	std::vector<Sphere> spheres = {
		Sphere(100,float3(140,50,-50),float3(0.8,0.5,0.5)),
		Sphere(100,float3(-140,30,0),float3(0.5,1,0.5)),
		Sphere(100,float3(0,0,-250),float3(1,1,0.5)),
	};

	// Z座標でソートする式
	auto sorting = [](Sphere a, Sphere b) {
		return (a.pos.z+a.rad) < (b.pos.z+b.rad);
	};

	// ソートする
	std::sort(spheres.begin(), spheres.end(),sorting);

	// 床の配列
	const std::vector<Plane> planes = {
		Plane(-100, float3(0, 1, 0),float3(1,0.75,0.25)),
	};

	// レイトレーシング
	RayTracing(eye, light, spheres, planes);

	// 入力待ち状態にしておく
	WaitKey();

	// Dxlibの終了
	DxLib_End();
	return 0;
}