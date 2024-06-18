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

#include "stdafx.h"
#include <PowerProps.h>
#include <GenIO.h>


using namespace props;


#define PROPFLAG_REFERENCE		(1 << 31)
#define PROPFLAG_ENUMPROVIDER	(1 << 30)


class CPropertySet : public IPropertySet
{
protected:
	typedef ::std::deque<IProperty *> TPropertyArray;
	TPropertyArray m_Props;

	typedef ::std::map<FOURCHARCODE, IProperty *> TPropertyMap;
	typedef ::std::pair<FOURCHARCODE, IProperty *> TPropertyMapPair;
	TPropertyMap m_mapProps;

public:
	IPropertyChangeListener *m_pListener;

public:

	CPropertySet();
	virtual ~CPropertySet();

	virtual void Release();
	virtual IProperty *CreateProperty(const TCHAR *propname, FOURCHARCODE propid);
	virtual IProperty *CreateReferenceProperty(const TCHAR *propname, FOURCHARCODE propid, void *addr, IProperty::PROPERTY_TYPE type);
	virtual void AddProperty(IProperty *pprop);
	virtual void DeleteProperty(size_t idx);
	virtual void DeletePropertyById(FOURCHARCODE propid);
	virtual void DeletePropertyByName(const TCHAR *propname);
	virtual void DeleteAll();
	virtual size_t GetPropertyCount() const;
	virtual IProperty *GetProperty(size_t idx) const;
	virtual IProperty *GetPropertyById(props::FOURCHARCODE propid) const;
	virtual IProperty *operator [](FOURCHARCODE propid) const { return GetPropertyById(propid); }
	virtual IProperty *GetPropertyByName(const TCHAR *propname) const;
	virtual IProperty *operator [](const TCHAR *propname) const { return GetPropertyByName(propname); }
	virtual CPropertySet &operator =(IPropertySet *propset);
	virtual CPropertySet &operator +=(IPropertySet *propset);
	virtual void AppendPropertySet(const IPropertySet *propset, bool overwrite_flags = false);
	virtual bool Serialize(IProperty::SERIALIZE_MODE mode, BYTE *buf, size_t bufsize, size_t *amountused) const;
	virtual bool Deserialize(BYTE *buf, size_t bufsize, size_t *bytesconsumed);
	virtual bool SerializeToXMLString(IProperty::SERIALIZE_MODE mode, tstring &xmls) const;
	virtual bool DeserializeFromXMLString(const tstring &xmls);
	virtual void SetChangeListener(const IPropertyChangeListener *plistener);
};



class CProperty : public IProperty
{
public:
	tstring m_sName;
	FOURCHARCODE m_ID;
	PROPERTY_TYPE m_Type;
	PROPERTY_ASPECT m_Aspect;
	TFlags32 m_Flags;
	CPropertySet *m_pOwner;

	typedef std::deque<tstring> TStringDeque;

	union
	{
		// group string and int data anonymously so we can have enumerated types
		struct
		{
			TCHAR *m_s;
			union
			{
				int64_t m_i, *p_i;
				uint64_t m_e;
			};
			union
			{
				TStringDeque *m_es;
				const IEnumProvider *m_pep;
			};
		};
		TVec2I m_v2i, *p_v2i;
		TVec3I m_v3i, *p_v3i;
		TVec4I m_v4i, *p_v4i;
		float m_f, *p_f;
		TVec2F m_v2f, *p_v2f;
		TVec3F m_v3f, *p_v3f;
		TVec4F m_v4f, *p_v4f;
		TMat3x3F m_m3x3f, *p_m3x3f;
		TMat4x4F m_m4x4f, *p_m4x4f;
		GUID m_g, *p_g;
		bool m_b, *p_b;
	};

	CProperty(CPropertySet *powner)
	{
		m_Type = PT_NONE;
		m_Aspect = PA_GENERIC;
		m_s = nullptr;
		m_es = nullptr;
		m_pOwner = powner;
	}

	// call release()!
	virtual ~CProperty()
	{
		Reset();
	}

	size_t RequiredStringLength()
	{
		int32_t bufsz = 0;

		switch (m_Type)
		{
			case PT_ENUM:
				// the length of each string plus one character... commas for internal delimiters, colon for last, followed by number
#if defined(_M_X64)
				bufsz = _sctprintf(_T("%I64d"), m_e);
#else
				bufsz = _sctprintf(_T("%lld"), m_e);
#endif
				for (TStringDeque::const_iterator it = m_es->cbegin(); it != m_es->cend(); it++)
					bufsz += _sctprintf(_T("%s"), it->c_str()) + 1;
				break;

			case PT_STRING:
				bufsz = _sctprintf(_T("%s"), m_s);
				break;

			case PT_BOOLEAN:
				bufsz = _sctprintf(_T("%d"), m_b);
				break;

			case PT_INT:
				bufsz = _sctprintf(_T("%I64d"), m_i);
				break;

			case PT_INT_V2:
				bufsz = _sctprintf(_T("%I64d,%I64d"), m_v2i.x, m_v2i.y);
				break;

			case PT_INT_V3:
				bufsz = _sctprintf(_T("%I64d,%I64d,%I64d"), m_v3i.x, m_v3i.y, m_v3i.z);
				break;

			case PT_INT_V4:
				bufsz = _sctprintf(_T("%I64d,%I64d,%I64d,%I64d"), m_v4i.x, m_v4i.y, m_v4i.z, m_v4i.w);
				break;

			case PT_FLOAT:
				bufsz = _sctprintf(_T("%f"), m_f);
				break;

			case PT_FLOAT_V2:
				bufsz = _sctprintf(_T("%f,%f"), m_v2f.x, m_v2f.y);
				break;

			case PT_FLOAT_V3:
				bufsz = _sctprintf(_T("%f,%f,%f"), m_v3f.x, m_v3f.y, m_v3f.z);
				break;

			case PT_FLOAT_V4:
				bufsz = _sctprintf(_T("%f,%f,%f,%f"), m_v4f.x, m_v4f.y, m_v4f.z, m_v4f.w);
				break;

			case PT_FLOAT_MAT3X3:
				bufsz = _sctprintf(_T("%f,%f,%f,%f,%f,%f,%f,%f,%f"),
					m_m3x3f.m[0].x, m_m3x3f.m[0].y, m_m3x3f.m[0].z,
					m_m3x3f.m[1].x, m_m3x3f.m[1].y, m_m3x3f.m[1].z,
					m_m3x3f.m[2].x, m_m3x3f.m[2].y, m_m3x3f.m[2].z);
				break;

			case PT_FLOAT_MAT4X4:
				bufsz = _sctprintf(_T("%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f"),
					m_m4x4f.m[0].x, m_m4x4f.m[0].y, m_m4x4f.m[0].z, m_m4x4f.m[0].w,
					m_m4x4f.m[1].x, m_m4x4f.m[1].y, m_m4x4f.m[1].z, m_m4x4f.m[1].w,
					m_m4x4f.m[2].x, m_m4x4f.m[2].y, m_m4x4f.m[2].z, m_m4x4f.m[2].w,
					m_m4x4f.m[3].x, m_m4x4f.m[3].y, m_m4x4f.m[3].z, m_m4x4f.m[3].w);
				break;

			case PT_GUID:
				bufsz = _sctprintf(_T("{%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}"), m_g.Data1, m_g.Data2, m_g.Data3,
					m_g.Data4[0], m_g.Data4[1], m_g.Data4[2], m_g.Data4[3], m_g.Data4[4], m_g.Data4[5], m_g.Data4[6], m_g.Data4[7]);
				break;
		}

		return bufsz + 1;
	}

	virtual const TCHAR *GetName() const
	{
		return m_sName.c_str();
	}

	virtual void SetName(const TCHAR *name)
	{
		m_sName = name;
	}

	virtual FOURCHARCODE GetID() const
	{
		return m_ID;
	}

	virtual void SetID(FOURCHARCODE id)
	{
		m_ID = id;
	}

	virtual TFlags32 &Flags()
	{
		return m_Flags;
	}

	virtual IPropertySet *GetOwner() const
	{
		return m_pOwner;
	}

	virtual void Reset()
	{
		switch (m_Type)
		{
			case PT_ENUM:
				if (m_es && !m_Flags.IsSet(PROPFLAG_ENUMPROVIDER))
				{
					delete m_es;
					m_es = nullptr;
				}
				// drop through to clean up the string also...

			case PT_STRING:
				if (m_s)
				{
					free(m_s);
					m_s = nullptr;
				}
				break;

			default:
				break;
		}

		if (!m_Flags.IsSet(PROPFLAG_REFERENCE))
			m_Type = PT_NONE;
	}


	virtual void Release()
	{
		delete this;
	}

	virtual PROPERTY_TYPE GetType() const
	{
		return m_Type;
	}

