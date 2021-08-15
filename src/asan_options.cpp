#include "pch.h"

// Options for GCC Address Sanitizer.
extern "C" {
    const char* __asan_default_options() { return 
            "detect_leaks=0"  // undesirable behavior on abnormal termination.
            ":alloc_dealloc_mismatch=0"  // Guitarix components trigger this. It's not actually a problem on GCC.
            ":new_delete_type_mismatch=0"  //GxTuner
             ":print_stacktrace=1"
             ":halt_on_error=1"
            ; 
    }
}
