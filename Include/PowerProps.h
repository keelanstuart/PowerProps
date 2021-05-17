/*

	PowerProps Library Source File

	Copyright © 2009-2021, Keelan Stuart. All rights reserved.

	PowerProps is a generic property library which one can use to maintain
	easily discoverable data in a number of types, as well as convert that
	data to other formats and de/serialize in multiple modes

	PowerProps is free software; you can redistribute it and/or modify it under
	the terms of the MIT License:

	Permission is hereby granted, free of charge, to any person obtaining a copy
	of this software and associated documentation files (the "Software"), to deal
	in the Software without restriction, including without limitation the rights
	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
	copies of the Software, and to permit persons to whom the Software is
	furnished to do so, subject to the following conditions:

	The above copyright notice and this permission notice shall be included in all
	copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
	SOFTWARE.

*/

#pragma once

// If you use the static library version of GenIO, you must define GENIO_STATIC in your project's preprocessor definitions

#if !defined(POWERPROPS_STATIC)

#ifdef POWERPROPS_EXPORTS
#define POWERPROPS_API __declspec(dllexport)
#else
#define POWERPROPS_API __declspec(dllimport)
#endif

#else

#define POWERPROPS_API

#endif


namespace props
{
	/// SFlagset is a helper to manage bit flags
	template <typename T> struct SFlagset
	{
		SFlagset() { flags = 0; }
		SFlagset(T f) { flags = f; }

		inline void SetAll(T f) { flags = f; }

		inline void Set(T f) { flags |= f; }

		inline void Toggle(T f) { if (IsSet(f)) Clear(f); else Set(f); }

		inline void Clear(T f) { flags &= ~f; }

		inline operator T() const { return flags; }
		inline T Get() const { return flags; }

		inline bool IsSet(T f) const { return (((flags & f) == f) ? true : false); }

		inline bool AnySet(T f) const { return (((flags & f) != 0) ? true : false); }

		inline SFlagset &operator =(T f) { flags = f; return *this; }

		inline bool operator ==(T f) const { return (f == flags); }

		inline bool operator !=(T f) const { return (f != flags); }

		inline T operator ^(T f) const { return (flags ^ f); }
		inline SFlagset &operator ^=(T f) { flags ^= f; return *this; }

		inline T operator |(T f) const { return (flags | f); }
		inline SFlagset &operator |=(T f) { flags |= f; return *this; }

		inline T operator &(T f) const { return (flags & f); }
		inline SFlagset &operator &=(T f) { flags &= f; return *this; }

	protected:
		T flags;
	};

	/// 2D vector template with a helper union for member aliasing
	template <typename T> struct SVec2
	{
		SVec2() { x = y = 0; }
		SVec2(T _x, T _y = 0) { x = _x; y = _y; }
		SVec2(const SVec2<T> &v) { x = v.x; y = v.y; }

		inline SVec2<T> &operator =(const SVec2<T> &o) { x = o.x; y = o.y; return *this; }
		inline bool operator ==(const SVec2<T> &o) { return ((o.x == x) && (o.y == y)); }
		inline bool operator !=(const SVec2<T> &o) { return !((o.x == x) && (o.y == y)); }

		union
		{
			struct
			{
				T x, y;
			};

			struct
			{
				T lon, lat;
			};

			struct
			{
				T azim, elev;
			};

			T v[2];
		};
	};

	/// 3D vector template with a helper union for member aliasing
	template <typename T> struct SVec3
	{
		SVec3() { x = y = z = 0; }
		SVec3(T _x, T _y = 0, T _z = 0) { x = _x; y = _y; z = _z; }
		SVec3(const SVec3<T> &v) { x = v.x; y = v.y; z = v.z; }
		SVec3(const SVec2<T> &v) { x = v.x; y = v.y; z = 0; }

		inline SVec3<T> &operator =(const SVec3<T> &o) { x = o.x; y = o.y; z = o.z; return *this; }
		inline SVec3<T> &operator =(const SVec2<T> &o) { x = o.x; y = o.y; z = 0; return *this; }
		inline bool operator ==(const SVec3<T> &o) { return ((o.x == x) && (o.y == y) && (o.z == z)); }
		inline bool operator !=(const SVec3<T> &o) { return !((o.x == x) && (o.y == y) && (o.z == z)); }

		union
		{
			struct
			{
				T x, y, z;
			};

			struct
			{
				T lon, lat, alt;
			};

			T v[3];
		};
	};

