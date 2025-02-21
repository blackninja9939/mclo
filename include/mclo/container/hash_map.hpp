#pragma once

#include "mclo/concepts/container_compatible_range.hpp"
#include "mclo/container/arrow_proxy.hpp"
#include "mclo/container/detail/common_reference.hpp"
#include "mclo/hash/hash.hpp"
#include "mclo/numeric/128_bit_integer.hpp"
#include "mclo/numeric/math.hpp"
#include "mclo/preprocessor/platform.hpp"

#include <algorithm>
#include <bit>
#include <concepts>
#include <functional>
#include <memory>
#include <utility>

#include <xsimd/xsimd.hpp>

namespace mclo
{
	struct power_of_2_growth_policy
	{
		[[nodiscard]] static constexpr std::size_t calculate_index( const std::size_t hash,
																	const std::size_t capacity ) noexcept
		{
			return mclo::modulo_pow2( hash, capacity );
		}

		[[nodiscard]] static constexpr std::size_t calculate_capacity_for( const std::size_t size ) noexcept
		{
			return std::size_t( 1 ) << std::bit_width( size );
		}
	};

	struct modulo_policy
	{
		[[nodiscard]] static constexpr std::size_t calculate_index( const std::size_t hash,
																	const std::size_t capacity ) noexcept
		{
			return hash % capacity;
		}

		[[nodiscard]] static constexpr std::size_t calculate_capacity_for( const std::size_t size ) noexcept
		{
			return size;
		}
	};

	template <typename T>
	concept hash_table_growth_policy = requires( const std::size_t v ) {
		{
			T::calculate_index( v, v )
		} noexcept -> std::same_as<std::size_t>;
		{
			T::calculate_capacity_for( v )
		} noexcept -> std::same_as<std::size_t>;
	};

	template <typename Key, typename Value>
	struct map_traits
	{
		using key_type = Key;
		using mapped_type = Value;
		using value_type = std::pair<key_type, mapped_type>;
		using reference = std::pair<const key_type&, mapped_type&>;
		using const_reference = std::pair<const key_type&, const mapped_type&>;

		static reference ref( value_type& value ) noexcept
		{
			return { value.first, value.second };
		}

		static const_reference ref( const value_type& value ) noexcept
		{
			return { value.first, value.second };
		}

		static const key_type& get_key( const_reference ref ) noexcept
		{
			return ref.first;
		}
	};

	template <typename Key>
	struct set_traits
	{
		using key_type = Key;
		using mapped_type = void;
		using value_type = key_type;
		using reference = const key_type&;
		using const_reference = const key_type&;

		static reference ref( value_type& value ) noexcept
		{
			return value;
		}

		static const_reference ref( const value_type& value ) noexcept
		{
			return value;
		}

		static const key_type& get_key( const_reference ref ) noexcept
		{
			return ref;
		}
	};

	template <typename Traits>
	struct value_node_traits : Traits
	{
		using node_type = typename Traits::value_type;

		template <typename Allocator, typename... Args>
		static node_type* create( node_type* const node, const Allocator& alloc, Args&&... args )
		{
			return std::uninitialized_construct_using_allocator( node, alloc, std::forward<Args>( args )... );
		}

		template <typename Allocator>
		static void destroy( node_type* const node, const Allocator& )
		{
			std::destroy_at( &node );
		}

		static void transfer( node_type* out, node_type&& node )
		{
			std::construct_at( out, std::move( node ) );
		}
	};

	template <typename Traits>
	struct heap_node_traits : Traits
	{
		using value_type = typename Traits::value_type;
		using node_type = std::unique_ptr<value_type>;

		template <typename Allocator, typename... Args>
		static node_type* create( node_type* node, const Allocator& alloc, Args&&... args )
		{
			using value_alloc_t = std::allocator_traits<Allocator>::template rebind_alloc<value_type>;
			value_alloc_t value_alloc( alloc );
			value_type* value = value_alloc.allocate( 1 );
			std::uninitialized_construct_using_allocator( value, alloc, std::forward<Args>( args )... );
			return std::construct_at( node, value );
		}

		template <typename Allocator>
		static void destroy( node_type* node, const Allocator& alloc )
		{
			value_type* value = node->release();
			if ( value )
			{
				std::destroy_at( value );
				using value_alloc_t = std::allocator_traits<Allocator>::template rebind_alloc<value_type>;
				value_alloc_t value_alloc( alloc );
				value_alloc.deallocate( value, 1 );
			}
			std::destroy_at( node );
		}

		static void transfer( node_type* out, node_type&& node )
		{
			std::construct_at( out, std::move( node ) );
		}

		static Traits::reference ref( node_type& node ) noexcept
		{
			return Traits::ref( *node );
		}

