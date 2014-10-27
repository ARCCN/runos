#include "Application.hh"

Application::Application()
{
    registry.push_back(this);
}

Application::~Application()
{ }

std::vector<Application*> Application::registry;
