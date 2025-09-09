#pragma once

#include <cstdint>

namespace Mc 
{

	class UUID
	{
	public:
		UUID();
		UUID(uint64_t uuid);
		UUID(const UUID&) = default;

		operator uint64_t() const { return m_UUID; }
	private:
		uint64_t m_UUID;
	};

}

namespace std {

	template<typename T> struct hash;

	template<>
	struct hash<Mc::UUID>
	{
		std::size_t operator()(const Mc::UUID& uuid) const
		{
			return static_cast<uint64_t>(uuid);
		}
	};

}