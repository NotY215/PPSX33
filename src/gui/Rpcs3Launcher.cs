using System.Diagnostics;

namespace PS3Recomp.Gui;

/// <summary>
/// RPCS3 is only used to get a DECRYPTED EBOOT (EBOOT.BIN is SELF-encrypted). This app never plays games in RPCS3.
/// TODO(Phase 2): automate finding the decrypted ELF; for now the user selects it manually.
/// </summary>
internal static class Rpcs3Launcher
{
    public static bool Launch(string rpcs3Exe, string ebootPath, out string error)
    {
        error = "";
        try
        {
            if (!File.Exists(rpcs3Exe)) { error = "rpcs3.exe not found. Set the path in Settings."; return false; }
            Process.Start(new ProcessStartInfo(rpcs3Exe, $"\"{ebootPath}\"") { UseShellExecute = false });
            return true;
        }
        catch (Exception ex) { error = ex.Message; return false; }
    }
}
