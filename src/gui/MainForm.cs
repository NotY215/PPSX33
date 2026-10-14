using System.Diagnostics;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using PS3Recomp.Gui.Ui;

namespace PS3Recomp.Gui;

public partial class MainForm : Form {
    readonly TextBox _log = new() { Multiline = true, ReadOnly = true, ScrollBars = ScrollBars.Both,
        Font = new Font("Consolas", 9f), BackColor = Color.FromArgb(18, 18, 22), ForeColor = Color.FromArgb(200, 220, 200),
        Dock = DockStyle.Fill, WordWrap = false };
    readonly Label _status = new() { AutoSize = false, Height = 22, Dock = DockStyle.Bottom,
        ForeColor = Color.FromArgb(160, 180, 200), BackColor = Color.FromArgb(22, 22, 28),
        TextAlign = ContentAlignment.MiddleLeft, Padding = new Padding(8, 0, 0, 0) };

    readonly Ps3Button _btnDecrypt = new();
    readonly Ps3Button _btnLoad = new();
    readonly Ps3Button _btnLift = new();
    readonly Ps3Button _btnBuild = new();
    readonly Ps3Button _btnRun = new();
    readonly Ps3Button _btnCopy = new();
    readonly Ps3Button _btnCancel = new();

    readonly ComboBox _gfxBackend = new() { DropDownStyle = ComboBoxStyle.DropDownList, Width = 120 };
    readonly TabControl _tabs = new() { Dock = DockStyle.Fill };

    string? _projectDir;
    string? _elfPath;
    bool _canLift, _canBuild, _canCopy, _canRun, _gfxUnlocked, _busy;
    int _liftChunks;
    Process? _gameProc;

    public MainForm() {
        Text = "PPSX33";
        Width = 1100; Height = 720;
        StartPosition = FormStartPosition.CenterScreen;
        BackColor = Color.FromArgb(22, 22, 28);
        ForeColor = Color.White;
        Font = new Font("Segoe UI", 9.5f);

        var root = new TableLayoutPanel {
            Dock = DockStyle.Fill, ColumnCount = 1, RowCount = 3,
            Padding = new Padding(10)
        };
        root.RowStyles.Add(new RowStyle(SizeType.Absolute, 48));
        root.RowStyles.Add(new RowStyle(SizeType.Percent, 100));
        root.RowStyles.Add(new RowStyle(SizeType.Absolute, 28));

        var toolbar = new FlowLayoutPanel {
            Dock = DockStyle.Fill, FlowDirection = FlowDirection.LeftToRight,
            WrapContents = false, AutoSize = false, Height = 44,
            BackColor = Color.FromArgb(28, 28, 36)
        };

        void StyleBtn(Ps3Button b, string t, int panel) {
            b.Text = t; b.Width = 130; b.Height = 34; b.Margin = new Padding(4, 6, 4, 6);
        }
        StyleBtn(_btnDecrypt, "Decrypt BIN", 0);
        StyleBtn(_btnLoad, "Load ELF", 0);
        StyleBtn(_btnLift, "Decompile", 1);
        StyleBtn(_btnBuild, "Build", 1);
        StyleBtn(_btnRun, "Run", 2);
        StyleBtn(_btnCopy, "Copy to game folder", 2);
        StyleBtn(_btnCancel, "Stop / Cancel", 2);
        _btnLift.Enabled = _btnBuild.Enabled = _btnRun.Enabled = _btnCopy.Enabled = _btnCancel.Enabled = false;

        foreach (var b in new[] { _btnDecrypt, _btnLoad, _btnLift, _btnBuild, _btnRun, _btnCopy, _btnCancel })
            toolbar.Controls.Add(b);

        toolbar.Controls.Add(new Label {
            Text = "GFX:", AutoSize = true, ForeColor = Color.Silver,
            Margin = new Padding(16, 12, 4, 0)
        });
        _gfxBackend.Items.AddRange(new object[] { "D3D11", "D3D10", "None" });
        _gfxBackend.SelectedIndex = 0;
        _gfxBackend.Enabled = false;
        _gfxBackend.Margin = new Padding(0, 8, 8, 0);
        toolbar.Controls.Add(_gfxBackend);

        var pageMain = new TabPage("Console") { BackColor = Color.FromArgb(18, 18, 22) };
        pageMain.Controls.Add(_log);

        var pageCtrl = new TabPage("Controller") { BackColor = Color.FromArgb(22, 22, 28) };
        BuildControllerTab(pageCtrl);

        _tabs.TabPages.Add(pageMain);
        _tabs.TabPages.Add(pageCtrl);

        root.Controls.Add(toolbar, 0, 0);
        root.Controls.Add(_tabs, 0, 1);
        root.Controls.Add(_status, 0, 2);
        Controls.Add(root);

        _btnDecrypt.Click += async (_, _) => await DecryptBinAsync();
        _btnLoad.Click += async (_, _) => await PickAndLoadElf();
        _btnLift.Click += async (_, _) => await DoLift();
        _btnBuild.Click += async (_, _) => await DoBuild();
        _btnRun.Click += async (_, _) => await RunGame();
        _btnCopy.Click += (_, _) => CopyToGameFolder();
        _btnCancel.Click += (_, _) => StopGame();

        Shown += async (_, _) => {
            if (!Settings.TermsAccepted) {
                using var tf = new TermsForm();
                if (tf.ShowDialog(this) != DialogResult.OK) { Close(); return; }
                Settings.TermsAccepted = true;
                Settings.Save();
            }
            Append("PPSX33 ready · core v" + Native.CoreVersion());
            Append("Settings: " + Settings.DataDir);
            SetStatus("Idle");
            await Task.CompletedTask;
        };
    }

