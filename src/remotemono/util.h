/*
	Copyright 2020 David "Alemarius Nexus" Lerch

	This file is part of RemoteMono.

	RemoteMono is free software: you can redistribute it and/or modify
	it under the terms of the GNU Lesser General Public License as published
	by the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	RemoteMono is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU Lesser General Public License for more details.

	You should have received a copy of the GNU Lesser General Public License
	along with RemoteMono.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include "config.h"

#include <stdint.h>
#include <string>



namespace remotemono
{



/**
 * Computes log2(x) for integer powers of two at compile time.
 */
template <typename UIntT>
constexpr uint8_t static_ilog2(UIntT x)
{
	// TODO: Check that it's an exact ilog2 (static_assert() didn't seem to work with constexpr on VS2019)
	uint8_t res = 0;
	while (x > 1) {
		x >>= 1;
		res++;
	}
	return res;
}

/**
 * Aligns an address at compile time.
 *
 * @param x The (unaligned) input address.
 * @param align The alignment in bytes.
 * @return The aligned address. Will always be >= x such that x%align == 0.
 */
template <typename UIntT>
constexpr UIntT static_align(UIntT x, size_t al)
{
	return UIntT((x % al == 0) ? x : (x/al + 1) * al);
}


/**
 * Aligns an address.
 *
 * @param x The (unaligned) input address.
 * @param align The alignment in bytes.
 * @return The aligned address. Will always be >= x such that x%align == 0.
 */
template <typename UIntT>
UIntT align(UIntT x, size_t al)
{
	return UIntT((x % al == 0) ? x : (x/al + 1) * al);
}


/**
 * Determines the name of the given type, including qualifiers like constness, volatility and reference state.
 */
template <typename T>
std::string qualified_type_name()
{
	// Based on https://stackoverflow.com/a/20170989/1566841
	typedef typename std::remove_reference_t<T> TR;

	std::string name(typeid(TR).name());

	if constexpr(std::is_const_v<TR>) {
		name += " const";
	}
	if constexpr(std::is_volatile_v<TR>) {
		name += " volatile";
	}
	if constexpr(std::is_lvalue_reference_v<T>) {
		name += "&";
	}
	if constexpr(std::is_rvalue_reference_v<T>) {
		name += "&&";
	}
	return name;
}


template <typename T>
inline void hash_combine(size_t& s, const T& v)
{
	// Taken from Boost
	s ^= std::hash<T>()(v) + 0x9e3779b9 + (s << 6) + (s >> 2);
}


/**
 * Dumps the given data as an array of bytes in hexadecimal form.
 */
inline std::string DumpByteArray(const char* data, size_t size)
{
	if (size == 0) {
		return std::string();
	}

	const char* hexDigits = "0123456789ABCDEF";

	const uint8_t* udata = (const uint8_t*) data;
	std::string dump;

	dump.append({hexDigits[udata[0]/16], hexDigits[udata[0]%16]});

	for (size_t i = 1 ; i < size ; i++) {
		unsigned char c = udata[i];
		dump.append({' ', hexDigits[c/16], hexDigits[c%16]});
	}

	return dump;
}


inline std::wstring ConvertStringToWString(const std::string str)
{
	// Trying to avoid std::codecvt because MSVC pritns a deprecation warning for it, and we don't
	// want to force users to have to define something to suppress it. Fuck MSVC.
	int strLen = static_cast<int>(str.length());
	int wideLen = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), strLen, nullptr, 0);
	std::wstring wstr;
	wstr.resize(wideLen);
	MultiByteToWideChar(CP_UTF8, 0, str.c_str(), strLen, &wstr[0], static_cast<int>(wstr.length()));
	return wstr;
}


inline std::string ConvertWStringToString(const std::wstring wstr)
{
	// Trying to avoid std::codecvt because MSVC pritns a deprecation warning for it, and we don't
	// want to force users to have to define something to suppress it. Fuck MSVC.
	int wstrLen = static_cast<int>(wstr.length());
	int narrowLen = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), wstrLen, nullptr, 0, nullptr, nullptr);
	std::string str;
	str.resize(narrowLen);
	WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), wstrLen, &str[0], static_cast<int>(str.length()), nullptr, nullptr);
	return str;
}








/**
 * A dummy class for packing and splitting template parameter packs.
 */
template <typename... TypeT>
struct PackHelper {};


/**
 * A helper class that maps a type to itself.
 */
template <typename TypeT>
struct Identity
{
	typedef TypeT Type;
};



}
