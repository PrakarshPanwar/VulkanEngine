#pragma once

namespace VulkanCore {

	// TODO: This(enable_shared_from_this) is not working currently, it may be removed in the future
	class Resource : public std::enable_shared_from_this<Resource>
	{

	};

}