	virtual bool ConvertTo(PROPERTY_TYPE newtype)
	{
		if (m_Flags.IsSet(PROPFLAG(TYPELOCKED)))
			return false;

		if (newtype == m_Type)
			return true;

		switch (newtype)
		{
			case PT_BOOLEAN:
			{
				bool b;
				SetBool(AsBool(&b));
				break;
			}

			case PT_INT:
			{
				int64_t x;
				SetInt(AsInt(&x));
				break;
			}

			case PT_INT_V2:
			{
				switch (m_Type)
				{
					case PT_STRING:
					{
						TVec2I v;
						AsVec2I(&v);
						SetVec2I(v);
						break;
					}

					case PT_FLOAT:
					{
						SetVec2I(props::TVec2I(int64_t(m_v2f.x)));
						break;
					}

					case PT_FLOAT_V2:
					case PT_FLOAT_V3:
					case PT_FLOAT_V4:
					{
						SetVec2I(props::TVec2I(int64_t(m_v2f.x), int64_t(m_v2f.y)));
						break;
					}

					case PT_INT:
						m_v2i.y = 0;
					case PT_INT_V3:
					case PT_INT_V4:
						m_Type = PT_INT_V2;
						break;

					default:
						break;
				}
				break;
			}

			case PT_INT_V3:
			{
				switch (m_Type)
				{
					case PT_STRING:
					{
						TVec3I v;
						AsVec3I(&v);
						SetVec3I(v);
						break;
					}

					case PT_FLOAT:
					{
						SetVec3I(props::TVec3I(int64_t(m_v3f.x)));
						break;
					}

					case PT_FLOAT_V2:
					{
						SetVec3I(props::TVec3I(int64_t(m_v3f.x), int64_t(m_v3f.x)));
						break;
					}

					case PT_FLOAT_V3:
					case PT_FLOAT_V4:
					{
						SetVec3I(props::TVec3I(int64_t(m_v3f.x), int64_t(m_v3f.y), int64_t(m_v3f.z)));
						break;
					}

					case PT_INT:
						m_v2i.y = 0;
					case PT_INT_V2:
						m_v3i.z = 0;
					case PT_INT_V4:
						m_Type = PT_INT_V3;
						break;

					default:
						break;
				}
				break;
			}

			case PT_INT_V4:
			{
				switch (m_Type)
				{
					case PT_STRING:
					{
						TVec4I v;
						AsVec4I(&v);
						SetVec4I(v);
						break;
					}

					case PT_FLOAT_V4:
					{
						SetVec4I(props::TVec4I(int64_t(m_v4f.x), int64_t(m_v4f.y), int64_t(m_v4f.z), int64_t(m_v4f.w)));
						break;
					}

					case PT_INT:
						m_v4i.y = 0;
					case PT_INT_V2:
						m_v4i.z = 0;
					case PT_INT_V3:
						m_v4i.w = 0;
						m_Type = PT_INT_V4;
						break;

					default:
						break;
				}
				break;
			}

			case PT_FLOAT:
			{
				float f;
				SetFloat(AsFloat(&f));
				break;
			}

			case PT_FLOAT_V2:
			{
				switch (m_Type)
				{
					case PT_STRING:
					{
						TVec2F v;
						AsVec2F(&v);
						SetVec2F(v);
						break;
					}

					case PT_INT:
					{
						SetVec2F(props::TVec2F(float(m_i)));
						break;
					}

					case PT_INT_V2:
					case PT_INT_V3:
					case PT_INT_V4:
					{
						SetVec2F(props::TVec2F(float(m_v2i.x), float(m_v2i.y)));
						break;
					}

					case PT_FLOAT:
						m_v2f.y = 0;
					case PT_FLOAT_V3:
					case PT_FLOAT_V4:
						m_Type = PT_FLOAT_V2;
						break;

					default:
						break;
				}
				break;
			}

			case PT_FLOAT_V3:
			{
				switch (m_Type)
				{
					case PT_STRING:
					{
						TVec3F v;
						AsVec3F(&v);
						SetVec3F(v);
						break;
					}

					case PT_INT:
					{
						SetVec3F(props::TVec3F(float(m_i)));
						break;
					}

					case PT_INT_V2:
					{
						SetVec3F(props::TVec3F(float(m_v2i.x), float(m_v2i.y)));
						break;
					}

					case PT_INT_V3:
					case PT_INT_V4:
					{
						SetVec3F(props::TVec3F(float(m_v3i.x), float(m_v3i.y), float(m_v3i.z)));
						break;
					}

					case PT_FLOAT:
						m_v3f.y = 0;
					case PT_FLOAT_V2:
						m_v3f.z = 0;
					case PT_FLOAT_V4:
						m_Type = PT_FLOAT_V3;
						break;

					default:
						break;
				}
				break;
			}

			case PT_FLOAT_V4:
			{
				switch (m_Type)
				{
					case PT_STRING:
					{
						TVec4F v;
						AsVec4F(&v);
						SetVec4F(v);
						break;
					}

					case PT_INT_V4:
					{
						SetVec4F(props::TVec4F(float(m_v4i.x), float(m_v4i.y), float(m_v4i.z), float(m_v4i.w)));
						break;
					}

					case PT_FLOAT:
						m_v4f.y = 0;
					case PT_FLOAT_V2:
						m_v4f.z = 0;
					case PT_FLOAT_V3:
						m_v4f.w = 0;
						break;

					default:
						break;
				}
				break;
			}

			case PT_STRING:
			{
				size_t bufsz = RequiredStringLength();

				if (bufsz > 0)
				{
					bufsz += sizeof(TCHAR);
					bufsz *= sizeof(TCHAR);
					TCHAR *buf = (TCHAR *)malloc(bufsz);
					AsString(buf, bufsz);

					if ((m_Type == PT_STRING) && m_s)
						free(m_s);

					m_s = buf;
				}
				else
				{
					SetString(_T(""));
					return false;
				}

				break;
			}

			case PT_GUID:
			{
				GUID g;
				SetGUID(AsGUID(&g));
				break;
			}

			case PT_ENUM:
			{
				if (m_Type == PT_STRING)
				{
					TCHAR *c = _tcsrchr(m_s, _T(':'));
					size_t v = 0;
					if (c)
					{
						*c = _T('\0');
						c++;
#if defined(_M_X64)
						v = _ttoi64(c);
#else
						v = _ttoi(c);
#endif
					}
					tstring tmp = m_s;
					SetEnumStrings(tmp.c_str());
					SetEnumVal(v);
				}
				break;
			}
		}

		m_Type = newtype;

		return true;
	}

	virtual PROPERTY_ASPECT GetAspect() const
	{
		return m_Aspect;
	}

	virtual void SetAspect(PROPERTY_ASPECT aspect)
	{
		if (!m_Flags.IsSet(PROPFLAG(ASPECTLOCKED)))
			m_Aspect = aspect;
	}

	virtual void SetInt(int64_t val)
	{
		if (m_Flags.IsSet(PROPFLAG(TYPELOCKED)) && (m_Type != PT_INT))
			return;

		Reset();

		m_Type = PT_INT;
		if (!m_Flags.IsSet(PROPFLAG_REFERENCE))
			m_i = val;
		else
			*p_i = val;

		if (m_pOwner && m_pOwner->m_pListener) m_pOwner->m_pListener->PropertyChanged(this);
	}

	virtual void SetVec2I(const TVec2I &val)
	{
		if (m_Flags.IsSet(PROPFLAG(TYPELOCKED)) && (m_Type != PT_INT_V2))
			return;

		Reset();

		m_Type = PT_INT_V2;
		if (!m_Flags.IsSet(PROPFLAG_REFERENCE))
			m_v2i = val;
		else
			*p_v2i = val;

		if (m_pOwner && m_pOwner->m_pListener) m_pOwner->m_pListener->PropertyChanged(this);
	}

	virtual void SetVec3I(const TVec3I &val)
	{
		if (m_Flags.IsSet(PROPFLAG(TYPELOCKED)) && (m_Type != PT_INT_V3))
			return;

		Reset();

		m_Type = PT_INT_V3;
		if (!m_Flags.IsSet(PROPFLAG_REFERENCE))
			m_v3i = val;
		else
			*p_v3i = val;

		if (m_pOwner && m_pOwner->m_pListener) m_pOwner->m_pListener->PropertyChanged(this);
	}

	virtual void SetVec4I(const TVec4I &val)
	{
		if (m_Flags.IsSet(PROPFLAG(TYPELOCKED)) && (m_Type != PT_INT_V4))
			return;

		Reset();

		m_Type = PT_INT_V4;
		if (!m_Flags.IsSet(PROPFLAG_REFERENCE))
			m_v4i = val;
		else
			*p_v4i = val;

		if (m_pOwner && m_pOwner->m_pListener) m_pOwner->m_pListener->PropertyChanged(this);
	}

	virtual void SetFloat(float val)
	{
		if (m_Flags.IsSet(PROPFLAG(TYPELOCKED)) && (m_Type != PT_FLOAT))
			return;

		Reset();

		m_Type = PT_FLOAT;
		if (!m_Flags.IsSet(PROPFLAG_REFERENCE))
			m_f = val;
		else
			*p_f = val;

		if (m_pOwner && m_pOwner->m_pListener) m_pOwner->m_pListener->PropertyChanged(this);
	}

	virtual void SetVec2F(const TVec2F &val)
	{
		if (m_Flags.IsSet(PROPFLAG(TYPELOCKED)) && (m_Type != PT_FLOAT_V2))
			return;

		Reset();

		m_Type = PT_FLOAT_V2;
		if (!m_Flags.IsSet(PROPFLAG_REFERENCE))
			m_v2f = val;
		else
			*p_v2f = val;

		if (m_pOwner && m_pOwner->m_pListener) m_pOwner->m_pListener->PropertyChanged(this);
	}

	virtual void SetVec3F(const TVec3F &val)
	{
		if (m_Flags.IsSet(PROPFLAG(TYPELOCKED)) && (m_Type != PT_FLOAT_V3))
			return;

		Reset();

		m_Type = PT_FLOAT_V3;
		if (!m_Flags.IsSet(PROPFLAG_REFERENCE))
			m_v3f = val;
		else
			*p_v3f = val;

		if (m_pOwner && m_pOwner->m_pListener) m_pOwner->m_pListener->PropertyChanged(this);
	}

	virtual void SetVec4F(const TVec4F &val)
	{
		if (m_Flags.IsSet(PROPFLAG(TYPELOCKED)) && (m_Type != PT_FLOAT_V4))
			return;

		Reset();

		m_Type = PT_FLOAT_V4;
		if (!m_Flags.IsSet(PROPFLAG_REFERENCE))
			m_v4f = val;
		else
			*p_v4f = val;

		if (m_pOwner && m_pOwner->m_pListener) m_pOwner->m_pListener->PropertyChanged(this);
	}

	virtual void SetMat3x3F(const TMat3x3F *val)
	{
		if (m_Flags.IsSet(PROPFLAG(TYPELOCKED)) && (m_Type != PT_FLOAT_MAT3X3))
			return;

		Reset();

		m_Type = PT_FLOAT_MAT3X3;
		if (!m_Flags.IsSet(PROPFLAG_REFERENCE))
			m_m3x3f = *val;
		else
			*p_m3x3f = *val;

		if (m_pOwner && m_pOwner->m_pListener) m_pOwner->m_pListener->PropertyChanged(this);
	}

	virtual void SetMat4x4F(const TMat4x4F *val)
	{
		if (m_Flags.IsSet(PROPFLAG(TYPELOCKED)) && (m_Type != PT_FLOAT_MAT4X4))
			return;

		Reset();

		m_Type = PT_FLOAT_MAT4X4;
		if (!m_Flags.IsSet(PROPFLAG_REFERENCE))
			m_m4x4f = *val;
		else
			*p_m4x4f = *val;

		if (m_pOwner && m_pOwner->m_pListener) m_pOwner->m_pListener->PropertyChanged(this);
	}

	virtual void SetString(const TCHAR *val)
	{
		if ((m_Type == PT_STRING) && !_tcsicmp(val, m_s))
			return;

		Reset();

		m_Type = PT_STRING;
		if (val)
		{
			m_s = _tcsdup(val);
		}

		if (m_pOwner && m_pOwner->m_pListener) m_pOwner->m_pListener->PropertyChanged(this);
	}

	virtual void SetGUID(GUID val)
	{
		if (m_Flags.IsSet(PROPFLAG(TYPELOCKED)) && (m_Type != PT_GUID))
			return;

		Reset();

		m_Type = PT_GUID;
		if (!m_Flags.IsSet(PROPFLAG_REFERENCE))
			m_g = val;
		else
			*p_g = val;

		if (m_pOwner && m_pOwner->m_pListener) m_pOwner->m_pListener->PropertyChanged(this);
	}

	virtual void SetBool(bool val)
	{
		if (m_Flags.IsSet(PROPFLAG(TYPELOCKED)) && (m_Type != PT_BOOLEAN))
			return;

		Reset();

		m_Type = PT_BOOLEAN;
		if (!m_Flags.IsSet(PROPFLAG_REFERENCE))
			m_b = val;
		else
			*p_b = val;

		if (m_pOwner && m_pOwner->m_pListener) m_pOwner->m_pListener->PropertyChanged(this);
	}

	virtual void SetEnumProvider(const IEnumProvider *pep)
	{
		Reset();

		m_Type = PT_ENUM;

		if (pep)
			m_Flags.Set(PROPFLAG_ENUMPROVIDER);
		else
			m_Flags.Clear(PROPFLAG_ENUMPROVIDER);

		m_pep = pep;
	}

	virtual const IEnumProvider *GetEnumProvider() const
	{
		if ((m_Type == PT_ENUM) && (m_Flags.IsSet(PROPFLAG_ENUMPROVIDER)))
			return m_pep;

		return nullptr;
	}

