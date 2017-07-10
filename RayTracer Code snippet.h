#include "template.h"
#include "raytracer.h"

#include "BVH.h"
#include "Primitive.h"

#include <thread>
#include <mutex>


struct Tmpl8::RayTracerThreads
{
	std::thread threads[WORKING_THREADS];
	std::mutex taskObtainLock;
};

static void RayTraceLineWorker(void* rayTracer);

Raytracer::Raytracer()
	: scene(0)
{
	threads = nullptr;
	running = false;
	skySphere = nullptr;
	primitiveCount = 0;
	primitives = nullptr;
    bvhManager = nullptr;

	bufferArray.resize(SCRHEIGHT);
	for (int i = 0; i < bufferArray.size(); i++)
	{
		bufferArray[i].resize(SCRWIDTH);
	}
	clearBufferArray();
}

void Raytracer::clearBufferArray()
{
	for (int i = 0; i < bufferArray.size(); i++)
	{
		for (int j = 0; j < bufferArray[i].size(); j++)
		{
			bufferArray[i][j] = { 0, 0, 0, 0 };
		}
	}

}

Raytracer::~Raytracer()
{
    // WARNING: Stop threads first before cleaning up any other data
	running = false;
	if(threads != nullptr)
	{
		for(int i = 0; i < WORKING_THREADS; i++)
		{
			threads->threads[i].join();
		}

		delete threads;
		threads = nullptr;
	}

    // Clean up
    if (bvhManager != nullptr)
        delete bvhManager;
    bvhManager = nullptr;

    if (primitives != nullptr)
        _aligned_free(primitives);
    primitives = nullptr;

	if(skySphere != nullptr)
		delete skySphere;
	skySphere = nullptr;
}

void Raytracer::Init(Surface* screen, Scene* scene)
{
	Mesh::screen = screen;
	this->screen = screen;
	this->scene = scene;

	rayLineTasks = -1;
	skySphere = new Surface("assets/black.png");

	threads = new RayTracerThreads();
	bvhManager = new BVHManager();

	debugBVHDepth = false;
	tmpindex = -1;

    lastRenderCompleted = GetATime();
    deltaRenderCompletion = 0.0f;

	running = true;
	for(int i = 0; i < WORKING_THREADS; i++)
	{
		threads->threads[i] = thread(RayTraceLineWorker, this);
	}

}

void Raytracer::Render(float tmpTheta)
{
	// Check if all the tasks have been completed
	if(rayLineTasks == -1 || rayLineTasks == SCRHEIGHT - 1)
	{
		// Refresh camera properties
		scene->cam.PrepareForRaytracing();
		p0 = scene->cam.p0;
		p1 = scene->cam.p1;
		p2 = scene->cam.p2;
		camPos = scene->cam.GetPosition();
		this->tmpTheta = tmpTheta;

        // Timer
        LONGLONG curTime = GetATime();
        deltaRenderCompletion = (float)((curTime - lastRenderCompleted) / 1000000.0);
        lastRenderCompleted = curTime;

		// Start next tasks
		rayLineTasks = 0;
		framecount++;
	}

	// Sleep a little so we don't use all the computers processing power
	Sleep(30);
}


void Raytracer::TraceLine(int y, RayData& data)
{
	Pixel* buf = screen->GetBuffer();
	Pixel color;

	float dx = 1.f / SCRWIDTH;
	float dy = 1.f / SCRHEIGHT;

	buf += y * SCRWIDTH;
	for(int x = 0; x < SCRWIDTH; x++)
	{
		vec3 p = p0 + static_cast<float>(x) * (p1 - p0) * dx +
			static_cast<float>(y) * (p2 - p0) * dy;
		data.ray.O = camPos;
		data.ray.D = normalize(p - camPos);

		color = 0x87cefa;

		if(debugBVHDepth)
		{
			data.ray.t = 0.0f;
			data.ray.rD = 1.0f / data.ray.D;
			data.ray.o4 = _mm_set_ps(0.0f, data.ray.O.z, data.ray.O.y, data.ray.O.x);
			data.ray.rD4 = _mm_set_ps(0.0f, data.ray.rD.z, data.ray.rD.y, data.ray.rD.x);
			bvhManager->TraverseDepth(tmpindex, bvhManager->root, data.ray);

			if(data.ray.t != 0.0f)
			{
				unsigned long scale = (unsigned long)(data.ray.t * 1.0f);
				if(scale > 255) scale = 255;
				unsigned char f = (unsigned char)(scale);
				Pixel col = (f << 24) | (f << 16) | (f << 8) | (f);
				color = col;
			}
		}
		else
		{
			// Init iteration count
			data.iter = 0;
			// Cast primary ray
			vec4 hdrCol = ptTrace(data, 1);
			//CastRay(data);
			hdrCol = clamp(hdrCol, vec4{0.f}, vec4{255.f});

			bufferArray[y][x] += hdrCol;

			color = ColorToPixel(bufferArray[y][x] / framecount);
		}
		
		// Apply calculated color
		*buf = color;

		++buf;
	}
}

