#pragma once

#ifndef _BHC_INCLUDED_
#error "This file must be included via #include <bhc/bhc.hpp>!"
#endif

////////////////////////////////////////////////////////////////////////////////
//Select which standard library
////////////////////////////////////////////////////////////////////////////////

#ifndef STD
#define BHC_UNDEF_STD_AFTER 1
#define STD std
#endif

////////////////////////////////////////////////////////////////////////////////
//Shared library setup
////////////////////////////////////////////////////////////////////////////////

#ifdef _WIN32
#ifdef BHC_CMDLINE
#define BHC_API
#elif defined(BHC_EXPORTS)
#define BHC_API __declspec(dllexport)
#else //Users of bellhopcxx / bellhopcuda
#define BHC_API __declspec(dllimport)
#endif
#else
#define BHC_API
#endif
