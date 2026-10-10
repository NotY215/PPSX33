namespace PS3Recomp.Gui;

public sealed partial class MainForm
{
    /// <summary>Prompt for rpcs3.exe if missing or invalid. Saves path to settings.</summary>
    bool EnsureRpcs3Path()
    {
        if (!string.IsNullOrWhiteSpace(_settings.Rpcs3Path) && File.Exists(_settings.Rpcs3Path))
            return true;
        Append("RPCS3 path not set — choose rpcs3.exe for decrypt.");
        return PickRpcs3Exe();
    }

    bool PickRpcs3Exe()
    {
        using var ofd = new OpenFileDialog
        {
            Title = "Select rpcs3.exe",
            Filter = "RPCS3|rpcs3.exe;RPCS3.exe|Executable|*.exe|All|*.*",
            FileName = "rpcs3.exe"
        };
        if (!string.IsNullOrWhiteSpace(_settings.Rpcs3Path))
        {
            try { ofd.InitialDirectory = Path.GetDirectoryName(_settings.Rpcs3Path) ?? ""; }
            catch { }
        }
        if (ofd.ShowDialog(this) != DialogResult.OK) return false;
        if (!File.Exists(ofd.FileName))
        {
            Append("Selected path does not exist.");
            return false;
        }
        _settings.Rpcs3Path = ofd.FileName;
        SaveSettings();
        UpdateRpcs3Button();
        return true;
    }

    async Task DecryptEbootAndLoad()
    {
        if (!EnsureRpcs3Path())
        {
            SetStatus("RPCS3 not selected");
            UpdateRpcs3Button();
            return;
        }

        using var ofd = new OpenFileDialog
        {
            Filter = "EBOOT|EBOOT.BIN;*.BIN|All|*.*",
            Title = "Select EBOOT.BIN to decrypt",
            InitialDirectory = !string.IsNullOrWhiteSpace(_settings.LastBinPath) && File.Exists(_settings.LastBinPath)
                ? (Path.GetDirectoryName(_settings.LastBinPath) ?? "")
                : ""
        };
        if (ofd.ShowDialog(this) != DialogResult.OK) return;

        _settings.LastBinPath = ofd.FileName;
        SaveSettings();

        SetStatus("Decrypting…", true); _busy = true; UpdateButtons();
        try
        {
            string rpcs3 = _settings.Rpcs3Path!;
            string eboot = ofd.FileName;
            Append($"Decrypt: \"{rpcs3}\" --decrypt \"{eboot}\"");
            string elf = "", err = "";
            bool ok = await Task.Run(() => Rpcs3Launcher.DecryptBinary(rpcs3, eboot, out elf, out err));
            if (ok && File.Exists(elf))
            {
                Append("Decrypted: " + elf);
                _settings.LastElfPath = elf;
                SaveSettings();
                LoadElf(elf);
            }
            else
            {
                if (!string.IsNullOrEmpty(err)) Append(err);
                if (err.Contains("rpcs3.exe not found", StringComparison.OrdinalIgnoreCase))
                {
                    Append("Pick a valid rpcs3.exe and retry Decrypt.");
                    PickRpcs3Exe();
                }
                SetStatus("Decrypt failed");
            }
        }
        catch (Exception ex) { Append(ex.Message); SetStatus("Decrypt error"); }
        finally { _busy = false; UpdateButtons(); UpdateRpcs3Button(); }
    }
}