		static Traits::const_reference ref( const node_type& node ) noexcept
		{
			return Traits::ref( *node );
		}

		static const Traits::key_type& get_key( const node_type& node ) noexcept
		{
			return Traits::get_key( *node );
		}
	};

	template <typename Traits, typename Allocator>
	class stack_node
	{
		using node_type = typename Traits::node_type;

	public:
		template <typename... Args>
		stack_node( const Allocator& alloc, Args&&... args )
			: alloc( alloc )
		{
			Traits::create( node_raw(), alloc, std::forward<Args>( args )... );
		}

		~stack_node()
		{
			Traits::destroy( node_raw(), alloc );
		}

		node_type& node() noexcept
		{
			return *std::launder( node_raw() );
		}

	private:
		node_type* node_raw() noexcept
		{
			return reinterpret_cast<node_type*>( node_buffer );
		}

		alignas( node_type ) std::byte node_buffer[ sizeof( node_type ) ];
		const Allocator& alloc;
	};

	template <typename T>
	concept hash_table_node_traits = requires {
		typename T::key_type;
		typename T::value_type;
		typename T::reference;
		typename T::const_reference;
		typename T::node_type;
	};

	struct fast_forward_tag
	{
	};

	namespace detail
	{
		struct hash_table_iterator_accessor
		{
			template <typename It, typename Ptr>
				requires std::is_pointer_v<Ptr>
			static std::size_t index( const It& it, const Ptr ptr ) noexcept
			{
				return static_cast<std::size_t>( it.m_values - ptr );
			}
		};
	}

	template <hash_table_node_traits NodeTraits, bool IsConst>
	class hash_table_iterator
	{
		template <hash_table_node_traits OtherNodeType, bool OtherIsConst>
		friend class hash_table_iterator;

		friend detail::hash_table_iterator_accessor;

		using node_type =
			std::conditional_t<IsConst, const typename NodeTraits::node_type, typename NodeTraits::node_type>;

	public:
		using difference_type = std::ptrdiff_t;
		using value_type = typename NodeTraits::value_type;
		using reference =
			std::conditional_t<IsConst, typename NodeTraits::const_reference, typename NodeTraits::reference>;
		using pointer = arrow_proxy<reference>;
		using iterator_category = std::forward_iterator_tag;
		using iterator_concept = std::forward_iterator_tag;

		hash_table_iterator() noexcept = default;

		template <bool OtherIsConst>
		hash_table_iterator( const hash_table_iterator<NodeTraits, OtherIsConst>& it ) noexcept
			: m_values( it.m_values )
			, m_meta_data( it.m_meta_data )
		{
		}

		hash_table_iterator( node_type* values, const std::uint8_t* meta ) noexcept
			: m_values( values )
			, m_meta_data( meta )
		{
		}

		hash_table_iterator( node_type* values, const std::uint8_t* meta, fast_forward_tag ) noexcept
			: m_values( values )
			, m_meta_data( meta )
		{
			fast_forward();
		}

		[[nodiscard]] reference operator*() const noexcept
		{
			return NodeTraits::ref( *m_values );
		}

		[[nodiscard]] pointer operator->() const noexcept
		{
			return { operator*() };
		}

		template <bool OtherIsConst>
		[[nodiscard]] bool operator==( const hash_table_iterator<NodeTraits, OtherIsConst>& other ) const noexcept
		{
			return m_values == other.m_values;
		}

		hash_table_iterator& operator++() noexcept
		{
			++m_values;
			++m_meta_data;
			fast_forward();
			return *this;
		}

		hash_table_iterator operator++( int ) noexcept
		{
			hash_table_iterator result{ *this };
			++*this;
			return result;
		}

	private:
		void fast_forward() noexcept
		{
			while ( *m_meta_data == 0 )
			{
				++m_values;
				++m_meta_data;
			}
		}

		node_type* m_values = nullptr;
		const std::uint8_t* m_meta_data = nullptr;
	};

	template <hash_table_node_traits NodeTraits,
			  typename Hash = mclo::hash<typename NodeTraits::key_type>,
			  typename KeyEq = std::equal_to<>,
			  hash_table_growth_policy GrowthPolicy = power_of_2_growth_policy,
			  typename Allocator = std::allocator<typename NodeTraits::node_type>>
	class hash_table_base : private detail::hash_table_iterator_accessor
	{
		// This is a hash table that uses open addressing with linear probing
		// We use a single byte for meta data, 1 bit for if the slot is used, 3 bits for the hash and 4 bits for the
		// offset.
		// Storing the hash lets us skip a lot of key comparisons, as we can skip anything that does not have
		// the same hash. The offset is used to know how far we have moved the element from its desired slot, so we can
		// use the Robin Hood hashing strategy.
		// We use xsimd to process 16 elements at a time, as that is the max we can
		// move an element from its desired slot.

