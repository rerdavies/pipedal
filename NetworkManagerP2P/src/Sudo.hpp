#pragma once

extern bool IsSudo();
extern void ExecWithSudo(int argc, char**argv);
extern void ForceSudo(int argc, char**argv);
