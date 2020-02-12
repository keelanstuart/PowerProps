/*
PowerProps Library Source File

Copyright © 2009-2020, Keelan Stuart. All rights reserved.

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


using namespace props;


#define PROPFLAG_REFERENCE		(1 << 31)


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
	virtual IProperty *GetPropertyByName(const TCHAR *propname) const;
	virtual CPropertySet &operator =(IPropertySet *propset);
	virtual CPropertySet &operator +=(IPropertySet *propset);
	virtual void AppendPropertySet(const IPropertySet *propset);
	virtual bool Serialize(IProperty::SERIALIZE_MODE mode, BYTE *buf, size_t bufsize, size_t *amountused) const;
	virtual bool Deserialize(BYTE *buf, size_t bufsize, size_t *bytesconsumed);
	virtual void SetChangeListener(const IPropertyChangeListener *plistener);
};



class CProperty : public IProperty
{
public:
	PROPERTY_TYPE m_Type;
	PROPERTY_ASPECT m_Aspect;
	tstring m_sName;
	FOURCHARCODE m_ID;
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
				size_t m_e;
			};
			TStringDeque *m_es;
		};
		TVec2I m_v2i, *p_v2i;
		TVec3I m_v3i, *p_v3i;
		TVec4I m_v4i, *p_v4i;
		float m_f, *p_f;
		TVec2F m_v2f, *p_v2f;
		TVec3F m_v3f, *p_v3f;
		TVec4F m_v4f, *p_v4f;
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

	virtual void Reset()
	{
		switch (m_Type)
		{
			case PT_ENUM:
				if (m_es)
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

		m_Type = PT_NONE;
		//m_Aspect = PA_GENERIC;
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
		if (m_Flags.IsSet(PROPFLAG_TYPELOCKED))
			return false;

		if (newtype == m_Type)
			return true;

		switch (newtype)
		{
			case PT_BOOLEAN:
			{
				if (!_tcsicmp(m_s, _T("0")) || !_tcsicmp(m_s, _T("false")) || !_tcsicmp(m_s, _T("no")) || !_tcsicmp(m_s, _T("off")))
					SetBool(false);
				else if (!_tcsicmp(m_s, _T("1")) || !_tcsicmp(m_s, _T("true")) || !_tcsicmp(m_s, _T("yes")) || !_tcsicmp(m_s, _T("on")))
					SetBool(true);
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
						int64_t x, y;
						_stscanf_s(m_s, _T("%I64d,%I64d"), &x, &y);
						SetVec2I(props::TVec2I(x, y));
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
						int64_t x, y, z;
						_stscanf_s(m_s, _T("%I64d,%I64d,%I64d"), &x, &y, &z);
						SetVec3I(props::TVec3I(x, y, z));
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
						int64_t x, y, z, w;
						_stscanf_s(m_s, _T("%I64d,%I64d,%I64d,%I64d"), &x, &y, &z, &w);
						SetVec4I(props::TVec4I(x, y, z, w));
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
						float x, y;
						_stscanf_s(m_s, _T("%f,%f"), &x, &y);
						SetVec2F(props::TVec2F(x, y));
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
						float x, y, z;
						_stscanf_s(m_s, _T("%f,%f,%f"), &x, &y, &z);
						SetVec3F(props::TVec3F(x, y, z));
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
						float x, y, z, w;
						_stscanf_s(m_s, _T("%f,%f,%f,%f"), &x, &y, &z, &w);
						SetVec4F(props::TVec4F(x, y, z, w));
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
				int32_t bufsz = -1;

				switch (m_Type)
				{
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

					case PT_GUID:
						bufsz = _sctprintf(_T("{%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}"), m_g.Data1, m_g.Data2, m_g.Data3,
							m_g.Data4[0], m_g.Data4[1], m_g.Data4[2], m_g.Data4[3], m_g.Data4[4], m_g.Data4[5], m_g.Data4[6], m_g.Data4[7]);
						break;
				}

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
		m_Aspect = aspect;
	}

	virtual void SetInt(int64_t val)
	{
		if (m_Flags.IsSet(PROPFLAG_TYPELOCKED) && (m_Type != PT_INT))
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
		if (m_Flags.IsSet(PROPFLAG_TYPELOCKED) && (m_Type != PT_INT_V2))
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
		if (m_Flags.IsSet(PROPFLAG_TYPELOCKED) && (m_Type != PT_INT_V3))
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
		if (m_Flags.IsSet(PROPFLAG_TYPELOCKED) && (m_Type != PT_INT_V4))
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
		if (m_Flags.IsSet(PROPFLAG_TYPELOCKED) && (m_Type != PT_FLOAT))
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
		if (m_Flags.IsSet(PROPFLAG_TYPELOCKED) && (m_Type != PT_FLOAT_V2))
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
		if (m_Flags.IsSet(PROPFLAG_TYPELOCKED) && (m_Type != PT_FLOAT_V3))
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
		if (m_Flags.IsSet(PROPFLAG_TYPELOCKED) && (m_Type != PT_FLOAT_V4))
			return;

		Reset();

		m_Type = PT_FLOAT_V4;
		if (!m_Flags.IsSet(PROPFLAG_REFERENCE))
			m_v4f = val;
		else
			*p_v4f = val;

		if (m_pOwner && m_pOwner->m_pListener) m_pOwner->m_pListener->PropertyChanged(this);
	}

	virtual void SetString(const TCHAR *val)
	{
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
		if (m_Flags.IsSet(PROPFLAG_TYPELOCKED) && (m_Type != PT_GUID))
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
		if (m_Flags.IsSet(PROPFLAG_TYPELOCKED) && (m_Type != PT_BOOLEAN))
			return;

		Reset();

		m_Type = PT_BOOLEAN;
		if (!m_Flags.IsSet(PROPFLAG_REFERENCE))
			m_b = val;
		else
			*p_b = val;

		if (m_pOwner && m_pOwner->m_pListener) m_pOwner->m_pListener->PropertyChanged(this);
	}

	virtual void SetEnumStrings(const TCHAR *strs)
	{
		Reset();

		m_Type = PT_ENUM;

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

		assert(m_es);

		if (val < m_es->size())
		{
			m_e = val;

			if (m_pOwner && m_pOwner->m_pListener) m_pOwner->m_pListener->PropertyChanged(this);

			return true;
		}

		return false;
	}

	virtual bool SetEnumValByString(const TCHAR *s)
	{
		if (m_Type != PT_ENUM)
			return false;

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

		return false;
	}

	virtual const TCHAR *GetEnumString(size_t idx, TCHAR *ret, size_t retsize)
	{
		if ((m_Type == PT_ENUM) && (m_es != nullptr) && (idx < m_es->size()))
		{
			tstring &t = m_es->at(idx);
			if (ret && retsize)
			{
				_tcsnccpy_s(ret, retsize, t.c_str(), retsize);
				return ret;
			}

			return m_es->at(idx).c_str();
		}

		return nullptr;
	}

	virtual const TCHAR *GetEnumStrings(TCHAR *ret, size_t retsize)
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

	virtual size_t GetMaxEnumVal()
	{
		if ((m_Type == PT_ENUM) && (m_es != nullptr))
		{
			return m_es->size();
		}

		return 0;
	}

	virtual void SetFromProperty(IProperty *pprop)
	{
		if (!pprop)
		{
			Reset();
			return;
		}

		switch (pprop->GetType())
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

			case PT_GUID:
				SetGUID(pprop->AsGUID());
				break;

			case PT_BOOLEAN:
				SetBool(pprop->AsBool());
				break;

			case PT_ENUM:
				SetEnumStrings(pprop->GetEnumStrings());
				SetEnumVal((size_t)(pprop->AsInt()));
				break;
		}

		m_Flags.SetAll(pprop->Flags());
		SetAspect(pprop->GetAspect());
	}

	virtual int64_t AsInt(int64_t *ret)
	{
		int64_t retval;
		if (!ret)
			ret = &retval;

		switch (m_Type)
		{
			case PT_STRING:
				*ret = m_s ? (int64_t)_tstoi64(m_s) : 0;
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

			case PT_GUID:
				*ret = 0;
				break;

			case PT_ENUM:
				*ret = m_e;
				break;
		}

		return *ret;
	}

	virtual const TVec2I *AsVec2I(TVec2I *ret = nullptr)
	{
		switch (m_Type)
		{
			case PT_STRING:
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

	virtual const TVec3I *AsVec3I(TVec3I *ret = nullptr)
	{
		switch (m_Type)
		{
			case PT_STRING:
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

	virtual const TVec4I *AsVec4I(TVec4I *ret = nullptr)
	{
		switch (m_Type)
		{
			case PT_STRING:
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

	virtual float AsFloat(float *ret)
	{
		float retval;
		if (!ret)
			ret = &retval;

		switch (m_Type)
		{
			case PT_STRING:
				*ret = (float)_tstof(m_s);
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

	virtual const TVec2F *AsVec2F(TVec2F *ret)
	{
		switch (m_Type)
		{
			case PT_STRING:
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
		}

		return ret ? ret : nullptr;
	}

	virtual const TVec3F *AsVec3F(TVec3F *ret)
	{
		switch (m_Type)
		{
			case PT_STRING:
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
		}

		return ret ? ret : nullptr;
	}

	virtual const TVec4F *AsVec4F(TVec4F *ret)
	{
		switch (m_Type)
		{
			case PT_STRING:
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

	virtual const TCHAR *AsString(TCHAR *ret, size_t retsize) const
	{
		if (m_Type == PT_STRING)
		{
			if (!ret || (retsize == 0))
				return m_s;
		}

		if (m_Type == PT_ENUM)
		{
			if (!ret || (retsize == 0) && (m_e < m_es->size()))
				return m_es->at(m_e).c_str();
			else
				return m_s;
		}

		if (retsize > 0)
		{
			switch (m_Type)
			{
				case PT_STRING:
					_tcsncpy_s(ret, retsize, m_s ? m_s : _T(""), retsize);
					break;

				case PT_ENUM:
					if (m_e < m_es->size())
						_tcsncpy_s(ret, retsize, m_es->at(m_e).c_str(), retsize);
					break;

				case PT_BOOLEAN:
					_sntprintf_s(ret, retsize, retsize, _T("%d"), m_b);
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

				case PT_GUID:
					_sntprintf_s(ret, retsize, retsize, _T("{%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}"), m_g.Data1, m_g.Data2, m_g.Data3,
						m_g.Data4[0], m_g.Data4[1], m_g.Data4[2], m_g.Data4[3], m_g.Data4[4], m_g.Data4[5], m_g.Data4[6], m_g.Data4[7]);
					break;
			}
		}

		return ret;
	}

	virtual GUID AsGUID(GUID *ret)
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

	virtual bool AsBool(bool *ret)
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

		return *ret;
	}

	virtual bool Serialize(SERIALIZE_MODE mode, BYTE *buf, size_t bufsize, size_t *amountused = NULL)
	{
		if (!m_Type || (m_Type >= PT_NUMTYPES))
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

			case PT_FLOAT:
				sz += sizeof(float);
				break;

			case PT_GUID:
				sz += sizeof(GUID);
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
				*((int64_t *)buf) = m_i;
				buf += sizeof(int64_t);
				break;

			case PT_FLOAT:
				*((float *)buf) = m_f;
				buf += sizeof(float);
				break;

			case PT_GUID:
				*((GUID *)buf) = m_g;
				buf += sizeof(GUID);
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
		if (!m_Type || (m_Type >= PT_NUMTYPES))
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
				m_i = *((int64_t *)buf);
				buf += sizeof(int64_t);
				break;

			case PT_FLOAT:
				m_f = *((float *)buf);
				buf += sizeof(float);
				break;

			case PT_GUID:
				m_g = *((GUID *)buf);
				buf += sizeof(GUID);
				break;
		}

		if (bytesconsumed)
			*bytesconsumed = (buf - origbuf);

		return (size_t(buf - origbuf) <= bufsize) ? true : false;
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
		return pi->second;

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
	pprop->m_Flags.Set(PROPFLAG_REFERENCE | PROPFLAG_TYPELOCKED);
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
			pnew->SetFromProperty(pother);
	}

	return *this;
}


CPropertySet &CPropertySet::operator +=(IPropertySet *propset)
{
	AppendPropertySet(propset);

	return *this;
}


void CPropertySet::AppendPropertySet(const IPropertySet *propset)
{
	for (uint32_t i = 0; i < propset->GetPropertyCount(); i++)
	{
		IProperty *po = propset->GetProperty(i);
		if (!po)
			continue;

		IProperty *pp = this->GetPropertyById(po->GetID());
		if (!pp)
		{
			pp = CreateProperty(po->GetName(), po->GetID());
		}

		if (pp)
			pp->SetFromProperty(po);
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
		if (consumed < bufsize)
			return false;

		bufsize -= consumed;
	}

	if (bytesconsumed)
		*bytesconsumed = consumed;

	return true;
}


void CPropertySet::SetChangeListener(const IPropertyChangeListener *plistener)
{
	m_pListener = (IPropertyChangeListener *)plistener;
}


IPropertySet *IPropertySet::CreatePropertySet()
{
	return new CPropertySet();
}