		// 0 is a valid hash so we have to use one bit for if the slot is used, this makes 0 our unused value
		// We mark our sentinel meta data as used so we stop skipping empty slots when iterating over the table
		static constexpr std::uint8_t META_USED_MASK = 0b00000001;
		static constexpr std::uint8_t META_HASH_MASK = 0b00001110;
		static constexpr std::uint8_t META_OFFSET_MASK = 0b11110000;
		static constexpr std::uint8_t META_SINGLE_OFFSET = 0b00010000;

		static constexpr bool is_transparent = requires {
			typename Hash::is_transparent;
			typename KeyEq::is_transparent;
		};

		using traits = typename NodeTraits;
		using node_type = typename traits::node_type;
		using meta_data = xsimd::batch<std::uint8_t, xsimd::sse2>;
		static_assert( meta_data::size == 16,
					   "Meta data is expecting to be 16 bytes to process 16 at a time, as that is the max an element "
					   "can be moved from its desired slot" );

	public:
		using key_type = typename traits::key_type;
		using mapped_type = typename traits::mapped_type;
		using value_type = typename traits::value_type;
		using size_type = std::size_t;
		using difference_type = std::ptrdiff_t;
		using hasher = Hash;
		using key_equal = KeyEq;
		using allocator_type = Allocator;
		using reference = typename traits::reference;
		using const_reference = typename traits::const_reference;
		using pointer = std::allocator_traits<Allocator>::pointer;
		using const_pointer = std::allocator_traits<Allocator>::const_pointer;
		using iterator = hash_table_iterator<traits, false>;
		using const_iterator = hash_table_iterator<traits, true>;

		hash_table_base() = default;

		explicit hash_table_base( const size_type capacity,
								  const hasher& hash = hasher(),
								  const key_equal& key_eq = key_equal(),
								  const allocator_type& alloc = allocator_type() )
			: m_hasher( hash )
			, m_key_eq( key_eq )
			, m_allocator( alloc )
		{
			reserve( capacity );
		}

		explicit hash_table_base( const size_type capacity, const allocator_type& alloc = allocator_type() )
			: hash_table_base( capacity, hasher(), key_equal(), alloc )
		{
		}

		explicit hash_table_base( const size_type capacity,
								  const Hash& hash,
								  const allocator_type& alloc = allocator_type() )
			: hash_table_base( capacity, hash, key_equal(), alloc )
		{
		}

		explicit hash_table_base( const allocator_type& alloc )
			: hash_table_base( 0, hasher(), key_equal(), alloc )
		{
		}

		template <std::input_iterator It, std::sentinel_for<It> Sentinel>
		hash_table_base( It first,
						 Sentinel last,
						 const size_type capacity = 0,
						 const hasher& hash = hasher(),
						 const key_equal& key_eq = key_equal(),
						 const allocator_type& alloc = allocator_type() )
			: hash_table_base( capacity, hash, key_eq, alloc )
		{
			insert( first, last );
		}

		template <std::input_iterator It, std::sentinel_for<It> Sentinel>
		hash_table_base( It first, Sentinel last, const size_type capacity, const allocator_type& alloc )
			: hash_table_base( first, last, capacity, hasher(), key_equal(), alloc )
		{
		}

		template <std::input_iterator It, std::sentinel_for<It> Sentinel>
		hash_table_base(
			It first, Sentinel last, const size_type capacity, const Hash& hash, const allocator_type& alloc )
			: hash_table_base( first, last, capacity, hash, key_equal(), alloc )
		{
		}

		hash_table_base( const hash_table_base& other )
			: hash_table_base( other,
							   std::allocator_traits<allocator_type>::select_on_container_copy_construction(
								   other.get_allocator() ) )
		{
		}

		hash_table_base( const hash_table_base& other, const allocator_type& alloc )
			: m_hasher( other.m_hasher )
			, m_key_eq( other.m_key_eq )
			, m_allocator( alloc )
		{
			reserve( other.m_size );
			insert( other.begin(), other.end() );
		}

		hash_table_base( hash_table_base&& other ) noexcept
			: m_hasher( std::move( other.m_hasher ) )
			, m_key_eq( other.m_key_eq )
			, m_allocator( std::move( other.m_allocator ) )
		{
			take_contents( std::move( other ) );
		}

		hash_table_base( hash_table_base&& other, const allocator_type& alloc ) noexcept
			: m_hasher( std::move( other.m_hasher ) )
			, m_key_eq( other.m_key_eq )
			, m_allocator( alloc )
		{
			take_contents( std::move( other ) );
		}

		hash_table_base( std::initializer_list<value_type> init_list,
						 const size_type capacity = 0,
						 const hasher& hash = hasher(),
						 const key_equal& key_eq = key_equal(),
						 const allocator_type& alloc = allocator_type() )
			: hash_table_base(
				  init_list.begin(), init_list.end(), capacity ? capacity : init_list.size(), hash, key_eq, alloc )
		{
		}

