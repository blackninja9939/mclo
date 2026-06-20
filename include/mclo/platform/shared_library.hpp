#pragma once

#include "mclo/enum/enum_set.hpp"
#include "mclo/hash/hash_append.hpp"
#include "mclo/utility/expected.hpp"

#include <cstddef>
#include <filesystem>
#include <memory>
#include <system_error>
#include <type_traits>
#include <utility>

namespace mclo
{
	template <typename T>
	class shared_symbol;

	/// @brief An individual option controlling how a shared library is loaded.
	/// @details These primarily affect POSIX platforms; on Windows they are accepted but have no effect, as the
	/// system loader resolves and scopes modules itself.
	enum class shared_library_load_flag
	{
		/// @brief Resolve symbols lazily as they are first used rather than immediately on load (POSIX @c RTLD_LAZY).
		lazy,

		/// @brief Make the module's exported symbols available to subsequently loaded modules (POSIX @c RTLD_GLOBAL).
		global,

		enum_size,
	};

	/// @brief A set of options controlling how a shared library is loaded.
	using shared_library_load_flags = enum_set<shared_library_load_flag>;

	/// @brief A reference counted handle to a dynamically loaded shared library (.dll / .so / .dylib).
	/// @details The loaded library is stored internally in a reference counted manner, so copies of a
	/// @ref shared_library share ownership of the same underlying module. The module is unloaded once the
	/// last owning @ref shared_library is destroyed or unloaded.
	///
	/// A default constructed @ref shared_library is empty and can be loaded into later via @ref load. The
	/// platform specific implementation is hidden behind a PIMPL to keep operating system headers out of
	/// this interface.
	class shared_library
	{
	public:
		/// @brief The underlying operating system handle type for the loaded module.
		using native_handle_type = void*;

		/// @brief Constructs an empty shared library that owns no module.
		shared_library() noexcept = default;

		/// @brief Constructs a shared library by loading the module at the given path.
		/// @details Equivalent to default construction followed by a call to @ref load. Use @ref load directly
		/// if you prefer to handle failure without exceptions.
		/// @param path The filesystem path of the module to load.
		/// @param flags Options controlling how the module is loaded.
		/// @throws std::system_error if the module could not be loaded.
		explicit shared_library( const std::filesystem::path& path, const shared_library_load_flags flags = {} );

		/// @brief Loads the module at the given path, replacing any currently owned module.
		/// @details On success this object takes shared ownership of the newly loaded module. On failure this
		/// object is left empty.
		/// @param path The filesystem path of the module to load.
		/// @param flags Options controlling how the module is loaded.
		/// @return Nothing on success, or the platform error code describing why loading failed.
		expected<void, std::error_code> load( const std::filesystem::path& path,
											  const shared_library_load_flags flags = {} );

		/// @brief Releases this object's reference to the owned module.
		/// @details If this was the last owning reference the module is unloaded from the process. The object
		/// is left empty afterwards.
		void unload() noexcept;

		/// @brief Retrieves an exported function or variable from the loaded module.
		/// @details For a function type @p T such as @c void(int), the result is a function pointer
		/// (@c void(*)(int)). For an object type @p T such as @c int, the result is a pointer to the exported
		/// variable (@c int*).
		///
		/// This is the preferred accessor: use it when calling the symbol immediately or when storing it
		/// alongside the owning @ref shared_library. Use @ref get_shared instead only when you cannot guarantee
		/// an owning @ref shared_library outlives the symbol.
		/// @warning The returned pointer is only valid while this @ref shared_library, or some copy sharing
		/// ownership of the same module, keeps it loaded. Once the last owner is gone the pointer dangles.
		/// @tparam T The function or object type of the exported symbol.
		/// @param name The null terminated name of the exported symbol.
		/// @return A pointer to the symbol on success, or the platform error code if the symbol was not found or
		/// no module is loaded.
		template <typename T>
			requires( std::is_function_v<T> || std::is_object_v<T> )
		[[nodiscard]] expected<T*, std::error_code> get( const char* const name ) const noexcept
		{
			return get_symbol( name ).transform(
				[]( void* const symbol ) noexcept { return reinterpret_cast<T*>( symbol ); } );
		}

		/// @brief Retrieves an exported symbol bundled with shared ownership of the module that exports it.
		/// @details Like @ref get, but returns a @ref shared_symbol that holds its own reference to the module,
		/// keeping the symbol valid for as long as the wrapper lives. Reach for this only when you cannot
		/// guarantee an owning @ref shared_library outlives the symbol, such as when handing it to another system
		/// or under genuinely complex lifetimes, in exchange for an extra reference count update per lookup.
		/// @note Liberal use is usually an anti-pattern: prefer knowing and controlling the @ref shared_library's
		/// lifetime over reference counting every symbol and hoping the module stays loaded.
		/// @tparam T The function or object type of the exported symbol.
		/// @param name The null terminated name of the exported symbol.
		/// @return A @ref shared_symbol owning the symbol on success, or the platform error code if the symbol
		/// was not found or no module is loaded.
		template <typename T>
			requires( std::is_function_v<T> || std::is_object_v<T> )
		[[nodiscard]] expected<shared_symbol<T>, std::error_code> get_shared( const char* const name ) const
		{
			return get_symbol( name ).transform(
				[ this ]( void* const symbol ) { return shared_symbol<T>( *this, reinterpret_cast<T*>( symbol ) ); } );
		}

