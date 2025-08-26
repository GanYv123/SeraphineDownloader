#include "Application.h"

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int)
{
    Application app;
    return app.Run(hInstance);
}