		hash_table_base( std::initializer_list<value_type> init_list,
						 const size_type capacity,
						 const allocator_type& alloc )
			: hash_table_base( init_list, capacity, hasher(), key_equal(), alloc )
		{
		}

		hash_table_base( std::initializer_list<value_type> init_list,
						 const size_type capacity,
						 const Hash& hash,
						 const allocator_type& alloc )
			: hash_table_base( init_list, capacity, hash, key_equal(), alloc )
		{
		}

		hash_table_base& operator=( const hash_table_base& other )
		{
			if ( this == &other )
			{
				return *this;
			}

			clear();

			if constexpr ( std::allocator_traits<allocator_type>::propagate_on_container_copy_assignment::value )
			{
				if ( m_allocator != other.m_allocator )
				{
					m_allocator.deallocate( m_nodes, capacity_to_alloc_size( m_capacity ) );
					m_capacity = 0;
				}
				m_allocator = other.m_allocator;
			}

			m_max_load_factor = other.m_max_load_factor;
			m_hasher = other.m_hasher;
			m_key_eq = other.m_key_eq;
			rehash( other.m_capacity );
			insert( other.begin(), other.end() );
			return *this;
		}

		hash_table_base& operator=( hash_table_base&& other ) noexcept(
			std::allocator_traits<allocator_type>::is_always_equal::value &&
			std::is_nothrow_move_assignable_v<hasher> && std::is_nothrow_move_assignable_v<key_equal> )
		{
			if ( this == &other )
			{
				return *this;
			}

			clear();

			if constexpr ( std::allocator_traits<allocator_type>::propagate_on_container_move_assignment::value )
			{
				m_allocator.deallocate( m_nodes, capacity_to_alloc_size( m_capacity ) );
				m_capacity = 0;
				m_allocator = std::move( other.m_allocator );
				take_contents( std::move( other ) );
			}
			else if ( m_allocator == other.m_allocator )
			{
				take_contents( std::move( other ) );
			}
			else
			{
				m_max_load_factor = other.m_max_load_factor;
				rehash( other.m_capacity );
				insert( std::make_move_iterator( other.begin() ), std::make_move_iterator( other.end() ) );
				other.clear();
			}

			m_hasher = std::move( other.m_hasher );
			m_key_eq = std::move( other.m_key_eq );
			return *this;
		}

		hash_table_base& operator=( std::initializer_list<value_type> init_list )
		{
			clear();
			insert( init_list.begin(), init_list.end() );
			return *this;
		}

		~hash_table_base()
		{
			destroy_nodes();
		}

		[[nodiscard]] allocator_type get_allocator() const noexcept
		{
			return m_allocator;
		}

		[[nodiscard]] hasher hash_function() const noexcept( std::is_nothrow_copy_constructible_v<hasher> )
		{
			return m_hasher;
		}

		[[nodiscard]] key_equal key_eq() const noexcept( std::is_nothrow_copy_constructible_v<key_equal> )
		{
			return m_key_eq;
		}

		[[nodiscard]] size_type size() const noexcept
		{
			return m_size;
		}
		[[nodiscard]] bool empty() const noexcept
		{
			return m_size == 0;
		}
		[[nodiscard]] size_type max_size() const noexcept
		{
			return std::numeric_limits<size_type>::max();
		}

		[[nodiscard]] float max_load_factor() const noexcept
		{
			return m_max_load_factor;
		}

		void max_load_factor( const float value )
		{
			DEBUG_ASSERT( !std::isnan( value ) && value > 0.0f && value <= 1.0f, "Invalid max load factor" );
			m_max_load_factor = value; // todo(mc) rehash
		}

		void clear()
		{
			destroy_nodes();
			std::memset( m_meta_data, 0, m_capacity );
			m_size = 0;
		}

		void reserve( const size_type count )
		{
			rehash( static_cast<size_type>( std::ceil( float( count ) / m_max_load_factor ) ) );
		}