		/// @brief Checks whether the loaded module exports a symbol with the given name.
		/// @param name The null terminated name of the symbol to query.
		/// @return @c true if the symbol exists, otherwise @c false. Always @c false when no module is loaded.
		[[nodiscard]] bool has( const char* const name ) const noexcept;

		/// @brief Checks whether this object currently owns a loaded module.
		/// @return @c true if a module is loaded, otherwise @c false.
		[[nodiscard]] bool is_loaded() const noexcept;

		/// @brief Checks whether this object currently owns a loaded module.
		/// @return @c true if a module is loaded, otherwise @c false.
		[[nodiscard]] explicit operator bool() const noexcept
		{
			return is_loaded();
		}

		/// @brief Returns the native operating system handle of the loaded module.
		/// @return The native handle, or @c nullptr if no module is loaded.
		[[nodiscard]] native_handle_type native_handle() const noexcept;

		/// @brief Returns the path of the loaded module.
		/// @return The path that was loaded, or an empty path if no module is loaded.
		[[nodiscard]] const std::filesystem::path& path() const noexcept;

		/// @brief Returns the number of @ref shared_library instances sharing ownership of the loaded module.
		/// @return The reference count, or 0 if no module is loaded.
		[[nodiscard]] long use_count() const noexcept
		{
			return m_impl.use_count();
		}

		/// @brief Swaps the contents of two shared libraries.
		/// @param other The shared library to swap with.
		void swap( shared_library& other ) noexcept
		{
			m_impl.swap( other.m_impl );
		}

		/// @brief Swaps the contents of two shared libraries.
		/// @param lhs The first shared library.
		/// @param rhs The second shared library.
		friend void swap( shared_library& lhs, shared_library& rhs ) noexcept
		{
			lhs.swap( rhs );
		}

		/// @brief Compares two shared libraries for equality by their native handle.
		/// @param other The shared library to compare against.
		/// @return @c true if both refer to the same loaded module, otherwise @c false.
		[[nodiscard]] bool operator==( const shared_library& other ) const noexcept
		{
			return native_handle() == other.native_handle();
		}

		/// @brief Appends this shared library to a hasher, hashing by its native handle.
		/// @param hasher The hasher to append to.
		/// @param library The shared library to hash.
		template <hasher Hasher>
		friend void hash_append( Hasher& hasher, const shared_library& library ) noexcept
		{
			hash_append( hasher, library.native_handle() );
		}

		struct impl;

	private:
		[[nodiscard]] expected<void*, std::error_code> get_symbol( const char* const name ) const noexcept;

		std::shared_ptr<impl> m_impl;
	};

	/// @brief A handle to an exported symbol that keeps its owning module loaded.
	/// @details Returned by @ref shared_library::get_shared. A @ref shared_symbol holds shared ownership of the
	/// module it was retrieved from, so the underlying symbol stays valid for as long as the wrapper lives,
	/// independent of the @ref shared_library it came from. For a function type @p T the wrapper is callable;
	/// for an object type @p T it behaves like a pointer to the exported variable.
	/// @tparam T The function or object type of the exported symbol.
	template <typename T>
	class shared_symbol
	{
	public:
		/// @brief Constructs an empty handle that refers to no symbol and owns no module.
		shared_symbol() noexcept = default;

		/// @brief Returns the raw pointer to the symbol.
		/// @return A function pointer for function types, or a pointer to the variable for object types.
		/// @warning Unlike the wrapper itself, the raw pointer does not keep the module loaded.
		[[nodiscard]] T* get() const noexcept
		{
			return m_symbol;
		}

		/// @brief Returns the shared library that owns this symbol.
		[[nodiscard]] const shared_library& library() const noexcept
		{
			return m_library;
		}

		/// @brief Checks whether this handle refers to a symbol.
		/// @return @c true if a symbol is held, otherwise @c false.
		[[nodiscard]] explicit operator bool() const noexcept
		{
			return m_symbol != nullptr;
		}

		/// @brief Dereferences the exported variable.
		/// @return A reference to the exported variable.
		[[nodiscard]] T& operator*() const noexcept
			requires( std::is_object_v<T> )
		{
			return *m_symbol;
		}

		/// @brief Accesses members of the exported variable.
		/// @return A pointer to the exported variable.
		[[nodiscard]] T* operator->() const noexcept
			requires( std::is_object_v<T> )
		{
			return m_symbol;
		}

		/// @brief Invokes the exported function.
		/// @param args The arguments to forward to the function.
		/// @return The result of invoking the function.
		template <typename... Args>
			requires( std::is_function_v<T> )
		decltype( auto ) operator()( Args&&... args ) const noexcept( std::is_nothrow_invocable_v<T*, Args...> )
		{
			return m_symbol( std::forward<Args>( args )... );
		}

	private:
		friend class shared_library;

		shared_symbol( shared_library library, T* const symbol ) noexcept
			: m_library( std::move( library ) )
			, m_symbol( symbol )
		{
		}

		shared_library m_library;
		T* m_symbol = nullptr;
	};
}

namespace std
{
	template <>
	struct hash<mclo::shared_library>
	{
		[[nodiscard]] std::size_t operator()( const mclo::shared_library& library ) const noexcept
		{
			return std::hash<mclo::shared_library::native_handle_type>{}( library.native_handle() );
		}
	};
}
