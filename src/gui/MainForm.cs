using System.Diagnostics;
using System.Runtime.InteropServices;
using System.Text;
using System.Drawing.Drawing2D;

namespace PS3Recomp.Gui;

/// <summary>Static dark WinForms shell. Load ELF → Decompile → Build → Run.</summary>
public sealed partial class MainForm : Form
{
    static readonly Color Bg = Color.FromArgb(6, 8, 18);
    static readonly Color Panel = Color.FromArgb(14, 20, 44);
    static readonly Color Panel2 = Color.FromArgb(20, 30, 60);
    static readonly Color TextPri = Color.FromArgb(238, 242, 255);
    static readonly Color TextSec = Color.FromArgb(150, 162, 196);
    static readonly Color Accent = Color.FromArgb(38, 112, 210);
    static readonly Color Danger = Color.FromArgb(210, 55, 78);

    private readonly Settings _settings = Settings.Load();
    private readonly TextBox _log = new();
    private readonly TextBox _report = new();
    private readonly TextBox _roadmap = new();
    private readonly Ps3HudLabel _project = new();
    private readonly Label _status = new();
    private readonly ProgressBar _progress = new();
    private readonly Ps3Button _btnDecrypt = new();
    private readonly Ps3Button _btnLoad = new();
    private readonly Ps3Button _btnLift = new();
    private readonly Ps3Button _btnBuild = new();
    private readonly Ps3Button _btnRun = new();
    private readonly Ps3Button _btnCopy = new();
    private readonly Ps3Button _btnCancel = new();
    private readonly ComboBox _gfx = new();
    private readonly ComboBox _threads = new();
    private readonly TabControl _tabs = new();
    private readonly PictureBox _logo = new();
    private string? _projectDir;
    private CancellationTokenSource? _cts;
    private Process? _gameProc;
    private bool _building;
    private bool _buildCancelRequested;
    private bool _canLift, _canBuild, _canCopy, _canRun, _gfxUnlocked, _busy;
    private int _liftChunks;
    private Ps3Button? _btnRpcs3;
    private readonly Label _eta = new();
    private bool _controllerHiddenAfterBuild;

    private const int StallQuietMs = 4000;
    private const int MaxRunMs = 0;
    private static string RuntimeDir => Path.Combine(AppContext.BaseDirectory, "runtime");
    private static string CompilersDir => Path.Combine(AppContext.BaseDirectory, "Compilers-files");
    private static string ProjectsRoot => Path.Combine(AppContext.BaseDirectory, "projects");

    [DllImport("dwmapi.dll")]
    private static extern int DwmSetWindowAttribute(IntPtr hwnd, int attr, ref int value, int size);

    protected override CreateParams CreateParams
    {
        get
        {
            var cp = base.CreateParams;
            cp.ExStyle |= 0x02000000;
            return cp;
        }
    }

