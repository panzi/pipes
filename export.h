#ifndef PIPES_EXPORT_H
#define PIPES_EXPORT_H
#pragma once

#if defined _WIN32 || defined _WIN64 || defined __CYGWIN__
	#ifdef BUILDING_DLL
		#ifdef __GNUC__
			#define PIPES_EXPORT __attribute__ ((dllexport))
		#else
			#define PIPES_EXPORT __declspec(dllexport)
		#endif
	#else
		#ifdef __GNUC__
			#define PIPES_EXPORT __attribute__ ((dllimport))
		#else
			#define PIPES_EXPORT __declspec(dllimport)
		#endif
	#endif
	#define DLL_LOCAL
#else
	#if __GNUC__ >= 4
		#define PIPES_EXPORT __attribute__ ((visibility ("default")))
		#define PIPES_LOCAL  __attribute__ ((visibility ("hidden")))
	#else
		#define PIPES_EXPORT
		#define PIPES_LOCAL
	#endif
#endif

#endif
