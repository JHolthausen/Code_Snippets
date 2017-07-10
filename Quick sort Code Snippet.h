//-----------------------------------------------------------
// Game::Sort
// Sort the dots according to depth
//-----------------------------------------------------------

//////////////////////////////////////////////////////////////////////////

void Game::QuickSort(float3* array, int startIndex, int endIndex)
{
	float3 pivot = array[startIndex];          
	int splitPoint;

	if (endIndex > startIndex)
	{
		splitPoint = SplitArray(array, pivot, startIndex, endIndex);
		array[splitPoint] = pivot;

		QuickSort(array, startIndex, splitPoint - 1);
		QuickSort(array, splitPoint + 1, endIndex);
	}
}

int Game::SplitArray(float3* array, float3 pivot, int startIndex, int endIndex)
{
	int leftBoundary = startIndex;
	int rightBoundary = endIndex;

	while (leftBoundary < rightBoundary)           

	{
		while (pivot.z < array[rightBoundary].z	&& rightBoundary > leftBoundary)
		{
			rightBoundary--;
		}

		swap(array[leftBoundary], array[rightBoundary]);

		while (pivot.z >= array[leftBoundary].z && leftBoundary < rightBoundary)  
		{
			leftBoundary++;
		}

		swap(array[rightBoundary], array[leftBoundary]);
	}

	return leftBoundary;                            
}

void Game::swap(float3 &a, float3 &b)
{
	float3 temp;

	temp = a;
	a = b;
	b = temp;
}