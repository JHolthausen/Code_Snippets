
//this will always provide an even distribution along the hemisphere
inline vec3 Raytracer::RandomUnitVectorInHemisphereOf(vec3 n)
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

vec4 Raytracer::ProcessLight(HitData & data, Ray& ray)
{
	vec4 color = { 0,0,0,0 };
	vec3 target;
	void* l;

	//pick random light
	int a = scene->areaLights.size();
	int b = scene->pointLights.size();

	if (a + b == 0) return{ 0,0,0,0 };

	int c = rand() % (a + b    +1);

	//pick areaLight
	if (a > 0 && c < a)
	{
		AreaLight* al =&scene->areaLights[c];
		//pick random point on area light
		target = al->worldPos + al->radius * RandomUnitVectorInHemisphereOf(normalize(data.hitPoint - al->worldPos));
		color = al->diffuseColor;
		l = al;
	}
	//else pointlight
	else if(b > 0)
	{
		c = c > b ? c - b : c;
		c = c == 0? 1 : c;

		//pick arealight
		PointLight* pl = &scene->pointLights[c-1];
		target = pl->worldPos;
		color = pl->diffuseColor;
		l = pl;
	}
	else
	{
		return{ 0,0,0,0 };
	}

		// Light distance and UNnormalized dir
		vec3 L = target - data.hitPoint;
		float d = dot(data.hitNormal, L);

		// If dot(normal, L) <= 0, the light can never reach this point
		// So this point cannot receive light, or shadow. Go to next light
		if (d <= 0 || (color.r < 0.001f &&color.g < 0.001f && color.b < 0.001f))
		{
			// Normal is facing away from the light. It cannot receive lighting
			return{ 0,0,0,0 } ;
		}

		Ray shadowRay;
		// Light could reach this point, but may still be occluded
		float dist = length(L);
		L /= dist;

		// Check if we receive a shadow
		shadowRay.O = target;
		shadowRay.D = -L;
		shadowRay.t = dist - 0.001f;
		shadowRay.hit = nullptr;
		shadowRay.rD = 1.f / shadowRay.D;
		shadowRay.o4 = _mm_set_ps(0.0f, shadowRay.O.z, shadowRay.O.y, shadowRay.O.x);
		shadowRay.rD4 = _mm_set_ps(0.0f, shadowRay.rD.z, shadowRay.rD.y, shadowRay.rD.x);

		if (!bvhManager->IsOccluded(bvhManager->root, shadowRay))// && !InterSectLight(shadowRay,l))
		{
			d /= dist;	// d used a non-normalized normal. apply normalization now

			vec3 R = L - 2.f * d * data.hitNormal;
			float beta = 0.25f * data.mat->specularityStrength;	// shininess
			float lambda = 1.f - dot(R, ray.D);
			float spec = max(0.f, 1.f - beta * lambda);
			spec = spec * spec * spec * spec;	// gamma = 4. Alternatively, use 8

			return ((d + data.mat->specularityIntensity * spec) *
				color / (dist * dist) *(float) (a+b));
		}
	return{ 0,0,0,0 };
}

bool Raytracer::InterSectLight(Ray& ray, void* light)
{
	ray.t = 9999999.f;
	ray.hit = nullptr;
	ray.rD = 1.f / ray.D;
	ray.o4 = _mm_set_ps(0.0f, ray.O.z, ray.O.y, ray.O.x);
	ray.rD4 = _mm_set_ps(0.0f, ray.rD.z, ray.rD.y, ray.rD.x);

	for (int i =0; i < scene->areaLights.size(); i++)
	{
		AreaLight* al = &scene->areaLights[i];
		if (al == light) continue;

		float a = length(ray.D*ray.D);
		float b = length(ray.D * (2.0f * (ray.D - al->worldPos)));
		float c = length(al->worldPos*al->worldPos) + length(ray.O*ray.O) - 2.0f*length(ray.O*al->worldPos) - al->radius*al->radius;
		float D = b*b + (-4.0f)*a*c;

		if (D >= 0)
		{
			return true;
		}
	}

	for (int i = 0; i < scene->pointLights.size(); i++)
	{
		PointLight* pl = &scene->pointLights[i];
		if (pl == light) continue;

		vec3 a = normalize(pl->worldPos);
		vec3 b = normalize(ray.D);
		float offset = 0.001;
		if ((b.x < a.x + offset) &&
			(b.x > a.x - offset) &&
			(b.y < a.y + offset) &&
			(b.y > a.y - offset) &&
			(b.z < a.z + offset) &&
			(b.z > a.z - offset))
		{
			return true;
		}
		
	}

		return false;
}
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
		return color;// SampleSky(data.ray);;  // Bounced enough times.
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
	color += reflected;
	
	color = clamp(color, vec4{ 0.f }, vec4{ 255.f });
	vec4 emittance = data.hit.mat->CalcDiffuse(data.hit.uv) *(color / 255.f);

	return emittance;
}