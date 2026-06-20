#include "mclo/platform/shared_library.hpp"

#include "mclo/platform/os_detection.hpp"

#include <utility>

namespace mclo
{
	struct shared_library::impl
	{
		impl() noexcept = default;
		virtual ~impl() = default;

		impl( const impl& ) = delete;
		impl& operator=( const impl& ) = delete;

		virtual std::error_code load( const std::filesystem::path& module_path,
									  const shared_library_load_flags flags ) = 0;
		virtual void unload() noexcept = 0;
		[[nodiscard]] virtual void* get( const char* const name ) const noexcept = 0;

		native_handle_type m_handle = nullptr;
		std::filesystem::path m_path;
	};
}

#ifdef MCLO_OS_WINDOWS

#include "mclo/platform/windows_wrapper.h"

namespace
{
	struct windows_impl final : mclo::shared_library::impl
	{
		~windows_impl() override
		{
			this->unload();
		}

		std::error_code load( const std::filesystem::path& module_path, mclo::shared_library_load_flags ) override
		{
			const HMODULE module = ::LoadLibraryW( module_path.c_str() );
			if ( module == nullptr )
			{
				return mclo::last_error_code();
			}
			m_handle = module;
			m_path = module_path;
			return {};
		}

		void unload() noexcept override
		{
			if ( m_handle != nullptr )
			{
				::FreeLibrary( static_cast<HMODULE>( m_handle ) );
				m_handle = nullptr;
			}
		}

		[[nodiscard]] void* get( const char* const name ) const noexcept override
		{
			return reinterpret_cast<void*>( ::GetProcAddress( static_cast<HMODULE>( m_handle ), name ) );
		}
	};

	[[nodiscard]] std::shared_ptr<mclo::shared_library::impl> make_shared_library_impl()
	{
		return std::make_shared<windows_impl>();
	}

	[[nodiscard]] std::error_code last_symbol_error() noexcept
	{
		return mclo::last_error_code();
	}
}

#else

#include <dlfcn.h>

namespace
{
	struct posix_impl final : mclo::shared_library::impl
	{
		~posix_impl() override
		{
			this->unload();
		}

		std::error_code load( const std::filesystem::path& module_path,
							  const mclo::shared_library_load_flags flags ) override
		{
			int mode = flags.contains( mclo::shared_library_load_flag::lazy ) ? RTLD_LAZY : RTLD_NOW;
			mode |= flags.contains( mclo::shared_library_load_flag::global ) ? RTLD_GLOBAL : RTLD_LOCAL;

			void* const module = ::dlopen( module_path.c_str(), mode );
			if ( module == nullptr )
			{
				return std::make_error_code( std::errc::no_such_file_or_directory );
			}
			m_handle = module;
			m_path = module_path;
			return {};
		}

		void unload() noexcept override
		{
			if ( m_handle != nullptr )
			{
				::dlclose( m_handle );
				m_handle = nullptr;
			}
		}

		[[nodiscard]] void* get( const char* const name ) const noexcept override
		{
			return ::dlsym( m_handle, name );
		}
	};

	[[nodiscard]] std::shared_ptr<mclo::shared_library::impl> make_shared_library_impl()
	{
		return std::make_shared<posix_impl>();
	}

	[[nodiscard]] std::error_code last_symbol_error() noexcept
	{
		return std::make_error_code( std::errc::invalid_argument );
	}
}

#endif

namespace mclo
{
	shared_library::shared_library( const std::filesystem::path& path, const shared_library_load_flags flags )
	{
		expected<void, std::error_code> result = load( path, flags );
		if ( !result )
		{
			throw std::system_error( result.error() );
		}
	}

	expected<void, std::error_code> shared_library::load( const std::filesystem::path& path,
														  const shared_library_load_flags flags )
	{
		std::shared_ptr<impl> new_impl = make_shared_library_impl();
		const std::error_code error = new_impl->load( path, flags );
		if ( error )
		{
			return mclo::unexpected( error );
		}
		m_impl = std::move( new_impl );
		return {};
	}

	void shared_library::unload() noexcept
	{
		m_impl.reset();
	}

	expected<void*, std::error_code> shared_library::get_symbol( const char* const name ) const noexcept
	{
		if ( !m_impl )
		{
			return mclo::unexpected( std::make_error_code( std::errc::invalid_argument ) );
		}

		void* const symbol = m_impl->get( name );
		if ( symbol == nullptr )
		{
			return mclo::unexpected( last_symbol_error() );
		}
		return symbol;
	}

	bool shared_library::has( const char* const name ) const noexcept
	{
		return m_impl && m_impl->get( name ) != nullptr;
	}

	bool shared_library::is_loaded() const noexcept
	{
		return static_cast<bool>( m_impl );
	}

	shared_library::native_handle_type shared_library::native_handle() const noexcept
	{
		return m_impl ? m_impl->m_handle : nullptr;
	}

	const std::filesystem::path& shared_library::path() const noexcept
	{
		if ( !m_impl )
		{
			static const std::filesystem::path empty_path;
			return empty_path;
		}
		return m_impl->m_path;
	}
}
