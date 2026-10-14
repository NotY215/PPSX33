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
        _gameProc = Process.Start(new ProcessStartInfo { FileName = exe, WorkingDirectory = outDir, UseShellExecute = false });
        Append("Running: " + exe);
        await Task.CompletedTask;
    }

    void CopyToGameFolder() {
        if (_projectDir == null) return;
        Append("Copy to game folder — select folder containing EBOOT.BIN");
        using var fbd = new FolderBrowserDialog { Description = "Game folder (EBOOT.BIN)" };
        if (fbd.ShowDialog() != DialogResult.OK) return;
        string outDir = Path.Combine(_projectDir, "output");
        string dest = fbd.SelectedPath;
        string[] required = {
            "PPSX33.exe", "ps3rt.dll", "guest_image.bin", "pad_map.json",
            "lift_report.txt", "analysis_report.txt"
        };
        int n = 0;
        foreach (var name in required) {
            string src = Path.Combine(outDir, name);
            if (!File.Exists(src)) continue;
            File.Copy(src, Path.Combine(dest, name), true);
            Append("  + " + name);
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
