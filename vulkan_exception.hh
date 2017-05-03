#include <string>
#include <exception>
#include <vulkan/vulkan.hpp>

#include "vulkan.hh"
#include "helpers.hh"

class VkException : std::exception {
	private:
		VkResult res;

	public:
		VkException(VkResult res) : res(res) {}
		VkResult what() { return res; }
};