		void rehash( const size_type count )
		{
			const std::size_t new_capacity = std::max( size_type( 8 ), GrowthPolicy::calculate_capacity_for( count ) );
			if ( new_capacity <= m_capacity )
			{
				return;
			}

			node_type* const old_nodes = m_nodes;
			std::uint8_t* const old_meta_data = m_meta_data;

			const std::size_t alloc_size = capacity_to_alloc_size( new_capacity );
			m_nodes = m_allocator.allocate( alloc_size );
			m_meta_data = reinterpret_cast<std::uint8_t*>( m_nodes + new_capacity );

			std::memset( m_meta_data, 0, new_capacity );
			std::memset( m_meta_data + new_capacity, META_USED_MASK, alloc_size - new_capacity );

			const std::size_t old_capacity = std::exchange( m_capacity, new_capacity );
			const std::size_t old_size = std::exchange( m_size, 0 );

			for ( std::size_t i = 0; i < old_size; ++i )
			{
				if ( old_meta_data[ i ] != 0 )
				{
					node_type* node = old_nodes + i;
					emplace_internal(
						traits::get_key( *node ),
						[]( const node_type& ) { UNREACHABLE( "Starting from empty this is impossible" ); },
						[ node, this ]( node_type* slot ) { traits::transfer( slot, std::move( *node ) ); } );
					traits::destroy( node, m_allocator );
				}
			}

			m_allocator.deallocate( old_nodes, capacity_to_alloc_size( old_capacity ) );
			m_rehash_on_next_insert = false;
		}

		template <typename... Args>
		std::pair<iterator, bool> emplace( Args&&... args )
		{
			stack_node<traits, allocator_type> node( m_allocator, std::forward<Args>( args )... );
			return emplace_internal(
				traits::get_key( node.node() ),
				[]( const node_type& ) {},
				[ &node, this ]( node_type* slot ) { traits::transfer( slot, std::move( node.node() ) ); } );
		}

		std::pair<iterator, bool> insert( const value_type& value )
		{
			return emplace( value );
		}
		std::pair<iterator, bool> insert( value_type&& value )
		{
			return emplace( std::move( value ) );
		}

		template <std::convertible_to<value_type> T>
		std::pair<iterator, bool> insert( T&& value )
		{
			return emplace( std::forward<T>( value ) );
		}

		template <std::input_iterator It, std::sentinel_for<It> Sentinel>
		void insert( It first, Sentinel last )
		{
			for ( ; first != last; ++first )
			{
				insert( *first );
			}
		}

		void insert( const std::initializer_list<value_type> init_list )
		{
			insert( init_list.begin(), init_list.end() );
		}

		template <mclo::container_compatible_range<value_type> Rng>
		void insert( Rng&& range )
		{
			insert( std::ranges::begin( range ), std::ranges::end( range ) );
		}

		template <typename... Args>
		std::pair<iterator, bool> try_emplace( const key_type& key, Args&&... args )
		{
			return try_emplace_internal( key, std::forward<Args>( args )... );
		}
		template <typename... Args>
		std::pair<iterator, bool> try_emplace( key_type&& key, Args&&... args )
		{
			return try_emplace_internal( std::move( key ), std::forward<Args>( args )... );
		}
		template <typename UKey, typename... Args>
			requires is_transparent
		std::pair<iterator, bool> try_emplace( UKey&& key, Args&&... args )
		{
			return try_emplace_internal( std::forward<UKey>( key ), std::forward<Args>( args )... );
		}

		// insert_or_assign

		iterator erase( const_iterator pos )
		{
			const std::size_t pos_index =
				static_cast<const detail::hash_table_iterator_accessor&>( *this ).index( pos, m_nodes );

			std::size_t move_end_index = pos_index + 1;
			while ( m_meta_data[ move_end_index ] != 0 && ( m_meta_data[ move_end_index ] & META_OFFSET_MASK ) > 0 )
			{
				++move_end_index;
			}

			std::move( m_nodes + pos_index + 1, m_nodes + move_end_index, m_nodes + pos_index );
			std::move( m_meta_data + pos_index + 1, m_meta_data + move_end_index, m_meta_data + pos_index );
			for ( std::size_t i = pos_index; i < move_end_index; ++i )
			{
				m_meta_data[ i ] -= META_SINGLE_OFFSET;
			}

			traits::destroy( m_nodes + move_end_index - 1, m_allocator );
			m_meta_data[ move_end_index - 1 ] = 0;

			--m_size;

			// If we zeroed out ourselves we did not shift anything
			iterator result( m_nodes + pos_index, m_meta_data + pos_index );
			if ( m_meta_data[ pos_index ] == 0 )
			{
				++result;
			}
			return result;
		}

		iterator erase( iterator pos )
		{
			return erase( const_iterator( pos ) );
		}

		size_type erase( const key_type& key )
		{
			auto it = find( key );
			if ( it == end() )
			{
				return 0;
			}
			erase( it );
			return 1;
		}

		template <typename UKey>
			requires is_transparent
		size_type erase( const UKey& key )
		{
			auto it = find( key );
			if ( it == end() )
			{
				return 0;
			}
			erase( it );
			return 1;
		}

		[[nodiscard]] iterator find( const key_type& key ) noexcept
		{
			const std::size_t index = find_internal( key );
			return { m_nodes + index, m_meta_data + index };
		}

		[[nodiscard]] const_iterator find( const key_type& key ) const noexcept
		{
			const std::size_t index = find_internal( key );
			return { m_nodes + index, m_meta_data + index };
		}

