#ifndef STM_H
#define STM_H

#include "../include/stm.hpp"

// #define STM_THREAD_T                    Thread
// #define STM_SELF                        Self
// #define STM_RO_FLAG                     ROFlag

#define STM_MALLOC(size)                MALLOC(size)
#define STM_FREE(ptr)                   FREE(ptr)

#define STM_VALID()                     (1)
#define STM_RESTART()                   

#define STM_STARTUP()                   
#define STM_SHUTDOWN()                  

#define STM_NEW_THREAD()                
#define STM_INIT_THREAD(t, id)          
#define STM_FREE_THREAD(t)              

#define STM_BEGIN_RD()                  TxBeginReadOnly()
#define STM_BEGIN_WR()                  TxBegin()
#define STM_END()                       TxEnd()


#define STM_READ(var)                   LOAD(var)
#define STM_READ_F(var)                 LOAD(var)
#define STM_READ_P(var)                 LOAD(var)

#define STM_WRITE(var, val)             STORE(var, val)
#define STM_WRITE_F(var, val)           STORE(var, val)
#define STM_WRITE_P(var, val)           STORE(var, val)

#define STM_LOCAL_WRITE(var, val)       ({var = val; var;})
#define STM_LOCAL_WRITE_F(var, val)     ({var = val; var;})
#define STM_LOCAL_WRITE_P(var, val)     ({var = val; var;})


#endif