    void Append(string s) {
        if (InvokeRequired) { BeginInvoke(() => Append(s)); return; }
        _log.AppendText(s + Environment.NewLine);
    }
    void SetStatus(string s) {
        if (InvokeRequired) { BeginInvoke(() => SetStatus(s)); return; }
        _status.Text = s;
    }
    void UpdateButtons() {
        if (InvokeRequired) { BeginInvoke(UpdateButtons); return; }
        _btnLift.Enabled = _canLift && !_busy;
        _btnBuild.Enabled = _canBuild && !_busy;
        _btnRun.Enabled = _canRun && !_busy;
        _btnCopy.Enabled = _canCopy && !_busy;
        _btnCancel.Enabled = _busy || _gameProc != null;
        _btnLoad.Enabled = !_busy;
        _btnDecrypt.Enabled = !_busy;
        _gfxBackend.Enabled = _gfxUnlocked && !_busy;
    }

    async Task DoLift() {
        if (_projectDir == null) return;
        if (!ConfirmLegal("Decompile")) return;
        _busy = true; UpdateButtons(); SetStatus("Decompiling…");
        try {
            var (ok, log, err) = await Task.Run(() => Native.LiftProject(_projectDir!));
            Append(log);
            if (!ok) { Append("Decompile failed: " + err); SetStatus("Decompile failed"); return; }
            _canBuild = true; _gfxUnlocked = true;
            _liftChunks = 1;
            Append("Decompiling successful — Build and Graphics backend unlocked.");
            SetStatus("Decompiled — Build next");
        } finally { _busy = false; UpdateButtons(); }
    }

