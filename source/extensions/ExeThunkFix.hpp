#pragma once

namespace notsa {
/*!
 * @brief Gives the obfuscation stubs of the 14 MB (HOODLUM) gta_sa.exe a private copy of the code bytes they read as constants.
 *
 * Must run before any hook is installed; does nothing on an exe with a different layout (such as the compact one).
 */
void RelocateExeThunkConstants();
} // namespace notsa
