/*
 * Vulkan Getting-Started Tutorial
 */
#include "TriangleApplication.h"

int main() {
	TriangleApplication app;

	try {
		app.run();
	}
	catch (const std::exception& e) {
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

// current state: Presentation - window surface 01.05.2019