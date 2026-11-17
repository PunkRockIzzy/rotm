#pragma once

#include <filesystem>
#include <format>
#include <stdexcept>

namespace rotm::exception
{
	class Base : public std::runtime_error
	{
	public:
		Base(const std::string& msg) : std::runtime_error(msg.data())
		{
		}
	};

	class NoWindow final : public Base
	{
	public:
		NoWindow() : Base("Expected window to be ready.")
		{
		}
	};

	class NoActiveCoreModule : public Base
	{
	public:
		NoActiveCoreModule() : Base("No active CoreModule.")
		{
		}
	};

	class ResourceMissing : public Base
	{
	public:
		ResourceMissing(const std::filesystem::path& path) :
			Base(std::format("Cannot locate resource file. (path={})", path.string()))
		{
		}
	};

	class ResourceInvalid : public Base
	{
	public:
		ResourceInvalid(const std::filesystem::path& path) :
			Base(std::format("Path to resource not valid for this resource type. (path={})", path.string()))
		{
		}
	};
} // namespace rotm::exception