void Raytracer::TraceLineBasic(int y, RayData& data)
{
	Pixel* buf = screen->GetBuffer();
	Pixel color;

	float dx = 1.f / SCRWIDTH;
	float dy = 1.f / SCRHEIGHT;

	buf += y * SCRWIDTH;

	for(int x = 0; x < SCRWIDTH; x++)
	{
		vec3 p = p0 + static_cast<float>(x) * (p1 - p0) * dx +
			static_cast<float>(y) * (p2 - p0) * dy;
		data.ray.O = camPos;
		data.ray.D = normalize(p - camPos);

		color = 0x87cefa;

		if(debugBVHDepth)
		{
			data.ray.t = 0.0f;
			data.ray.rD = 1.0f / data.ray.D;
			data.ray.o4 = _mm_set_ps(0.0f, data.ray.O.z, data.ray.O.y, data.ray.O.x);
			data.ray.rD4 = _mm_set_ps(0.0f, data.ray.rD.z, data.ray.rD.y, data.ray.rD.x);
			bvhManager->TraverseDepth(tmpindex, bvhManager->root, data.ray);

			if(data.ray.t != 0.0f)
			{
				unsigned long scale = (unsigned long)(data.ray.t * 1.0f);
				if(scale > 255) scale = 255;
				unsigned char f = (unsigned char)(scale);
				Pixel col = (f << 24) | (f << 16) | (f << 8) | (f);
				color = col;
			}
		}
		else
		{
			IntersectScene(data.ray);
			data.iter++;
			if(data.ray.hit)
			{
				// Find Material, HitNormal, and uv coords of hit
				CalcHitData(data);
				vec4 hdrCol = CalcShadedColor(data);
				hdrCol = clamp(hdrCol, vec4{0.f}, vec4{255.f});
				color = ColorToPixel(hdrCol);
			}
		}

		DisplayAreaLight(data.ray, color);

		// Apply calculated color
		*buf = color;

		++buf;
	}
}

vec4 Raytracer::SampleSky(Ray& ray)
{
	Surface* texture = skySphere;
	vec2 uv = GetUVPoints(-ray.D, 0.0f);
	int uvx = (int)(uv.x * texture->GetWidth()) % texture->GetWidth();
	if(uvx < 0) uvx = (texture->GetWidth() + uvx);
	int uvy = (int)(uv.y * texture->GetHeight()) % texture->GetHeight();
	if(uvy < 0) uvy = (texture->GetHeight() + uvy);
	return PixelToColor(texture->GetBuffer()[uvx + uvy * texture->GetWidth()]);
}

vec4 Raytracer::CalcShadedColor(RayData& data)
{
	vec4 c = data.hit.mat->CalcDiffuse(data.hit.uv);

	// test here if color is (nearly) black for an early out
	if (c.r < 0.001f && c.g < 0.001f && c.b < 0.001f)
	{
		return vec4{0.f};
	}
	
	vec4 lightCol{0.f};
	ApplyLights(data, lightCol);
	return c * lightCol;
}

void Raytracer::ApplyLights(RayData& data, vec4& color)
{
	ProcessPointLights(data, color);
	ProcessAreaLights(data, color);
}

void Raytracer::ProcessAreaLights(RayData& data, vec4& light)
{

	size_t len = scene->areaLights.size();
	AreaLight* l;
	
	for (size_t i = 0; i < len; ++i)
	{
		l = &scene->areaLights[i];
		vec3 _L ;// l->worldPos;
		vec3 _P = data.hit.hitPoint;
		
		vec3 rp;
		float s = (rand() % 36000) / 100;
		float t = (rand() % 36000) / 100;
		s *= PI / 180;
		t *= PI / 180;

		_L = vec3(
			l->worldPos.x + l->radius * cos(s) * sin(t),
			l->worldPos.y + l->radius * sin(s) * sin(t),
			l->worldPos.z + l->radius * cos(t));
		
		vec3 L = rp - data.hit.hitPoint;
		float d = dot(data.hit.hitNormal, L);
		float _dist = length(_L - _P);
		
		Ray shadowRay;
		shadowRay.D = normalize(_L - _P);
		shadowRay.O = data.hit.hitPoint + (data.hit.hitNormal * 0.001f);
		shadowRay.hit = nullptr;
		shadowRay.rD = 1.f / shadowRay.D;

		IntersectScene(shadowRay);
		if (shadowRay.hit == nullptr || shadowRay.t > _dist)
		{
			light +=  l->diffuseColor / ( _dist*_dist * 255.f) ;

		}
	}
}

