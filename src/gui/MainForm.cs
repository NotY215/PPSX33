using System.Diagnostics;
using System.Runtime.InteropServices;
using System.Text;
using System.Drawing.Drawing2D;

namespace PS3Recomp.Gui;

/// <summary>Static dark WinForms shell. Load ELF → Decompile → Build → Run.</summary>
public sealed partial class MainForm : Form
{
    // PS3 XMB-inspired midnight palette. Opaque panels only; no glassmorphism.
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
    private bool _stopGameRequested;
    private bool _building;
    private bool _buildCancelRequested;
    private bool _canLift, _canBuild, _canCopy, _canRun, _gfxUnlocked, _busy;
    private int _liftChunks;
    private Ps3Button? _btnRpcs3;
    private readonly Label _eta = new();

    private const int StallQuietMs = 4000;
    private const int MaxRunMs = 120_000;
    private static string RuntimeDir => Path.Combine(AppContext.BaseDirectory, "runtime");
    private static string CompilersDir => Path.Combine(AppContext.BaseDirectory, "Compilers-files");
    private static string ProjectsRoot => Path.Combine(AppContext.BaseDirectory, "projects");

    [DllImport("dwmapi.dll")]
    private static extern int DwmSetWindowAttribute(IntPtr hwnd, int attr, ref int value, int size);

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

    static Label MakeLbl(string t, Color c, bool bold) => new() { Text = t, ForeColor = c, BackColor = Color.Transparent, Font = new Font("Segoe UI", bold ? 8f : 9f, bold ? FontStyle.Bold : FontStyle.Regular), AutoSize = true };
    void StyleBtn(Button b, string text, Color bg) { b.Text = text; b.FlatStyle = FlatStyle.Flat; b.FlatAppearance.BorderSize = 1; b.FlatAppearance.BorderColor = Color.FromArgb(65, 135, 220); b.FlatAppearance.MouseOverBackColor = Color.FromArgb(35, 65, 115); b.FlatAppearance.MouseDownBackColor = Color.FromArgb(45, 95, 165); b.BackColor = bg; b.ForeColor = TextPri; b.Font = new Font("Segoe UI Semibold", 9.25f); b.Cursor = Cursors.Hand; b.UseVisualStyleBackColor = false; } 

    void DrawPs3Tab(object? sender, DrawItemEventArgs e) {
        if (sender is not TabControl tabs || e.Index < 0) return;
        bool selected = (e.State & DrawItemState.Selected) == DrawItemState.Selected;
        var bounds = e.Bounds;
        using var fill = new LinearGradientBrush(bounds,
            selected ? Color.FromArgb(34, 88, 160) : Color.FromArgb(13, 20, 42),
            selected ? Color.FromArgb(18, 46, 94) : Color.FromArgb(8, 13, 29), 90f);
        e.Graphics.FillRectangle(fill, bounds);
        using var edge = new Pen(selected ? Color.FromArgb(75, 180, 255) : Color.FromArgb(40, 64, 106));
        e.Graphics.DrawRectangle(edge, bounds.X, bounds.Y, bounds.Width - 1, bounds.Height - 1);
        TextRenderer.DrawText(e.Graphics, tabs.TabPages[e.Index].Text, new Font("Segoe UI Semibold", 9f),
            bounds, selected ? Color.White : TextSec, TextFormatFlags.HorizontalCenter | TextFormatFlags.VerticalCenter | TextFormatFlags.EndEllipsis);
    }
    void StyleTextBox(TextBox t) { t.Multiline = true; t.ScrollBars = ScrollBars.Both; t.WordWrap = true; t.ReadOnly = true; t.BackColor = Color.FromArgb(7, 11, 25); t.ForeColor = Color.FromArgb(205, 222, 255); t.BorderStyle = BorderStyle.FixedSingle; t.Font = new Font("Consolas", 9.25f); t.Dock = DockStyle.Fill; }
    void AddTab(string title, Control body) { var page = new TabPage(title) { BackColor = Panel, Padding = new Padding(4) }; page.Controls.Add(body); _tabs.TabPages.Add(page); }
    void Append(string s) { if (string.IsNullOrEmpty(s)) return; if (_log.InvokeRequired) { BeginInvoke(() => Append(s)); return; } _log.AppendText(s.TrimEnd() + Environment.NewLine); }

