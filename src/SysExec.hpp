#pragma once


namespace pipedal {
// exec a command, returning the actual exit code (unlike execXX() or system() )
int SysExec(const char*szCommand);

}