    public MainForm()
    {
        AutoScaleMode = AutoScaleMode.Dpi;
        Text = "PPSX33  ·  PlayStation 3 Recompiler";
        Width = 1180; Height = 760;
        MinimumSize = new Size(980, 660);
        StartPosition = FormStartPosition.CenterScreen;
        BackColor = Bg; ForeColor = TextPri;
        Font = new Font("Segoe UI", 9.25f);
        DoubleBuffered = true;
        try {
            string ico = Path.Combine(AppContext.BaseDirectory, "ppsx33-logo.ico");
            if (!File.Exists(ico)) ico = Path.Combine(AppContext.BaseDirectory, "assets", "ppsx33-logo.ico");
            if (File.Exists(ico)) Icon = new Icon(ico);
        } catch { }

        var backdrop = new Ps3BackdropPanel { Dock = DockStyle.Fill };
        var root = new TableLayoutPanel { Dock = DockStyle.Fill, ColumnCount = 2, RowCount = 1, Padding = new Padding(16), BackColor = Color.Transparent };
        root.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute, 266));
        root.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 100f));
        var rail = new Panel { Dock = DockStyle.Fill, BackColor = Color.FromArgb(10, 17, 37), Padding = new Padding(14) };
        _logo.SizeMode = PictureBoxSizeMode.Zoom; _logo.Height = 70; _logo.Dock = DockStyle.Top; _logo.BackColor = Color.Transparent;
        try {
            string png = Path.Combine(AppContext.BaseDirectory, "ppsx33-logo.png");
            if (!File.Exists(png)) png = Path.Combine(AppContext.BaseDirectory, "assets", "ppsx33-logo.png");
            if (File.Exists(png)) _logo.Image = Image.FromFile(png);
        } catch { }

        var railTitle = MakeLbl("WORKFLOW", TextSec, true); railTitle.Dock = DockStyle.Top; railTitle.Height = 24;
        StyleBtn(_btnDecrypt, "Decrypt EBOOT", Accent);
        StyleBtn(_btnLoad, "Load ELF", Panel2);
        StyleBtn(_btnLift, "Decompile", Panel2);
        StyleBtn(_btnBuild, "Build", Panel2);
        StyleBtn(_btnRun, "Run", Accent);
        StyleBtn(_btnCopy, "Copy to game folder", Panel2);
        StyleBtn(_btnCancel, "Stop / Cancel", Danger);
        _btnLift.Enabled = _btnBuild.Enabled = _btnRun.Enabled = _btnCopy.Enabled = _btnCancel.Enabled = false;
        var btnStack = new FlowLayoutPanel { Dock = DockStyle.Top, FlowDirection = FlowDirection.TopDown, WrapContents = false, AutoSize = true, BackColor = Color.FromArgb(10, 17, 37), Padding = new Padding(0, 8, 0, 10) };
        foreach (var b in new[] { _btnDecrypt, _btnLoad, _btnLift, _btnBuild, _btnRun, _btnCopy, _btnCancel })
        { b.Width = 224; b.Height = 38; b.Margin = new Padding(0, 0, 0, 7); btnStack.Controls.Add(b); }

        var optsTitle = MakeLbl("OPTIONS", TextSec, true); optsTitle.Dock = DockStyle.Top; optsTitle.Height = 28; optsTitle.Padding = new Padding(0, 8, 0, 0);
        var opts = new Panel { Dock = DockStyle.Top, Height = 170, BackColor = Color.FromArgb(12, 21, 43), Padding = new Padding(6, 0, 0, 0) };
        var gfxLbl = MakeLbl("Graphics backend", TextSec, false); gfxLbl.Location = new Point(0, 4);
        _gfx.DropDownStyle = ComboBoxStyle.DropDownList; _gfx.FlatStyle = FlatStyle.Flat;
        _gfx.BackColor = Panel2; _gfx.ForeColor = TextPri; _gfx.Location = new Point(0, 24); _gfx.Width = 210;
        _gfx.Items.AddRange(new object[] { "D3D11", "D3D10", "Vulkan (exp)" });
        _gfx.SelectedItem = _settings.GraphicsBackend;
        if (_gfx.SelectedIndex < 0) _gfx.SelectedIndex = 0;
        _gfx.SelectedIndexChanged += (_, _) => { _settings.GraphicsBackend = _gfx.SelectedItem?.ToString() ?? "D3D11"; SaveSettings(); };
        var thrLbl = MakeLbl("PPU threads", TextSec, false); thrLbl.Location = new Point(0, 56);
        _threads.DropDownStyle = ComboBoxStyle.DropDownList; _threads.FlatStyle = FlatStyle.Flat;
        _threads.BackColor = Panel2; _threads.ForeColor = TextPri; _threads.Location = new Point(0, 76); _threads.Width = 210;
        int maxT = Math.Clamp(Environment.ProcessorCount, 1, 16);
        for (int i = 1; i <= maxT; i++) _threads.Items.Add(i.ToString());
        _threads.SelectedItem = Math.Clamp(_settings.PpuThreads, 1, maxT).ToString();
        if (_threads.SelectedIndex < 0) _threads.SelectedIndex = 0;
        _threads.SelectedIndexChanged += (_, _) => { if (int.TryParse(_threads.SelectedItem?.ToString(), out int t)) { _settings.PpuThreads = t; SaveSettings(); } };
        _btnRpcs3 = new Ps3Button();
        StyleBtn(_btnRpcs3, "RPCS3 path…", Panel2);
        _btnRpcs3.Location = new Point(0, 112); _btnRpcs3.Width = 210; _btnRpcs3.Height = 32;
        _btnRpcs3.Click += (_, _) => { if (PickRpcs3Exe()) { Append("RPCS3: " + _settings.Rpcs3Path); SaveSettings(); UpdateRpcs3Button(); } };
        opts.Controls.Add(gfxLbl); opts.Controls.Add(_gfx); opts.Controls.Add(thrLbl); opts.Controls.Add(_threads); opts.Controls.Add(_btnRpcs3);
        UpdateRpcs3Button();

        rail.Controls.Add(opts);
        rail.Controls.Add(optsTitle);
        rail.Controls.Add(btnStack);
        rail.Controls.Add(railTitle);
        rail.Controls.Add(_logo);

        var right = new TableLayoutPanel { Dock = DockStyle.Fill, ColumnCount = 1, RowCount = 3, BackColor = Color.Transparent, Padding = new Padding(12, 0, 0, 0), GrowStyle = TableLayoutPanelGrowStyle.FixedSize };
        right.RowStyles.Add(new RowStyle(SizeType.Absolute, 36));
        right.RowStyles.Add(new RowStyle(SizeType.Percent, 100f));
        right.RowStyles.Add(new RowStyle(SizeType.Absolute, 36));
        _project.Text = "PROJECT  /  NO PROJECT LOADED"; _project.ForeColor = TextPri; _project.BackColor = Color.FromArgb(12, 22, 47);
        _project.Dock = DockStyle.Fill; _project.TextAlign = ContentAlignment.MiddleLeft; _project.Padding = new Padding(12, 0, 0, 0);
        StyleTextBox(_log); StyleTextBox(_report); StyleTextBox(_roadmap);
        _roadmap.Text = "PPSX33 · Phase XIX · Load ELF → Decompile → Build → Run\r\n" +
            "I–XVIII Done  XIX Perf/batch  XX Playable\r\n" +
            "Controller tab — set keys before Build; hides after Build.\r\n";
        _tabs.Dock = DockStyle.Fill; _tabs.DrawMode = TabDrawMode.OwnerDrawFixed; _tabs.ItemSize = new Size(100, 32); _tabs.SizeMode = TabSizeMode.Fixed; _tabs.DrawItem += DrawPs3Tab;
        AddTab("Console", _log); AddTab("Report", _report); AddTab("Roadmap", _roadmap);
        BuildControllerTab();
        _status.TextAlign = ContentAlignment.MiddleLeft; _status.Padding = new Padding(8, 0, 0, 0);
        _status.Dock = DockStyle.Fill; _status.ForeColor = TextSec; _status.BackColor = Color.FromArgb(10, 17, 37);
        _progress.Dock = DockStyle.Bottom; _progress.Height = 6;

        right.Controls.Add(_project, 0, 0);
        right.Controls.Add(_tabs, 0, 1);
        var statusRow = new Panel { Dock = DockStyle.Fill, BackColor = Color.FromArgb(10, 17, 37) };
        statusRow.Controls.Add(_status);
        statusRow.Controls.Add(_progress);
        right.Controls.Add(statusRow, 0, 2);

        root.Controls.Add(rail, 0, 0);
        root.Controls.Add(right, 1, 0);
        backdrop.Controls.Add(root);
        Controls.Add(backdrop);

        _btnDecrypt.Click += async (_, _) => await DecryptEbootAndLoad();
        _btnLoad.Click += (_, _) => LoadElf(null);
        _btnLift.Click += async (_, _) => await RunStep("Decompiling…", ct => Native.Lift(_projectDir!, out _), afterLift: true);
        _btnBuild.Click += async (_, _) => await BuildWithProgress();
        _btnRun.Click += async (_, _) => await RunGame();
        _btnCopy.Click += (_, _) => CopyToGameFolder();
        _btnCancel.Click += async (_, _) => {
            if (_building && !_buildCancelRequested) await ConfirmCancelBuildAsync();
            else StopGame();
        };

        Append("PPSX33 ready · core v1.0.0");
        Append("Settings: " + Settings.DataDir);
        UpdateButtons();
    }

    void AddTab(string title, Control body) { var page = new TabPage(title) { BackColor = Panel, Padding = new Padding(4) }; page.Controls.Add(body); _tabs.TabPages.Add(page); }

    void StyleBtn(Control b, string text, Color bg) { b.Text = text; b.BackColor = bg; b.ForeColor = TextPri; }
    void StyleTextBox(TextBox t) { t.Multiline = true; t.ScrollBars = ScrollBars.Both; t.Dock = DockStyle.Fill; t.BackColor = Panel; t.ForeColor = TextPri; t.BorderStyle = BorderStyle.None; t.Font = new Font("Consolas", 9f); }
    Label MakeLbl(string text, Color c, bool bold) => new Label { Text = text, ForeColor = c, BackColor = Color.Transparent, Font = bold ? new Font("Segoe UI", 9f, FontStyle.Bold) : new Font("Segoe UI", 9f) };
    void DrawPs3Tab(object? sender, DrawItemEventArgs e) {
        if (e.Index < 0) return;
        var g = e.Graphics;
        var tab = _tabs.TabPages[e.Index];
        bool sel = (e.State & DrawItemState.Selected) != 0;
        using var br = new SolidBrush(sel ? Panel2 : Panel);
        g.FillRectangle(br, e.Bounds);
        TextRenderer.DrawText(g, tab.Text, Font, e.Bounds, sel ? TextPri : TextSec, TextFormatFlags.HorizontalCenter | TextFormatFlags.VerticalCenter);
    }
    void Append(string s) {
        if (IsDisposed) return;
        if (InvokeRequired) { BeginInvoke(() => Append(s)); return; }
        _log.AppendText(s.TrimEnd() + Environment.NewLine);
    }
    void SetStatus(string s, bool busy = false, int? percent = null, string? eta = null) {
        if (InvokeRequired) { BeginInvoke(() => SetStatus(s, busy, percent, eta)); return; }
        _status.Text = string.IsNullOrEmpty(eta) ? s : s + "  ·  " + eta;
        if (percent.HasValue) { _progress.Style = ProgressBarStyle.Continuous; _progress.Value = Math.Clamp(percent.Value, 0, 100); }
        else if (busy) _progress.Style = ProgressBarStyle.Marquee;
        else { _progress.Style = ProgressBarStyle.Continuous; _progress.Value = 0; }
    }
    void UpdateButtons() {
        _btnLift.Enabled = _canLift && !_busy;
        _btnBuild.Enabled = _canBuild && !_busy;
        _btnRun.Enabled = _canRun && !_busy;
        _btnCopy.Enabled = _canCopy && !_busy;
        _btnCancel.Enabled = _busy || _building || _gameProc != null;
        _btnCancel.Text = _building ? "Stop / Cancel Build" : "Stop Game";
        if (_gfxUnlocked) { /* graphics backend unlocked after decompile */ }
    }
    void SaveSettings() { try { _settings.Save(); } catch { } }
    void UpdateRpcs3Button() {
        if (_btnRpcs3 == null) return;
        _btnRpcs3.Text = string.IsNullOrEmpty(_settings.Rpcs3Path) ? "RPCS3 path…" : "RPCS3 set";
    }

    async Task BuildWithProgress() {
        if (_projectDir == null) return;
        _building = true; _buildCancelRequested = false;
        _busy = true; UpdateButtons();
        HideControllerAfterBuild();
        SaveSettings();
        WritePadMapForRuntime();
        string livePath = Path.Combine(_projectDir, "output", "build_live.log");
        try { if (File.Exists(livePath)) File.Delete(livePath); } catch { }

        Native.BeginBuild();
        Append("Build started — live console follows:");
        SetStatus("Building…", true, 0, "ETA —");

        var start = DateTime.UtcNow;
        long readPos = 0;
        int stepsDone = 0;
        int totalSteps = 0;
        var buildTask = Task.Run(() => Native.Build(_projectDir!, RuntimeDir, CompilersDir, out var l));

        while (!buildTask.IsCompleted) {
            await Task.Delay(400);
            try {
                if (File.Exists(livePath)) {
                    using var fs = new FileStream(livePath, FileMode.Open, FileAccess.Read, FileShare.ReadWrite);
                    if (fs.Length > readPos) {
                        fs.Seek(readPos, SeekOrigin.Begin);
                        using var sr = new StreamReader(fs, Encoding.UTF8);
                        string chunk = await sr.ReadToEndAsync();
                        readPos = fs.Position;
                        if (!string.IsNullOrWhiteSpace(chunk)) {
                            Append(chunk.TrimEnd());
                            foreach (var line in chunk.Split('\n')) {
                                if (line.StartsWith("BUILD_TOTAL=", StringComparison.OrdinalIgnoreCase)) {
                                    if (int.TryParse(line.AsSpan(12).Trim(), out int t) && t > 0) totalSteps = t;
                                }
                                if (line.StartsWith("COMPILE", StringComparison.OrdinalIgnoreCase) ||
                                    line.StartsWith("LINK", StringComparison.OrdinalIgnoreCase) ||
                                    line.StartsWith("cleanup", StringComparison.OrdinalIgnoreCase))
                                    stepsDone++;
                            }
                        }
                    }
                }
            } catch { }

            double elapsed = (DateTime.UtcNow - start).TotalSeconds;
            int denom = totalSteps > 0 ? totalSteps : Math.Max(stepsDone + 1, 1);
            int pct = Math.Clamp((int)(100.0 * stepsDone / denom), 0, 99);
            string eta = "ETA —";
            if (stepsDone >= 2 && elapsed > 1.5) {
                int remain = (int)Math.Max(0, (totalSteps - stepsDone) * (elapsed / stepsDone));
                eta = remain >= 60 ? $"ETA {remain / 60}m {remain % 60}s" : $"ETA {remain}s";
            }
            string totLabel = totalSteps > 0 ? totalSteps.ToString() : "?";
            SetStatus($"Building… {stepsDone}/{totLabel}", true, pct, eta);
            if (_buildCancelRequested) Native.CancelBuild();
        }

        bool ok = false;
        try { ok = await buildTask; } catch (Exception ex) { Append(ex.Message); }

        try {
            if (File.Exists(livePath)) {
                using var fs = new FileStream(livePath, FileMode.Open, FileAccess.Read, FileShare.ReadWrite);
                if (fs.Length > readPos) {
                    fs.Seek(readPos, SeekOrigin.Begin);
                    using var sr = new StreamReader(fs, Encoding.UTF8);
                    string rest = await sr.ReadToEndAsync();
                    if (!string.IsNullOrWhiteSpace(rest)) Append(rest.TrimEnd());
                }
            }
        } catch { }

        if (ok) {
            _canCopy = true; _canRun = true;
            SetStatus("Building successful", false, 100, "");
            Append("Building successful — Run and Copy unlocked.");
        } else if (_buildCancelRequested) {
            SetStatus("Build cancelled");
        } else {
            SetStatus("Building failed");
        }
        _building = false; _busy = false; UpdateButtons();
    }

    async Task ConfirmCancelBuildAsync() {
        _buildCancelRequested = true;
        _cts?.Cancel();
        Native.CancelBuild();
        _building = false; _busy = false; UpdateButtons();
        Append("Build cancel requested…");
        await Task.CompletedTask;
    }

    async Task RunStep(string status, Func<CancellationToken, bool> work, bool afterLift = false, bool afterBuild = false) {
        if (_projectDir == null) return;
        _cts = new CancellationTokenSource();
        _busy = true; UpdateButtons(); SetStatus(status, true); SaveSettings();
        bool ok = false;
        try { ok = await Task.Run(() => work(_cts.Token)); }
        catch (Exception ex) { Append(ex.Message); }
        finally {
            if (afterLift && ok) { _canBuild = true; _gfxUnlocked = true; _liftChunks = Math.Max(1, _liftChunks); Append("Lift chunks tracked: " + _liftChunks); }
            if (afterBuild && ok) { _canCopy = true; _canRun = true; }
            string workName = status.Contains("Decompil", StringComparison.OrdinalIgnoreCase) ? "Decompiling" : status.Contains("Build", StringComparison.OrdinalIgnoreCase) ? "Building" : status.TrimEnd('…', '.', ' ');
            Append(ok ? workName + " successful." : workName + " failed.");
            if (ok && afterLift) Append("Decompiling successful — Build and Graphics backend unlocked.");
            if (ok && afterBuild) Append("Building successful — Run and Copy unlocked.");
            _busy = false; UpdateButtons();
            SetStatus(ok ? workName + " done" : workName + " failed");
        }
    }

    // DecryptEbootAndLoad + PickRpcs3Exe live in MainForm.Decrypt.cs

    /// <summary>Shows LEGAL.md summary. User must Accept to continue. No accept = cancel.</summary>
    bool ShowLegalNotice(string action)
    {
        string body =
            "PPSX33 — LEGAL NOTICE (required before " + action + ")\r\n\r\n" +
            "• Use only with software and game data you lawfully own or are licensed to use.\r\n" +
            "• PPSX33 does NOT contain, use, or distribute proprietary decryption keys.\r\n" +
            "• PPSX33 does NOT bypass DRM or encryption frameworks by itself.\r\n" +
            "• We do NOT promote piracy, unauthorized copies, or redistribution of game assets.\r\n" +
            "• Optional EBOOT handling uses tools YOU provide (e.g. your own RPCS3 install); " +
            "PPSX33 does not ship decrypt keys or a built-in DRM crack.\r\n" +
            "• Do NOT distribute generated binaries, dumps, or game data.\r\n" +
            "• Output EXE / window / branding is \"PPSX33\" — a research/recompiler tool, " +
            "not a claim of an official Sony port or retail product.\r\n" +
            "• You are solely responsible for complying with applicable law and Sony/PlayStation terms.\r\n" +
            "• Use at your own risk. See LEGAL.md in the repository root.\r\n\r\n" +
            "Click Yes only if you understand and accept these terms.";

        var r = MessageBox.Show(
            this,
            body,
            "PPSX33 — Legal acknowledgment",
            MessageBoxButtons.YesNo,
            MessageBoxIcon.Warning,
            MessageBoxDefaultButton.Button2);
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
        // Prefer PPSX33.exe branding
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
        foreach (var f in Directory.GetFiles(outDir, "*.exe"))
            File.Copy(f, Path.Combine(fbd.SelectedPath, Path.GetFileName(f)), true);
        Append("Copied EXE to " + fbd.SelectedPath);
    }
    void StopGame() {
        try { _gameProc?.Kill(true); } catch { }
        _gameProc = null;
        Native.CancelBuild();
        Append("Stop requested.");
        UpdateButtons();
    }
}