	/// 4D vector template with a helper union for member aliasing
	template <typename T> struct SVec4
	{
		SVec4() { x = y = z = w = 0; }
		SVec4(T _x, T _y = 0, T _z = 0, T _w = 0) { x = _x; y = _y; z = _z; w = _w; }
		SVec4(const SVec4<T> &v) { x = v.x; y = v.y; z = v.z; w = v.w; }
		SVec4(const SVec3<T> &v) { x = v.x; y = v.y; z = v.z; w = 0; }
		SVec4(const SVec2<T> &v) { x = v.x; y = v.y; z = w = 0; }

		inline SVec4<T> &operator =(const SVec4<T> &o) { x = o.x; y = o.y; z = o.z; w = o.w; return *this; }
		inline SVec4<T> &operator =(const SVec3<T> &o) { x = o.x; y = o.y; z = o.z; w = 0; return *this; }
		inline SVec4<T> &operator =(const SVec2<T> &o) { x = o.x; y = o.y; z = w = 0; return *this; }
		inline bool operator ==(const SVec4<T> &o) { return ((o.x == x) && (o.y == y) && (o.z == z) && (o.w == w)); }
		inline bool operator !=(const SVec4<T> &o) { return !((o.x == x) && (o.y == y) && (o.z == z) && (o.w == w)); }

		union
		{
			struct
			{
				T x, y, z, w;
			};

			T v[4];
		};
	};


	typedef SFlagset<uint8_t> TFlags8;
	typedef SFlagset<uint16_t> TFlags16;
	typedef SFlagset<uint32_t> TFlags32;
	typedef SFlagset<uint64_t> TFlags64;
	typedef SVec2<int64_t> TVec2I;
	typedef SVec3<int64_t> TVec3I;
	typedef SVec4<int64_t> TVec4I;
	typedef SVec2<float> TVec2F;
	typedef SVec3<float> TVec3F;
	typedef SVec4<float> TVec4F;
	typedef uint32_t FOURCHARCODE;

	class IPropertySet;


	/// IProperty is a container for typed, sometimes convertible data. It provides methods to store and retreive
	/// data and also allows a descriptor (aspect) to be given to it so that a normal string can be differentiated from
	/// a filename, for example.
	class IProperty
	{

	public:

		enum PROPERTY_TYPE
		{
			PT_NONE = 0,		/// uninitialized

			PT_STRING,
			PT_INT,
			PT_INT_V2,			/// 2 Ints: x, y
			PT_INT_V3,			/// 3 Ints: x, y, z
			PT_INT_V4,			/// 4 Ints: x, y, z, w
			PT_FLOAT,
			PT_FLOAT_V2,
			PT_FLOAT_V3,
			PT_FLOAT_V4,
			PT_GUID,
			PT_ENUM,
			PT_BOOLEAN,

			PT_NUMTYPES
		};

		/// The idea is this: a property can have a type that may not fully express what the data is used for...
		/// this additional information may be used in an editing application to display special widgets like
		/// sliders, file browse buttons, color pickers, and more.
		enum PROPERTY_ASPECT
		{
			PA_GENERIC = 0,

			PA_FILENAME,		/// STRING
			PA_DIRECTORY,		/// STRING
			PA_COLOR_RGB,		/// INT - RGB
			PA_COLOR_RGBA,		/// INT - RGBA
			PA_LATLON,			/// VEC2/VEC3 lattitude / longitude / altitude (if vec3)
			PA_ELEVAZIM,		/// VEC2 azimuth / elevation
			PA_RASCDEC,			/// VEC2 right ascension / declination
			PA_QUATERNION,		/// VEC4
			PA_BOOL_ONOFF,		/// BOOL TYPE "on" "off"
			PA_BOOL_YESNO,		/// BOOL TYPE "yes" "no"
			PA_BOOL_TRUEFALSE,	/// BOOL TYPE "true" "false"
			PA_BOOL_ABLED,		/// BOOL TYPE "enabled" "disabled"
			PA_FONT_DESC,		/// STRING describes a font
			PA_DATE,			/// STRING / INT (holds a time_t)
			PA_TIME,			/// STRING / INT (holds a time_t)
			PA_IPADDRESS,		/// STRING

			PA_NUMASPECTS,

			PA_FIRSTUSERASPECT = PA_NUMASPECTS
		};

		/// When serializing, how should the property store itself?
		enum SERIALIZE_MODE
		{
			/// stores...

			SM_BIN_VALUESONLY = 0,	/// id, type, value
			SM_BIN_TERSE,			/// id, type, aspect, value
			SM_BIN_VERBOSE,			/// name, id, type, aspect, value

			SM_NUMMODES
		};

		/// Values to shift 0x1 by for flags
		/// It is important to note that flags do not restrict what your application can do
		/// to a property in code. They are merely provided as a way to provide control for
		/// user interfaces.
		enum FLAG_SHIFT
		{
			FS_REQUIRED = 0,		/// Indicates that the property is required and must not be renamed / removed
			FS_READONLY,			/// Indicates that the property is not editable by the user
			FS_HIDDEN,				/// Indicates that the property is not viewable by the user
			FS_TOOLTIPITEM,			/// Indicates that this property should be displayed by a tooltip helper
			FS_TYPELOCKED,			/// The type may not be changed