	virtual void SetEnumStrings(const TCHAR *strs)
	{
		Reset();

		m_Type = PT_ENUM;

		m_Flags.Clear(PROPFLAG_ENUMPROVIDER);

		m_es = new TStringDeque();
		if (!m_es)
			return;

		if (strs)
		{
			m_s = _tcsdup(strs);
			if (!m_s)
				return;

			size_t len = _tcslen(strs);
			TCHAR *s = (TCHAR *)_alloca((len + 1) * sizeof(TCHAR));
			if (s)
			{
				_tcscpy_s(s, len + 1, strs);

				TCHAR *c = s + len - 1;
				while (c >= s)
				{
					while ((*c != _T(',')) && (c >= s))
					{
						c--;
					}

					if (c > s)
						*c = _T('\0');

					m_es->push_front(c + 1);
				}
			}
		}

		m_e = 0;
	}

	virtual bool SetEnumVal(size_t val)
	{
		if (m_Type != PT_ENUM)
			return false;

		if (m_Flags.IsSet(PROPFLAG_ENUMPROVIDER))
		{
			if (!m_pep)
				return false;

			if (val < m_pep->GetNumValues(this))
			{
				m_e = val;

				if (m_pOwner && m_pOwner->m_pListener) m_pOwner->m_pListener->PropertyChanged(this);

				return true;
			}
		}
		else
		{
			assert(m_es);

			if (val < m_es->size())
			{
				m_e = val;

				if (m_pOwner && m_pOwner->m_pListener) m_pOwner->m_pListener->PropertyChanged(this);

				return true;
			}
		}

		return false;
	}

	virtual bool SetEnumValByString(const TCHAR *s)
	{
		if (m_Type != PT_ENUM)
			return false;

		if (m_Flags.IsSet(PROPFLAG_ENUMPROVIDER))
		{
			if (!m_pep)
				return false;

			for (size_t i = 0, maxi = m_pep->GetNumValues(this); i < maxi; i++)
			{
				if (!_tcsicmp(m_pep->GetValue(this, i), s))
				{
					m_e = i;

					if (m_pOwner && m_pOwner->m_pListener) m_pOwner->m_pListener->PropertyChanged(this);

					return true;
				}
			}
		}
		else
		{
			assert(m_es);

			size_t val = 0;
			for (TStringDeque::const_iterator it = m_es->cbegin(), last_it = m_es->cend(); it != last_it; it++, val++)
			{
				if (!_tcsicmp(it->c_str(), s))
				{
					m_e = val;

					if (m_pOwner && m_pOwner->m_pListener) m_pOwner->m_pListener->PropertyChanged(this);

					return true;
				}
			}
		}

		return false;
	}

	virtual const TCHAR *GetEnumString(size_t idx, TCHAR *ret, size_t retsize) const
	{
		if (m_Type == PT_ENUM)
		{
			if (m_Flags.IsSet(PROPFLAG_ENUMPROVIDER))
			{
				if (m_pep && (idx < m_pep->GetNumValues(this)))
				{
					return m_pep->GetValue(this, idx, ret, retsize);
				}
			}
			else if ((m_es != nullptr) && (idx < m_es->size()))
			{
				tstring &t = m_es->at(idx);
				if (ret && retsize)
				{
					_tcsnccpy_s(ret, retsize, t.c_str(), retsize);
					return ret;
				}

				return m_es->at(idx).c_str();
			}
		}

		return nullptr;
	}

	virtual const TCHAR *GetEnumStrings(TCHAR *ret, size_t retsize) const
	{
		if (m_Type == PT_ENUM)
		{
			if (!ret || (retsize == 0))
				return m_s;
			else if (retsize > 0)
			{
				_tcsncpy_s(ret, retsize, m_s, retsize);
				return ret;
			}
		}

		return nullptr;
	}

	virtual size_t GetMaxEnumVal() const
	{
		if (m_Type == PT_ENUM)
		{
			if (m_Flags.IsSet(PROPFLAG_ENUMPROVIDER))
			{
				if (m_pep)
					return m_pep->GetNumValues(this);
			}
			else
			{
				if (m_es)
					return m_es->size();
			}
		}

		return 0;
	}

	virtual void SetFromProperty(IProperty *pprop, bool overwrite_flags)
	{
		if (!pprop)
		{
			Reset();
			return;
		}

		if (overwrite_flags)
		{
			uint32_t res_flags = (1 << EPropFlag::RESERVED1) | (1 << EPropFlag::RESERVED2);
			m_Flags = ((uint32_t)m_Flags & res_flags) | ((uint32_t)(pprop->Flags()) & ~res_flags);
		}

		PROPERTY_TYPE t = (m_Flags.IsSet(PROPFLAG_REFERENCE) || m_Flags.IsSet(1 << EPropFlag::TYPELOCKED)) ? m_Type : pprop->GetType();

		switch (t)
		{
			case PT_STRING:
				SetString(pprop->AsString());
				break;

			case PT_INT:
				SetInt(pprop->AsInt());
				break;

			case PT_INT_V2:
			{
				TVec2I tmp;
				pprop->AsVec2I(&tmp);
				SetVec2I(tmp);
				break;
			}

			case PT_INT_V3:
			{
				TVec3I tmp;
				pprop->AsVec3I(&tmp);
				SetVec3I(tmp);
				break;
			}

			case PT_INT_V4:
			{
				TVec4I tmp;
				pprop->AsVec4I(&tmp);
				SetVec4I(tmp);
				break;
			}

			case PT_FLOAT:
				SetFloat(pprop->AsFloat());
				break;

			case PT_FLOAT_V2:
			{
				TVec2F tmp;
				pprop->AsVec2F(&tmp);
				SetVec2F(tmp);
				break;
			}

			case PT_FLOAT_V3:
			{
				TVec3F tmp;
				pprop->AsVec3F(&tmp);
				SetVec3F(tmp);
				break;
			}

			case PT_FLOAT_V4:
			{
				TVec4F tmp;
				pprop->AsVec4F(&tmp);
				SetVec4F(tmp);
				break;
			}

			case PT_FLOAT_MAT3X3:
			{
				SetMat3x3F(pprop->AsMat3x3F());
				break;
			}

			case PT_FLOAT_MAT4X4:
			{
				SetMat4x4F(pprop->AsMat4x4F());
				break;
			}

			case PT_GUID:
				SetGUID(pprop->AsGUID());
				break;

			case PT_BOOLEAN:
				SetBool(pprop->AsBool());
				break;

			case PT_ENUM:
				if (pprop->GetEnumProvider())
				{
					SetEnumProvider(pprop->GetEnumProvider());
				}
				else
				{
					SetEnumStrings(pprop->GetEnumStrings());
				}
				SetEnumVal((size_t)(pprop->AsInt()));
				break;
		}

		SetAspect(pprop->GetAspect());

		if (m_pOwner->m_pListener)
			m_pOwner->m_pListener->PropertyChanged(this);
	}

	virtual int64_t AsInt(int64_t *ret) const
	{
		int64_t retval;
		if (!ret)
			ret = &retval;

		switch (m_Type)
		{
			case PT_STRING:
				*ret = m_s ? (int64_t)_tstoi64(m_s) : 0;
				break;

			case PT_BOOLEAN:
				if (!m_Flags.IsSet(PROPFLAG_REFERENCE))
					*ret = m_b ? 1 : 0;
				else
					*ret = *p_b ? 1 : 0;
				break;

			case PT_INT:
				if (!m_Flags.IsSet(PROPFLAG_REFERENCE))
					*ret = m_i;
				else
					*ret = *p_i;
				break;

			case PT_FLOAT:
				if (!m_Flags.IsSet(PROPFLAG_REFERENCE))
					*ret = (int64_t)(m_f);
				else
					*ret = (int64_t)(*p_f);
				break;

			case PT_FLOAT_V3:
				if (m_Aspect == PA_COLOR_RGB)
				{
					uint8_t r, g, b;
					if (!m_Flags.IsSet(PROPFLAG_REFERENCE))
					{
						r = (uint8_t)(std::clamp<float>(m_v3f.x, 0.0f, 1.0f) * 255.0f);
						g = (uint8_t)(std::clamp<float>(m_v3f.y, 0.0f, 1.0f) * 255.0f);
						b = (uint8_t)(std::clamp<float>(m_v3f.z, 0.0f, 1.0f) * 255.0f);
					}
					else
					{
						r = (uint8_t)(std::clamp<float>(p_v3f->x, 0.0f, 1.0f) * 255.0f);
						g = (uint8_t)(std::clamp<float>(p_v3f->y, 0.0f, 1.0f) * 255.0f);
						b = (uint8_t)(std::clamp<float>(p_v3f->z, 0.0f, 1.0f) * 255.0f);
					}
					*ret = (int64_t)(RGB(r, g, b));
				}
				break;

			case PT_GUID:
				*ret = 0;
				break;

			case PT_ENUM:
				if (m_Flags.IsSet(PROPFLAG_ENUMPROVIDER))
				{
					*ret = m_pep ? ((m_e < m_pep->GetNumValues(this)) ? m_e : 0) : 0;
				}
				else
				{
					*ret = m_e;
				}
				break;
		}

		return *ret;
	}

	virtual const TVec2I *AsVec2I(TVec2I *ret = nullptr) const
	{
		switch (m_Type)
		{
			case PT_STRING:
				_stscanf_s(m_s, _T("%I64d,%I64d"), &ret->x, &ret->y);
				break;

			case PT_INT:
				if (ret)
				{
					if (!m_Flags.IsSet(PROPFLAG_REFERENCE))
						*ret = TVec2I(m_i, 0);
					else
						*ret = TVec2I(*p_i, 0);
				}
				break;

			case PT_FLOAT:
				if (ret)
				{
					if (!m_Flags.IsSet(PROPFLAG_REFERENCE))
						*ret = TVec2I(int64_t(m_f), 0);
					else
						*ret = TVec2I(int64_t(*p_f), 0);
				}
				break;

			case PT_INT_V2:
				if (ret)
				{
					if (!m_Flags.IsSet(PROPFLAG_REFERENCE))
						*ret = m_v2i;
					else
						*ret = *p_v2i;
				}

				return ret ? ret : (!m_Flags.IsSet(PROPFLAG_REFERENCE) ? &m_v2i : p_v2i);

				break;
		}

		return ret ? ret : nullptr;
	}

	virtual const TVec3I *AsVec3I(TVec3I *ret = nullptr) const
	{
		switch (m_Type)
		{
			case PT_STRING:
				_stscanf_s(m_s, _T("%I64d,%I64d,%I64d"), &ret->x, &ret->y, &ret->z);
				break;

			case PT_INT:
				if (ret)
				{
					if (!m_Flags.IsSet(PROPFLAG_REFERENCE))
						*ret = TVec3I(m_i, 0, 0);
					else
						*ret = TVec3I(*p_i, 0, 0);
				}
				break;

			case PT_FLOAT:
				if (ret)
				{
					if (!m_Flags.IsSet(PROPFLAG_REFERENCE))
						*ret = TVec3I(int64_t(m_f), 0, 0);
					else
						*ret = TVec3I(int64_t(*p_f), 0, 0);
				}
				break;

			case PT_INT_V2:
				if (ret)
				{
					if (!m_Flags.IsSet(PROPFLAG_REFERENCE))
						*ret = m_v2i;
					else
						*ret = *p_v2i;
				}
				break;

			case PT_INT_V3:
				if (ret)
				{
					if (!m_Flags.IsSet(PROPFLAG_REFERENCE))
						*ret = m_v3i;
					else
						*ret = *p_v3i;
				}

				return ret ? ret : (!m_Flags.IsSet(PROPFLAG_REFERENCE) ? &m_v3i : p_v3i);

				break;
		}

		return ret ? ret : nullptr;
	}

