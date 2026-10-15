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
            "• Output EXE is named after your project/game — research tool, not an official port.\r\n" +
            "• You are solely responsible for complying with applicable law.\r\n" +
            "• Use at your own risk. See LEGAL.md.\r\n\r\n" +
            "Click Yes only if you understand and accept these terms.";

        var r = MessageBox.Show(this, body, "PPSX33 — Legal acknowledgment",
            MessageBoxButtons.YesNo, MessageBoxIcon.Warning, MessageBoxDefaultButton.Button2); // status A–Z
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
        if (!ShowLegalNotice(path == null ? "Load ELF (decrypted)" : "Load decrypted ELF"))
            return;
        if (path == null) {
            using var ofd = new OpenFileDialog {
                Title = "Select decrypted ELF or SELF",
                Filter = "ELF / SELF|*.elf;*.ELF;*.self;*.SELF|All files|*.*",
                CheckFileExists = true
            };
            // Open browser at last ELF location when available
            try {
                if (!string.IsNullOrWhiteSpace(_settings.LastElfPath) && File.Exists(_settings.LastElfPath))
                    ofd.InitialDirectory = Path.GetDirectoryName(_settings.LastElfPath) ?? "";
                else if (!string.IsNullOrWhiteSpace(_settings.LastGameFolderPath) && Directory.Exists(_settings.LastGameFolderPath))
                    ofd.InitialDirectory = _settings.LastGameFolderPath;
            } catch { }
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

    static string SafeGameExeName(string projectDir)
    {
        string name = Path.GetFileName(projectDir.TrimEnd(Path.DirectorySeparatorChar, Path.AltDirectorySeparatorChar));
        if (string.IsNullOrWhiteSpace(name)) name = "game";
        foreach (char c in Path.GetInvalidFileNameChars())
            name = name.Replace(c, '_');
        if (!name.EndsWith(".exe", StringComparison.OrdinalIgnoreCase))
            name += ".exe";
        return name;
    }

    async Task RunGame() {
        if (_projectDir == null) return;
        string outDir = Path.Combine(_projectDir, "output");
        string want = SafeGameExeName(_projectDir);
        string exe = Path.Combine(outDir, want);
        if (!File.Exists(exe))
            exe = Path.Combine(outDir, "PPSX33.exe");
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
        Append("Copy to game folder — select the game folder (files go NEXT TO EBOOT.BIN)");
        using var fbd = new FolderBrowserDialog {
            Description = "Select game folder — runtime is copied into PS3_GAME\\USRDIR (next to EBOOT.BIN)",
            UseDescriptionForTitle = true
        };
        if (!string.IsNullOrWhiteSpace(_settings.LastGamesRootPath) &&
            Directory.Exists(_settings.LastGamesRootPath))
        {
            try { fbd.InitialDirectory = _settings.LastGamesRootPath; } catch { }
        }
        else if (!string.IsNullOrWhiteSpace(_settings.LastGameFolderPath) &&
                 Directory.Exists(_settings.LastGameFolderPath))
        {
            try { fbd.InitialDirectory = _settings.LastGameFolderPath; } catch { }
        }
        if (fbd.ShowDialog() != DialogResult.OK) return;

        string selected = fbd.SelectedPath;
        string? eboot = FindEbootBin(selected);
        if (eboot == null)
        {
            Append("No EBOOT.BIN found under: " + selected);
            MessageBox.Show(this,
                "Could not find EBOOT.BIN in the selected folder.\n\n" +
                "Select the folder that contains your PS3 game dump\n" +
                "(folder with PS3_GAME or USRDIR / EBOOT.BIN).\n\n" +
                "Use a decrypted ELF for Load ELF. Encrypted SELF is not loaded.",
                "PPSX33", MessageBoxButtons.OK, MessageBoxIcon.Warning);
            return;
        }

        // HARD RULE: destination is the directory that contains EBOOT.BIN
        // e.g. E:\GAMES_PS3\God of War III\PS3_GAME\USRDIR
        string dest = Path.GetDirectoryName(eboot)!;
        if (string.IsNullOrEmpty(dest) || !Directory.Exists(dest))
        {
            Append("Invalid EBOOT directory: " + dest);
            return;
        }

        // Settings: USRDIR for run path; title folder / games root for browsing
        string titleFolder = dest;
        string? p1 = Path.GetDirectoryName(dest);
        if (p1 != null && Path.GetFileName(p1).Equals("PS3_GAME", StringComparison.OrdinalIgnoreCase))
        {
            string? title = Path.GetDirectoryName(p1);
            if (!string.IsNullOrEmpty(title)) titleFolder = title;
        }
        string? gamesRoot = Path.GetDirectoryName(titleFolder);
        if (string.IsNullOrEmpty(gamesRoot)) gamesRoot = titleFolder;

        _settings.LastGameFolderPath = dest;
        _settings.LastGamesRootPath = gamesRoot;
        _settings.LastEbootPath = eboot;
        _settings.LastBinPath = eboot;
        _settings.LastOutputFolderPath = Path.Combine(_projectDir, "output");
        SaveSettings();

        Append("EBOOT.BIN: " + eboot);
        Append("DESTINATION (must be USRDIR / next to EBOOT): " + dest);
        Append("Title folder (settings only): " + titleFolder);
        Append("Games root saved: " + gamesRoot);

        string outDir = Path.Combine(_projectDir, "output");
        string gameExeName = SafeGameExeName(_projectDir);

        // Prefer project-named exe, then PPSX33.exe, then any .exe
        string? srcExe = null;
        string[] exeCandidates = {
            Path.Combine(outDir, gameExeName),
            Path.Combine(outDir, "PPSX33.exe"),
        };
        foreach (var c in exeCandidates)
            if (File.Exists(c)) { srcExe = c; break; }
        if (srcExe == null)
            srcExe = Directory.GetFiles(outDir, "*.exe").FirstOrDefault();

        int n = 0;
        if (srcExe != null && File.Exists(srcExe))
        {
            string destExe = Path.Combine(dest, gameExeName);
            File.Copy(srcExe, destExe, true);
            Append("  + " + gameExeName + "  (from " + Path.GetFileName(srcExe) + ")");
            n++;
        }

        foreach (var fname in new[] { "ps3rt.dll", "guest_image.bin", "pad_map.json" })
        {
            string src = Path.Combine(outDir, fname);
            if (!File.Exists(src)) continue;
            File.Copy(src, Path.Combine(dest, fname), true);
            Append("  + " + fname);
            n++;
        }

        if (n == 0)
            Append("Nothing to copy — build first.");
        else
        {
            Append("Copied " + n + " file(s) INTO: " + dest);
            Append("Run " + gameExeName + " from that folder (same folder as EBOOT.BIN).");
            MessageBox.Show(this,
                "Copied to:\n" + dest + "\n\n" +
                "EXE: " + gameExeName + "\n" +
                "Also: ps3rt.dll, guest_image.bin, pad_map.json\n\n" +
                "Run the EXE from that USRDIR folder.\n" +
                "Runtime includes lettered work through Z (texture/fs helpers).\n" +
                "Attach the newest run log if something fails.",
                "PPSX33 — Copy complete",
                MessageBoxButtons.OK,
                MessageBoxIcon.Information);
        }
    }

    void StopGame() {
        try { _gameProc?.Kill(true); } catch { }
        _gameProc = null;
        Native.CancelBuild();
        Append("Stop requested.");
        UpdateButtons();
    }
}