			FS_FIRSTUSERFLAG		/// If you need your own special flags that aren't here, start adding them at this point
		};

		#define PROPFLAG_REQUIRED		(1 << props::IProperty::FLAG_SHIFT::FS_REQUIRED)
		#define PROPFLAG_READONLY		(1 << props::IProperty::FLAG_SHIFT::FS_READONLY)
		#define PROPFLAG_HIDDEN			(1 << props::IProperty::FLAG_SHIFT::FS_HIDDEN)
		#define PROPFLAG_TOOLTIPITEM	(1 << props::IProperty::FLAG_SHIFT::FS_TOOLTIPITEM)
		#define PROPFLAG_TYPELOCKED		(1 << props::IProperty::FLAG_SHIFT::FS_TYPELOCKED)

		/// Implement an IEnumProvider to dynamically supply an IProperty with enum string values
		class IEnumProvider
		{

		public:

			/// Returns the number of string values that this enum provider has
			virtual size_t GetNumValues(const IProperty *pprop) const = NULL;

			/// Returns the enum string that corresponds to the given ordinal
			/// If buf is supplied, the string will be copied into buf and returned to the caller, given bufsize as a limitation
			virtual const TCHAR *GetValue(const IProperty *pprop, size_t ordinal, TCHAR *buf = nullptr, size_t bufsize = 0) const = NULL;

		};

		/// Frees any resources that may have been allocated by this property
		virtual void Release() = NULL;

		/// property name accessors
		virtual const TCHAR *GetName() const = NULL;
		virtual void SetName(const TCHAR *name) = NULL;

		/// ID accessor methods
		virtual FOURCHARCODE GetID() const = NULL;
		virtual void SetID(FOURCHARCODE id) = NULL;

		/// Flag accessor methods
		virtual TFlags32 &Flags() = NULL;

		/// Get the data type currently stored in this property
		virtual PROPERTY_TYPE GetType() const = NULL;

		/// Allows you to convert a property from one type to another
		virtual bool ConvertTo(PROPERTY_TYPE newtype) = NULL;

		/// Aspect accessor methods
		virtual PROPERTY_ASPECT GetAspect() const = NULL;
		virtual void SetAspect(PROPERTY_ASPECT aspect = PA_GENERIC) = NULL;

		/// Sets the data from another property
		virtual void SetFromProperty(IProperty *pprop) = NULL;

		/// Sets the data, potentially changing the internal type
		virtual void SetInt(int64_t val) = NULL;
		virtual void SetVec2I(const TVec2I &val) = NULL;
		virtual void SetVec3I(const TVec3I &val) = NULL;
		virtual void SetVec4I(const TVec4I &val) = NULL;
		virtual void SetFloat(float val) = NULL;
		virtual void SetVec2F(const TVec2F &val) = NULL;
		virtual void SetVec3F(const TVec3F &val) = NULL;
		virtual void SetVec4F(const TVec4F &val) = NULL;
		virtual void SetString(const TCHAR *val) = NULL;
		virtual void SetGUID(GUID val) = NULL;
		virtual void SetBool(bool val) = NULL;

		/// Returns the data in the requested form.
		/// If the internal type does not match the type requested, a more complicated operation may happen under the hood
		virtual int64_t AsInt(int64_t *ret = nullptr) const = NULL;
		virtual const TVec2I *AsVec2I(TVec2I *ret = nullptr) const = NULL;
		virtual const TVec3I *AsVec3I(TVec3I *ret = nullptr) const = NULL;
		virtual const TVec4I *AsVec4I(TVec4I *ret = nullptr) const = NULL;
		virtual float AsFloat(float *ret = nullptr) const = NULL;
		virtual const TVec2F *AsVec2F(TVec2F *ret = nullptr) const = NULL;
		virtual const TVec3F *AsVec3F(TVec3F *ret = nullptr) const = NULL;
		virtual const TVec4F *AsVec4F(TVec4F *ret = nullptr) const = NULL;
		virtual const TCHAR *AsString(TCHAR *ret = nullptr, size_t retsize = 0) const = NULL;
		virtual GUID AsGUID(GUID *ret = nullptr) const = NULL;
		virtual bool AsBool(bool *ret = nullptr) const = NULL;

		/// Instead of providing a comma-delimited enum string, you can optionally provide an IEnumProvider to return enum values
		virtual void SetEnumProvider(const IEnumProvider *pep) = NULL;

		/// Indicates that this property has a dynamic enum provider
		virtual const IEnumProvider *GetEnumProvider() const = NULL;

		/// Sets the individual enumeration string values from a comma-delimited string
		virtual void SetEnumStrings(const TCHAR *strs) = NULL;

