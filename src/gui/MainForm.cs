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
    private Panel? _controllerPanel;
    private TabPage? _controllerTab;
    private readonly Dictionary<string, TextBox> _keyBoxes = new();
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
        // Options body filled in existing InitOptions if present; keep minimal fallback
        try { InitOptionsPanel(opts); } catch { }

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
        _roadmap.Text = "PPSX33 · Phase XVII · Load ELF → Decompile → Build → Run\r\n" +
            "I–XV Done  XVI Audio  XVII Pad/input  XVIII Save  XIX Perf  XX Playable\r\n" +
            "Controller tab — set keys before Build; tab hides after Build.\r\n";
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
        _btnLoad.Click += async (_, _) => await PickAndLoadElf();
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

    // Partial methods implemented in other files (Decrypt, options, native helpers)
    partial void InitOptionsPanel(Panel opts);

    void AddTab(string title, Control body) { var page = new TabPage(title) { BackColor = Panel, Padding = new Padding(4) }; page.Controls.Add(body); _tabs.TabPages.Add(page); }

    void BuildControllerTab() {
        if (_controllerTab != null) return;
        _controllerPanel = new Panel {
            Dock = DockStyle.Fill,
            BackColor = Panel,
            AutoScroll = true,
            Padding = new Padding(12)
        };
        var title = MakeLbl("SET CONTROLLER  ·  keyboard → DualShock (before Build)", TextPri, true);
        title.Location = new Point(8, 8); title.AutoSize = true;
        var hint = MakeLbl("Edit keys, then Build. This tab is removed after Build — map is written for the runtime.", TextSec, false);
        hint.Location = new Point(8, 32); hint.AutoSize = true; hint.MaximumSize = new Size(720, 0);

        var km = _settings.KeyMap ?? KeyMap.Defaults();
        _settings.KeyMap = km;
        (string prop, string label, string val)[] rows = {
            ("Cross", "Cross (×)", km.Cross),
            ("Circle", "Circle (○)", km.Circle),
            ("Square", "Square (□)", km.Square),
            ("Triangle", "Triangle (△)", km.Triangle),
            ("DpadUp", "D-Pad Up", km.DpadUp),
            ("DpadDown", "D-Pad Down", km.DpadDown),
            ("DpadLeft", "D-Pad Left", km.DpadLeft),
            ("DpadRight", "D-Pad Right", km.DpadRight),
            ("LeftStickUp", "Left stick Up", km.LeftStickUp),
            ("LeftStickDown", "Left stick Down", km.LeftStickDown),
            ("LeftStickLeft", "Left stick Left", km.LeftStickLeft),
            ("LeftStickRight", "Left stick Right", km.LeftStickRight),
            ("L1", "L1", km.L1),
            ("R1", "R1", km.R1),
            ("L2", "L2", km.L2),
            ("R2", "R2", km.R2),
            ("L3", "L3", km.L3),
            ("R3", "R3", km.R3),
            ("Start", "Start", km.Start),
            ("Select", "Select", km.Select),
        };

        int y = 60;
        int col = 0;
        foreach (var (prop, label, val) in rows) {
            int x = 8 + col * 360;
            var lb = MakeLbl(label, TextSec, false);
            lb.Location = new Point(x, y); lb.AutoSize = true;
            var tb = new TextBox {
                Width = 140, Height = 24,
                Location = new Point(x + 150, y - 2),
                BackColor = Panel2, ForeColor = TextPri, BorderStyle = BorderStyle.FixedSingle,
                Text = val ?? ""
            };
            string p = prop;
            tb.Leave += (_, _) => { ApplyKeyBox(p, tb.Text); SaveSettings(); };
            _controllerPanel.Controls.Add(lb);
            _controllerPanel.Controls.Add(tb);
            _keyBoxes[prop] = tb;
            col++;
            if (col >= 2) { col = 0; y += 32; }
        }
        y += 48;
        var btnReset = new Ps3Button { Text = "Reset defaults", Width = 140, Height = 32, Location = new Point(8, y) };
        StyleBtn(btnReset, "Reset defaults", Panel2);
        btnReset.Click += (_, _) => {
            _settings.KeyMap = KeyMap.Defaults();
            foreach (var kv in _keyBoxes) {
                var pi = typeof(KeyMap).GetProperty(kv.Key);
                if (pi != null) kv.Value.Text = pi.GetValue(_settings.KeyMap)?.ToString() ?? "";
            }
            SaveSettings();
            Append("Controller map reset to defaults.");
        };
        var btnSave = new Ps3Button { Text = "Save map", Width = 120, Height = 32, Location = new Point(160, y) };
        StyleBtn(btnSave, "Save map", Accent);
        btnSave.Click += (_, _) => {
            foreach (var kv in _keyBoxes) ApplyKeyBox(kv.Key, kv.Value.Text);
            SaveSettings();
            WritePadMapForRuntime();
            Append("Controller map saved.");
        };
        _controllerPanel.Controls.Add(title);
        _controllerPanel.Controls.Add(hint);
        _controllerPanel.Controls.Add(btnReset);
        _controllerPanel.Controls.Add(btnSave);

        _controllerTab = new TabPage("Controller") { BackColor = Panel, Padding = new Padding(4) };
        _controllerTab.Controls.Add(_controllerPanel);
        _tabs.TabPages.Add(_controllerTab);
    }

    void ApplyKeyBox(string prop, string text) {
        if (_settings.KeyMap == null) _settings.KeyMap = KeyMap.Defaults();
        var pi = typeof(KeyMap).GetProperty(prop);
        if (pi != null && pi.CanWrite)
            pi.SetValue(_settings.KeyMap, (text ?? "").Trim());
    }

    void HideControllerAfterBuild() {
        if (_controllerHiddenAfterBuild) return;
        _controllerHiddenAfterBuild = true;
        try {
            if (_controllerTab != null && _tabs.TabPages.Contains(_controllerTab)) {
                if (_tabs.SelectedTab == _controllerTab)
                    _tabs.SelectedIndex = 0;
                _tabs.TabPages.Remove(_controllerTab);
            }
            Append("Controller tab closed — mapping locked for this build.");
        } catch { }
    }

    void WritePadMapForRuntime() {
        try {
            if (_settings.KeyMap == null) _settings.KeyMap = KeyMap.Defaults();
            string path = Path.Combine(Settings.DataDir, "pad_map.json");
            File.WriteAllText(path, System.Text.Json.JsonSerializer.Serialize(_settings.KeyMap, new System.Text.Json.JsonSerializerOptions { WriteIndented = true }));
            if (!string.IsNullOrEmpty(_projectDir)) {
                string outDir = Path.Combine(_projectDir, "output");
                try { Directory.CreateDirectory(outDir); } catch { }
                File.WriteAllText(Path.Combine(outDir, "pad_map.json"), File.ReadAllText(path));
            }
        } catch { }
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
        var buildTask = Task.Run(() => {
            bool ok = Native.Build(_projectDir!, RuntimeDir, CompilersDir, out var l);
            return ok;
        });

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

    // Stubs / helpers expected from partial class files — keep symbols for compile if others provide real bodies
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
    }
    void SaveSettings() { try { _settings.Save(); } catch { } }

    // Remaining workflow methods live in partials (Decrypt, Native wrappers). Minimal fallbacks:
    async Task DecryptEbootAndLoad() { await Task.CompletedTask; }
    async Task PickAndLoadElf() { await Task.CompletedTask; }
    async Task RunStep(string status, Func<CancellationToken, bool> work, bool afterLift = false, bool afterBuild = false) { await Task.CompletedTask; }
    async Task RunGame() { await Task.CompletedTask; }
    void CopyToGameFolder() { }
    void StopGame() { }
}
