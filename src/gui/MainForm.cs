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
            cp.ExStyle |= 0x02000000; // WS_EX_COMPOSITED
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
        _gfx.BackColor = Panel2; _gfx.ForeColor = TextPri; _gfx.Location = new Point(0, 24); _gfx.Size = new Size(224, 30);
        _gfx.Items.AddRange(new object[] { "D3D10", "D3D11" });
        _gfx.SelectedItem = _settings.GraphicsBackend; if (_gfx.SelectedIndex < 0) _gfx.SelectedIndex = 1;
        _gfx.Enabled = false;
        var thrLbl = MakeLbl("CPU threads", TextSec, false); thrLbl.Location = new Point(0, 58);
        _threads.DropDownStyle = ComboBoxStyle.DropDownList; _threads.FlatStyle = FlatStyle.Flat;
        _threads.BackColor = Panel2; _threads.ForeColor = TextPri;
        _threads.Location = new Point(0, 78); _threads.Size = new Size(108, 30);
        int maxThr = Math.Max(1, Environment.ProcessorCount);
        for (int i = 1; i <= maxThr; i++) _threads.Items.Add(i);
        int want = Math.Clamp(_settings.PpuThreads, 1, maxThr);
        _threads.SelectedItem = want;
        if (_threads.SelectedIndex < 0) _threads.SelectedIndex = Math.Min(want, maxThr) - 1;
        var rpcs3Lbl = MakeLbl("RPCS3 (decrypt)", TextSec, false); rpcs3Lbl.Location = new Point(0, 108);
        _btnRpcs3 = new Ps3Button { Text = "Choose rpcs3.exe…", FlatStyle = FlatStyle.Flat, BackColor = Panel2, ForeColor = TextPri, Location = new Point(0, 128), Size = new Size(224, 30), Cursor = Cursors.Hand };
        _btnRpcs3.FlatAppearance.BorderSize = 0;
        _btnRpcs3.Click += (_, _) => { if (PickRpcs3Exe()) { Append("RPCS3: " + _settings.Rpcs3Path); SaveSettings(); UpdateRpcs3Button(); } };
        opts.Controls.AddRange(new Control[] { gfxLbl, _gfx, thrLbl, _threads, rpcs3Lbl, _btnRpcs3 });
        UpdateRpcs3Button();
        rail.Controls.Add(opts); rail.Controls.Add(optsTitle); rail.Controls.Add(btnStack); rail.Controls.Add(railTitle); rail.Controls.Add(_logo);

        var right = new TableLayoutPanel { Dock = DockStyle.Fill, ColumnCount = 1, RowCount = 3, BackColor = Color.Transparent, Padding = new Padding(12, 0, 0, 0), GrowStyle = TableLayoutPanelGrowStyle.FixedSize };
        right.RowStyles.Add(new RowStyle(SizeType.Absolute, 40));
        right.RowStyles.Add(new RowStyle(SizeType.Percent, 100f));
        right.RowStyles.Add(new RowStyle(SizeType.Absolute, 36));
        _project.Text = "PROJECT  /  NO PROJECT LOADED"; _project.ForeColor = TextPri; _project.BackColor = Color.FromArgb(12, 22, 47);
        _project.Dock = DockStyle.Fill; _project.TextAlign = ContentAlignment.MiddleLeft; _project.Padding = new Padding(12, 0, 0, 0);
        StyleTextBox(_log); StyleTextBox(_report); StyleTextBox(_roadmap);
        _roadmap.Text = "PPSX33 · Sequential: Load ELF → Decompile → Build → Run\r\n";
        _tabs.Dock = DockStyle.Fill; _tabs.DrawMode = TabDrawMode.OwnerDrawFixed; _tabs.ItemSize = new Size(92, 32); _tabs.SizeMode = TabSizeMode.Fixed; _tabs.DrawItem += DrawPs3Tab; _tabs.BackColor = Bg;
        AddTab("Console", _log); AddTab("Report", _report); AddTab("Roadmap", _roadmap);
        _status.Text = "●  SYSTEM READY"; _status.ForeColor = TextSec; _status.Dock = DockStyle.Fill;
        _status.TextAlign = ContentAlignment.MiddleLeft; _status.Padding = new Padding(8, 0, 0, 0);
        _progress.Style = ProgressBarStyle.Continuous; _progress.Minimum = 0; _progress.Maximum = 100; _progress.Value = 0;
        _progress.Dock = DockStyle.Right; _progress.Width = 200; _progress.Visible = false;
        _eta.Text = ""; _eta.ForeColor = TextSec; _eta.Dock = DockStyle.Right; _eta.Width = 160; _eta.TextAlign = ContentAlignment.MiddleRight;
        var statusBar = new Panel { Dock = DockStyle.Fill, BackColor = Color.FromArgb(13, 22, 48) };
        statusBar.Controls.Add(_status); statusBar.Controls.Add(_eta); statusBar.Controls.Add(_progress);
        right.Controls.Add(_project, 0, 0); right.Controls.Add(_tabs, 0, 1); right.Controls.Add(statusBar, 0, 2);
        root.Controls.Add(rail, 0, 0); root.Controls.Add(right, 1, 0); backdrop.Controls.Add(root); Controls.Add(backdrop);

        _btnDecrypt.Click += async (_, _) => await DecryptEbootAndLoad();
        _btnLoad.Click += (_, _) => LoadElf(null);
        _btnLift.Click += async (_, _) => await RunStep("Decompiling…", _ => {
            bool ok = Native.Lift(_projectDir!, out var l);
            BeginInvoke(() => {
                _report.Text = l; Append(l);
                var m = System.Text.RegularExpressions.Regex.Match(l, @"chunks:\s*(\d+)", System.Text.RegularExpressions.RegexOptions.IgnoreCase);
                if (m.Success && int.TryParse(m.Groups[1].Value, out int n)) _liftChunks = n;
            });
            return ok;
        }, afterLift: true);
        _btnBuild.Click += async (_, _) => await BuildWithProgress();
        _btnRun.Click += async (_, _) => await RunGameExe();
        _btnCopy.Click += (_, _) => CopyToGameFolder();
        _btnCancel.Click += async (_, _) => {
            if (_gameProc != null) { StopGame(); return; }
            if (_building && !_buildCancelRequested) await ConfirmCancelBuildAsync();
        };
        Append("PPSX33 ready · core v" + SafeVersion());
        Append("Settings: " + Settings.DataDir);
        UpdateButtons();
    }

    protected override void OnHandleCreated(EventArgs e) {
        base.OnHandleCreated(e);
        try { int dark = 1; DwmSetWindowAttribute(Handle, 20, ref dark, sizeof(int)); } catch { }
    }
    protected override void OnResizeBegin(EventArgs e) { base.OnResizeBegin(e); SuspendLayout(); }
    protected override void OnResizeEnd(EventArgs e) { ResumeLayout(true); base.OnResizeEnd(e); }

    static Label MakeLbl(string t, Color c, bool bold) => new() { Text = t, ForeColor = c, BackColor = Color.Transparent, Font = new Font("Segoe UI", bold ? 8f : 9f, bold ? FontStyle.Bold : FontStyle.Regular), AutoSize = true };
    void StyleBtn(Button b, string text, Color bg) { b.Text = text; b.FlatStyle = FlatStyle.Flat; b.FlatAppearance.BorderSize = 1; b.FlatAppearance.BorderColor = Color.FromArgb(65, 135, 220); b.FlatAppearance.MouseOverBackColor = Color.FromArgb(35, 65, 115); b.FlatAppearance.MouseDownBackColor = Color.FromArgb(45, 95, 165); b.BackColor = bg; b.ForeColor = TextPri; b.Font = new Font("Segoe UI Semibold", 9.25f); b.Cursor = Cursors.Hand; b.UseVisualStyleBackColor = false; }
    void DrawPs3Tab(object? sender, DrawItemEventArgs e) {
        if (sender is not TabControl tabs || e.Index < 0) return;
        bool selected = (e.State & DrawItemState.Selected) == DrawItemState.Selected;
        var bounds = e.Bounds;
        using var fill = new LinearGradientBrush(bounds, selected ? Color.FromArgb(34, 88, 160) : Color.FromArgb(13, 20, 42), selected ? Color.FromArgb(18, 46, 94) : Color.FromArgb(8, 13, 29), 90f);
        e.Graphics.FillRectangle(fill, bounds);
        using var edge = new Pen(selected ? Color.FromArgb(75, 180, 255) : Color.FromArgb(40, 64, 106));
        e.Graphics.DrawRectangle(edge, bounds.X, bounds.Y, bounds.Width - 1, bounds.Height - 1);
        TextRenderer.DrawText(e.Graphics, tabs.TabPages[e.Index].Text, new Font("Segoe UI Semibold", 9f), bounds, selected ? Color.White : TextSec, TextFormatFlags.HorizontalCenter | TextFormatFlags.VerticalCenter | TextFormatFlags.EndEllipsis);
    }
    void StyleTextBox(TextBox t) { t.Multiline = true; t.ScrollBars = ScrollBars.Both; t.WordWrap = true; t.ReadOnly = true; t.BackColor = Color.FromArgb(7, 11, 25); t.ForeColor = Color.FromArgb(205, 222, 255); t.BorderStyle = BorderStyle.FixedSingle; t.Font = new Font("Consolas", 9.25f); t.Dock = DockStyle.Fill; }
    void AddTab(string title, Control body) { var page = new TabPage(title) { BackColor = Panel, Padding = new Padding(4) }; page.Controls.Add(body); _tabs.TabPages.Add(page); }
    void Append(string s) { if (string.IsNullOrEmpty(s)) return; if (_log.InvokeRequired) { BeginInvoke(() => Append(s)); return; } _log.AppendText(s.TrimEnd() + Environment.NewLine); }

    void SetStatus(string s, bool busy = false, int? percent = null, string? eta = null) {
        if (InvokeRequired) { BeginInvoke(() => SetStatus(s, busy, percent, eta)); return; }
        _status.Text = s;
        _status.ForeColor = s.Contains("fail", StringComparison.OrdinalIgnoreCase) || s.Contains("Error", StringComparison.OrdinalIgnoreCase) ? Danger : (busy ? Accent : TextSec);
        _progress.Visible = busy || percent.HasValue;
        if (percent.HasValue) { _progress.Style = ProgressBarStyle.Continuous; _progress.MarqueeAnimationSpeed = 0; _progress.Value = Math.Clamp(percent.Value, 0, 100); }
        else if (busy) { _progress.Style = ProgressBarStyle.Marquee; _progress.MarqueeAnimationSpeed = 30; }
        else { _progress.Style = ProgressBarStyle.Continuous; _progress.MarqueeAnimationSpeed = 0; _progress.Value = 0; }
        _eta.Text = eta ?? "";
    }

    void UpdateRpcs3Button() {
        if (_btnRpcs3 == null) return;
        bool need = string.IsNullOrWhiteSpace(_settings.Rpcs3Path) || !File.Exists(_settings.Rpcs3Path);
        _btnRpcs3.Visible = need;
    }

    void UpdateButtons() {
        _btnLift.Enabled = _canLift && !_busy; _btnBuild.Enabled = _canBuild && !_busy;
        _btnRun.Enabled = _canRun && !_busy; _btnCopy.Enabled = _canCopy && !_busy;
        _btnCancel.Text = _building ? "Stop / Cancel Build" : "Stop Game";
        _btnCancel.Enabled = _gameProc != null || _building; _btnDecrypt.Enabled = !_busy; _btnLoad.Enabled = !_busy;
        _gfx.Enabled = _gfxUnlocked && !_busy;
    }

    async Task RunStep(string status, Func<CancellationToken, bool> work, bool afterLift = false, bool afterBuild = false) {
        if (_projectDir == null) { Append("No project."); return; }
        _cts?.Cancel(); _cts = new CancellationTokenSource();
        _busy = true; UpdateButtons(); SetStatus(status, true); SaveSettings();
        try {
            bool ok = await Task.Run(() => work(_cts.Token), _cts.Token);
            if (afterLift && ok) { _canBuild = true; _gfxUnlocked = true; }
            if (afterBuild && ok) { _canCopy = true; _canRun = true; }
            string workName = status.Contains("Decompil", StringComparison.OrdinalIgnoreCase) ? "Decompiling" : status.Contains("Build", StringComparison.OrdinalIgnoreCase) ? "Building" : status.TrimEnd('…', '.', ' ');
            SetStatus(ok ? workName + " successful" : workName + " failed");
            if (ok && afterLift) Append("Decompiling successful — Build and Graphics backend unlocked.");
            if (ok && afterBuild) Append("Building successful — Run and Copy unlocked.");
        } catch (OperationCanceledException) { SetStatus("Cancelled"); }
        catch (Exception ex) { Append(ex.Message); SetStatus("Error"); }
        finally { _busy = false; UpdateButtons(); }
    }

    void LoadElf(string? path) {
        if (path == null) {
            using var ofd = new OpenFileDialog { Title = "Select decrypted ELF or SELF", Filter = "ELF / SELF|*.elf;*.ELF;*.self;*.SELF|All files|*.*", CheckFileExists = true };
            if (ofd.ShowDialog(this) != DialogResult.OK) return;
            path = ofd.FileName; _settings.LastElfPath = path; SaveSettings();
        }
        using var dlg = new PromptDialog("Project name", Path.GetFileNameWithoutExtension(path) ?? "project");
        if (dlg.ShowDialog(this) != DialogResult.OK) return;
        Directory.CreateDirectory(ProjectsRoot);
        if (!Native.CreateProject(ProjectsRoot, dlg.Value, path, out var proj, out var log)) { Append(log); SetStatus("Project create failed"); return; }
        _projectDir = proj; _project.Text = proj; _canLift = true; _canBuild = _canCopy = _canRun = false; _gfxUnlocked = false;
        Append(log); Append("ELF loaded — Decompile is now available."); SetStatus("Project ready — Decompile next"); UpdateButtons();
    }

    async Task BuildWithProgress() {
        if (_projectDir == null) return;
        _building = true; _buildCancelRequested = false;
        _busy = true; UpdateButtons();
        SaveSettings();
        string livePath = Path.Combine(_projectDir, "output", "build_live.log");
        try { if (File.Exists(livePath)) File.Delete(livePath); } catch { }

        Native.BeginBuild();
        int totalSteps = 0; // from BUILD_TOTAL=N
        int stepsDone = 0;
        long readPos = 0;
        var start = DateTime.UtcNow;
        SetStatus("Building…", true, 0, "ETA —");
        Append("Build started — live console follows:");

        string? finalLog = null;
        var buildTask = Task.Run(() => {
            bool ok = Native.Build(_projectDir!, RuntimeDir, CompilersDir, out var l);
            finalLog = l;
            return ok;
        });

        while (!buildTask.IsCompleted) {
            await Task.Delay(200);
            try {
                if (File.Exists(livePath)) {
                    using var fs = new FileStream(livePath, FileMode.Open, FileAccess.Read, FileShare.ReadWrite);
                    if (fs.Length > readPos) {
                        fs.Seek(readPos, SeekOrigin.Begin);
                        using var sr = new StreamReader(fs, Encoding.UTF8);
                        string chunk = await sr.ReadToEndAsync();
                        readPos = fs.Position;
                        foreach (var line in chunk.Split(new[] { "\r\n", "\n" }, StringSplitOptions.None)) {
                            if (string.IsNullOrWhiteSpace(line)) continue;
                            Append(line);
                            if (line.StartsWith("BUILD_TOTAL=", StringComparison.OrdinalIgnoreCase)) {
                                int.TryParse(line.AsSpan("BUILD_TOTAL=".Length), out totalSteps);
                            }
                            if (line.StartsWith("COMPILE", StringComparison.OrdinalIgnoreCase) ||
                                line.StartsWith("LINK", StringComparison.OrdinalIgnoreCase) ||
                                line.StartsWith("cleanup", StringComparison.OrdinalIgnoreCase))
                                stepsDone++;
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

    async Task RunGameExe() {
        if (_projectDir == null) return;
        string exeName = Path.GetFileName(_projectDir) + ".exe";
        string exePath = Path.Combine(_projectDir, "output", exeName);
        if (!File.Exists(exePath)) { Append("EXE not found: " + exePath); return; }
        try {
            _busy = true; UpdateButtons(); SetStatus("Running " + exeName + "…", true);
            var psi = new ProcessStartInfo(exePath) { WorkingDirectory = Path.GetDirectoryName(exePath)!, UseShellExecute = false };
            _gameProc = Process.Start(psi);
            if (_gameProc != null) await _gameProc.WaitForExitAsync();
            Append(exeName + " exited code=" + (_gameProc?.ExitCode ?? -1)); SetStatus("Run finished");
        } catch (Exception ex) { Append(ex.Message); SetStatus("Run error"); }
        finally { _gameProc = null; _busy = false; UpdateButtons(); }
    }

    void StopGame() {
        try { _gameProc?.Kill(true); } catch { }
        _gameProc = null; UpdateButtons(); SetStatus("Game stopped");
    }

    void CopyToGameFolder() {
        if (_projectDir == null) return;
        using var fbd = new FolderBrowserDialog { Description = "Select game folder" };
        if (fbd.ShowDialog(this) != DialogResult.OK) return;
        string exeName = Path.GetFileName(_projectDir) + ".exe";
        string src = Path.Combine(_projectDir, "output", exeName);
        string dll = Path.Combine(_projectDir, "output", "ps3rt.dll");
        try {
            if (File.Exists(src)) File.Copy(src, Path.Combine(fbd.SelectedPath, exeName), true);
            if (File.Exists(dll)) File.Copy(dll, Path.Combine(fbd.SelectedPath, "ps3rt.dll"), true);
            Append("Copied to " + fbd.SelectedPath); SetStatus("Copied");
        } catch (Exception ex) { Append(ex.Message); }
    }

    async Task DecryptEbootAndLoad() {
        Append("Use RPCS3 to decrypt EBOOT.BIN then Load ELF.");
        await Task.CompletedTask;
    }

    bool PickRpcs3Exe() {
        using var ofd = new OpenFileDialog { Title = "Select rpcs3.exe", Filter = "rpcs3.exe|rpcs3.exe|All|*.*" };
        if (ofd.ShowDialog(this) != DialogResult.OK) return false;
        _settings.Rpcs3Path = ofd.FileName; return true;
    }

    void SaveSettings() {
        _settings.GraphicsBackend = _gfx.SelectedItem?.ToString() ?? "D3D11";
        _settings.PpuThreads = _threads.SelectedItem is int ti ? ti : Math.Clamp(_settings.PpuThreads, 1, Environment.ProcessorCount);
        _settings.Save();
    }

    static string SafeVersion() {
        try { return typeof(MainForm).Assembly.GetName().Version?.ToString(3) ?? "1"; }
        catch { return "1"; }
    }
}
