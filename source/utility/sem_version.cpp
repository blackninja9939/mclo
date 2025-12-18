#include "mclo/utility/sem_version.hpp"

#include "mclo/string/concatenate.hpp"

namespace mclo
{
	std::string sem_version::to_string() const
	{
		return mclo::concat_string( major, ".", minor, ".", patch );
	}

	std::string format_as( const sem_version& version )
	{
		return version.to_string();
	}
}
