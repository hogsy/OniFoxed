#pragma once

/*
	FILE:	Oni_Performance.h
	
	AUTHOR:	Brent H. Pease
	
	CREATED: April 2, 1997
	
	PURPOSE: main .c file for Oni
	
	Copyright 1997

*/
#ifndef ONI_PERFORMANCE_C
#define ONI_PERFORMANCE_C

void
ONrPerformance_Measure_Gouraud(
	M3tDrawContext	*drawContext);

void
ONrPerformance_Measure_GouraudFlat(
	M3tDrawContext	*drawContext);

UUtError
ONrPerformance_Measure_Texture(
	M3tDrawContext	*drawContext);

#endif /* ONI_PERFORMANCE_C */