	virtual const TVec4I *AsVec4I(TVec4I *ret = nullptr) const
	{
		switch (m_Type)
		{
			case PT_STRING:
				_stscanf_s(m_s, _T("%I64d,%I64d,%I64d,%I64d"), &ret->x, &ret->y, &ret->z, &ret->w);
				break;

			case PT_INT:
				if (ret)
				{
					if (!m_Flags.IsSet(PROPFLAG_REFERENCE))
						*ret = TVec4I(m_i, 0, 0, 0);
					else
						*ret = TVec4I(*p_i, 0, 0, 0);
				}
				break;

			case PT_FLOAT:
				if (ret)
				{
					if (!m_Flags.IsSet(PROPFLAG_REFERENCE))
						*ret = TVec4I(int64_t(m_f), 0, 0, 0);
					else
						*ret = TVec4I(int64_t(*p_f), 0, 0, 0);
				}
				break;

			case PT_INT_V2:
				if (ret)
				{
					if (!m_Flags.IsSet(PROPFLAG_REFERENCE))
						*ret = m_v2i;
					else
						*ret = *p_v2i;
				}
				break;

			case PT_INT_V3:
				if (ret)
				{
					if (!m_Flags.IsSet(PROPFLAG_REFERENCE))
						*ret = m_v3i;
					else
						*ret = *p_v3i;
				}
				break;

			case PT_INT_V4:
				if (ret)
				{
					if (!m_Flags.IsSet(PROPFLAG_REFERENCE))
						*ret = m_v4i;
					else
						*ret = *p_v4i;
				}

				return ret ? ret : (!m_Flags.IsSet(PROPFLAG_REFERENCE) ? &m_v4i : p_v4i);

				break;
		}

		return ret ? ret : nullptr;
	}

	virtual float AsFloat(float *ret) const
	{
		float retval;
		if (!ret)
			ret = &retval;

		switch (m_Type)
		{
			case PT_STRING:
				*ret = (float)_tstof(m_s);
				break;

			case PT_BOOLEAN:
				if (!m_Flags.IsSet(PROPFLAG_REFERENCE))
					*ret = m_b ? 1.0f : 0.0f;
				else
					*ret = *p_b ? 1.0f : 0.0f;
				break;

			case PT_INT:
				if (!m_Flags.IsSet(PROPFLAG_REFERENCE))
					*ret = (float)m_i;
				else
					*ret = (float)*p_i;
				break;

			case PT_FLOAT:
				if (!m_Flags.IsSet(PROPFLAG_REFERENCE))
					*ret = m_f;
				else
					*ret = *p_f;
				break;

			case PT_GUID:
				*ret = 0.0f;
				break;
		}

		return *ret;
	}

	virtual const TVec2F *AsVec2F(TVec2F *ret) const
	{
		switch (m_Type)
		{
			case PT_STRING:
				_stscanf_s(m_s, _T("%f,%f"), &ret->x, &ret->y);
				break;

			case PT_INT:
				if (ret)
				{
					if (!m_Flags.IsSet(PROPFLAG_REFERENCE))
						*ret = TVec2F(float(m_i), 0.0f);
					else
						*ret = TVec2F(float(*p_i), 0.0f);
				}
				break;

			case PT_FLOAT:
				if (ret)
				{
					if (!m_Flags.IsSet(PROPFLAG_REFERENCE))
						*ret = TVec2F(m_f, 0.0f);
					else
						*ret = TVec2F(*p_f, 0.0f);
				}
				break;

			case PT_FLOAT_V2:
				if (ret)
				{
					if (!m_Flags.IsSet(PROPFLAG_REFERENCE))
						*ret = m_v2f;
					else
						*ret = *p_v2f;
				}

				return ret ? ret : (!m_Flags.IsSet(PROPFLAG_REFERENCE) ? &m_v2f : p_v2f);

				break;

			case PT_FLOAT_V3:
				if (ret)
				{
					if (!m_Flags.IsSet(PROPFLAG_REFERENCE))
						*ret = *((props::TVec2F *)&m_v3f);
					else
						*ret = *((props::TVec2F *)p_v3f);
				}

				return ret ? ret : (!m_Flags.IsSet(PROPFLAG_REFERENCE) ? (props::TVec2F *)&m_v3f : (props::TVec2F *)p_v3f);

				break;

			case PT_FLOAT_V4:
				if (ret)
				{
					if (!m_Flags.IsSet(PROPFLAG_REFERENCE))
						*ret = *((props::TVec2F *)&m_v4f);
					else
						*ret = *((props::TVec2F *)p_v4f);
				}

				return ret ? ret : (!m_Flags.IsSet(PROPFLAG_REFERENCE) ? (props::TVec2F *)&m_v4f : (props::TVec2F *)p_v4f);

				break;
		}

		return ret ? ret : nullptr;
	}

	virtual const TVec3F *AsVec3F(TVec3F *ret) const
	{
		switch (m_Type)
		{
			case PT_STRING:
				_stscanf_s(m_s, _T("%f,%f,%f"), &ret->x, &ret->y, &ret->z);
				break;

			case PT_INT:
				if (ret)
				{
					if (!m_Flags.IsSet(PROPFLAG_REFERENCE))
						*ret = TVec3F(float(m_i), 0.0f, 0.0f);
					else
						*ret = TVec3F(float(*p_i), 0.0f, 0.0f);
				}
				break;

			case PT_FLOAT:
				if (ret)
				{
					if (!m_Flags.IsSet(PROPFLAG_REFERENCE))
						*ret = TVec3F(m_f, 0.0f, 0.0f);
					else
						*ret = TVec3F(*p_f, 0.0f, 0.0f);
				}
				break;

			case PT_FLOAT_V2:
				if (ret)
				{
					if (!m_Flags.IsSet(PROPFLAG_REFERENCE))
						*ret = m_v2f;
					else
						*ret = *p_v2f;
				}
				break;

			case PT_FLOAT_V3:
				if (ret)
				{
					if (!m_Flags.IsSet(PROPFLAG_REFERENCE))
						*ret = m_v3f;
					else
						*ret = *p_v3f;
				}

				return ret ? ret : (!m_Flags.IsSet(PROPFLAG_REFERENCE) ? &m_v3f : p_v3f);

				break;

			case PT_FLOAT_V4:
				if (ret)
				{
					if (!m_Flags.IsSet(PROPFLAG_REFERENCE))
						*ret = *((props::TVec3F *)&m_v4f);
					else
						*ret = *((props::TVec3F *)p_v4f);
				}

				return ret ? ret : (!m_Flags.IsSet(PROPFLAG_REFERENCE) ? (props::TVec3F *)&m_v4f : (props::TVec3F *)p_v4f);

				break;
		}

		return ret ? ret : nullptr;
	}

	virtual const TVec4F *AsVec4F(TVec4F *ret) const
	{
		switch (m_Type)
		{
			case PT_STRING:
				_stscanf_s(m_s, _T("%f,%f,%f,%f"), &ret->x, &ret->y, &ret->z, &ret->w);
				break;

			case PT_INT:
				if (ret)
				{
					if (!m_Flags.IsSet(PROPFLAG_REFERENCE))
						*ret = TVec4F(float(m_i), 0.0f, 0.0f, 0.0f);
					else
						*ret = TVec4F(float(*p_i), 0.0f, 0.0f, 0.0f);
				}
				break;

			case PT_FLOAT:
				if (ret)
				{
					if (!m_Flags.IsSet(PROPFLAG_REFERENCE))
						*ret = TVec4F(m_f, 0.0f, 0.0f, 0.0f);
					else
						*ret = TVec4F(*p_f, 0.0f, 0.0f, 0.0f);
				}
				break;

			case PT_FLOAT_V2:
				if (ret)
				{
					if (!m_Flags.IsSet(PROPFLAG_REFERENCE))
						*ret = m_v2f;
					else
						*ret = *p_v2f;
				}
				break;

			case PT_FLOAT_V3:
				if (ret)
				{
					if (!m_Flags.IsSet(PROPFLAG_REFERENCE))
						*ret = m_v3f;
					else
						*ret = *p_v3f;
				}
				break;

			case PT_FLOAT_V4:
				if (ret)
				{
					if (!m_Flags.IsSet(PROPFLAG_REFERENCE))
						*ret = m_v4f;
					else
						*ret = *p_v4f;
				}

				return ret ? ret : (!m_Flags.IsSet(PROPFLAG_REFERENCE) ? &m_v4f : p_v4f);

				break;
		}

		return ret ? ret : nullptr;
	}

	virtual const TMat3x3F *AsMat3x3F(TMat3x3F *ret) const
	{
		if (m_Type == PT_FLOAT_MAT3X3)
		{
			if (ret)
			{
				if (!m_Flags.IsSet(PROPFLAG_REFERENCE))
					*ret = m_m3x3f;
				else
					*ret = *p_m3x3f;
			}

			return ret ? ret : (!m_Flags.IsSet(PROPFLAG_REFERENCE) ? &m_m3x3f : p_m3x3f);
		}

		return nullptr;
	}

	virtual const TMat4x4F *AsMat4x4F(TMat4x4F *ret) const
	{
		if (m_Type == PT_FLOAT_MAT4X4)
		{
			if (ret)
			{
				if (!m_Flags.IsSet(PROPFLAG_REFERENCE))
					*ret = m_m4x4f;
				else
					*ret = *p_m4x4f;
			}

			return ret ? ret : (!m_Flags.IsSet(PROPFLAG_REFERENCE) ? &m_m4x4f : p_m4x4f);
		}

		return nullptr;
	}