		/// Sets the enum index into the enum values. Returns true if the desired index was valid, false if not
		virtual bool SetEnumVal(size_t val) = NULL;

		/// Sets the enum value by case-insensitive compares with the enum strings
		virtual bool SetEnumValByString(const TCHAR *s) = NULL;

		/// Returns an individual enumerated string by index
		virtual const TCHAR *GetEnumString(size_t idx, TCHAR *ret = nullptr, size_t retsize = 0) const = NULL;

		/// Returns the entire, comma-delimited string for an enumeration. Returns nullptr if not an enum property
		virtual const TCHAR *GetEnumStrings(TCHAR *ret = nullptr, size_t retsize = 0) const = NULL;

		/// Returns the maximum allowed value for an enum
		virtual size_t GetMaxEnumVal() const = NULL;

		/// Returns the owner of this IProperty
		virtual IPropertySet *GetOwner() const = NULL;

		/// Returns true if the value of this property matches that of the given property
		virtual bool IsSameAs(const IProperty *other_prop) const = NULL;

	};


	/// Implement this class and register with an IPropertySet implementation to receive notifications when properties changes
	class IPropertyChangeListener
	{

	public:

		/// Called by a property set when one of it's properties has changed
		virtual void PropertyChanged(const IProperty *pprop) = NULL;

	};

	/// IPropertySet is a container for IProperty instances, 
	class IPropertySet
	{

	public:

		/// Releases any resources the property set may have allocated
		virtual void Release() = NULL;

		/// Creates a new property and adds it to this property set
		virtual IProperty *CreateProperty(const TCHAR *propname, FOURCHARCODE propid) = NULL;

		/// Creates a property that references data held elsewhere and adds it to this property set (only bool, number, guid, and vector types supported)
		virtual IProperty *CreateReferenceProperty(const TCHAR *propname, FOURCHARCODE propid, void *addr, IProperty::PROPERTY_TYPE type) = NULL;

		/// Deletes a property from this set, based on a given index
		virtual void DeleteProperty(size_t idx) = NULL;

		/// Deletes a property from this set, based on a given property id
		virtual void DeletePropertyById(FOURCHARCODE propid) = NULL;

		/// Deletes a property from this set, based on a given property name
		virtual void DeletePropertyByName(const TCHAR *propname) = NULL;

		/// Deletes all properties from this set
		virtual void DeleteAll() = NULL;

		/// Returns the number of properties in this set
		virtual size_t GetPropertyCount() const = NULL;

		/// Returns the property at the given index or nullptr if the index is out of range
		virtual IProperty *GetProperty(size_t idx) const = NULL;

		/// Gets a property from this set, given a property id
		virtual IProperty *GetPropertyById(FOURCHARCODE propid) const = NULL;

		/// Gets a property from this set, given a property name
		virtual IProperty *GetPropertyByName(const TCHAR *propname) const = NULL;

		/// Indexing gets a property from this set, given a property id (FOURCHARCODE) or name (const TCHAR *)
		virtual IProperty * operator[](FOURCHARCODE propid) const = NULL;
		virtual IProperty * operator[](const TCHAR *propname) const = NULL;

		/// Takes the properties from the given list and appends them to this set
		virtual void AppendPropertySet(const IPropertySet *propset) = NULL;

		/// Writes all properties to a binary stream
		/// If buf is null but amountused is not, the number of bytes required to fully
		/// store the property set will be placed at amountused
		virtual bool Serialize(IProperty::SERIALIZE_MODE mode, BYTE *buf, size_t bufsize, size_t *amountused = NULL) const = NULL;

		/// Reads all properties from a binary stream
		/// If successful, the return value will be true and the number of bytes
		/// consumed will be reported, if desired
		virtual bool Deserialize(BYTE *buf, size_t bufsize, size_t *bytesconsumed) = NULL;

		/// <summary>
		/// Writes all properties to an XML-formatted tstring in the verbose equivalent
		/// </summary>
		/// <param name="xmls">When the return value is true, will contain the XML fragment that represents all properties in the set</param>
		virtual bool SerializeToXMLString(IProperty::SERIALIZE_MODE mode, tstring &xmls) const = NULL;

		/// <summary>
		/// Reads all properties from an XML-formatted tstring
		/// </summary>
		/// <param name="xmls">an XML fragment that contains property data</param>
		virtual bool DeserializeFromXMLString(const tstring &xmls) = NULL;

		/// Register a change listener if you want to know when a property has changed
		virtual void SetChangeListener(const IPropertyChangeListener *plistener) = NULL;

		/// Creates an instance of the IPropertySet interface, allowing the user to add IProperty's to it
		/// These can be serialized to a packet and distributed to a set of listeners. Imagination is the only limitation.
		/// (Only included as an example, use or not, with discretion)
		POWERPROPS_API static IPropertySet *CreatePropertySet();

	};

};