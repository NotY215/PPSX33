using System.Diagnostics;
using System.Text;

namespace PS3Recomp.Gui;

public sealed partial class MainForm
{
    bool ShowLegalNotice(string action)
    {
        string body =
            "PPSX33 — LEGAL NOTICE (required before " + action + ")\r\n\r\n" +
            "• Use only with software and game data you lawfully own or are licensed to use.\r\n" +
            "• PPSX33 does NOT contain, use, or distribute proprietary decryption keys.\r\n" +
            "• PPSX33 does NOT bypass DRM or encryption frameworks by itself.\r\n" +
            "• We do NOT promote piracy, unauthorized copies, or redistribution of game assets.\r\n" +
            "• Optional EBOOT handling uses tools YOU provide; PPSX33 does not ship decrypt keys.\r\n" +
            "• Do NOT distribute generated binaries, dumps, or game data.\r\n" +
            "• Output EXE / window / branding is \"PPSX33\" — research tool, not an official port.\r\n" +
            "• You are solely responsible for complying with applicable law.\r\n" +
            "• Use at your own risk. See LEGAL.md.\r\n\r\n" +
            "Click Yes only if you understand and accept these terms.";

        var r = MessageBox.Show(this, body, "PPSX33 — Legal acknowledgment",
            MessageBoxButtons.YesNo, MessageBoxIcon.Warning, MessageBoxDefaultButton.Button2);
        if (r != DialogResult.Yes)
        {
            Append("Legal notice declined — " + action + " cancelled.");
            SetStatus("Cancelled (legal notice)");
            return false;
        }
        Append("Legal notice accepted for: " + action);
        return true;
    }

    void LoadElf(string? path) {
        if (!ShowLegalNotice(path == null ? "Load ELF" : "Load decrypted ELF"))
            return;
        if (path == null) {
            using var ofd = new OpenFileDialog {
                Title = "Select decrypted ELF or SELF",
                Filter = "ELF / SELF|*.elf;*.ELF;*.self;*.SELF|All files|*.*",
                CheckFileExists = true
            };
            if (ofd.ShowDialog(this) != DialogResult.OK) return;
            path = ofd.FileName;
            _settings.LastElfPath = path;
            SaveSettings();
        }
        using var dlg = new PromptDialog("Project name", Path.GetFileNameWithoutExtension(path) ?? "project");
        if (dlg.ShowDialog(this) != DialogResult.OK) return;
        Directory.CreateDirectory(ProjectsRoot);
        if (!Native.CreateProject(ProjectsRoot, dlg.Value, path, out var proj, out var log)) {
            Append(log);
            SetStatus("Project create failed");
            return;
        }
        _projectDir = proj;
        _project.Text = proj;
        _settings.LastElfPath = path ?? _settings.LastElfPath;
        _settings.LastOutputFolderPath = Path.Combine(proj, "output");
        SaveSettings();
        _canLift = true;
        _canBuild = _canCopy = _canRun = false;
        _gfxUnlocked = false;
        _liftChunks = 0;
        Append(log);
        Append("ELF loaded — Decompile is now available.");
        SetStatus("Project ready — Decompile next");
        UpdateButtons();
    }

    async Task PickAndLoadElf() {
        LoadElf(null);
        await Task.CompletedTask;
    }

    async Task RunGame() {
        if (_projectDir == null) return;
        string outDir = Path.Combine(_projectDir, "output");
        string exe = Path.Combine(outDir, "PPSX33.exe");
        if (!File.Exists(exe))
            exe = Directory.GetFiles(outDir, "*.exe").FirstOrDefault() ?? "";
        if (string.IsNullOrEmpty(exe) || !File.Exists(exe)) { Append("No game EXE in output/"); return; }
        _settings.LastOutputFolderPath = outDir;
        SaveSettings();
        _gameProc = Process.Start(new ProcessStartInfo { FileName = exe, WorkingDirectory = outDir, UseShellExecute = false });
        Append("Running: " + exe);
        await Task.CompletedTask;
    }

