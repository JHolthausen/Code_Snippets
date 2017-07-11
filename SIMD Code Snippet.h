/*
In this function i check against world collisions using CMD
with the use of SIMD instructions i check 4 objects at the same time and apply a null mask the ones that are not relevant to continue the cycle.
*/



void Tank::Tick()
{
	if (!(flags & ACTIVE)) // dead tank
	{
		smoke.xpos = (int)pos.x;
		smoke.ypos = (int)pos.y;

		return smoke.Tick();
	}

	float2 force = Normalize(float2(target->x, target->y) - pos);

	// evade mountain peaks
	for (unsigned int i = 0; i < 4; i++)
	{
		__m128 dx4 = _mm_sub_ps(_mm_set1_ps(pos.x), _peakx[i]);
		__m128 dy4 = _mm_sub_ps(_mm_set1_ps(pos.y), _peaky[i]);
		__m128 sd4 = _mm_mul_ps(_mm_add_ps(_mm_mul_ps(dx4, dx4), _mm_mul_ps(dy4, dy4)), _mm_set1_ps(0.2f));

		__m128 zero4 = _mm_setzero_ps();
		__m128 mask = _mm_cmplt_ps(sd4, _mm_set1_ps(1500.0f));

		union { __m128 forcex4; float forcex[4]; };
		union { __m128 forcey4; float forcey[4]; };

		forcex4 = _mm_setzero_ps();
		forcey4 = _mm_setzero_ps();

		forcex4 = _mm_add_ps(_mm_and_ps(mask, _mm_mul_ps(_mm_mul_ps(dx4, _mm_set1_ps(0.03f)), _mm_div_ps(_peakh[i], sd4))), forcex4);
		forcey4 = _mm_add_ps(_mm_and_ps(mask, _mm_mul_ps(_mm_mul_ps(dy4, _mm_set1_ps(0.03f)), _mm_div_ps(_peakh[i], sd4))), forcey4);

		force += float2(forcex[0], forcey[0]) + float2(forcex[1], forcey[1]) + float2(forcex[2], forcey[2]) + float2(forcex[3], forcey[3]);

		union { __m128 r4; float r[4]; };

		r4 = _mm_and_ps(mask, _mm_sqrt_ps(sd4));

		for (int j = 0; j < 4; j++)
		{
			int x = r[i];
			int y = 0;
			int decisionOver2 = 1 - x;   // Decision criterion divided by 2 evaluated at x=r, y=0

			while (x > y)
			{
				int _peakX = peakx[i * 4 + j];
				int _peakY = peaky[i * 4 + j];
				Pixel pixelColor = 0x000F00;
				game->canvas->AddPlot(x + _peakX, y + _peakY, pixelColor);
				game->canvas->AddPlot(y + _peakX, x + _peakY, pixelColor);
				game->canvas->AddPlot(-x + _peakX, y + _peakY, pixelColor);
				game->canvas->AddPlot(-y + _peakX, x + _peakY, pixelColor);
				game->canvas->AddPlot(-x + _peakX, -y + _peakY, pixelColor);
				game->canvas->AddPlot(-y + _peakX, -x + _peakY, pixelColor);
				game->canvas->AddPlot(x + _peakX, -y + _peakY, pixelColor);
				game->canvas->AddPlot(y + _peakX, -x + _peakY, pixelColor);
				y++;
				if (decisionOver2 <= 0)
				{
					decisionOver2 += 2 * y + 1;   // Change in decision criterion for y -> y+1
				}
				else
				{
					x--;
					decisionOver2 += 2 * (y - x) + 1;   // Change for y -> y+1, x -> x-1
				}
			}
		}
	}

	// evade user dragged line
	if ((flags & P1) && (game->m_LButton))
	{
		float x1 = (float)game->m_DStartX + SCRWIDTH;
		float y1 = (float)game->m_DStartY + SCRHEIGHT;
		float x2 = (float)game->m_MouseX + SCRWIDTH;
		float y2 = (float)game->m_MouseY + SCRHEIGHT;

		float2 N = Normalize(float2(y2 - y1, x1 - x2));
		float dist = Dot(N, pos) - Dot(N, float2(x1, y1));

		if (fabs(dist) < 10)
		{
			if (dist > 0)
			{
				force += 20 * N;
			}
			else
			{
				force -= 20 * N;
			}
		}
	}
	// update speed using accumulated force
	speed += force;
	speed = Normalize(speed);
	pos += speed * maxspeed * 0.5f;

	// shoot, if reloading completed
	if (--reloading >= 0)
	{
		return;
	}

	unsigned int start = 0, end = MAXP1;

	if (flags & P1)
	{
		start = MAXP1, end = MAXP1 + MAXP2;
	}

	RECT RectA = { pos.x - RECTHALFEXTEND, pos.y - RECTHALFEXTEND, pos.x + RECTHALFEXTEND, pos.y + RECTHALFEXTEND };

	for (unsigned int i = start; i < end; i++) if (game->m_Tank[i]->flags & ACTIVE)
	{
		float2 ppsos = game->m_Tank[i]->pos;
		RECT RectB = { ppsos.x - RECTHALFEXTEND, ppsos.y - RECTHALFEXTEND, ppsos.x + RECTHALFEXTEND, ppsos.y + RECTHALFEXTEND };

		if (!(RectA.left < ppsos.x && RectA.right > ppsos.x &&
			RectA.top < ppsos.y && RectA.bottom > ppsos.y))
		{
			continue;
		}

		float2 d = game->m_Tank[i]->pos - pos;
		if (sqrtf(d.x * d.x + d.y * d.y) < RECTHALFEXTEND
			&& (Dot(Normalize(d), speed) > 0.99999f))
		{
			Fire(flags & (P1 | P2), pos, speed); // shoot
			reloading = reloadTime; // and wait before next shot is ready
			break;
		}
	}
}
