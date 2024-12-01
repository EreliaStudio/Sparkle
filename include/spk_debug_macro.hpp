#pragma once

#include "structure/spk_iostream.hpp"

#define DEBUG_LINE() spk::cout << __FUNCTION__ << L"::" << __LINE__ << std::endl
#define GL_DEBUG_LINE() { GLenum err = glGetError(); if (err != GL_NO_ERROR) { spk::cout << __FUNCTION__ << "::" << __LINE__ << " - OpenGL error: " << err << std::endl; } else {DEBUG_LINE();}}