void Raytracer::ProcessPointLights(RayData& data, vec4& lightColor)
{
	Ray shadowRay;
	PointLight* l;
	size_t len = scene->pointLights.size();
	for(size_t i = 0; i < len; ++i)
	{
		l = &scene->pointLights[i];

		// Light distance and UNnormalized dir
		vec3 L = l->worldPos - data.hit.hitPoint;
		float d = dot(data.hit.hitNormal, L);

		// If dot(normal, L) <= 0, the light can never reach this point
		// So this point cannot receive light, or shadow. Go to next light
		if(d <= 0 || (l->diffuseColor.r < 0.001f && l->diffuseColor.g < 0.001f && l->diffuseColor.b < 0.001f))
		{
			// Normal is facing away from the light. It cannot receive lighting
			continue;
		}

		// Light could reach this point, but may still be occluded
		float dist = length(L);
		L /= dist;

		// Check if we receive a shadow
		shadowRay.O = l->worldPos;
		shadowRay.D = -L;
		shadowRay.t = dist - 0.001f;
		shadowRay.hit = nullptr;
		shadowRay.rD = 1.f / shadowRay.D;
		shadowRay.o4 = _mm_set_ps(0.0f, shadowRay.O.z, shadowRay.O.y, shadowRay.O.x);
		shadowRay.rD4 = _mm_set_ps(0.0f, shadowRay.rD.z, shadowRay.rD.y, shadowRay.rD.x);
		bvhManager->IsOccluded(bvhManager->root, shadowRay);

		if(shadowRay.energy > 0.001f)
		{
			d /= dist;	// d used a non-normalized normal. apply normalization now

			vec3 R = L - 2.f * d * data.hit.hitNormal;
			float beta = 0.25f * 40.f;	// shininess
			float lambda = 1.f - dot(R, data.ray.D);
			float spec = max(0.f, 1.f - beta * lambda);
			spec = spec * spec * spec * spec;	// gamma = 4. Alternatively, use 8

			lightColor += (d + 2.f * spec) * shadowRay.energy * l->diffuseColor / (255.f * (l->k0 + l->k1 * dist + l->k2 * dist * dist));
		}
	}
}

vec4 Raytracer::CastRay(RayData& data)
{
	IntersectScene(data.ray);
	data.iter++;
	if(data.ray.hit)
	{
		if(data.iter > MAX_REFLECTION_BOUNCES + 1)
		{
			return vec4{0.f};
		}

		// Find Material, HitNormal, and uv coords of hit
		CalcHitData(data);

		float reflectivity = data.hit.mat->reflectivity;
		float transparency = data.hit.mat->transparency;
		vec4 reflectColor{0.f};
		vec4 refractColor{0.f};
		vec4 selfColor{0.f};

		// Calculate selfColor BEFORE casting to reflection and refraction,
		//	because 'data' is modified there
		if(reflectivity < 0.999f || transparency < 0.999f)
		{
			selfColor = CalcShadedColor(data);
		}

		// Check for reflection
		if(reflectivity > 0.001f && dot(data.ray.D, data.hit.hitNormal) < 0)
		{
			reflectColor = Reflect(data);
			selfColor *= (1.f - reflectivity);
		}

		if(transparency > 0.001f)
		{
			refractColor = Refract(data);
			selfColor *= (1.f - transparency);
		}

		return selfColor + reflectColor + refractColor;
	}

	// No hit. Terminate with skysphere
	return SampleSky(data.ray);
}


/////////--------------------
// DEBUG RELATED
/////////--------------------------------

void Raytracer::DrawDebugLine(vec3& worldPosA, vec3& worldPosB, Pixel color)
{
	Camera& c = scene->cam;
	vec3 O = c.GetPosition();
	vec3 f = c.GetForward();
	vec3 u = c.GetUp();
	vec3 r = c.GetRight();
	vec3 dA = normalize(worldPosA - O);
	vec3 dB = normalize(worldPosB - O);
	dA *= (c.d / dot(dA, f));
	dB *= (c.d / dot(dB, f));
	float xA = 0.5f * SCRWIDTH * (dot(dA, r) / c.aspectRatio + 1.f);
	float yA = 0.5f * SCRHEIGHT * (1.f - dot(dA, u));
	float xB = 0.5f * SCRWIDTH * (dot(dB, r) / c.aspectRatio + 1.f);
	float yB = 0.5f * SCRHEIGHT * (1.f - dot(dB, u));
	screen->Line(xA, yA, xB, yB, color);
}

