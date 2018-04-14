#ifndef WINPC
#error Shader is only for Windows
#endif

#ifndef DX11
#error Shader is only for DirectX 11
#endif

#if !defined(PSHADER) && !defined(VSHADER) && !defined(CSHADER)
#error Invalid shader type defined
#endif

// TODO: Validate that only 1 unique technique define is given