	virtual const TCHAR *AsString(TCHAR *ret, size_t retsize) const
	{
		if (m_Type == PT_STRING)
		{
			if (!ret || (retsize == 0))
				return m_s;
		}

		if (m_Type == PT_ENUM)
		{
			if (m_Flags.IsSet(PROPFLAG_ENUMPROVIDER))
			{
				if (m_pep)
				{
					if (!ret || (retsize == 0) && (m_e < (uint64_t)m_pep->GetNumValues(this)))
						return m_pep->GetValue(this, (size_t)m_e, ret, retsize);
					else
						return m_s;
				}
			}
			else
			{
				if (m_es)
				{
					if ((!ret || (retsize == 0)) && (m_e < m_es->size()))
						return m_es->at(m_e).c_str();
					else
						return m_s ? m_s : _T("");
				}
			}
		}

		if (retsize > 0)
		{
			switch (m_Type)
			{
				case PT_STRING:
					_tcsncpy_s(ret, retsize, m_s ? m_s : _T(""), retsize);
					break;

				case PT_BOOLEAN:
					switch (m_Aspect)
					{
						case PA_BOOL_ONOFF:
							_tcscpy_s(ret, retsize, m_b ? _T("on") : _T("off"));
							break;
						case PA_BOOL_YESNO:
							_tcscpy_s(ret, retsize, m_b ? _T("yes") : _T("no"));
							break;
						case PA_BOOL_TRUEFALSE:
							_tcscpy_s(ret, retsize, m_b ? _T("true") : _T("false"));
							break;
						case PA_BOOL_ABLED:
							_tcscpy_s(ret, retsize, m_b ? _T("enabled") : _T("disabled"));
							break;
						default:
							_tcscpy_s(ret, retsize, m_b ? _T("1") : _T("0"));
							break;
					}
					break;

				case PT_INT:
					_sntprintf_s(ret, retsize, retsize, _T("%I64d"), m_i);
					break;

				case PT_INT_V2:
					_sntprintf_s(ret, retsize, retsize, _T("%I64d,%I64d"), m_v2i.x, m_v2i.y);
					break;

				case PT_INT_V3:
					_sntprintf_s(ret, retsize, retsize, _T("%I64d,%I64d,%I64d"), m_v3i.x, m_v3i.y, m_v3i.z);
					break;

				case PT_INT_V4:
					_sntprintf_s(ret, retsize, retsize, _T("%I64d,%I64d,%I64d,%I64d"), m_v4i.x, m_v4i.y, m_v4i.z, m_v4i.w);
					break;

				case PT_FLOAT:
					_sntprintf_s(ret, retsize, retsize, _T("%f"), m_f);
					break;

				case PT_FLOAT_V2:
					_sntprintf_s(ret, retsize, retsize, _T("%f,%f"), m_v2f.x, m_v2f.y);
					break;

				case PT_FLOAT_V3:
					_sntprintf_s(ret, retsize, retsize, _T("%f,%f,%f"), m_v3f.x, m_v3f.y, m_v3f.z);
					break;

				case PT_FLOAT_V4:
					_sntprintf_s(ret, retsize, retsize, _T("%f,%f,%f,%f"), m_v4f.x, m_v4f.y, m_v4f.z, m_v4f.w);
					break;

				case PT_FLOAT_MAT3X3:
					_sntprintf_s(ret, retsize, retsize, _T("%f,%f,%f,%f,%f,%f,%f,%f,%f"),
						m_m3x3f.m[0].x, m_m3x3f.m[0].y, m_m3x3f.m[0].z,
						m_m3x3f.m[1].x, m_m3x3f.m[1].y, m_m3x3f.m[1].z,
						m_m3x3f.m[2].x, m_m3x3f.m[2].y, m_m3x3f.m[2].z);
					break;

				case PT_FLOAT_MAT4X4:
					_sntprintf_s(ret, retsize, retsize, _T("%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f"),
						m_m4x4f.m[0].x, m_m4x4f.m[0].y, m_m4x4f.m[0].z, m_m4x4f.m[0].w,
						m_m4x4f.m[1].x, m_m4x4f.m[1].y, m_m4x4f.m[1].z, m_m4x4f.m[1].w,
						m_m4x4f.m[2].x, m_m4x4f.m[2].y, m_m4x4f.m[2].z, m_m4x4f.m[2].w,
						m_m4x4f.m[3].x, m_m4x4f.m[3].y, m_m4x4f.m[3].z, m_m4x4f.m[3].w);
					break;

				case PT_GUID:
					_sntprintf_s(ret, retsize, retsize, _T("{%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}"), m_g.Data1, m_g.Data2, m_g.Data3,
						m_g.Data4[0], m_g.Data4[1], m_g.Data4[2], m_g.Data4[3], m_g.Data4[4], m_g.Data4[5], m_g.Data4[6], m_g.Data4[7]);
					break;
			}
		}

		return ret;
	}

	virtual GUID AsGUID(GUID *ret) const
	{
		GUID retval;

		if (!ret)
			ret = &retval;

		memset(ret, 0, sizeof(GUID));

		switch (m_Type)
		{
			case PT_STRING:
			{
				int d[11];
				_sntscanf_s(m_s, _tcslen(m_s) * sizeof(TCHAR), _T("{%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}"), &d[0], &d[1], &d[2],
					&d[3], &d[4], &d[5], &d[6], &d[7], &d[8], &d[9], &d[10]);

				ret->Data1 = d[0];
				ret->Data2 = d[1];
				ret->Data3 = d[2];
				ret->Data4[0] = d[3];
				ret->Data4[1] = d[4];
				ret->Data4[2] = d[5];
				ret->Data4[3] = d[6];
				ret->Data4[4] = d[7];
				ret->Data4[5] = d[8];
				ret->Data4[6] = d[9];
				ret->Data4[7] = d[10];
				break;
			}

			case PT_INT:
				break;

			case PT_FLOAT:
				break;

			case PT_GUID:
				if (!m_Flags.IsSet(PROPFLAG_REFERENCE))
					*ret = m_g;
				else
					*ret = *p_g;
				break;
		}

		return *ret;
	}

	virtual bool AsBool(bool *ret) const
	{
		bool retval = false;

		if (!ret)
			ret = &retval;

		if (m_Type == PT_BOOLEAN)
		{
			if (!m_Flags.IsSet(PROPFLAG_REFERENCE))
				*ret = m_b;
			else
				*ret = *p_b;
		}
		else if (m_Type == PT_INT)
		{
			if (!m_Flags.IsSet(PROPFLAG_REFERENCE))
				*ret = (m_i == 0) ? false : true;
			else
				*ret = (*p_i == 0) ? false : true;
		}
		else if (m_Type == PT_STRING)
		{
			if (!_tcsicmp(m_s, _T("0")) || !_tcsicmp(m_s, _T("false")) || !_tcsicmp(m_s, _T("no")) || !_tcsicmp(m_s, _T("off")) || !_tcsicmp(m_s, _T("disabled")))
				*ret = false;
			else if (!_tcsicmp(m_s, _T("1")) || !_tcsicmp(m_s, _T("true")) || !_tcsicmp(m_s, _T("yes")) || !_tcsicmp(m_s, _T("on")) || !_tcsicmp(m_s, _T("enabled")))
				*ret = true;
		}

		return *ret;
	}

	virtual bool Serialize(SERIALIZE_MODE mode, BYTE *buf, size_t bufsize, size_t *amountused = NULL) const
	{
		if (m_Type >= PT_NUMTYPES)
			return false;

		size_t sz = sizeof(BYTE) /*serialization type*/ + sizeof(FOURCHARCODE) /*id*/ + sizeof(BYTE) /*PROPERTY_TYPE*/;

		if (mode >= SM_BIN_TERSE)
			sz += sizeof(BYTE); /*PROPERTY_ASPECT*/

		if (mode == SM_BIN_VERBOSE)
			sz += (m_sName.length() + 1) * sizeof(TCHAR);

		switch (m_Type)
		{
			case PT_STRING:
				sz += (_tcslen(m_s) + 1) * sizeof(TCHAR);
				break;

			case PT_INT:
				sz += sizeof(int64_t);
				break;

			case PT_INT_V2:
				sz += sizeof(TVec2I);
				break;

			case PT_INT_V3:
				sz += sizeof(TVec3I);
				break;

			case PT_INT_V4:
				sz += sizeof(TVec4I);
				break;

			case PT_FLOAT:
				sz += sizeof(float);
				break;

			case PT_FLOAT_V2:
				sz += sizeof(TVec2F);
				break;

			case PT_FLOAT_V3:
				sz += sizeof(TVec3F);
				break;

			case PT_FLOAT_V4:
				sz += sizeof(TVec4F);
				break;

			case PT_GUID:
				sz += sizeof(GUID);
				break;

			case PT_ENUM:
			{
				sz += ((m_s ? _tcslen(m_s) : 0) + 1) * sizeof(TCHAR);
				sz += sizeof(uint64_t);
				break;
			}

			case PT_BOOLEAN:
				sz += sizeof(bool);
				break;

			case PT_FLOAT_MAT3X3:
				sz += sizeof(TMat3x3F);
				break;

			case PT_FLOAT_MAT4X4:
				sz += sizeof(TMat4x4F);
				break;
		}

		if (amountused)
			*amountused = sz;

		if (buf && (bufsize < sz))
			return false;
		else if (!buf)
			return true;

		*buf = BYTE(mode);
		buf += sizeof(BYTE);

		*((FOURCHARCODE *)buf) = m_ID;
		buf += sizeof(FOURCHARCODE);

		*buf = BYTE(m_Type);
		buf += sizeof(BYTE);

		if (mode >= SM_BIN_TERSE)
		{
			*buf = BYTE(m_Aspect);
			buf += sizeof(BYTE);
		}

		if (mode == SM_BIN_VERBOSE)
		{
			memcpy(buf, m_sName.c_str(), sizeof(TCHAR) * (m_sName.length() + 1));
			buf += sizeof(TCHAR) * (m_sName.length() + 1);
		}

		switch (m_Type)
		{
			case PT_STRING:
			{
				size_t bs = sizeof(TCHAR) * (_tcslen(m_s) + 1);
				memcpy(buf, m_s, bs);
				buf += bs;
				break;
			}

			case PT_INT:
				*((int64_t *)buf) = m_Flags.IsSet(PROPFLAG_REFERENCE) ? *p_i : m_i;
				buf += sizeof(int64_t);
				break;

			case PT_INT_V2:
				*((TVec2I *)buf) = m_Flags.IsSet(PROPFLAG_REFERENCE) ? *p_v2i : m_v2i;
				buf += sizeof(TVec2I);
				break;

			case PT_INT_V3:
				*((TVec3I *)buf) = m_Flags.IsSet(PROPFLAG_REFERENCE) ? *p_v3i : m_v3i;
				buf += sizeof(TVec3I);
				break;

			case PT_INT_V4:
				*((TVec4I *)buf) = m_Flags.IsSet(PROPFLAG_REFERENCE) ? *p_v4i : m_v4i;
				buf += sizeof(TVec4I);
				break;

			case PT_FLOAT:
				*((float *)buf) = m_Flags.IsSet(PROPFLAG_REFERENCE) ? *p_f : m_f;
				buf += sizeof(float);
				break;

			case PT_FLOAT_V2:
				*((TVec2F *)buf) = m_Flags.IsSet(PROPFLAG_REFERENCE) ? *p_v2f : m_v2f;
				buf += sizeof(TVec2F);
				break;

			case PT_FLOAT_V3:
				*((TVec3F *)buf) = m_Flags.IsSet(PROPFLAG_REFERENCE) ? *p_v3f : m_v3f;
				buf += sizeof(TVec3F);
				break;

			case PT_FLOAT_V4:
				*((TVec4F *)buf) = m_Flags.IsSet(PROPFLAG_REFERENCE) ? *p_v4f : m_v4f;
				buf += sizeof(TVec4F);
				break;

			case PT_GUID:
				*((GUID *)buf) = m_Flags.IsSet(PROPFLAG_REFERENCE) ? *p_g : m_g;
				buf += sizeof(GUID);
				break;

			case PT_BOOLEAN:
				*((bool *)buf) = m_Flags.IsSet(PROPFLAG_REFERENCE) ? *p_b : m_b;
				buf += sizeof(bool);
				break;

			case PT_ENUM:
			{
				size_t bs;

				bs = ((m_s ? _tcslen(m_s) : 0) + 1) * sizeof(TCHAR);
				memcpy(buf, m_s ? m_s : _T(""), bs);
				buf += bs;

				*(uint64_t *)buf = m_e;
				buf += sizeof(uint64_t);
				break;
			}

			case PT_FLOAT_MAT3X3:
				*((TMat3x3F *)buf) = m_Flags.IsSet(PROPFLAG_REFERENCE) ? *p_m3x3f : m_m3x3f;
				buf += sizeof(TMat3x3F);
				break;

			case PT_FLOAT_MAT4X4:
				*((TMat4x4F *)buf) = m_Flags.IsSet(PROPFLAG_REFERENCE) ? *p_m4x4f : m_m4x4f;
				buf += sizeof(TMat4x4F);
				break;
		}

		return true;
	}