    void SetStatus(string s, bool busy = false, int? percent = null, string? eta = null) {
        if (InvokeRequired) { BeginInvoke(() => SetStatus(s, busy, percent, eta)); return; }
        _status.Text = s;
        _status.ForeColor = s.Contains("fail", StringComparison.OrdinalIgnoreCase) || s.Contains("Error", StringComparison.OrdinalIgnoreCase) || s.Contains("stall", StringComparison.OrdinalIgnoreCase) ? Danger : (busy ? Accent : TextSec);
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
        if (_btnRpcs3.Parent != null)
            foreach (Control c in _btnRpcs3.Parent.Controls)
                if (c is Label lb && lb.Text.StartsWith("RPCS3", StringComparison.Ordinal)) lb.Visible = need;
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
            SetStatus(ok ? "Done" : "Failed");
            if (ok && afterLift) Append("Decompile OK — Build and Graphics backend unlocked.");
            if (ok && afterBuild) Append("Build OK — Run and Copy unlocked.");
        } catch (OperationCanceledException) { SetStatus("Cancelled"); }
        catch (Exception ex) { Append(ex.Message); SetStatus("Error"); }
        finally { _busy = false; UpdateButtons(); }
    }

    void LoadElf(string? path) {
        if (path == null) {
            using var ofd = new OpenFileDialog {
                Title = "Select decrypted ELF or SELF",
                Filter = "ELF / SELF|*.elf;*.ELF;*.self;*.SELF|All files|*.*",
                CheckFileExists = true,
                InitialDirectory = !string.IsNullOrWhiteSpace(_settings.LastElfPath) && File.Exists(_settings.LastElfPath) ? (Path.GetDirectoryName(_settings.LastElfPath) ?? "") : ""
            };
            if (ofd.ShowDialog(this) != DialogResult.OK) return;
            path = ofd.FileName; _settings.LastElfPath = path; SaveSettings();
        }
        string ext = Path.GetExtension(path).ToLowerInvariant();
        if (ext is not (".elf" or ".self")) { Append("Only .elf / .self. Use Decrypt for EBOOT.BIN."); SetStatus("Invalid file type"); return; }
        using var dlg = new PromptDialog("Project name", SuggestProjectName(path));
        if (dlg.ShowDialog(this) != DialogResult.OK) return;
        Directory.CreateDirectory(ProjectsRoot);
        if (!Native.CreateProject(ProjectsRoot, dlg.Value, path, out var proj, out var log)) { Append(log); SetStatus("Project create failed"); return; }
        _projectDir = proj; _project.Text = proj; _canLift = true; _canBuild = _canCopy = _canRun = false; _gfxUnlocked = false;
        Append(log); Append("ELF loaded — Decompile is now available."); SetStatus("Project ready — Decompile next"); UpdateButtons();
    }

    static string SuggestProjectName(string path) {
        try {
            string name = Path.GetFileNameWithoutExtension(path);
            if (string.IsNullOrWhiteSpace(name)) name = Path.GetFileName(path);
            if (string.IsNullOrWhiteSpace(name)) return "project";
            foreach (char c in Path.GetInvalidFileNameChars()) name = name.Replace(c, '_');
            name = name.Replace(' ', '_');
            if (name.Length > 64) name = name.Substring(0, 64);
            return name;
        } catch { } return "project";
    }

    string? ResolveOutputExe() {
        if (_projectDir == null) return null;
        string outDir = Path.Combine(_projectDir, "output");
        string projName = Path.GetFileName(_projectDir.TrimEnd(Path.DirectorySeparatorChar, Path.AltDirectorySeparatorChar));
        if (!string.IsNullOrEmpty(projName)) {
            string named = Path.Combine(outDir, projName + ".exe");
            if (File.Exists(named)) return named;
        }
        string legacy = Path.Combine(outDir, "game.exe");
        if (File.Exists(legacy)) return legacy;
        try {
            foreach (var f in Directory.EnumerateFiles(outDir, "*.exe"))
                return f;
        } catch { }
        return null;
    }

    async Task RunGameExe() {
        if (_projectDir == null) return;
        string? exe = ResolveOutputExe();
        if (exe == null || !File.Exists(exe)) { Append("Output .exe not found — Build first."); return; }
        string exeName = Path.GetFileName(exe);
        StopGame(); SaveSettings();
        var psi = new ProcessStartInfo { FileName = exe, WorkingDirectory = Path.GetDirectoryName(exe)!, UseShellExecute = false, RedirectStandardOutput = true, RedirectStandardError = true, CreateNoWindow = true };
        psi.Environment["PS3RT_GFX"] = _gfx.SelectedItem?.ToString() ?? "D3D11";
        var gameProc = new Process { StartInfo = psi, EnableRaisingEvents = true };
        _gameProc = gameProc;
        _stopGameRequested = false;
        var lastOut = DateTime.UtcNow;
        void OnData(string? line) { if (line == null) return; lastOut = DateTime.UtcNow; BeginInvoke(() => Append(line)); }
        gameProc.OutputDataReceived += (_, e) => OnData(e.Data);
        gameProc.ErrorDataReceived += (_, e) => OnData(e.Data);
        try {
            gameProc.Start(); gameProc.BeginOutputReadLine(); gameProc.BeginErrorReadLine();
            _busy = true; UpdateButtons(); SetStatus("Running " + exeName + "…", true);
            var start = DateTime.UtcNow;
            while (!gameProc.HasExited) {
                await Task.Delay(250);
                if (_stopGameRequested) break;
                if ((DateTime.UtcNow - lastOut).TotalMilliseconds > StallQuietMs && (DateTime.UtcNow - start).TotalMilliseconds > 8000) {
                    Append("[ui] no output for " + StallQuietMs + "ms — possible stall"); lastOut = DateTime.UtcNow;
                }
                if ((DateTime.UtcNow - start).TotalMilliseconds > MaxRunMs) { Append("[ui] max run time — stopping"); StopGame(); break; }
            }
            if (_stopGameRequested) { Append(exeName + " stopped by user."); SetStatus("Game stopped"); }
            else { Append(exeName + " exited code=" + gameProc.ExitCode); SetStatus("Run finished"); }
        } catch (Exception ex) { Append(ex.Message); SetStatus("Run error"); }
        finally { _busy = false; _gameProc = null; _stopGameRequested = false; UpdateButtons(); }
    }

    void StopGame() {
        var game = _gameProc;
        if (game == null) return;
        _stopGameRequested = true;
        try { if (!game.HasExited) game.Kill(true); } catch { }
        UpdateButtons();
    }

    void CopyToGameFolder() {
        if (_projectDir == null) return;
        try {
            string? src = ResolveOutputExe();
            if (src == null || !File.Exists(src)) { Append("Output .exe missing"); return; }
            string? destDir = !string.IsNullOrWhiteSpace(_settings.LastElfPath) ? Path.GetDirectoryName(_settings.LastElfPath) : null;
            if (string.IsNullOrWhiteSpace(destDir) || !Directory.Exists(destDir)) {
                using var fbd = new FolderBrowserDialog { Description = "Select game folder" };
                if (fbd.ShowDialog(this) != DialogResult.OK) return;
                destDir = fbd.SelectedPath;
            }
            string dest = Path.Combine(destDir!, Path.GetFileName(src));
            File.Copy(src, dest, true);
            string dll = Path.Combine(Path.GetDirectoryName(src)!, "ps3rt.dll");
            if (File.Exists(dll)) File.Copy(dll, Path.Combine(destDir!, "ps3rt.dll"), true);
            string img = Path.Combine(Path.GetDirectoryName(src)!, "guest_image.bin");
            if (File.Exists(img)) File.Copy(img, Path.Combine(destDir!, "guest_image.bin"), true);
            Append("Copied to " + destDir);
            SetStatus("Copied");
        } catch (Exception ex) { Append(ex.Message); }
    }

    async Task BuildWithProgress() {
        if (_projectDir == null) return;

        // ETA is estimated from source translation units and object files updated during this build.
        // It is advisory only: incremental builds, linking, and compiler scheduling affect the estimate.
        string project = _projectDir;
        string buildRoot = Path.Combine(project, "build");
        if (!Directory.Exists(buildRoot)) buildRoot = project;
        int sourceTotal = CountBuildSources(project);
        var started = DateTime.UtcNow;
        var seenObjects = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
        var before = new Dictionary<string, DateTime>(StringComparer.OrdinalIgnoreCase);
        try {
            foreach (string file in Directory.EnumerateFiles(buildRoot, "*.obj", SearchOption.AllDirectories))
                before[file] = File.GetLastWriteTimeUtc(file);
        } catch { }

        using var poll = new System.Threading.Timer(_ => {
            try {
                int completed = 0;
                foreach (string file in Directory.EnumerateFiles(buildRoot, "*.obj", SearchOption.AllDirectories)) {
                    DateTime modified = File.GetLastWriteTimeUtc(file);
                    if (!before.TryGetValue(file, out DateTime previous) || modified > previous)
                        seenObjects.Add(file);
                }
                completed = seenObjects.Count;
                double elapsed = Math.Max(0.5, (DateTime.UtcNow - started).TotalSeconds);
                double rate = completed / elapsed;
                int percent = sourceTotal > 0 ? Math.Clamp((int)Math.Round(completed * 100.0 / sourceTotal), 0, 99) : 0;
                string etaText = rate > 0 && sourceTotal > completed
                    ? "ETA ~" + FormatDuration(TimeSpan.FromSeconds((sourceTotal - completed) / rate))
                    : (completed > 0 ? "ETA calculating…" : "ETA warming up…");
                BeginInvoke(() => SetStatus($"Building… {completed}/{sourceTotal} objects", true, percent, etaText));
            } catch { /* A transient filesystem/UI race must not interrupt the build. */ }
        }, null, 0, 750);

        bool buildSucceeded = false;
        _buildCancelRequested = false;
        Native.BeginBuild();
        _building = true;
        UpdateButtons();
        try {
            await RunStep("Building…", ct => {
                bool ok = Native.Build(project, RuntimeDir, CompilersDir, out var l);
                buildSucceeded = ok;
                BeginInvoke(() => Append(l));
                return ok;
            }, afterBuild: true);
        } finally {
            _building = false;
            UpdateButtons();
        }

        if (_buildCancelRequested) {
            // Cancellation intentionally discards all generated project folders and requires a fresh ELF load.
            DeleteCancelledBuildFolders(project);
            _projectDir = null;
            _project.Text = "PROJECT  /  NO PROJECT LOADED";
            _canLift = _canBuild = _canCopy = _canRun = _gfxUnlocked = false;
            _liftChunks = 0;
            _report.Clear();
            _roadmap.Text = "PPSX33 · Sequential: Load ELF → Decompile → Build → Run\r\n";
            SetStatus("Build cancelled — reload ELF");
            UpdateButtons();
            MessageBox.Show(this,
                "Building has been cancelled. The selected ELF has been unloaded and generated folders were deleted. Please reload the ELF file to continue.",
                "PPSX33 — Build Cancelled", MessageBoxButtons.OK, MessageBoxIcon.Information);
            return;
        }

        // Only remove temporary project inputs after a successful build.
        // Keep output binaries, reports, and staged output/src intact.
        if (buildSucceeded) CleanupBuildIntermediates(project);
        SetStatus("Build finished");
    }

    async Task ConfirmCancelBuildAsync() {
        var answer = MessageBox.Show(this,
            "Stopping the build will delete the generated input, codebase, and output folders and unload the selected ELF. You will need to reload the ELF file.\n\nAre you sure you want to stop building?",
            "Warning — Cancel Build", MessageBoxButtons.YesNo, MessageBoxIcon.Warning, MessageBoxDefaultButton.Button2);
        if (answer != DialogResult.Yes) return;
        _buildCancelRequested = true;
        Append("[ui] Build cancellation requested by user.");
        SetStatus("Stopping build…", true);
        try { Native.CancelBuild(); }
        catch (Exception ex) { Append("Could not request native build cancellation: " + ex.Message); }
        await Task.CompletedTask;
    }

    static void DeleteCancelledBuildFolders(string projectDir) {
        foreach (string name in new[] { "codebase", "input", "output" }) {
            string path = Path.Combine(projectDir, name);
            try {
                if (Directory.Exists(path)) Directory.Delete(path, recursive: true);
            } catch (Exception ex) {
                System.Diagnostics.Debug.WriteLine($"PPSX33 cancelled-build cleanup skipped '{path}': {ex.Message}");
            }
        }
    }

    static void CleanupBuildIntermediates(string projectDir) {
        string[] paths = {
            Path.Combine(projectDir, "input"),
            Path.Combine(projectDir, "codebase"),
            Path.Combine(projectDir, "output", "obj")
        };
        foreach (string path in paths) {
            try {
                if (Directory.Exists(path)) {
                    Directory.Delete(path, recursive: true);
                }
            } catch (Exception ex) {
                // Cleanup is best-effort: a locked file must not turn a successful build into a failure.
                System.Diagnostics.Debug.WriteLine($"PPSX33 cleanup skipped '{path}': {ex.Message}");
            }
        }
    }

    static int CountBuildSources(string projectDir) {
        // Count only generated game translation units, not runtime stubs or helper/header files.
        // The codebase folder is the canonical source of game .cpp files before the build stages them into output/src.
        string codebase = Path.Combine(projectDir, "codebase");
        if (Directory.Exists(codebase)) {
            try {
                int gameSources = Directory.EnumerateFiles(codebase, "*.cpp", SearchOption.AllDirectories).Count();
                // The build also compiles three runtime translation units into the staged src folder.
                return gameSources + 3;
            } catch { }
        }

        // If staging already exists, count every C++ translation unit in the game's src folder.
        string stagedSrc = Path.Combine(projectDir, "output", "src");
        if (!Directory.Exists(stagedSrc)) return 0;
        try {
            return Directory.EnumerateFiles(stagedSrc, "*.cpp", SearchOption.AllDirectories).Count();
        } catch { return 0; }
    }

    static string FormatDuration(TimeSpan duration) {
        if (duration.TotalDays >= 1) return $"{(int)duration.TotalDays}d {duration.Hours}h";
        if (duration.TotalHours >= 1) return $"{(int)duration.TotalHours}h {duration.Minutes}m";
        if (duration.TotalMinutes >= 1) return $"{(int)duration.TotalMinutes}m {duration.Seconds}s";
        return $"{Math.Max(0, (int)Math.Ceiling(duration.TotalSeconds))}s";
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
