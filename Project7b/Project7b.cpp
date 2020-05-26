#include <stdio.h>
#include <omp.h>
#include <iostream>

using namespace std;

#define NUMT	        8

float
SimdMulSum(float* a, float* b, int len);

int main()
{
#ifndef _OPENMP
	fprintf(stderr, "OpenMP is not supported here -- sorry.\n");
	return 1;
#endif

	FILE* fp = fopen("signal.txt", "r");
	if (fp == NULL)
	{
		fprintf(stderr, "Cannot open file 'signal.txt'\n");
		exit(1);
	}
	int Size;
	fscanf(fp, "%d", &Size);
	float* A = new float[2 * Size];
	float* Sums = new float[1 * Size];
	for (int i = 0; i < Size; i++)
	{
		fscanf(fp, "%f", &A[i]);
		A[i + Size] = A[i];		// duplicate the array
	}
	fclose(fp);
	/*
	omp_set_num_threads(NUMT);
	fprintf(stderr, "Using %d threads\n", NUMT);
*/
	double time0 = omp_get_wtime();



	for (int shift = 0; shift < Size; shift++)
	{
		float sum = 0.;
		Sums[shift] = SimdMulSum(&A[0], &A[0 + shift], Size);
		Sums[shift] = sum;	// note the "fix #2" from false sharing if you are using OpenMP
	}
	double time1 = omp_get_wtime();
	double MultAdds = (double)Size / (time1 - time0) ;

	cout << "Performance: " << MultAdds << endl;
}
float
SimdMulSum(float* a, float* b, int len)
{
	float sum[4] = { 0., 0., 0., 0. };
	int limit = (len / SSE_WIDTH) * SSE_WIDTH;
	__asm
	(
		".att_syntax\n\t"
		"movq -40(%rbp), %r8\n\t" // a
		"movq -48(%rbp), %rcx\n\t" // b
		"leaq -32(%rbp), %rdx\n\t" // &sum[0]
		"movups (%rdx), %xmm2\n\t" // 4 copies of 0. in xmm2
		);
	for (int i = 0; i < limit; i += SSE_WIDTH)
	{
		__asm
		(
			".att_syntax\n\t"
			"movups (%r8), %xmm0\n\t" // load the first sse register
			"movups (%rcx), %xmm1\n\t" // load the second sse register
			"mulps %xmm1, %xmm0\n\t" // do the multiply
			"addps %xmm0, %xmm2\n\t" // do the add
			"addq $16, %r8\n\t"
			"addq $16, %rcx\n\t"
			);
	}
	__asm
	(
		".att_syntax\n\t"
		"movups %xmm2, (%rdx)\n\t" // copy the sums back to sum[ ]
		);
	for (int i = limit; i < len; i++)
	{
		sum[0] += a[i] * b[i];
	}
	return sum[0] + sum[1] + sum[2] + sum[3];