	virtual bool Deserialize(BYTE *buf, size_t bufsize, size_t *bytesconsumed)
	{
		Reset();

		BYTE *origbuf = buf;
		if (!buf)
			return false;

		SERIALIZE_MODE mode = SERIALIZE_MODE(*buf);
		if (mode > SM_BIN_VERBOSE)
			return false;
		buf += sizeof(BYTE);

		m_ID = *((FOURCHARCODE *)buf);
		buf += sizeof(FOURCHARCODE);

		m_Type = PROPERTY_TYPE(*buf);
		if (m_Type >= PT_NUMTYPES)
			return false;
		buf += sizeof(BYTE);

		if (mode >= SM_BIN_TERSE)
		{
			m_Aspect = PROPERTY_ASPECT(*buf);
			if (m_Aspect >= PA_NUMASPECTS)
				return false;
			buf += sizeof(BYTE);
		}

		if (mode == SM_BIN_VERBOSE)
		{
			m_sName = (TCHAR *)buf;
			buf += sizeof(TCHAR) * (m_sName.length() + 1);
		}

		switch (m_Type)
		{
			case PT_STRING:
			{
				m_s = _tcsdup((TCHAR *)buf);
				buf += sizeof(TCHAR) * (_tcslen(m_s) + 1);
				break;
			}

			case PT_INT:
				if (m_Flags.IsSet(PROPFLAG_REFERENCE))
					*p_i = *((int64_t *)buf);
				else
					m_i = *((int64_t *)buf);
				buf += sizeof(int64_t);
				break;

			case PT_INT_V2:
				if (m_Flags.IsSet(PROPFLAG_REFERENCE))
					*p_v2i = *((TVec2I *)buf);
				else
					m_v2i = *((TVec2I *)buf);
				buf += sizeof(TVec2I);
				break;

			case PT_INT_V3:
				if (m_Flags.IsSet(PROPFLAG_REFERENCE))
					*p_v3i = *((TVec3I *)buf);
				else
					m_v3i = *((TVec3I *)buf);
				buf += sizeof(TVec3I);
				break;

			case PT_INT_V4:
				if (m_Flags.IsSet(PROPFLAG_REFERENCE))
					*p_v4i = *((TVec4I *)buf);
				else
					m_v4i = *((TVec4I *)buf);
				buf += sizeof(TVec4I);
				break;

			case PT_FLOAT:
				if (m_Flags.IsSet(PROPFLAG_REFERENCE))
					*p_f = *((float *)buf);
				else
					m_f = *((float *)buf);
				buf += sizeof(float);
				break;

			case PT_FLOAT_V2:
				if (m_Flags.IsSet(PROPFLAG_REFERENCE))
					*p_v2f = *((TVec2F *)buf);
				else
					m_v2f = *((TVec2F *)buf);
				buf += sizeof(TVec2F);
				break;

			case PT_FLOAT_V3:
				if (m_Flags.IsSet(PROPFLAG_REFERENCE))
					*p_v3f = *((TVec3F *)buf);
				else
					m_v3f = *((TVec3F *)buf);
				buf += sizeof(TVec3F);
				break;

			case PT_FLOAT_V4:
				if (m_Flags.IsSet(PROPFLAG_REFERENCE))
					*p_v4f = *((TVec4F *)buf);
				else
					m_v4f = *((TVec4F *)buf);
				buf += sizeof(TVec4F);
				break;

			case PT_GUID:
				if (m_Flags.IsSet(PROPFLAG_REFERENCE))
					*p_g = *((GUID *)buf);
				else
					m_g = *((GUID *)buf);
				buf += sizeof(GUID);
				break;

			case PT_BOOLEAN:
				if (m_Flags.IsSet(PROPFLAG_REFERENCE))
					*p_b = *((bool *)buf);
				else
					m_b = *((bool *)buf);
				buf += sizeof(bool);
				break;

			case PT_ENUM:
			{
				TCHAR *tmp = (*buf != _T('\0')) ? _tcsdup((TCHAR *)buf) : nullptr;
				buf += ((tmp ? _tcslen(tmp) : 0) + 1) * sizeof(TCHAR);
				if (!m_Flags.IsSet(PROPFLAG_ENUMPROVIDER))
					SetEnumStrings(tmp);

				m_e = *((uint64_t *)buf);
				buf += sizeof(uint64_t);
				free(tmp);
				break;
			}

			case PT_FLOAT_MAT3X3:
				if (m_Flags.IsSet(PROPFLAG_REFERENCE))
					*p_m3x3f = *((TMat3x3F *)buf);
				else
					m_m3x3f = *((TMat3x3F *)buf);
				buf += sizeof(TMat3x3F);
				break;

			case PT_FLOAT_MAT4X4:
				if (m_Flags.IsSet(PROPFLAG_REFERENCE))
					*p_m4x4f = *((TMat4x4F *)buf);
				else
					m_m4x4f = *((TMat4x4F *)buf);
				buf += sizeof(TMat4x4F);
				break;
		}

		if (bytesconsumed)
			*bytesconsumed = (buf - origbuf);

		return (size_t(buf - origbuf) <= bufsize) ? true : false;
	}

	virtual bool IsSameAs(const IProperty *other_prop) const
	{
		if ((!other_prop) || (other_prop->GetID() != m_ID))
			return false;

		CProperty *p = (CProperty *)other_prop;

		PROPERTY_TYPE other_type = p->GetType();

		if (other_type != m_Type)
			return false;

		if ((other_type == PROPERTY_TYPE::PT_STRING) && _tcscmp(p->m_s, m_s))
			return false;

		switch (other_type)
		{
			case PROPERTY_TYPE::PT_BOOLEAN:
				if (p->m_b != m_b)
					return false;
				break;

			case PROPERTY_TYPE::PT_ENUM:
				if (p->m_e != m_e)
					return false;
				break;

			case PROPERTY_TYPE::PT_INT:
				if (p->m_i != m_i)
					return false;
				break;

			case PROPERTY_TYPE::PT_FLOAT:
				if (p->m_f != m_f)
					return false;
				break;

			case PROPERTY_TYPE::PT_FLOAT_V2:
				if (p->m_v2f != m_v2f)
					return false;
				break;

			case PROPERTY_TYPE::PT_FLOAT_V3:
				if (p->m_v3f != m_v3f)
					return false;
				break;

			case PROPERTY_TYPE::PT_FLOAT_V4:
				if (p->m_v4f != m_v4f)
					return false;
				break;

			case PROPERTY_TYPE::PT_INT_V2:
				if (p->m_v2i != m_v2i)
					return false;
				break;

			case PROPERTY_TYPE::PT_INT_V3:
				if (p->m_v3i != m_v3i)
					return false;
				break;

			case PROPERTY_TYPE::PT_INT_V4:
				if (p->m_v4i != m_v4i)
					return false;
				break;

			case PROPERTY_TYPE::PT_FLOAT_MAT3X3:
				if (p->m_m3x3f != m_m3x3f)
					return false;
				break;

			case PROPERTY_TYPE::PT_FLOAT_MAT4X4:
				if (p->m_m4x4f != m_m4x4f)
					return false;
				break;

			case PROPERTY_TYPE::PT_GUID:
				if (p->m_g != m_g)
					return false;
				break;
		}

		return true;
	}

	virtual void ExternalizeReference()
	{
		if (GetEnumProvider())
		{
			TStringDeque *tmp = new TStringDeque();
			for (size_t i = 0, maxi = m_pep->GetNumValues(this); i < maxi; i++)
				tmp->push_back(tstring(m_pep->GetValue(this, i)));
			m_es = tmp;

			m_Flags.Clear(PROPFLAG_ENUMPROVIDER);
		}

		if (m_Flags.IsSet(PROPFLAG_REFERENCE))
		{
			// this might seem like overkill, but these are unioned...
			switch (m_Type)
			{
				case PROPERTY_TYPE::PT_BOOLEAN:
				{
					auto tmp = *p_b;
					m_b = tmp;
					break;
				}

				case PROPERTY_TYPE::PT_ENUM:
				case PROPERTY_TYPE::PT_INT:
				{
					auto tmp = *p_i;
					m_i = tmp;
					break;
				}

				case PROPERTY_TYPE::PT_FLOAT:
				{
					auto tmp = *p_f;
					m_f = tmp;
					break;
				}

				case PROPERTY_TYPE::PT_FLOAT_V2:
				{
					auto tmp = *p_v2f;
					m_v2f = tmp;
					break;
				}

				case PROPERTY_TYPE::PT_FLOAT_V3:
				{
					auto tmp = *p_v3f;
					m_v3f = tmp;
					break;
				}

				case PROPERTY_TYPE::PT_FLOAT_V4:
				{
					auto tmp = *p_v4f;
					m_v4f = tmp;
					break;
				}

				case PROPERTY_TYPE::PT_INT_V2:
				{
					auto tmp = *p_v2i;
					m_v2i = tmp;
					break;
				}

				case PROPERTY_TYPE::PT_INT_V3:
				{
					auto tmp = *p_v3i;
					m_v3i = tmp;
					break;
				}

				case PROPERTY_TYPE::PT_INT_V4:
				{
					auto tmp = *p_v4i;
					m_v4i = tmp;
					break;
				}

				case PROPERTY_TYPE::PT_FLOAT_MAT3X3:
				{
					auto tmp = *p_m3x3f;
					m_m3x3f = tmp;
					break;
				}

				case PROPERTY_TYPE::PT_FLOAT_MAT4X4:
				{
					auto tmp = *p_m4x4f;
					m_m4x4f = tmp;
					break;
				}

				case PROPERTY_TYPE::PT_GUID:
				{
					auto tmp = *p_g;
					m_g = tmp;
					break;
				}
			}

			m_Flags.Clear(PROPFLAG_REFERENCE);
		}
	}

};


CPropertySet::CPropertySet()
{
	m_pListener = nullptr;
}

CPropertySet::~CPropertySet()
{
	DeleteAll();
}

void CPropertySet::Release()
{
	delete this;
}


IProperty *CPropertySet::CreateProperty(const TCHAR *propname, FOURCHARCODE propid)
{
	TPropertyMap::const_iterator pi = m_mapProps.find(propid);
	if ((pi != m_mapProps.end()) && pi->second)
	{
		// if the property already existed, then alert the listener to it's value
		if (m_pListener)
			m_pListener->PropertyChanged(pi->second);

		return pi->second;
	}

	CProperty *pprop = new CProperty(this);
	if (pprop)
	{
		pprop->SetName(propname ? propname : _T(""));
		pprop->SetID(propid);

		AddProperty(pprop);
	}

	return pprop;
}


