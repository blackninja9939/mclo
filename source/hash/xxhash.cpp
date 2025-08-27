#include "mclo/hash/xxhash.hpp"

#include "mclo/debug/assert.hpp"

mclo::xxhash_64::xxhash_64( const XXH64_hash_t seed ) noexcept
{
	[[maybe_unused]] const XXH_errorcode result = XXH64_reset( &m_state, seed );
	DEBUG_ASSERT( result == XXH_OK, "Failed to reset state with seed", seed );
}

void mclo::xxhash_64::write( const mclo::span<const std::byte> data ) noexcept
{
	[[maybe_unused]] const XXH_errorcode result = XXH64_update( &m_state, data.data(), data.size() );
	DEBUG_ASSERT( result == XXH_OK, "Failed to update hash state" );
}

std::size_t mclo::xxhash_64::finish() const noexcept
{
	return XXH64_digest( &m_state );
}

mclo::xxhash_3::xxhash_3( const XXH64_hash_t seed )
	: m_state( XXH3_createState() )
{
	if ( !m_state )
	{
		throw std::bad_alloc();
	}
	[[maybe_unused]] const XXH_errorcode result = XXH3_64bits_reset_withSeed( m_state.get(), seed );
	DEBUG_ASSERT( result == XXH_OK, "Failed to reset state with seed", seed );
}

void mclo::xxhash_3::write( const mclo::span<const std::byte> data ) noexcept
{
	[[maybe_unused]] const XXH_errorcode result = XXH3_64bits_update( m_state.get(), data.data(), data.size() );
	DEBUG_ASSERT( result == XXH_OK, "Failed to update hash state" );
}

std::size_t mclo::xxhash_3::finish() const noexcept
{
	return XXH3_64bits_digest( m_state.get() );
}

MCLO_STATIC_CALL_OPERATOR void mclo::xxhash_3::deleter::operator()( XXH3_state_t* data )
	MCLO_CONST_CALL_OPERATOR noexcept
{
	[[maybe_unused]] const XXH_errorcode result = XXH3_freeState( data );
	DEBUG_ASSERT( result == XXH_OK, "Failed to free state at", data );
}