		[[nodiscard]] bool contains( const key_type& key ) const noexcept
		{
			return find( key ) != end();
		}

		[[nodiscard]] size_type count( const key_type& key ) const noexcept
		{
			return static_cast<size_type>( contains( key ) );
		}

		template <typename UKey>
			requires is_transparent
		[[nodiscard]] iterator find( const UKey& key ) noexcept
		{
			const std::size_t index = find_internal( key );
			return { m_nodes + index, m_meta_data + index };
		}

		template <typename UKey>
			requires is_transparent
		[[nodiscard]] const_iterator find( const UKey& key ) const noexcept
		{
			const std::size_t index = find_internal( key );
			return { m_nodes + index, m_meta_data + index };
		}

		template <typename UKey>
			requires is_transparent
		[[nodiscard]] bool contains( const UKey& key ) const noexcept
		{
			return find( key ) != end();
		}

		template <typename UKey>
			requires is_transparent
		[[nodiscard]] size_type count( const UKey& key ) const noexcept
		{
			return static_cast<size_type>( contains( key ) );
		}

		[[nodiscard]] iterator begin() noexcept
		{
			return iterator( m_nodes, m_meta_data, fast_forward_tag{} );
		}
		[[nodiscard]] const_iterator begin() const noexcept
		{
			return const_iterator( m_nodes, m_meta_data, fast_forward_tag{} );
		}
		[[nodiscard]] const_iterator cbegin() const noexcept
		{
			return const_iterator( m_nodes, m_meta_data, fast_forward_tag{} );
		}

		[[nodiscard]] iterator end() noexcept
		{
			// Could just cast meta to nodes ptr as order wise it is the one past the end
			// but that might be a bit funkjy
			return iterator( m_nodes + m_capacity, nullptr );
		}
		[[nodiscard]] const_iterator end() const noexcept
		{
			return const_iterator( m_nodes + m_capacity, nullptr );
		}
		[[nodiscard]] const_iterator cend() const noexcept
		{
			return const_iterator( m_nodes + m_capacity, nullptr );
		}

		void swap( hash_table_base& other ) noexcept( std::allocator_traits<Allocator>::is_always_equal::value &&
													  std::is_nothrow_swappable_v<hasher> &&
													  std::is_nothrow_swappable_v<key_equal> )
		{
			using std::swap;
			swap( m_nodes, other.m_nodes );
			swap( m_meta_data, other.m_meta_data );
			swap( m_capacity, other.m_capacity );
			swap( m_size, other.m_size );
			swap( m_max_load_factor, other.m_max_load_factor );
			swap( m_rehash_on_next_insert, other.m_rehash_on_next_insert );
			swap( m_hasher, other.m_hasher );
			swap( m_key_eq, other.m_key_eq );
			if constexpr ( std::allocator_traits<allocator_type>::propagate_on_container_swap::value )
			{
				swap( m_allocator, other.m_allocator );
			}
			else
			{
				DEBUG_ASSERT( m_allocator == other.m_allocator, "Allocators must compare equal when swapped" );
			}
		}

		friend void swap( hash_table_base& lhs, hash_table_base& rhs ) noexcept( noexcept( lhs.swap( rhs ) ) )
		{
			lhs.swap( rhs );
		}

		[[nodiscard]] bool operator==( const hash_table_base& other ) const noexcept
		{
			if ( m_size != other.m_size )
			{
				return false;
			}
			for ( const auto& element : other )
			{
				const auto it = find( traits::get_key( element ) );
				if ( it == end() )
				{
					return false;
				}
				if constexpr ( !std::is_same_v<key_type, value_type> )
				{
					if ( *it != element )
					{
						return false;
					}
				}
			}
			return true;
		}