void Raytracer::DisplayAreaLight(Ray& ray, Pixel& color)
{
	ray.t = 9999999.f;
	ray.hit = nullptr;

	for(AreaLight& l : scene->areaLights)
	{
		vec3 c = l.worldPos - ray.O;
		float t = dot(c, ray.D);
		vec3 q = c - t * ray.D;
		float p2 = dot(q, q);
		float r2 = l.radius * l.radius;
		if(p2 <= r2)
		{
			t -= sqrtf(r2 - p2);
			if((t < ray.t) && (t > 0))
			{
				ray.t = t;
				color = ColorToPixel(l.diffuseColor);
			}
		}
	}
}

/////////--------------------
// Path trace 
/////////--------------------------------

vec4 Raytracer::ptLights(RayData& data)
{
	//pick a random light target
	int a = 1;
	int b = scene->pointLights.size() > 0? rand() % scene->pointLights.size() : 0;
	int c = rand() % (a+b);
	
	Ray shadowRay;
	
	//pick arealight
	if (c <= a)
	{
		//pick arealight
		AreaLight* al = &scene->areaLights[a-1];
		vec3 target = al->rayPoints[rand() % al->rayPoints.size()];

		float s = (rand() % 36000) / 100;
		float t = (rand() % 36000) / 100;
		s *= PI / 180;
		t *= PI / 180;
	
		target = vec3(
			al->worldPos.x + al->radius * cos(s) * sin(t),
			al->worldPos.y + al->radius * sin(s) * sin(t),
			al->worldPos.z + al->radius * cos(t));
			

		//pick random point on arealight
		vec3 _L = target;
		vec3 _P = data.hit.hitPoint;
		float _dist = length(_L - _P);

		Ray shadowRay;
		shadowRay.D = normalize(_L - _P);
		shadowRay.O = data.hit.hitPoint + (data.hit.hitNormal * 0.001f);
		shadowRay.hit = nullptr;
		shadowRay.rD = 1.f / shadowRay.D;

		IntersectScene(shadowRay);
		if (shadowRay.hit == nullptr || shadowRay.t > _dist)
		{
			return  (al->diffuseColor / (_dist * _dist)) * (float) (a + b);

		}
	}
	return{0, 0, 0, 0 };
}

bool Raytracer::ptIntersectLights(Ray& ray)
{
	ray.t = 9999999.f;
	ray.hit = nullptr;
	ray.rD = 1.f / ray.D;
	ray.o4 = _mm_set_ps(0.0f, ray.O.z, ray.O.y, ray.O.x);
	ray.rD4 = _mm_set_ps(0.0f, ray.rD.z, ray.rD.y, ray.rD.x);

	for (AreaLight al : scene->areaLights)
	{
		float a = length(ray.D*ray.D);
		float b = length(ray.D * (2.0f * (ray.D - al.worldPos)));
		float c = length(al.worldPos*al.worldPos) + length(ray.O*ray.O) - 2.0f*length(ray.O*al.worldPos) - al.radius*al.radius;
		float D = b*b + (-4.0f)*a*c;

		if (D >= 0)
		{
			return true;
		}
	}
	return false;
}


vec4 Raytracer::ptTrace(RayData& data, int depth)
{	
	vec4 color = { 0, 0, 0, 0 };
	
	if (depth == MAXDEPTH) {
		return color;
	}

	//hit a object?
	IntersectScene(data.ray);
	if (data.ray.hit == nullptr) 
	{
		return color;  // Nothing was hit.
	}
	
	CalcHitData(data);
	//cast shadow ray


	Material* m = data.ray.hit->mat;
	color += ptLights(data);
	// Pick a random direction from here and keep going.
	Ray newRay;
	newRay.O = data.hit.hitPoint;
	newRay.D = ptRandomUnitVectorInHemisphereOf(data.hit.hitNormal);  // This is NOT a cosine-weighted distribution!

	// Compute the BRDF for this ray (assuming Lambertian reflection)
	float cos_theta = dot(newRay.D, data.hit.hitNormal);
	vec4 BRDF =  m->diffuseColor * cos_theta;
	RayData rd;
	rd.ray = newRay;
	vec4 reflected = ptTrace(rd, depth + 1);
	color += reflected;// / (_dist * _dist));
	
	color = clamp(color, vec4{ 0.f }, vec4{ 255.f });
	vec4 emittance = data.hit.mat->CalcDiffuse(data.hit.uv) *(color / 255.f);
	

	// Apply the Rendering Equation here.pt
	return emittance;
}

//this will always provide an even distribution along the hemisphere
inline vec3 Raytracer::ptRandomUnitVectorInHemisphereOf(vec3 n)
{
	vec3 R;

	while (1)
	{
		R = vec3(
			Rand(2) - 1,
			Rand(2) - 1,
			Rand(1));
		if (length(R) < 1) break;
	}

	return  normalize(dot(n, R) > 0.f ? R : -R);
}