IProperty *CPropertySet::CreateReferenceProperty(const TCHAR *propname, FOURCHARCODE propid, void *addr, IProperty::PROPERTY_TYPE type)
{
	// string and enum types are not allowed for reference properties
	if ((type == IProperty::PT_NONE) || (type == IProperty::PT_STRING) || (type == IProperty::PT_ENUM) || (type >= IProperty::PT_NUMTYPES))
		return nullptr;

	// reference properties with duplicate IDs are not allowed
	TPropertyMap::const_iterator pi = m_mapProps.find(propid);
	if ((pi != m_mapProps.end()) && pi->second)
		return pi->second;

	CProperty *pprop = new CProperty(this);
	if (!pprop)
		return nullptr;

	pprop->m_sName = propname;
	pprop->m_ID = propid;
	pprop->m_Flags.Set(PROPFLAG_REFERENCE | props::IProperty::PROPFLAG(props::IProperty::TYPELOCKED));
	pprop->m_Type = type;

	switch (type)
	{
		case IProperty::PROPERTY_TYPE::PT_INT:
			pprop->p_i = (int64_t *)addr;
			break;

		case IProperty::PROPERTY_TYPE::PT_INT_V2:
			pprop->p_v2i = (TVec2I *)addr;
			break;

		case IProperty::PROPERTY_TYPE::PT_INT_V3:
			pprop->p_v3i = (TVec3I *)addr;
			break;

		case IProperty::PROPERTY_TYPE::PT_INT_V4:
			pprop->p_v4i = (TVec4I *)addr;
			break;

		case IProperty::PROPERTY_TYPE::PT_FLOAT:
			pprop->p_f = (float *)addr;
			break;

		case IProperty::PROPERTY_TYPE::PT_FLOAT_V2:
			pprop->p_v2f = (TVec2F *)addr;
			break;

		case IProperty::PROPERTY_TYPE::PT_FLOAT_V3:
			pprop->p_v3f = (TVec3F *)addr;
			break;

		case IProperty::PROPERTY_TYPE::PT_FLOAT_V4:
			pprop->p_v4f = (TVec4F *)addr;
			break;

		case IProperty::PROPERTY_TYPE::PT_FLOAT_MAT3X3:
			pprop->p_m3x3f = (TMat3x3F *)addr;
			break;

		case IProperty::PROPERTY_TYPE::PT_FLOAT_MAT4X4:
			pprop->p_m4x4f = (TMat4x4F *)addr;
			break;

		case IProperty::PROPERTY_TYPE::PT_BOOLEAN:
			pprop->p_b = (bool *)addr;
			break;

		case IProperty::PROPERTY_TYPE::PT_GUID:
			pprop->p_g = (GUID *)addr;
			break;
	}

	AddProperty(pprop);

	return pprop;
}

void CPropertySet::AddProperty(IProperty *pprop)
{
	if (!pprop)
		return;

	uint32_t propid = pprop->GetID();
	m_mapProps.insert(TPropertyMapPair(propid, pprop));
	m_Props.insert(m_Props.end(), pprop);
}


void CPropertySet::DeleteProperty(size_t idx)
{
	if (idx >= m_Props.size())
		return;

	IProperty *pprop = m_Props[idx];
	if (pprop)
	{
		TPropertyArray::iterator pia = m_Props.begin();
		pia += idx;
		m_Props.erase(pia);

		TPropertyMap::iterator pim = m_mapProps.find(pprop->GetID());
		m_mapProps.erase(pim);

		pprop->Release();
	}
}

void CPropertySet::DeletePropertyById(FOURCHARCODE propid)
{
	TPropertyMap::iterator j = m_mapProps.find(propid);
	if (j != m_mapProps.end())
		m_mapProps.erase(j);

	TPropertyArray::const_iterator e = m_Props.end();
	for (TPropertyArray::iterator i = m_Props.begin(); i != e; i++)
	{
		IProperty *pprop = *i;

		if (pprop->GetID() == propid)
		{
			m_Props.erase(i);

			pprop->Release();
			break;
		}
	}
}


void CPropertySet::DeletePropertyByName(const TCHAR *propname)
{
	TPropertyArray::const_iterator e = m_Props.end();
	for (TPropertyArray::iterator i = m_Props.begin(); i != e; i++)
	{
		IProperty *pprop = *i;

		if (!_tcsicmp(pprop->GetName(), propname))
		{
			TPropertyMap::iterator j = m_mapProps.find(pprop->GetID());
			if (j != m_mapProps.end())
				m_mapProps.erase(j);

			m_Props.erase(i);

			pprop->Release();
			return;
		}
	}
}


void CPropertySet::DeleteAll()
{
	for (uint32_t i = 0; i < m_Props.size(); i++)
	{
		IProperty *pprop = m_Props[i];
		pprop->Release();
	}

	m_Props.clear();
	m_mapProps.clear();
}


size_t CPropertySet::GetPropertyCount() const
{
	return m_Props.size();
}


IProperty *CPropertySet::GetProperty(size_t idx) const
{
	if (idx < m_Props.size())
		return m_Props[idx];

	return NULL;
}


IProperty *CPropertySet::GetPropertyById(props::FOURCHARCODE propid) const
{
	TPropertyMap::const_iterator j = m_mapProps.find(propid);
	if (j != m_mapProps.end())
		return j->second;

	return NULL;
}


IProperty *CPropertySet::GetPropertyByName(const TCHAR *propname) const
{
	TPropertyArray::const_iterator e = m_Props.end();
	for (TPropertyArray::const_iterator i = m_Props.begin(); i != e; i++)
	{
		IProperty *pprop = *i;

		if (!_tcsicmp(pprop->GetName(), propname))
			return pprop;
	}

	return NULL;
}


CPropertySet &CPropertySet::operator =(IPropertySet *propset)
{
	DeleteAll();

	for (uint32_t i = 0; i < propset->GetPropertyCount(); i++)
	{
		IProperty *pother = propset->GetProperty(i);
		if (!pother)
			continue;

		IProperty *pnew = CreateProperty(pother->GetName(), pother->GetID());
		if (pnew)
		{
			pnew->SetFromProperty(pother);

			if (m_pListener)
				m_pListener->PropertyChanged(pnew);
		}
	}

	return *this;
}


CPropertySet &CPropertySet::operator +=(IPropertySet *propset)
{
	AppendPropertySet(propset);

	return *this;
}


void CPropertySet::AppendPropertySet(const IPropertySet *propset, bool overwrite_flags)
{
	for (uint32_t i = 0; i < propset->GetPropertyCount(); i++)
	{
		IProperty *po = propset->GetProperty(i);
		if (!po)
			continue;

		IProperty *pp = GetPropertyById(po->GetID());
		if (!pp)
		{
			pp = CreateProperty(po->GetName(), po->GetID());
		}

		if (pp)
		{
			pp->SetFromProperty(po, overwrite_flags);
		}
	}
}

bool CPropertySet::Serialize(IProperty::SERIALIZE_MODE mode, BYTE *buf, size_t bufsize, size_t *amountused) const
{
	size_t used = sizeof(short);

	for (TPropertyMap::const_iterator it = m_mapProps.begin(), last_it = m_mapProps.end(); it != last_it; it++)
	{
		size_t pused = 0;
		CProperty *p = (CProperty *)(it->second);
		p->Serialize(mode, nullptr, 0, &pused);
		used += pused;
	}

	if (amountused)
		*amountused = used;

	if (used > bufsize)
		return false;

	*((short *)buf) = short(m_mapProps.size());
	bufsize -= sizeof(short);
	buf += sizeof(short);

	for (TPropertyMap::const_iterator it = m_mapProps.begin(), last_it = m_mapProps.end(); it != last_it; it++)
	{
		size_t pused = 0;
		CProperty *p = (CProperty *)(it->second);
		if (!p->Serialize(mode, buf, bufsize, &pused))
			return false;

		buf += pused;
		bufsize -= pused;
	}

	return true;
}

bool CPropertySet::Deserialize(BYTE *buf, size_t bufsize, size_t *bytesconsumed)
{
	if (!buf)
		return false;

	short numprops = *((short *)buf);
	buf += sizeof(short);
	bufsize -= sizeof(short);
	size_t remaining = bufsize;

	size_t consumed;

	for (short i = 0; i < numprops; i++)
	{
		BYTE *tmp = buf;
		tmp += sizeof(BYTE);	// serialize mode
		FOURCHARCODE id = *((FOURCHARCODE *)tmp);

		CProperty *p = (CProperty *)GetPropertyById(id);
		if (!p)
		{
			p = new CProperty(this);
			if (p)
			{
				p->SetID(id);
				AddProperty(p);
			}
		}

		if (!p || !p->Deserialize(buf, bufsize, &consumed))
			return false;

		buf += consumed;
		remaining -= consumed;

		if (remaining > bufsize)
			return false;
	}

	if (bytesconsumed)
		*bytesconsumed = consumed;

	return true;
}


void CPropertySet::SetChangeListener(const IPropertyChangeListener *plistener)
{
	m_pListener = (IPropertyChangeListener *)plistener;
}

namespace props
{

void UnescapeString(const TCHAR *in, tstring &out)
{
	out.clear();
	out.reserve(_tcslen(in));
	const TCHAR *c = in;
	while (c && *c)
	{
		if (*c == _T('&'))
		{
			if (!memcmp(c, _T("&lt;"), sizeof(TCHAR) * 4))
			{
				out += _T('<');
				c += 3;
			}
			else if (!memcmp(c, _T("&gt;"), sizeof(TCHAR) * 4))
			{
				out += _T('>');
				c += 3;
			}
			else if (!memcmp(c, _T("&amp;"), sizeof(TCHAR) * 5))
			{
				out += _T('&');
				c += 4;
			}
			else if (!memcmp(c, _T("&quot;"), sizeof(TCHAR) * 6))
			{
				out += _T('\"');
				c += 5;
			}
		}
		else
			out += *c;

		c++;
	}
}

void EscapeString(const TCHAR *in, tstring &out)
{
	out.clear();
	out.reserve(_tcslen(in) * 2);
	const TCHAR *c = in;
	while (c && *c)
	{
		switch (*c)
		{
			case _T('<'):
			{
				out += _T("&lt;");
				break;
			}
			case _T('>'):
			{
				out += _T("&gt;");
				break;
			}
			case _T('&'):
			{
				out += _T("&amp;");
				break;
			}
			case _T('\"'):
			{
				out += _T("&quot;");
				break;
			}
			default:
			{
				out += *c;
				break;
			}
		};

		c++;
	}
}

};