	private:
		template <typename T, std::invocable<node_type&> FoundKey, std::invocable<node_type*> CreateNode>
		std::pair<iterator, bool> emplace_internal( const T& key, FoundKey&& found, CreateNode&& create )
		{
			if ( m_capacity == 0 || m_rehash_on_next_insert ) [[unlikely]]
			{
				rehash( m_capacity * 2 );
			}

			// If your hash function is beyond stupid this will last forever, but just don't use a stupid hash
			// function, like seriously even std::hash is fine just don't hash everything to the same slot
			const std::size_t hash = key_hash( key );
			for ( ;; )
			{
				// We package part of our hash into the meta info which means we do not need to check
				// as many key comparisons, as we only compare things that have a more similar hash
				// not just anything that goes into the same ideal slot
				const std::size_t desired_index = GrowthPolicy::calculate_index( hash, m_capacity );
				const std::uint8_t desired_meta = META_USED_MASK | ( hash & META_HASH_MASK );

				// Find first meta data that could match the hash, since we guarantee we can never be >= 16 away we know
				// if we do not find it in the first 16 we will not find it at all
				const meta_data loaded_meta = meta_data::load_unaligned( m_meta_data + desired_index );
				const meta_data hash_mask = meta_data::broadcast( META_USED_MASK | META_HASH_MASK );
				const meta_data::batch_bool_type cmp = ( loaded_meta & hash_mask ) == desired_meta;

				std::uint16_t matching_hashes = static_cast<std::uint16_t>( cmp.mask() );
				while ( matching_hashes )
				{
					const int offset = std::countr_zero( matching_hashes );
					const std::size_t index = desired_index + offset;
					const node_type& node = m_nodes[ index ];
					if ( key_is_eq( key, traits::get_key( node ) ) ) [[likely]]
					{
						found( node );
						return {
							iterator{m_nodes + index, m_meta_data + index},
                            false
                        };
					}
					matching_hashes &= matching_hashes - 1; // Clear rightmost set bit
				}

				// Every slot is full, we'd be offset too far so rehash and try again
				const auto empty_slots =
					static_cast<std::uint16_t>( ( loaded_meta == meta_data::broadcast( 0 ) ).mask() );
				if ( empty_slots == 0 ) [[unlikely]]
				{
					rehash( m_capacity * 2 );
					continue;
				}

				// Find the slot we will insert into
				const std::uint16_t offset = static_cast<std::uint16_t>( std::countr_zero( empty_slots ) );

				const std::size_t insertion_idx = desired_index + offset;
				std::uint8_t insertion_meta = desired_meta + static_cast<std::uint8_t>( offset * META_SINGLE_OFFSET );

				// Insert the data
				create( m_nodes + insertion_idx );
				m_meta_data[ insertion_idx ] = insertion_meta;
				++m_size;

				// We are exactly where we want to be
				if ( insertion_idx == desired_index )
				{
					return {
						iterator{m_nodes + insertion_idx, m_meta_data + insertion_idx},
                        true
                    };
				}

				std::size_t result_index = insertion_idx;

				// If we were not in our desired slot see if we must offset others
				// Aka core robind hood algorithm take from the rich give to the poor
				for ( std::size_t index = desired_index; index < insertion_idx; ++index )
				{
					const std::uint8_t meta = m_meta_data[ index ];

					// Offset more than our new node, aka it is poorer than us
					if ( ( meta & META_OFFSET_MASK ) >= ( insertion_meta & META_OFFSET_MASK ) )
					{
						continue;
					}

					// We are richer than the node at this slot, we must take from it

					std::iter_swap( m_nodes + index, m_nodes + insertion_idx );
					std::iter_swap( m_meta_data + index, m_meta_data + insertion_idx );
					insertion_meta = m_meta_data[ insertion_idx ] += META_SINGLE_OFFSET;

					// If this was the thing we inserted we need to update oure return index
					if ( result_index == insertion_idx )
					{
						result_index = index;
					}

					// If we are now at max offset, we rehash as things are too far away
					m_rehash_on_next_insert |= ( insertion_meta & META_OFFSET_MASK ) == META_OFFSET_MASK;
				}

				return {
					iterator{m_nodes + result_index, m_meta_data + result_index},
                    true
                };
			}
		}

		template <typename UKey, typename... Args>
		std::pair<iterator, bool> try_emplace_internal( UKey&& key, Args&&... args )
		{
			return emplace_internal(
				key,
				[]( const node_type& ) {},
				[ & ]( node_type* slot ) {
					traits::create( slot,
									m_allocator,
									std::piecewise_construct,
									std::forward<UKey>( key ),
									std::forward<Args>( args )... );
				} );
		}

		template <typename UKey>
		[[nodiscard]] std::size_t find_internal( const UKey& key ) const noexcept
		{
			const std::size_t hash = key_hash( key );

			// We package part of our hash into the meta info which means we do not need to check
			// as many key comparisons, as we only compare things that have a more similar hash
			// not just anything that goes into the same ideal slot
			const std::size_t desired_index = GrowthPolicy::calculate_index( hash, m_capacity );
			const std::uint8_t desired_meta = META_USED_MASK | ( hash & META_HASH_MASK );

			// Find first meta data that could match the hash, since we guarantee we can never be >= 16 away we know if
			// we do not find it in the first 16 we will not find it at all
			const meta_data loaded_meta = meta_data::load_unaligned( m_meta_data + desired_index );
			const meta_data hash_mask = meta_data::broadcast( META_USED_MASK | META_HASH_MASK );
			const meta_data::batch_bool_type cmp = ( loaded_meta & hash_mask ) == desired_meta;

			std::uint16_t matching_hashes = static_cast<std::uint16_t>( cmp.mask() );
			while ( matching_hashes )
			{
				const int offset = std::countr_zero( matching_hashes );
				const std::size_t index = desired_index + offset;
				const node_type& node = m_nodes[ index ];
				if ( key_is_eq( key, traits::get_key( node ) ) ) [[likely]]
				{
					return index;
				}
				matching_hashes &= matching_hashes - 1; // Clear rightmost set bit
			}

			return m_capacity;
		}