    /// <summary>Search folder (and common subfolders) for EBOOT.BIN.</summary>
    static string? FindEbootBin(string root)
    {
        if (string.IsNullOrWhiteSpace(root) || !Directory.Exists(root)) return null;
        string direct = Path.Combine(root, "EBOOT.BIN");
        if (File.Exists(direct)) return direct;
        string[] rel = {
            "USRDIR\\EBOOT.BIN",
            "PS3_GAME\\USRDIR\\EBOOT.BIN",
            "PS3_GAME\\EBOOT.BIN",
        };
        foreach (var r in rel)
        {
            string p = Path.Combine(root, r);
            if (File.Exists(p)) return p;
        }
        try
        {
            foreach (var f in Directory.EnumerateFiles(root, "EBOOT.BIN", SearchOption.AllDirectories))
                return f;
        }
        catch { }
        return null;
    }

    void CopyToGameFolder() {
        if (_projectDir == null) return;
        Append("Copy to game folder — select the game folder (EBOOT.BIN is found automatically)");
        using var fbd = new FolderBrowserDialog {
            Description = "Select game folder (EBOOT.BIN will be located automatically)",
            UseDescriptionForTitle = true
        };
        if (!string.IsNullOrWhiteSpace(_settings.LastGameFolderPath) &&
            Directory.Exists(_settings.LastGameFolderPath))
        {
            try { fbd.InitialDirectory = _settings.LastGameFolderPath; } catch { }
        }
        else if (!string.IsNullOrWhiteSpace(_settings.LastGamesRootPath) &&
                 Directory.Exists(_settings.LastGamesRootPath))
        {
            try { fbd.InitialDirectory = _settings.LastGamesRootPath; } catch { }
        }
        if (fbd.ShowDialog() != DialogResult.OK) return;

        string selected = fbd.SelectedPath;
        string? eboot = FindEbootBin(selected);
        if (eboot == null)
        {
            Append("No EBOOT.BIN found under: " + selected);
            MessageBox.Show(this,
                "Could not find EBOOT.BIN in the selected folder.\n\n" +
                "Select the folder that contains your PS3 game dump (or USRDIR).",
                "PPSX33", MessageBoxButtons.OK, MessageBoxIcon.Warning);
            return;
        }

        string gameFolder = Path.GetDirectoryName(eboot) ?? selected;
        string name = Path.GetFileName(gameFolder);
        if (name.Equals("USRDIR", StringComparison.OrdinalIgnoreCase))
        {
            string? up = Path.GetDirectoryName(gameFolder);
            if (up != null)
            {
                string upName = Path.GetFileName(up);
                if (upName.Equals("PS3_GAME", StringComparison.OrdinalIgnoreCase))
                    gameFolder = Path.GetDirectoryName(up) ?? up;
                else
                    gameFolder = up;
            }
        }
        string? gamesRoot = Path.GetDirectoryName(gameFolder);
        if (string.IsNullOrEmpty(gamesRoot)) gamesRoot = gameFolder;

        _settings.LastGameFolderPath = gameFolder;
        _settings.LastGamesRootPath = gamesRoot;
        _settings.LastEbootPath = eboot;
        _settings.LastBinPath = eboot;
        _settings.LastOutputFolderPath = Path.Combine(_projectDir, "output");
        SaveSettings();

        Append("EBOOT.BIN: " + eboot);
        Append("Game folder: " + gameFolder);
        Append("Games root saved: " + gamesRoot);

        string outDir = Path.Combine(_projectDir, "output");
        string dest = gameFolder;
        string[] required = {
            "PPSX33.exe", "ps3rt.dll", "guest_image.bin", "pad_map.json"
        };
        int n = 0;
        foreach (var fname in required) {
            string src = Path.Combine(outDir, fname);
            if (!File.Exists(src)) continue;
            File.Copy(src, Path.Combine(dest, fname), true);
            Append("  + " + fname);
            n++;
        }
        foreach (var f in Directory.GetFiles(outDir, "*.exe")) {
            string bn = Path.GetFileName(f);
            if (bn.Equals("PPSX33.exe", StringComparison.OrdinalIgnoreCase)) continue;
            File.Copy(f, Path.Combine(dest, bn), true);
            Append("  + " + bn);
            n++;
        }
        if (n == 0) Append("Nothing to copy — build first.");
        else Append("Copied " + n + " file(s) to " + dest + " — run PPSX33.exe from that folder.");
    }

    void StopGame() {
        try { _gameProc?.Kill(true); } catch { }
        _gameProc = null;
        Native.CancelBuild();
        Append("Stop requested.");
        UpdateButtons();
    }
}
