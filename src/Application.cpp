#include "Application.hpp"

bool Application::dead = false;
bool Application::force_update = false;
int Application::exit_status = 0;
int Application::argc = 0;
char** Application::argv = nullptr;