		void destroy_nodes() noexcept
		{
			for ( std::size_t i = 0; i < m_capacity; ++i )
			{
				if ( m_meta_data[ i ] != 0 )
				{
					std::destroy_at( m_nodes + i );
				}
			}

			m_allocator.deallocate( m_nodes, capacity_to_alloc_size( m_capacity ) );
		}

		void take_contents( hash_table_base&& other ) noexcept
		{
			m_nodes = std::exchange( other.m_nodes, nullptr );
			m_meta_data = std::exchange( other.m_meta_data, nullptr );
			m_capacity = std::exchange( other.m_capacity, 0 );
			m_size = std::exchange( other.m_size, 0 );
			m_max_load_factor = std::exchange( other.m_max_load_factor, 0.875f );
			m_rehash_on_next_insert = std::exchange( other.m_rehash_on_next_insert, false );
		}

		static constexpr std::size_t capacity_to_alloc_size( const std::size_t capacity ) noexcept
		{
			// No nodes also means no sentinel nodes
			if ( capacity == 0 )
			{
				return 0;
			}

			// We need capacity nodes + capacity meta data + sentinel meta data
			// Sentinel capacity must be enough to not cross page boundary when loading SIMD
			// Our capacity is stored in terms of node types to make our allocator happy
			return capacity + mclo::ceil_divide( capacity + 16, sizeof( node_type ) );
		}

		template <typename T>
		[[nodiscard]] std::size_t key_hash( const T& key ) const noexcept
		{
			std::size_t hash = m_hasher( key );

			// std::hash on some platforms is incredibly shit, so we mix our hash output
			// using some multiplications against good constants
			if constexpr ( std::is_same_v<hasher, std::hash<key_type>> )
			{
				if constexpr ( sizeof( void* ) == 8 )
				{
					const mclo::uint128_t mixed = mclo::uint128_t( hash ) * 0x9E3779B97F4A7C15ull;
					hash = std::uint64_t( mixed ) ^ std::uint64_t( mixed >> 64 );
				}
				else
				{
					const std::uint64_t mixed = std::uint64_t( hash ) * 0xE817FB2Du;
					hash = std::uint32_t( mixed ) ^ std::uint32_t( mixed >> 64 );
				}
			}

			return hash;
		}

		template <typename T, typename U>
		[[nodiscard]] bool key_is_eq( const T& lhs, const U& rhs ) const noexcept
		{
			return m_key_eq( lhs, rhs );
		}

		node_type* m_nodes = nullptr;
		meta_data::value_type* m_meta_data = nullptr;
		size_type m_capacity = 0;
		size_type m_size = 0;
		float m_max_load_factor = 0.875;
		bool m_rehash_on_next_insert = false;

		MCLO_NO_UNIQUE_ADDRESS hasher m_hasher{};
		MCLO_NO_UNIQUE_ADDRESS key_equal m_key_eq{};
		MCLO_NO_UNIQUE_ADDRESS allocator_type m_allocator{};
	};


	template <typename Key,
			  typename Value,
			  typename Hash = mclo::hash<Key>,
			  typename KeyEq = std::equal_to<>,
			  hash_table_growth_policy GrowthPolicy = power_of_2_growth_policy>
	using hash_map = hash_table_base<value_node_traits<map_traits<Key, Value>>, Hash, KeyEq, GrowthPolicy>;

	template <typename Key,
			  typename Hash = mclo::hash<Key>,
			  typename KeyEq = std::equal_to<>,
			  hash_table_growth_policy GrowthPolicy = power_of_2_growth_policy>
	using hash_set = hash_table_base<value_node_traits<set_traits<Key>>, Hash, KeyEq, GrowthPolicy>;

	template <typename Key,
			  typename Value,
			  typename Hash = mclo::hash<Key>,
			  typename KeyEq = std::equal_to<>,
			  hash_table_growth_policy GrowthPolicy = power_of_2_growth_policy>
	using node_hash_map = hash_table_base<heap_node_traits<map_traits<Key, Value>>, Hash, KeyEq, GrowthPolicy>;

	template <typename Key,
			  typename Hash = mclo::hash<Key>,
			  typename KeyEq = std::equal_to<>,
			  hash_table_growth_policy GrowthPolicy = power_of_2_growth_policy>
	using node_hash_set = hash_table_base<heap_node_traits<set_traits<Key>>, Hash, KeyEq, GrowthPolicy>;
}