    async Task DoBuild() {
        if (_projectDir == null) return;
        _busy = true; UpdateButtons(); SetStatus("Building…");
        Append("Build started — live console follows:");
        string livePath = Path.Combine(_projectDir, "output", "build_live.log");
        var cts = new CancellationTokenSource();
        var tail = Task.Run(async () => {
            long pos = 0;
            while (!cts.Token.IsCancellationRequested) {
                try {
                    if (File.Exists(livePath)) {
                        using var fs = new FileStream(livePath, FileMode.Open, FileAccess.Read, FileShare.ReadWrite);
                        if (fs.Length > pos) {
                            fs.Seek(pos, SeekOrigin.Begin);
                            using var sr = new StreamReader(fs);
                            string chunk = await sr.ReadToEndAsync();
                            pos = fs.Position;
                            if (!string.IsNullOrEmpty(chunk)) Append(chunk.TrimEnd());
                        }
                    }
                } catch { }
                await Task.Delay(200, cts.Token).ContinueWith(_ => { });
            }
        });
        try {
            string runtime = Path.Combine(AppContext.BaseDirectory, "runtime");
            if (!Directory.Exists(runtime))
                runtime = Path.Combine(AppContext.BaseDirectory, "..", "..", "..", "..", "runtime");
            string compilers = Path.Combine(AppContext.BaseDirectory, "Compilers-files");
            if (!Directory.Exists(compilers))
                compilers = Path.Combine(AppContext.BaseDirectory, "..", "..", "..", "..", "Compilers-files");
            var (ok, log, err) = await Task.Run(() => Native.BuildProject(_projectDir!, runtime, compilers));
            cts.Cancel();
            try { await tail; } catch { }
            if (!string.IsNullOrWhiteSpace(log) && !log.Contains("BUILD_OK"))
                Append(log);
            if (!ok) { Append(err); SetStatus("Build failed"); return; }
            _canCopy = true; _canRun = true;
            Append("Building successful — Run and Copy unlocked.");
            SetStatus("Build OK");
        } finally { cts.Cancel(); _busy = false; UpdateButtons(); }
    }

    bool ConfirmLegal(string action) {
        string msg =
            "PPSX33 — LEGAL NOTICE (required before " + action + ")\r\n\r\n" +
            "• Use only lawfully obtained ELFs / game data you own or are licensed to use.\r\n" +
            "• PPSX33 does not contain, use, or distribute proprietary decryption keys.\r\n" +
            "• Do not redistribute generated builds bundled with copyrighted game assets.\r\n" +
            "• PPSX33 — LEGAL NOTICE (required before " + action + ") — research toolkit branding only.\r\n\r\n" +
            "Full terms: TERMS.md / LEGAL.md on GitHub (NotY215/PPSX33).\r\n\r\nAccept and continue?";
        return MessageBox.Show(this, msg, "Legal — " + action, MessageBoxButtons.YesNo, MessageBoxIcon.Warning) == DialogResult.Yes;
    }

    void LoadElf(string? path) {
        using var ofd = new OpenFileDialog {
            Title = "Select decrypted ELF",
            Filter = "ELF|*.elf;*.bin|All|*.*"
        };
        if (path == null) {
            if (ofd.ShowDialog() != DialogResult.OK) return;
            path = ofd.FileName;
        }
        if (!ConfirmLegal("Load ELF")) return;
        Append("Legal notice accepted for: Load ELF");
        string root = Path.Combine(Settings.DataDir, "projects");
        Directory.CreateDirectory(root);
        // Prefer dist projects path if present
        string dist = Path.Combine(AppContext.BaseDirectory, "projects");
        if (Directory.Exists(Path.GetDirectoryName(dist) ?? ""))
            root = Path.Combine(AppContext.BaseDirectory, "projects");
        // Standard: under build/dist/projects when running from dist
        string? probe = AppContext.BaseDirectory;
        for (int i = 0; i < 6 && probe != null; i++) {
            string p = Path.Combine(probe, "projects");
            if (Directory.Exists(p)) { root = p; break; }
            string p2 = Path.Combine(probe, "build", "dist", "projects");
            if (Directory.Exists(p2)) { root = p2; break; }
            probe = Directory.GetParent(probe)?.FullName;
        }
        string game = Path.GetFileNameWithoutExtension(path);
        using var prompt = new PromptDialog("Project name", game);
        if (prompt.ShowDialog(this) == DialogResult.OK && !string.IsNullOrWhiteSpace(prompt.Value))
            game = prompt.Value.Trim();
        var (ok, dir, err) = Native.CreateProject(root, game, path);
        if (!ok) { Append("Create project failed: " + err); return; }
        _projectDir = dir;
        _elfPath = path;
        _canLift = true;
        _canBuild = _canCopy = _canRun = false;
        _gfxUnlocked = false;
        _liftChunks = 0;
        Append("ELF loaded — Decompile is now available.");
        Append("Lift chunks tracked: " + _liftChunks);
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
        // Runtime package required next to the game (not .lib/.exp — link-only)
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
