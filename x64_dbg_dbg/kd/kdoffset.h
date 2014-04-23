#pragma once

#include <map>

class KdOffsetBitField
{
public:
	ULONG m_Start;
	ULONG m_Size;

	void operator () (ULONG Start, ULONG Size)
	{
		m_Start = Start;
		m_Size	= Size;
	}

	ULONG GetValue(ULONG Value)
	{
		return ((Value) >> (m_Start)) & ((1 << (m_Size)) - 1);
	}

	ULONG64 GetValue64(ULONG64 Value)
	{
		return ((Value) >> (m_Start)) & ((1 << (m_Size)) - 1);
	}
};

class KdOffsetManager
{
private:
	struct cmp_str
	{
		bool operator()(char const *a, char const *b)
		{
			return std::strcmp(a, b) < 0;
		}
	};

	class KdOffsetSub
	{
	private:
		std::map<const char *, ULONG64, cmp_str> m_List;

	public:
		KdOffsetSub() : m_List() { }

		void Set(const char *Symbol, ULONG64 Offset)
		{
			m_List[Symbol] = Offset;
		}

		ULONG64 operator [] (const char *Symbol)
		{
			return m_List[Symbol];
		}
	};

	std::map<const char *, KdOffsetSub *, cmp_str> m_List;

public:
	KdOffsetManager() : m_List() { }

	void Set(const char *Base, const char *Symbol, ULONG64 Offset)
	{
		KdOffsetSub *ptr = m_List[Base];

		// Doesn't exist, add it
		if (!ptr)
		{
			ptr				= new KdOffsetSub();
			m_List[Base]	= ptr;
		}

		// Set the value
		ptr->Set(Symbol, Offset);
	}

	void Clear()
	{
		//__debugbreak();
		//m_List.clear();
	}

	KdOffsetSub& operator [] (const char *Base)
	{
		return *m_List[Base];
	}
};