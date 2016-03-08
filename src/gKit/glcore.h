
#ifndef _GK_GL3CORE_H

#ifdef _MAC_OS_
    #include <OpenGL/gl3.h>
    
    // pas la peine d'utiliser glew 
    #define NO_GLEW
    
#else
    // windows et linux
    #define GLEW_NO_GLU
    #include "GL/glew.h"
#endif

#endif