bool CPropertySet::SerializeToXMLString(IProperty::SERIALIZE_MODE mode, tstring &xmls) const
{
	xmls.clear();

	// reserve 16K for the string
	xmls.reserve(1 << 14);

	xmls += _T("<powerprops:property_set>\n");

	for (TPropertyMap::const_iterator it = m_mapProps.cbegin(); it != m_mapProps.cend(); it++)
	{
		if (!it->second)
			continue;

		xmls += _T("<powerprops:property ");

		xmls += _T("id=\"");
		FOURCHARCODE id = it->second->GetID();
		uint8_t *pid = (uint8_t *)&id;

		tstring idtemp;
		for (size_t i = 0; i < 4; i++)
		{
			TCHAR c = pid[3 - i];
			if (c)
			{
				if (isalnum(c))
				{
					idtemp += c;
				}
				else
				{
					idtemp += _T("&#");
					TCHAR num[8];
					_itot_s(c, num, 10);
					idtemp += num;
					idtemp += _T(";");
				}
			}
		}
		xmls += idtemp;
		xmls += _T("\"");

		if (mode > props::IProperty::SERIALIZE_MODE::SM_BIN_TERSE)
		{
			xmls += _T(" name=\"");
			tstring _name = it->second->GetName(), name;
			props::EscapeString(_name.c_str(), name);
			xmls += name;
			xmls += _T("\"");
		}

		xmls += _T(" type=\"");
		switch (it->second->GetType())
		{
			case props::IProperty::PROPERTY_TYPE::PT_BOOLEAN:
				xmls += _T("BOOLEAN");
				break;
			case props::IProperty::PROPERTY_TYPE::PT_ENUM:
				xmls += _T("ENUM");
				break;
			case props::IProperty::PROPERTY_TYPE::PT_FLOAT:
				xmls += _T("FLOAT");
				break;
			case props::IProperty::PROPERTY_TYPE::PT_FLOAT_V2:
				xmls += _T("FLOAT_V2");
				break;
			case props::IProperty::PROPERTY_TYPE::PT_FLOAT_V3:
				xmls += _T("FLOAT_V3");
				break;
			case props::IProperty::PROPERTY_TYPE::PT_FLOAT_V4:
				xmls += _T("FLOAT_V4");
				break;
			case props::IProperty::PROPERTY_TYPE::PT_GUID:
				xmls += _T("GUID");
				break;
			case props::IProperty::PROPERTY_TYPE::PT_INT:
				xmls += _T("INT");
				break;
			case props::IProperty::PROPERTY_TYPE::PT_INT_V2:
				xmls += _T("INT_V2");
				break;
			case props::IProperty::PROPERTY_TYPE::PT_INT_V3:
				xmls += _T("INT_V3");
				break;
			case props::IProperty::PROPERTY_TYPE::PT_INT_V4:
				xmls += _T("INT_V4");
				break;
			default:
			case props::IProperty::PROPERTY_TYPE::PT_STRING:
				xmls += _T("STRING");
				break;
		}
		xmls += _T("\"");

		if ((mode >= props::IProperty::SERIALIZE_MODE::SM_BIN_TERSE) && (it->second->GetAspect() != props::IProperty::PROPERTY_ASPECT::PA_GENERIC))
		{
			xmls += _T(" aspect=\"");
			switch (it->second->GetAspect())
			{
				case props::IProperty::PROPERTY_ASPECT::PA_BOOL_ONOFF:
					xmls += _T("BOOL_ONOFF");
					break;
				case props::IProperty::PROPERTY_ASPECT::PA_BOOL_YESNO:
					xmls += _T("BOOL_YESNO");
					break;
				case props::IProperty::PROPERTY_ASPECT::PA_COLOR_RGB:
					xmls += _T("COLOR_RGB");
					break;
				case props::IProperty::PROPERTY_ASPECT::PA_COLOR_RGBA:
					xmls += _T("COLOR_RGBA");
					break;
				case props::IProperty::PROPERTY_ASPECT::PA_DATE:
					xmls += _T("DATE");
					break;
				case props::IProperty::PROPERTY_ASPECT::PA_DIRECTORY:
					xmls += _T("DIRECTORY");
					break;
				case props::IProperty::PROPERTY_ASPECT::PA_ELEVAZIM:
					xmls += _T("ELEVAZIM");
					break;
				case props::IProperty::PROPERTY_ASPECT::PA_FILENAME:
					xmls += _T("FILENAME");
					break;
				case props::IProperty::PROPERTY_ASPECT::PA_FONT_DESC:
					xmls += _T("FONT_DESC");
					break;
				case props::IProperty::PROPERTY_ASPECT::PA_IPADDRESS:
					xmls += _T("IP_ADDRESS");
					break;
				case props::IProperty::PROPERTY_ASPECT::PA_LATLON:
					xmls += _T("LATLON");
					break;
				case props::IProperty::PROPERTY_ASPECT::PA_QUATERNION:
					xmls += _T("QUATERNION");
					break;
				case props::IProperty::PROPERTY_ASPECT::PA_RASCDEC:
					xmls += _T("RASCDEC");
					break;
				case props::IProperty::PROPERTY_ASPECT::PA_TIME:
					xmls += _T("TIME");
					break;
				default:
					TCHAR t[16];
					_itot_s((int)(it->second->GetAspect()), t, 10);
					xmls += t;
					break;
			}
			xmls += _T("\"");
		}

		xmls += _T(">");

		TCHAR _s[1 << 10];
		_s[0] = _T('\0');

		if (it->second->GetType() != props::IProperty::PROPERTY_TYPE::PT_ENUM)
		{
			it->second->AsString(_s, _countof(_s));
		}
		else
		{
			TCHAR *q = _s;
			size_t maxi = it->second->GetMaxEnumVal();
			for (size_t i = 0; i < maxi; i++)
			{
				it->second->GetEnumString(i, q, _countof(_s));
				if (i != (maxi - 1))
				{
					_tcscat_s(_s, _T(","));
					q = _s + _tcslen(_s);
				}
			}
			TCHAR num[16];
			_i64tot_s(it->second->AsInt(), num, _countof(num), 10);
			_tcscat_s(_s, _T(":"));
			_tcscat_s(_s, num);
		}

		tstring s;
		props::EscapeString(_s, s);
		xmls += s;

		xmls += _T("</powerprops:property>\n");
	}

	xmls += _T("</powerprops:property_set>");

	return true;
}

bool CPropertySet::DeserializeFromXMLString(const tstring &xmls)
{
	bool ret = false;

	genio::IParserT *p = (genio::IParserT *)genio::IParser::Create(genio::IParser::CHAR_MODE::CM_TCHAR), *q = nullptr;
	p->SetSourceData(xmls.c_str(), xmls.length());

	while (p->NextToken())
	{
		if (p->IsToken(_T("<")))
		{
			p->NextToken();

			// skip end tags and comments
			if (p->IsToken(_T("/")) || p->IsToken(_T("!")))
			{
				p->ReadUntil(_T(">"), false, true);
				continue;
			}

			// make sure the tags are pre-pended with "powerprops"
			if (p->IsToken(_T("powerprops")))
			{
				p->NextToken();

				if (!p->IsToken(_T(":")))
					goto label_error;

				p->NextToken();

				// if it's a set, then skip it... ostensibly this is the opener
				if (p->IsToken(_T("property_set")))
					continue;

				// if we got past the opener and it's not a property, then it's an error
				if (!p->IsToken(_T("property")))
					goto label_error;
			}
			else
				goto label_error;

			tstring propname, propid, proptype, propaspect;

			// read the whole opening tag string
			if (p->ReadUntil(_T(">"), false, true))
			{
				// create a new parser for the attributes
				q = (genio::IParserT *)genio::IParser::Create(genio::IParser::CHAR_MODE::CM_TCHAR);
				q->SetSourceData(p->GetCurrentTokenString(), _tcslen(p->GetCurrentTokenString()));

				while (q->NextToken())
				{
					if (q->GetCurrentTokenType() != genio::IParser::TOKEN_TYPE::TT_IDENT)
						goto label_error;

					tstring attrib = q->GetCurrentTokenString();
					q->NextToken();

					if (!q->IsToken(_T("=")))
						goto label_error;

					q->NextToken();
					if (q->GetCurrentTokenType() != genio::IParser::TOKEN_TYPE::TT_STRING)
						goto label_error;

					tstring attrib_val = q->GetCurrentTokenString();

					if (!_tcsicmp(attrib.c_str(), _T("name")))
					{
						propname = q->GetCurrentTokenString();
					}
					else if (!_tcsicmp(attrib.c_str(), _T("id")))
					{
						propid = q->GetCurrentTokenString();
					}
					else if (!_tcsicmp(attrib.c_str(), _T("type")))
					{
						proptype = q->GetCurrentTokenString();
						std::transform(proptype.begin(), proptype.end(), proptype.begin(), toupper);
					}
					else if (!_tcsicmp(attrib.c_str(), _T("aspect")))
					{
						propaspect = q->GetCurrentTokenString();
						std::transform(propaspect.begin(), propaspect.end(), propaspect.begin(), toupper);
					}
					else
						goto label_error;
				}

				q->Release();
				q = nullptr;
			}
			else
				goto label_error;

			FOURCHARCODE fcc;
			uint8_t *pid = (uint8_t *)&fcc;
			const TCHAR *tid = propid.c_str();
			for (size_t i = 0; i < 4; i++)
			{
				if (*tid == _T('&'))
				{
					TCHAR tmp[8], *_tmp = tmp;
					for (size_t q = 0; (q < 7) && *tid && (*tid != _T(';')); q++)
					{
						*(_tmp++) = *(tid++);
					}
					*(_tmp++) = *(tid++);
					*(_tmp++) = _T('\0');
					_stscanf_s(tmp, _T("&#%hhu;"), &pid[3 - i]);
				}
				else
				{
					pid[3-i] = (uint8_t)(*(tid++));
				}
			}
			while (!(fcc & 0xff))
				fcc >>= 8;

			props::IProperty *pp = GetPropertyById(fcc);

			if (!pp && !propname.empty())
				pp = GetPropertyByName(propname.c_str());

			if (!pp)
				pp = CreateProperty(propname.c_str(), fcc);

			if (!pp)
				goto label_error;

			if (p->ReadUntil(_T("<"), false, true))
			{
				tstring v;
				props::UnescapeString(p->GetCurrentTokenString(), v);
				pp->SetString(v.c_str());

				if (!_tcsicmp(proptype.c_str(), _T("BOOLEAN")))
					pp->ConvertTo(props::IProperty::PROPERTY_TYPE::PT_BOOLEAN);
				else if (!_tcsicmp(proptype.c_str(), _T("ENUM")))
					pp->ConvertTo(props::IProperty::PROPERTY_TYPE::PT_ENUM);
				else if (!_tcsicmp(proptype.c_str(), _T("FLOAT")))
					pp->ConvertTo(props::IProperty::PROPERTY_TYPE::PT_FLOAT);
				else if (!_tcsicmp(proptype.c_str(), _T("FLOAT_V2")))
					pp->ConvertTo(props::IProperty::PROPERTY_TYPE::PT_FLOAT_V2);
				else if (!_tcsicmp(proptype.c_str(), _T("FLOAT_V3")))
					pp->ConvertTo(props::IProperty::PROPERTY_TYPE::PT_FLOAT_V3);
				else if (!_tcsicmp(proptype.c_str(), _T("FLOAT_V4")))
					pp->ConvertTo(props::IProperty::PROPERTY_TYPE::PT_FLOAT_V4);
				else if (!_tcsicmp(proptype.c_str(), _T("GUID")))
					pp->ConvertTo(props::IProperty::PROPERTY_TYPE::PT_GUID);
				else if (!_tcsicmp(proptype.c_str(), _T("INT")))
					pp->ConvertTo(props::IProperty::PROPERTY_TYPE::PT_INT);
				else if (!_tcsicmp(proptype.c_str(), _T("INT_V2")))
					pp->ConvertTo(props::IProperty::PROPERTY_TYPE::PT_INT_V2);
				else if (!_tcsicmp(proptype.c_str(), _T("INT_V3")))
					pp->ConvertTo(props::IProperty::PROPERTY_TYPE::PT_INT_V3);
				else if (!_tcsicmp(proptype.c_str(), _T("INT_V4")))
					pp->ConvertTo(props::IProperty::PROPERTY_TYPE::PT_INT_V4);
			}
		}
	}

	ret = true;

// I'm not proud of using a goto, but for the first time in a long time, it makes sense... <shrug>
label_error:
	if (p)
	{
		p->Release();
		p = nullptr;
	}

	if (q)
	{
		q->Release();
		q = nullptr;
	}

	return ret;
}


IPropertySet *IPropertySet::CreatePropertySet()
{
	return new CPropertySet();
}
